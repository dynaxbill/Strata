// Standalone memory safety/Graph test for the private hybrid spill copies; no model or math kernels.
#include "strata/core/p0_pcie_spill.hpp"
#include <cuda_runtime.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#define CK(expression) do { const auto result=(expression); if(result!=cudaSuccess) throw std::runtime_error(std::string(#expression)+": "+cudaGetErrorString(result)); } while(0)
static void require(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
int main() { try {
    using namespace strata::core;
    unsigned checks=0;
    int devices=0;CK(cudaGetDeviceCount(&devices));require(devices==2,"Expected original two-GPU test profile");
    for(int device=0;device<devices;++device) {
    CK(cudaSetDevice(device));
    cudaStream_t stream;CK(cudaStreamCreateWithFlags(&stream,cudaStreamNonBlocking));
    for(int capacity : {8,16}) for(int bytes : {16,256,4096}) {
        constexpr int max_groups=64;constexpr size_t guard=128;
        uint8_t *host=nullptr,*alias=nullptr,*raw=nullptr;
        unsigned long long* pointers=nullptr;int32_t* count=nullptr;
        CK(cudaHostAlloc((void**)&host,(size_t)max_groups*bytes,cudaHostAllocMapped|cudaHostAllocPortable));
        CK(cudaHostGetDevicePointer((void**)&alias,host,0));
        const size_t allocated=2*guard+(size_t)capacity*bytes;
        CK(cudaMalloc((void**)&raw,allocated));
        CK(cudaMalloc((void**)&pointers,max_groups*sizeof(unsigned long long)));
        CK(cudaMalloc((void**)&count,sizeof(int32_t)));
        std::vector<unsigned long long> original(max_groups),actual(max_groups);
        for(int i=0;i<max_groups;++i)original[i]=(unsigned long long)(alias+(size_t)i*bytes);
        std::vector<uint8_t> readback(allocated);
        cudaGraph_t graph;cudaGraphExec_t executable;
        CK(cudaStreamBeginCapture(stream,cudaStreamCaptureModeThreadLocal));
        p0_pcie_spill_fetch(pointers,count,raw+guard,bytes,capacity,stream);
        p0_pcie_spill_rebase(pointers,count,raw+guard,bytes,capacity,stream);
        CK(cudaStreamEndCapture(stream,&graph));CK(cudaGraphInstantiate(&executable,graph,0));
        for(int pattern : {0,1}) for(int n : {0,1,7,8,9,15,16,17,23,60,64}) {
            for(int i=0;i<max_groups;++i)for(int j=0;j<bytes;++j)
                host[(size_t)i*bytes+j]=(uint8_t)(i*19+j*7+pattern*31);
            CK(cudaMemcpyAsync(pointers,original.data(),max_groups*8,cudaMemcpyHostToDevice,stream));
            CK(cudaMemcpyAsync(count,&n,4,cudaMemcpyHostToDevice,stream));
            CK(cudaMemsetAsync(raw,0xa7,allocated,stream));
            CK(cudaGraphLaunch(executable,stream));CK(cudaStreamSynchronize(stream));
            CK(cudaMemcpy(readback.data(),raw,allocated,cudaMemcpyDeviceToHost));
            CK(cudaMemcpy(actual.data(),pointers,max_groups*8,cudaMemcpyDeviceToHost));
            const int staged=std::min(n,capacity);
            require(std::all_of(readback.begin(),readback.begin()+guard,[](uint8_t v){return v==0xa7;}),"prefix canary modified");
            require(std::all_of(readback.end()-guard,readback.end(),[](uint8_t v){return v==0xa7;}),"suffix canary modified");
            require(std::memcmp(readback.data()+guard,host,(size_t)staged*bytes)==0,"staged byte mismatch");
            require(std::all_of(readback.begin()+guard+(size_t)staged*bytes,readback.end()-guard,[](uint8_t v){return v==0xa7;}),"unowned staging bytes modified");
            for(int i=0;i<max_groups;++i)require(actual[i]==(i<staged?(unsigned long long)(raw+guard+(size_t)i*bytes):original[i]),"pointer prefix/overflow alias mismatch");
            ++checks;
        }
        CK(cudaGraphExecDestroy(executable));CK(cudaGraphDestroy(graph));
        CK(cudaFree(raw));CK(cudaFree(pointers));CK(cudaFree(count));CK(cudaFreeHost(host));
    }
    CK(cudaStreamDestroy(stream));
    }
    std::cout<<"{\"status\":\"GPU_GRAPH_COPY_CANARIES_AND_OVERFLOW_ALIASES_EXACT\",\"checks\":"<<checks<<",\"devices\":"<<devices<<",\"model_used\":false}"<<std::endl;
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<std::endl;return 1; } }
