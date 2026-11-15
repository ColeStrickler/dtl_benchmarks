#ifndef GAP_BFS_DTU_HPP
#define GAP_BFS_DTU_HPP
#include "bfs.hpp"
#include "dtl_api.hpp"
#include <algorithm>
#include <stdexcept>

namespace gap_bfs {
class HardwareParentGather final : public gap_bfs::ParentGather {
    DTL::DTUResourceHandle* handle;
public:
    uint64_t syncs = 0;
    uint64_t published_entries = 0;
    explicit HardwareParentGather(DTL::DTUResourceHandle* h) : handle(h) {}
    void reset(const std::vector<gap_bfs::Node>& parent) override {
        auto* write = static_cast<gap_bfs::Node*>(handle->GetWriteRegion());
        std::copy(parent.begin(), parent.end(), write);
        syncs = 0;
        published_entries = parent.size();
    }
    const gap_bfs::Node* publish(const std::vector<gap_bfs::Node>& parent,
                                const gap_bfs::Node* frontier, size_t count) override {
        auto* write = static_cast<gap_bfs::Node*>(handle->GetWriteRegion());
        // Every discovered vertex appears in exactly one frontier. Older
        // snapshot positives remain valid, so no full copy is needed here.
        for (size_t i = 0; i < count; ++i) {
            const auto u = frontier[i];
            write[u] = parent[u];
        }
        published_entries += count;
        DTL::DeviceMemoryFence();
        // Backing contents changed: obtain fresh shadow physical addresses so
        // CPU/LLC lines from an earlier level cannot return old gather values.
        // Call the checked low-level method; no dependent views exist here.
        if (handle->m_MainRegion->Sync() != 0)
            throw std::runtime_error("BFS parent snapshot Sync failed");
        DTL::DeviceMemoryFence();
        ++syncs;
        return static_cast<const gap_bfs::Node*>(handle->GetReadRegion());
    }
};
} // namespace gap_bfs
#endif
