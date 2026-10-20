// Private experimental memory-transfer object. Never change original math/kernel objects.
#include "strata/core/p0_pcie_spill.hpp"
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdlib>
namespace strata::core {
namespace {
__global__ void fetch_prefix(const unsigned long long* __restrict__ pointers,
                             const int32_t* __restrict__ count, uint4* __restrict__ staging,
                             long long vectors_per_blob, int capacity) {
    const int groups = *count < capacity ? *count : capacity;
    const long long total = (long long) groups * vectors_per_blob;
    for (long long i = (long long) blockIdx.x * blockDim.x + threadIdx.x;
         i < total; i += (long long) gridDim.x * blockDim.x) {
        const long long group = i / vectors_per_blob;
        const long long offset = i - group * vectors_per_blob;
        staging[i] = ((const uint4*) pointers[group])[offset];
    }
}
__global__ void rebase_prefix(unsigned long long* pointers, const int32_t* count,
                              unsigned long long staging, long long bytes, int capacity) {
    const int group = threadIdx.x;
    if (group < *count && group < capacity)
        pointers[group] = staging + (unsigned long long) group * (unsigned long long) bytes;
    // Overflow aliases remain mapped host pointers, and the original grouped expert kernel uses them.
}
void check_launch(const char* operation) {
    const cudaError_t result = cudaGetLastError();
    if (result != cudaSuccess) {
        std::fprintf(stderr, "P0 PCIe spill %s: %s\n", operation, cudaGetErrorString(result));
        std::exit(1);
    }
}
void validate(int64_t bytes, int capacity) {
    if (bytes <= 0 || bytes % 16 != 0 || capacity < 1 || capacity > 16) {
        std::fprintf(stderr, "P0 PCIe spill: invalid physical staging geometry\n");
        std::exit(1);
    }
}
}
void p0_pcie_spill_fetch(const unsigned long long* pointers, const int32_t* count,
                        uint8_t* staging, int64_t bytes, int capacity, void* stream) {
    validate(bytes, capacity);
    fetch_prefix<<<48 * 8, 256, 0, (cudaStream_t) stream>>>(
        pointers, count, (uint4*) staging, bytes / 16, capacity);
    check_launch("fetch");
}
void p0_pcie_spill_rebase(unsigned long long* pointers, const int32_t* count,
                         uint8_t* staging, int64_t bytes, int capacity, void* stream) {
    validate(bytes, capacity);
    rebase_prefix<<<1, 128, 0, (cudaStream_t) stream>>>(
        pointers, count, (unsigned long long) staging, bytes, capacity);
    check_launch("rebase");
}
}
