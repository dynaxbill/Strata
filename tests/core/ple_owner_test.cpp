#include "strata/kernels/ngram.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
using namespace strata::kernels;
int main(int argc,char** argv){
    assert(argc==2);setenv("STRATA_PLE_OWNER","single",1);
    using Outputs=std::array<std::array<float,NG_N_EMBD>,4>;
    Outputs reference{};
    for(int mode=0;mode<3;++mode){
        PleTable t;std::string err;PleIoOptions io;io.mode=mode==0?PleIo::Direct:PleIo::Mmap;io.lock=mode==2;io.cache_rows=128;
        if(!t.open(argv[1],err,io)){std::cerr<<err<<'\n';return 2;}
        assert(t.single_owner_mode());assert(t.owner_identity());assert(t.cache_capacity_bytes()<=128*PLE_ROW_BYTES);
        PleTable duplicate;assert(!duplicate.open(argv[1],err,io));
        if(mode==2)assert(t.locked());
        std::array<std::array<uint32_t,PLE_N_HEADS>,4> rows{};
        for(unsigned k=0;k<4;++k)for(unsigned h=0;h<PLE_N_HEADS;++h)
            rows[k][h]=(uint32_t)((h*(t.rows()>100000?20000011ull:3ull)+k*113ull)%t.rows());
        std::array<PleTable::Request,4> requests;
        for(unsigned k=0;k<4;++k)assert(t.issue(requests[k],rows[k].data()));
        assert(!t.issue(requests[0],rows[1].data()));
        Outputs outputs{};
        std::array<std::thread,4> workers;
        for(unsigned k=0;k<4;++k)workers[k]=std::thread([&,k]{std::string e;assert(t.collect(requests[k],outputs[k].data(),e));});
        for(auto& w:workers)w.join();
        for(unsigned k=1;k<4;++k)assert(std::memcmp(outputs[0].data(),outputs[k].data(),sizeof(outputs[0])));
        if(mode==0)reference=outputs;else for(unsigned k=0;k<4;++k)assert(!std::memcmp(reference[k].data(),outputs[k].data(),sizeof(outputs[k])));
        // Second round is the same independent requests against a warm row cache.
        for(unsigned k=0;k<4;++k)assert(t.issue(requests[k],rows[k].data()));
        for(int k=3;k>=0;--k){assert(t.collect(requests[(size_t)k],outputs[(size_t)k].data(),err));assert(!std::memcmp(reference[(size_t)k].data(),outputs[(size_t)k].data(),sizeof(outputs[0])));}
        assert(!t.collect(requests[0],outputs[0].data(),err));
        // Closing the facade cannot close backing still owned by an in-flight request.
        assert(t.issue(requests[0],rows[0].data()));t.close();
        assert(t.collect(requests[0],outputs[0].data(),err));
        assert(!std::memcmp(reference[0].data(),outputs[0].data(),sizeof(outputs[0])));
        assert(t.open(argv[1],err,io));
        {PleTable::Request abandoned;assert(t.issue(abandoned,rows[0].data()));} // destructor drains
        std::array<int32_t,2> prev{-1,-1};int32_t token=5;std::array<uint32_t,PLE_N_HEADS> a{},b{};
        const auto c=ple_artifact_consts();ngram_rows(&token,prev.data(),1,c,a.data());
        prev={PLE_EOS_TOKEN_ID,-1};ngram_rows(&token,prev.data(),1,c,b.data());assert(a==b);
    }
    std::cout<<"PASS PLE single owner/C4 independent tickets/direct-mmap-ram parity/close-drain/reopen/duplicate/EOS\n";
}
