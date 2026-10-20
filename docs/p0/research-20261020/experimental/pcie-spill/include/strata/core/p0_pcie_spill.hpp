#pragma once
#include <cstdlib>
#include <cstdint>
namespace strata::core {
inline bool p0_pcie_spill_enabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("STRATA_P0_PCIE_SPILL");
        return value && value[0] == '1' && value[1] == '\0';
    }();
    return enabled;
}
// Only memory movement and address updates. All expert arithmetic stays in the original kernels.
void p0_pcie_spill_fetch(const unsigned long long* pointers, const int32_t* count,
                        uint8_t* staging, int64_t bytes, int physical_capacity, void* stream);
void p0_pcie_spill_rebase(unsigned long long* pointers, const int32_t* count,
                         uint8_t* staging, int64_t bytes, int physical_capacity, void* stream);
}
