#ifndef NAS_CG_DTU_HPP
#define NAS_CG_DTU_HPP

#include "cg.hpp"
#include "dtl_api.hpp"
#include <algorithm>
#include <stdexcept>

namespace nas_cg {

class HardwareVectorGather final : public VectorGather {
    DTL::DTUResourceHandle* handle;
    std::size_t rows;
public:
    uint64_t syncs = 0, published_elements = 0;
    HardwareVectorGather(DTL::DTUResourceHandle* resource, std::size_t n)
        : handle(resource), rows(n) {}
    const float* publish(const float* vector, std::size_t count) override {
        if (count != rows || count * sizeof(float) > handle->GetPositionalObjectSize())
            throw std::runtime_error("Invalid NAS CG publication size");
        auto* write = static_cast<float*>(handle->GetWriteRegion());
        std::copy_n(vector, count, write);
        DTL::DeviceMemoryFence();
        // p changes every CG iteration; the residual uses z instead. A fresh
        // shadow PA prevents old gathered cache lines from serving either one.
        if (handle->m_MainRegion->Sync() != 0)
            throw std::runtime_error("NAS CG vector Sync failed");
        DTL::DeviceMemoryFence();
        ++syncs;
        published_elements += count;
        return static_cast<const float*>(handle->GetReadRegion());
    }
};

} // namespace nas_cg

#endif
