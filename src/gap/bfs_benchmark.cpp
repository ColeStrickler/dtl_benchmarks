#include "benchmarks.hpp"
#include "bfs_dtu.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>

namespace {
struct HandleDeleter {
    DTL::API* api;
    void operator()(DTL::DTUResourceHandle* handle) const {
        api->FreeDTUResourceHandle(handle);
    }
};
using Handle = std::unique_ptr<DTL::DTUResourceHandle, HandleDeleter>;

} // namespace

std::string benchmark::bench_wrapper_bfs(const BenchmarkData& bench_data, DTL::API* api) {
    const auto data = gap_bfs::make_data(bench_data.params.at("SCALE"),
        bench_data.other.at("DEGREE"), bench_data.other.at("UNIFORM") != 0,
        bench_data.other.at("SOURCE"), bench_data.input_artifact);
    const auto source = gap_bfs::make_dtl_program(data);
    const auto start = std::chrono::steady_clock::now();
    auto program = api->CompileProgram(source.source);
    if (program.m_Error != DTL::NO_ERROR_OK)
        throw std::runtime_error("Failed to compile BFS DTL program");
    if (!api->CheckHasAvailableResources(program))
        throw std::runtime_error("Insufficient DTU configurations for BFS");
    Handle parents(api->MapToDTUHardware(program), HandleDeleter{api});
    if (!parents)
        throw std::runtime_error("Failed to map BFS parents");
    parents->m_MainRegion->SetDataSize(sizeof(gap_bfs::Node));
    auto* metadata = parents->m_MetaDataRegions.at("neighbors");
    if (!metadata->isNormalMemoryRegion() ||
        metadata->m_RegionSize < uint64_t(source.stream_elements) * sizeof(gap_bfs::Node))
        throw std::runtime_error("Invalid BFS neighbor metadata region");
    auto* write = static_cast<gap_bfs::Node*>(metadata->GetMetadataWriteRegion());
    std::copy(data.neighbors.begin(), data.neighbors.end(), write);
    std::fill(write + data.neighbors.size(), write + source.stream_elements, 0);
    DTL::DeviceMemoryFence();
    const auto setup_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();

    auto expected = gap_bfs::make_output(data);
    auto actual = gap_bfs::make_output(data);
    gap_bfs::HardwareParentGather gather(parents.get());
    PerfManager perf;
    perf.ClearCounters();
    perf.CollectCounters();
    gap_bfs::bfs_cpu(data, expected);
    perf.CollectDelta();
    const std::string cpu_counters = perf.PrintCounters();

    perf.ClearCounters();
    DTL::DeviceMemoryFence();
    perf.CollectCounters();
    gap_bfs::bfs_dtu(data, gather, actual);
    perf.CollectDelta();
    const std::string dtu_counters = perf.PrintCounters();
    gap_bfs::verify_results(data, expected, actual);

    const std::string input = bench_data.input_artifact.empty() ?
        (bench_data.other.at("UNIFORM") ? "uniform" : "rmat") : "file";
    const std::string name = "bfs_td_" + input + "_n" +
        std::to_string(actual.parent.size()) + "_e" + std::to_string(data.neighbors.size()) +
        "_r" + std::to_string(data.source);
    const auto visited = std::count_if(actual.parent.begin(), actual.parent.end(),
                                     [](gap_bfs::Node p) { return p >= 0; });
    std::cerr << name << ": verified exact parents and shortest-path tree; visited=" << visited
              << " levels=" << actual.levels << " DTU setup_us=" << setup_us
              << " timed_snapshot_entries=" << gather.published_entries
              << " timed_syncs=" << gather.syncs << "\n";
    return name + ",cpu,0," + cpu_counters + std::to_string(gap_bfs::checksum(expected)) + "\n" +
           name + ",dtu,0," + dtu_counters + std::to_string(gap_bfs::checksum(actual)) + "\n";
}
