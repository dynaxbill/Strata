#include "strata/platform/socket_rom.hpp"
#include <cassert>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#if defined(__linux__)
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/mempolicy.h>
#endif
using strata::platform::SocketRom;
using strata::platform::RomSpan;
int main(int argc,char** argv) {
#if defined(__linux__)
    std::string err,h;
    assert(SocketRom::digest("abc",3,h,err));
    assert(h=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(SocketRom::digest("",0,h,err));
    assert(h=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    SocketRom rom;
    auto fill=[](const RomSpan& s,uint8_t* dst,std::string&){for(size_t i=0;i<s.bytes;++i)dst[i]=(uint8_t)(i*17+(unsigned)s.expert);return true;};
    if(!rom.build({{"expert.0.0",0,0,0,1048576,{}},{"embedding",-1,-1,0,8193,{}}},fill,7,err)){
        std::cerr<<"ROM_BUILD_REFUSED: "<<err<<'\n';return 3;
    }
    assert(rom.ready());assert(rom.verify(err));
    const auto* a=rom.at(0,0,7);const auto* b=rom.at(0,1,7);
    assert(a&&b&&a!=b&&std::memcmp(a,b,1048576)==0);
    assert(!rom.at(0,0,8)&&!rom.at(0,2,7)&&!rom.at(99,0,7));
    int policy_before=-1,policy_after=-1;unsigned long nodes_before[16]{},nodes_after[16]{};
    cpu_set_t cpus_before,cpus_after;assert(!sched_getaffinity(0,sizeof(cpus_before),&cpus_before));
    assert(!syscall(SYS_get_mempolicy,&policy_before,nodes_before,1024,nullptr,0));
    assert(rom.with_socket_registration_placement(1,[&](std::string&){
        assert(std::find(rom.placement()[1].cpus.begin(),rom.placement()[1].cpus.end(),sched_getcpu())!=rom.placement()[1].cpus.end());
        void* p=mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(p!=MAP_FAILED);*(volatile uint8_t*)p=17;
        void* addresses[]={p};int node=-1;assert(!syscall(SYS_move_pages,0,1,addresses,nullptr,&node,0));
        assert(std::find(rom.placement()[1].nodes.begin(),rom.placement()[1].nodes.end(),node)!=rom.placement()[1].nodes.end());munmap(p,4096);return true;
    },err));
    assert(!rom.with_socket_registration_placement(1,[](std::string& e){e="injected registration failure";return false;},err));
    assert(!syscall(SYS_get_mempolicy,&policy_after,nodes_after,1024,nullptr,0));
    assert(policy_before==policy_after&&!std::memcmp(nodes_before,nodes_after,sizeof(nodes_before)));
    assert(!sched_getaffinity(0,sizeof(cpus_after),&cpus_after));assert(CPU_EQUAL(&cpus_before,&cpus_after));
    pid_t child=fork();assert(child>=0);
    if(child==0){rlimit lim{0,0};setrlimit(RLIMIT_CORE,&lim);*(volatile uint8_t*)const_cast<uint8_t*>(a)=42;_exit(99);}
    int status=0;assert(waitpid(child,&status,0)==child);assert(WIFSIGNALED(status)&&WTERMSIG(status)==SIGSEGV);
    assert(a[0]==0&&b[0]==0);
    {SocketRom bad;assert(!bad.build({{"bad",0,0,0,4096,"bad-hash"}},fill,8,err));assert(!bad.ready());}
    {SocketRom bad;assert(!bad.build({{"bad",0,0,0,4096,{}}},[](const RomSpan&,uint8_t*,std::string& e){e="injected copy failure";return false;},8,err));assert(!bad.ready());}
    {SocketRom bad;assert(!bad.build({{"overflow",0,0,0,UINT64_MAX,{}}},fill,8,err));assert(!bad.ready());}
    // One 4 KiB lock can succeed, the other must fail; no half-ready pointer escapes.
    child=fork();assert(child>=0);
    if(child==0){
        rlimit limit{4096,4096};assert(!setrlimit(RLIMIT_MEMLOCK,&limit));
        {SocketRom partial;assert(!partial.build({{"partial",0,0,0,4096,{}}},fill,9,err));assert(!partial.ready());assert(!partial.at(0,0,9));assert(err.find("mlock")!=std::string::npos);}
        std::ifstream status_file("/proc/self/status");std::string line;
        while(std::getline(status_file,line))if(line.rfind("VmLck:",0)==0)assert(std::stoull(line.substr(6))==0);
        _exit(0);
    }
    assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
    // Bound only this child address space: exercise a real mmap ENOMEM without
    // consuming host RAM or exposing a partially initialized image to readers.
    child=fork();assert(child>=0);
    if(child==0){
        std::ifstream stat("/proc/self/status");std::string line;uint64_t current=0;
        while(std::getline(stat,line))if(line.rfind("VmSize:",0)==0)current=std::stoull(line.substr(7))*1024;
        assert(current);rlimit limit{current+(16ull<<20),current+(16ull<<20)};assert(!setrlimit(RLIMIT_AS,&limit));
        {SocketRom oom;assert(!oom.build({{"oom",0,0,0,64ull<<20,{}}},fill,10,err));assert(!oom.ready());assert(!oom.at(0,0,10));assert(err=="anonymous ROM mmap failed");}
        _exit(0);
    }
    assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
    assert(!rom.build({{"again",0,0,0,1,{}}},fill,8,err));
    if(argc>1){assert(rom.write_report(argv[1],err));}
    std::cout<<"PASS sha256/empty/copy/readonly/SIGSEGV/placement/generation/checksum/failure/overflow/partial-mlock-cleanup/mmap-ENOMEM/single-use pid="<<getpid()<<std::endl;
    if(argc>2){const int seconds=std::stoi(argv[2]);for(int i=0;i<seconds;++i)sleep(1);}
    return 0;
#else
    std::cout<<"UNSUPPORTED: P0 ROM requires Linux\n";return 77;
#endif
}
