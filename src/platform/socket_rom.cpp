#include "strata/platform/socket_rom.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <thread>

#if defined(__linux__)
#include <dirent.h>
#include <dlfcn.h>
#include <linux/if_alg.h>
#include <linux/mempolicy.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace strata::platform {
namespace {
std::array<int, 1024> cpu_roles = [] { std::array<int, 1024> a{}; a.fill(-1); return a; }();
std::string read(const std::string& p) { std::ifstream f(p); return {std::istreambuf_iterator<char>(f), {}}; }
uint64_t metric(const std::string& s, const std::string& key) {
    std::istringstream lines(s); std::string line;
    while (std::getline(lines, line)) {
        const auto p = line.find(key + ":");
        if (p != std::string::npos) { std::istringstream n(line.substr(p + key.size() + 1)); uint64_t x = 0; n >> x; return x * 1024; }
    }
    return 0;
}
std::vector<int> list(std::string s) {
    std::vector<int> out; std::istringstream in(s); std::string p;
    while (std::getline(in, p, ',')) {
        const auto dash = p.find('-'); int a = std::stoi(p), b = dash == std::string::npos ? a : std::stoi(p.substr(dash + 1));
        if (a < 0 || b < a || b >= 1024) throw std::runtime_error("invalid CPU/node list");
        for (int n = a; n <= b; ++n) out.push_back(n);
    }
    return out;
}
std::string quote(const std::string& s) {
    std::string r = "\""; for (unsigned char c : s) { if (c == '"' || c == '\\') r += '\\'; if (c < 32) throw std::runtime_error("control in ROM span name"); r += (char)c; } return r + '"';
}
}
bool socket_rom_requested() { const char* v = std::getenv("STRATA_DS_ROM"); return v && std::strcmp(v, "full") == 0; }
bool socket_rom_config_valid(std::string& err) {
    const char* v = std::getenv("STRATA_DS_ROM");
    if (v && *v && std::strcmp(v,"off") && std::strcmp(v,"full")) { err="STRATA_DS_ROM supports off|full; complement is not qualified"; return false; }
    if (!socket_rom_requested()) return true;
    const char* strict = std::getenv("STRATA_NUMA_STRICT");
    if (!strict || std::strcmp(strict,"1")) { err="P0-FULL requires STRATA_NUMA_STRICT=1"; return false; }
    const char* ple=std::getenv("STRATA_PLE_OWNER");
    if(!ple||std::strcmp(ple,"single")){err="P0-FULL requires STRATA_PLE_OWNER=single";return false;}
    for (const char* k : {"STRATA_IO_PREFETCH", "STRATA_FS_SLOTS", "STRATA_LOOKAHEAD_STATS"}) {
        const char* x=std::getenv(k); if (x && *x && std::strcmp(x,"0")) { err=std::string(k)+" needs a separate CPU-router scope qualification"; return false; }
    }
    return true;
}
SocketRom::~SocketRom() {
    ready_ = false;
    if(images_[0].base||images_[1].base)std::fprintf(stderr,"P0 ROM shutdown: begin reader/DMA drain\n");
    if (cleanup_ && !cleanup_()) return; // retain mappings on an unproven DMA drain
#if defined(__linux__)
    for (auto& i : images_) if (i.base) {
        if(munlock(i.base,i.bytes))std::fprintf(stderr,"P0 ROM shutdown: munlock returned %s\n",std::strerror(errno));
        if(munmap(i.base,i.bytes))std::fprintf(stderr,"P0 HOLD: ROM munmap failed: %s\n",std::strerror(errno));
        else std::fprintf(stderr,"P0 ROM shutdown: released role=%u bytes=%zu\n",i.socket_id,i.bytes);
    }
#endif
}
bool SocketRom::digest(const void* data, size_t bytes, std::string& out, std::string& err) {
#if defined(__linux__)
    // Use the image's existing OpenSSL 3 ABI without adding headers or a link-time dependency.
    // Unlike AF_ALG, this works under the ordinary Docker/AppArmor socket restrictions.
    using Sha256 = unsigned char* (*)(const unsigned char*,size_t,unsigned char*);
    static const Sha256 sha=[] {
        void* handle=dlopen("libcrypto.so.3",RTLD_NOW|RTLD_LOCAL);
        return handle?reinterpret_cast<Sha256>(dlsym(handle,"SHA256")):nullptr;
    }();
    if(!sha){err="OpenSSL 3 SHA256 unavailable; ROM cannot be verified";return false;}
    unsigned char d[32];
    if(!sha((const unsigned char*)data,bytes,d)){err="OpenSSL SHA256 failed";return false;}
    std::ostringstream s;for(unsigned char c:d)s<<std::hex<<std::setw(2)<<std::setfill('0')<<(unsigned)c;out=s.str();return true;
#else
    (void)data;(void)bytes;(void)out;err="P0 ROM requires Linux";return false;
#endif
}
bool SocketRom::discover(std::array<SocketPlacement,2>& out,std::string& err) {
#if defined(__linux__)
    try {
        cpu_set_t allowed;CPU_ZERO(&allowed);if(sched_getaffinity(0,sizeof(allowed),&allowed))throw std::runtime_error("sched_getaffinity failed");
        std::map<int,SocketPlacement> sockets;
        for(int cpu=0;cpu<CPU_SETSIZE&&cpu<1024;++cpu)if(CPU_ISSET(cpu,&allowed)){
            const std::string s=read("/sys/devices/system/cpu/cpu"+std::to_string(cpu)+"/topology/physical_package_id");
            if(s.empty())throw std::runtime_error("missing CPU socket identity");
            const int socket=std::stoi(s);sockets[socket].socket=socket;sockets[socket].cpus.push_back(cpu);
        }
        if(sockets.size()!=2)throw std::runtime_error("P0 requires exactly two allowed sockets");
        for(int node:list(read("/sys/devices/system/node/online"))){
            const auto cpus=list(read("/sys/devices/system/node/node"+std::to_string(node)+"/cpulist"));int owner=-1;
            for(auto& pair:sockets)for(int c:cpus)if(std::find(pair.second.cpus.begin(),pair.second.cpus.end(),c)!=pair.second.cpus.end()){
                if(owner!=-1&&owner!=pair.first)throw std::runtime_error("guest node spans sockets");
                owner=pair.first;
            }
            if(owner!=-1){auto& p=sockets.at(owner);p.nodes.push_back(node);p.capacity+=metric(read("/sys/devices/system/node/node"+std::to_string(node)+"/meminfo"),"MemTotal");}
        }
        // Respect both cpuset and NUMA policy admission; do not infer allowed memory from CPUs.
        unsigned long allowed_nodes[16]{};
        if(syscall(SYS_get_mempolicy,nullptr,allowed_nodes,1024,nullptr,MPOL_F_MEMS_ALLOWED))throw std::runtime_error("get_mempolicy allowed nodes unavailable");
        size_t r=0;for(auto& pair:sockets){auto p=pair.second;if(p.nodes.empty()||!p.capacity)throw std::runtime_error("socket has no memory node");
            for(int n:p.nodes)if(!(allowed_nodes[n/(8*sizeof(unsigned long))]&(1ul<<(n%(8*sizeof(unsigned long))))))throw std::runtime_error("socket memory node not allowed");
            out[r++]=std::move(p);}
        return true;
    } catch(const std::exception& e){err=e.what();return false;}
#else
    (void)out;err="P0 ROM requires Linux";return false;
#endif
}
bool SocketRom::build(std::vector<RomSpan> spans,Copy copy,uint64_t generation,std::string& err,BeforeProtect before_protect) {
#if defined(__linux__)
    if(ready_||images_[0].base||images_[1].base){err="ROM owner is single use";return false;}
    if(!generation||spans.empty()){err="empty ROM or zero generation";return false;}
    if(!discover(placement_,err))return false;
    uint64_t bytes=0;const uint64_t page=(uint64_t)sysconf(_SC_PAGESIZE);
    for(auto& s:spans){if(!s.bytes||s.bytes>SIZE_MAX-page||bytes>SIZE_MAX-s.bytes-page){err="ROM span size overflow";return false;}
        s.offset=bytes;bytes=(bytes+s.bytes+page-1)&~(page-1);}
    const uint64_t available=metric(read("/proc/meminfo"),"MemAvailable");
    uint64_t reserves=0;for(const auto& p:placement_)reserves+=std::max<uint64_t>(16ull<<30,p.capacity/10);
    if(bytes>(UINT64_MAX-reserves)/2||available<2*bytes+reserves){err="FULL capacity gate: insufficient global/cgroup headroom";return false;}
    spans_=std::move(spans);
    for(unsigned role=0;role<2;++role){
        uint64_t local=0;for(int n:placement_[role].nodes){const auto m=read("/sys/devices/system/node/node"+std::to_string(n)+"/meminfo");
            const uint64_t clean=metric(m,"MemFree")+metric(m,"Inactive(file)")+metric(m,"SReclaimable");
            const uint64_t dirty=metric(m,"Dirty")+metric(m,"Writeback");local+=clean>dirty?clean-dirty:0;}
        const uint64_t reserve=std::max<uint64_t>(16ull<<30,placement_[role].capacity/10);
        if(local<bytes||local-bytes<reserve){err="FULL capacity gate: socket-local headroom insufficient";return false;}
        auto& image=images_[role];image.socket_id=role;image.generation=generation;image.bytes=(size_t)bytes;
        image.base=mmap(nullptr,image.bytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        if(image.base==MAP_FAILED){image.base=nullptr;err="anonymous ROM mmap failed";return false;}
        // Avoid unobserved huge-page placement/migration during the first qualification.
        if(madvise(image.base,image.bytes,MADV_NOHUGEPAGE)||madvise(image.base,image.bytes,MADV_UNMERGEABLE)){err="guest ROM madvise failed (not host KSM evidence)";return false;}
        unsigned long mask[16]{};for(int n:placement_[role].nodes)mask[n/(8*sizeof(unsigned long))]|=1ul<<(n%(8*sizeof(unsigned long)));
        if(syscall(SYS_mbind,image.base,image.bytes,MPOL_BIND,mask,1024,MPOL_MF_STRICT)){err="ROM mbind failed: "+std::string(std::strerror(errno));return false;}
    }
    std::array<std::string,2> failures;std::array<std::vector<std::string>,2> hashes;
    auto worker=[&](unsigned r){try{
        cpu_set_t cpus;CPU_ZERO(&cpus);for(int c:placement_[r].cpus)CPU_SET(c,&cpus);
        if(sched_setaffinity(0,sizeof(cpus),&cpus))throw std::runtime_error("copy worker affinity failed");
        for(const auto& span:spans_){auto* dst=(uint8_t*)images_[r].base+span.offset;
            // Touch all bytes, including alignment gaps, from the owning socket worker.
            const size_t padded=(size_t)((span.bytes+page-1)&~(page-1));std::memset(dst,0,padded);
            if(!copy(span,dst,failures[r]))throw std::runtime_error(failures[r].empty()?"source copy failed":failures[r]);
            std::string h;if(!digest(dst,(size_t)span.bytes,h,failures[r]))throw std::runtime_error(failures[r]);
            hashes[r].push_back(h);
        }
        if(mlock(images_[r].base,images_[r].bytes))throw std::runtime_error("ROM mlock failed: "+std::string(std::strerror(errno)));
    }catch(const std::exception& e){failures[r]=e.what();}};
    std::thread a,b;
    try { a=std::thread(worker,0);b=std::thread(worker,1); }
    catch(const std::exception& e){if(a.joinable())a.join();err=e.what();return false;}
    a.join();b.join();
    for(unsigned r=0;r<2;++r)if(!failures[r].empty()){err=failures[r];return false;}
    if(hashes[0]!=hashes[1]||hashes[0].size()!=spans_.size()){err="ROM mirror checksum mismatch";return false;}
    for(size_t i=0;i<spans_.size();++i){if(!spans_[i].sha256.empty()&&spans_[i].sha256!=hashes[0][i]){err="ROM source checksum mismatch";return false;}spans_[i].sha256=hashes[0][i];}
    // Driver registration is allowed only before publishing a pointer. The final
    // transition to PROT_READ is mandatory, followed by a complete placement scan.
    if(before_protect){
        if(!verify(err,"before-register",false))return false;
        if(!before_protect(images_,err))return false;
        std::string registration_placement;
        if(!verify(registration_placement,"after-register",false))
            std::fprintf(stderr,"P0 registration placement audit: %s\n",registration_placement.c_str());
    }
    for(auto& image:images_){if(mprotect(image.base,image.bytes,PROT_READ)){err="ROM read-only protection failed";return false;}image.read_only=true;}
    if(!verify(err))return false;
    for(unsigned r=0;r<2;++r)for(int c:placement_[r].cpus)cpu_roles[(size_t)c]=(int)r;
    ready_=true;return true;
#else
    (void)spans;(void)copy;(void)generation;(void)before_protect;err="P0 ROM requires Linux";return false;
#endif
}
bool SocketRom::verify(std::string& err,const char* phase,bool require_read_only) const {
#if defined(__linux__)
    const size_t page=(size_t)sysconf(_SC_PAGESIZE);std::array<void*,4096> addresses{};std::array<int,4096> nodes{};
    std::array<std::map<int,uint64_t>,2> counts;std::array<uint64_t,2> bad_count{};
    struct Bad {unsigned role;uintptr_t address;size_t offset;int status;};std::vector<Bad> first_bad;
    for(unsigned r=0;r<2;++r){const auto& i=images_[r];if(!i.base||(require_read_only&&!i.read_only)){err="ROM not protected";return false;}
        for(size_t off=0;off<i.bytes;){size_t n=0;while(n<addresses.size()&&off<i.bytes){addresses[n++]=(uint8_t*)i.base+off;off+=page;}
            if(syscall(SYS_move_pages,0,n,addresses.data(),nullptr,nodes.data(),0)){err="move_pages verification unavailable: "+std::string(std::strerror(errno));return false;}
            for(size_t j=0;j<n;++j){++counts[r][nodes[j]];
                if(nodes[j]<0||std::find(placement_[r].nodes.begin(),placement_[r].nodes.end(),nodes[j])==placement_[r].nodes.end()){
                    if(bad_count[r]<16)first_bad.push_back({r,(uintptr_t)addresses[j],(size_t)((uint8_t*)addresses[j]-(uint8_t*)i.base),nodes[j]});
                    ++bad_count[r];
                }
            }}
    }
    if(const char* report=std::getenv("STRATA_DS_ROM_REPORT");report&&*report){
        std::ofstream f(std::string(report)+".placement-"+phase+".json");
        f<<"{\"phase\":"<<quote(phase)<<",\"namespace_pid\":"<<getpid()<<",\"bad_pages\":["<<bad_count[0]<<','<<bad_count[1]<<"],\"counts\":[";
        for(unsigned r=0;r<2;++r){if(r)f<<',';f<<'{';bool first=true;for(const auto& p:counts[r]){if(!first)f<<',';first=false;f<<quote(std::to_string(p.first))<<':'<<p.second;}f<<'}';}
        f<<"],\"first_bad\":[";for(size_t n=0;n<first_bad.size();++n){if(n)f<<',';const auto& p=first_bad[n];f<<"{\"role\":"<<p.role<<",\"address\":"<<p.address<<",\"offset\":"<<p.offset<<",\"status\":"<<p.status<<'}';}f<<"]}\n";f.flush();
        if(!f){err="placement evidence write failed";return false;}
    }
    if(!first_bad.empty()){const auto& p=first_bad[0];err="ROM placement: role="+std::to_string(p.role)+" offset="+std::to_string(p.offset)+" node/status="+std::to_string(p.status);return false;}
    return true;
#else
    (void)phase;(void)require_read_only;
    err="P0 ROM requires Linux";return false;
#endif
}
const uint8_t* SocketRom::at(size_t span,unsigned role,uint64_t generation) const {
    if(!ready_||role>=2||span>=spans_.size()||generation!=images_[role].generation)return nullptr;
    return (const uint8_t*)images_[role].base+spans_[span].offset;
}
int SocketRom::current_role() {
#if defined(__linux__)
    const int c=sched_getcpu();return c>=0&&c<(int)cpu_roles.size()?cpu_roles[(size_t)c]:-1;
#else
    return -1;
#endif
}
bool SocketRom::with_socket_registration_placement(unsigned role,const std::function<bool(std::string&)>& operation,std::string& err) const {
#if defined(__linux__)
    if(role>=2||placement_[role].nodes.empty()){err="invalid registration socket policy";return false;}
    int original_mode=0;unsigned long original_nodes[16]{},nodes[16]{};
    if(syscall(SYS_get_mempolicy,&original_mode,original_nodes,1024,nullptr,0)){err="cannot read registration thread memory policy";return false;}
    if(original_mode&~7){err="registration thread has unsupported memory-policy flags";return false;}
    cpu_set_t original_cpus,target_cpus;CPU_ZERO(&original_cpus);CPU_ZERO(&target_cpus);
    if(sched_getaffinity(0,sizeof(original_cpus),&original_cpus)){err="cannot read registration CPU affinity";return false;}
    bool has_cpu=false;for(int c:placement_[role].cpus)if(CPU_ISSET(c,&original_cpus)){CPU_SET(c,&target_cpus);has_cpu=true;}
    if(!has_cpu){err="registration socket has no allowed CPU";return false;}
    if(sched_setaffinity(0,sizeof(target_cpus),&target_cpus)){err="cannot bind registration CPU affinity";return false;}
    for(int n:placement_[role].nodes)nodes[n/(8*sizeof(unsigned long))]|=1ul<<(n%(8*sizeof(unsigned long)));
    if(syscall(SYS_set_mempolicy,MPOL_BIND,nodes,1024)){(void)sched_setaffinity(0,sizeof(original_cpus),&original_cpus);err="cannot bind registration allocation policy";return false;}
    std::fprintf(stderr,"P0 registration placement: role=%u cpu=%d temporary MPOL_BIND\n",role,sched_getcpu());
    const auto restore=[&]{const bool memory=syscall(SYS_set_mempolicy,original_mode,original_mode==MPOL_DEFAULT?nullptr:original_nodes,original_mode==MPOL_DEFAULT?0:1024)==0;const bool cpu=sched_setaffinity(0,sizeof(original_cpus),&original_cpus)==0;return memory&&cpu;};
    bool ok=false;
    try {ok=operation(err);} catch(...){(void)restore();throw;}
    if(!restore()){err="cannot restore registration thread placement";return false;}
    return ok;
#else
    (void)role;(void)operation;err="P0 memory policy requires Linux";return false;
#endif
}
bool SocketRom::write_report(const std::string& path,std::string& err) const {
    if(!ready_){err="ROM is not READY";return false;}
    std::ofstream f(path,std::ios::out|std::ios::trunc);if(!f){err="ROM report cannot be opened";return false;}
    f<<"{\"status\":\"GUEST_VERIFIED_HOST_PFNS_UNVERIFIED\",\"state\":\"READY\",\"pid\":";
#if defined(__linux__)
    f<<getpid();
#else
    f<<0;
#endif
    f<<",\"generation\":"<<images_[0].generation<<",\"images\":[";
    for(unsigned r=0;r<2;++r){if(r)f<<',';f<<"{\"role\":"<<r<<",\"base\":"<<(uintptr_t)images_[r].base<<",\"bytes\":"<<images_[r].bytes<<",\"readonly\":true,\"nodes\":[";
        for(size_t n=0;n<placement_[r].nodes.size();++n){if(n)f<<',';f<<placement_[r].nodes[n];}f<<"]}";}
    f<<"],\"spans\":[";for(size_t s=0;s<spans_.size();++s){if(s)f<<',';const auto& x=spans_[s];f<<"{\"name\":"<<quote(x.name)<<",\"layer\":"<<x.layer<<",\"expert\":"<<x.expert<<",\"offset\":"<<x.offset<<",\"bytes\":"<<x.bytes<<",\"sha256\":"<<quote(x.sha256)<<'}';}
    f<<"]}\n";f.flush();if(!f){err="ROM report write failed";return false;}return true;
}
}
