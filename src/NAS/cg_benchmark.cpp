#include "benchmarks.hpp"
#include "cg_dtu.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <memory>
#include <stdexcept>

namespace {
struct HandleDeleter {
    DTL::API* api;
    void operator()(DTL::DTUResourceHandle* handle) const {
        api->FreeDTUResourceHandle(handle);
        delete handle;
    }
};
using Handle = std::unique_ptr<DTL::DTUResourceHandle, HandleDeleter>;
}

std::string benchmark::bench_wrapper_nas_cg(const BenchmarkData& bench_data, DTL::API* api) {
    nas_cg::Config config;
    config.rows = bench_data.params.at("ROWS");
    config.nonzer = bench_data.other.at("NONZER");
    config.cg_iterations = bench_data.other.at("CGITMAX");
    config.iterations = bench_data.other.at("NITER");
    config.shift = bench_data.other.at("SHIFT");
    const auto data = nas_cg::make_data(config);
    const auto source = nas_cg::make_dtl_program(data);
    const auto start = std::chrono::steady_clock::now();
    auto program = api->CompileProgram(source.source, sizeof(float));
    if (program.m_Error != DTL::NO_ERROR_OK)
        throw std::runtime_error("Failed to compile NAS CG vector gather");
    if (!api->CheckHasAvailableResources(program))
        throw std::runtime_error("Insufficient DTU configurations for NAS CG");
    Handle handle(api->MapToDTUHardware(program), HandleDeleter{api});
    if (!handle || handle->GetLogicalObjectSize() < uint64_t(source.stream_elements) * sizeof(float) ||
        handle->GetPositionalObjectSize() < uint64_t(config.rows) * sizeof(float))
        throw std::runtime_error("Failed to map NAS CG vector gather");
    handle->m_MainRegion->SetDataSize(sizeof(float));
    auto* metadata = handle->m_MetaDataRegions.at("columns");
    if (!metadata->isNormalMemoryRegion() ||
        metadata->m_RegionSize < uint64_t(source.stream_elements) * sizeof(int))
        throw std::runtime_error("Invalid NAS CG column metadata region");
    auto* indices = static_cast<int*>(metadata->GetMetadataWriteRegion());
    std::copy(data.colidx.begin(), data.colidx.end(), indices);
    std::fill(indices + data.colidx.size(), indices + source.stream_elements, 0);
    DTL::DeviceMemoryFence();
    const auto setup_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();

    auto expected = nas_cg::make_output(data);
    auto actual = nas_cg::make_output(data);
    nas_cg::HardwareVectorGather gather(handle.get(), config.rows);
    PerfManager perf;
    perf.ClearCounters();
    perf.CollectCounters();
    nas_cg::run_cpu(data, expected);
    perf.CollectDelta();
    const auto cpu_counters = perf.PrintCounters();
    perf.ClearCounters();
    DTL::DeviceMemoryFence();
    perf.CollectCounters();
    nas_cg::run_dtu(data, gather, actual);
    perf.CollectDelta();
    const auto dtu_counters = perf.PrintCounters();
    nas_cg::verify_results(data, expected, actual);

    const auto name = "nas_cg_fp32_n" + std::to_string(config.rows) + "_nz" +
        std::to_string(data.a.size()) + "_nonzer" + std::to_string(config.nonzer) +
        "_cg" + std::to_string(config.cg_iterations) + "_it" + std::to_string(config.iterations) +
        "_shift" + std::to_string(bench_data.other.at("SHIFT"));
    std::cerr << std::setprecision(12) << name << ": verified all vectors and iteration scalars; rnorm="
              << actual.rnorm_history.back() << " zeta=" << actual.zeta_history.back()
              << " DTU setup_us=" << setup_us << " timed_syncs=" << gather.syncs
              << " timed_published_elements=" << gather.published_elements << "\n";
    return name + ",cpu,0," + cpu_counters + std::to_string(nas_cg::checksum(expected)) + "\n" +
           name + ",dtu,0," + dtu_counters + std::to_string(nas_cg::checksum(actual)) + "\n";
}
