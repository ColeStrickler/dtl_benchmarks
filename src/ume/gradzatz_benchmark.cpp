#include "benchmarks.hpp"
#include "gradzatz_invert.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace {

// The root owns its composed producer; free it once through the API.
struct HandleDeleter {
    DTL::API* api;
    void operator()(DTL::DTUResourceHandle* handle) const {
        api->FreeDTUResourceHandle(handle);
        delete handle;
    }
};
using Handle = std::unique_ptr<DTL::DTUResourceHandle, HandleDeleter>;

void initialize_corners(DTL::DTUResourceHandle* handle, const gradzatz::Data& data,
                        uint32_t stream_elements) {
    auto* metadata = handle->m_MetaDataRegions.at("corners");
    if (!metadata->isNormalMemoryRegion() ||
        metadata->m_RegionSize < stream_elements * sizeof(int))
        throw std::runtime_error("Invalid gradzatz corner metadata region");
    auto* write = static_cast<int*>(metadata->GetMetadataWriteRegion());
    std::copy(data.z_to_c_indices.begin(), data.z_to_c_indices.end(), write);
    // Every entry in the last cache line must index valid backing data.
    std::fill(write + data.z_to_c_indices.size(), write + stream_elements, 0);
}

double checksum(const std::vector<float>& output) {
    double sum = 0.0;
    for (float value : output)
        sum += value;
    return sum;
}

} // namespace

std::string benchmark::bench_wrapper_gradzatz_invert(const BenchmarkData& bench_data,
                                                     DTL::API* api) {
    const auto data = gradzatz::make_data(
        bench_data.params.at("CORNERS"), bench_data.other.at("PADDING"),
        bench_data.other.at("DISTANCE"), bench_data.other.at("ACTIVE_PERCENT"),
        bench_data.other.at("SEED"));
    const auto programs = gradzatz::make_dtl_programs(data);
    const auto start = std::chrono::steady_clock::now();
    auto volume_prog = api->CompileProgram(programs.volume);
    auto gradient_prog = api->CompileProgram(programs.gradient);
    if (volume_prog.m_Error != DTL::NO_ERROR_OK ||
        gradient_prog.m_Error != DTL::NO_ERROR_OK)
        throw std::runtime_error("Failed to compile gradzatz DTL programs");

    // Check the combined requirement before mapping either program.
    DTL::CompiledDTLProgram combined;
    combined.m_NeededConfigs = volume_prog.m_NeededConfigs + gradient_prog.m_NeededConfigs;
    if (!api->CheckHasAvailableResources(combined))
        throw std::runtime_error("Insufficient DTU configurations for gradzatz");
    Handle volumes(api->MapTODTUHardware(volume_prog), HandleDeleter{api});
    if (!volumes)
        throw std::runtime_error("Failed to map gradzatz volumes");
    Handle gradients(api->MapTODTUHardware(gradient_prog), HandleDeleter{api});
    if (!gradients)
        throw std::runtime_error("Failed to map gradzatz gradients");
    volumes->m_MainRegion->SetDataSize(sizeof(float));
    auto* point_ids = gradients->m_SubHandles.at("point_ids");
    initialize_corners(volumes.get(), data, programs.stream_elements);
    initialize_corners(point_ids, data, programs.stream_elements);
    std::memcpy(volumes->GetWriteRegion(), data.corner_volume.data(),
                data.corner_volume.size() * sizeof(float));
    std::memcpy(point_ids->GetWriteRegion(), data.c_to_p_map.data(),
                data.c_to_p_map.size() * sizeof(int));
    std::memcpy(gradients->GetWriteRegion(), data.point_gradient.data(),
                data.point_gradient.size() * sizeof(float));
    DTL::DeviceMemoryFence();
    const auto setup_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();

    std::vector<float> expected(data.zone_type.size());
    std::vector<float> actual(data.zone_type.size());
    PerfManager perf;
    perf.ClearCounters();
    perf.CollectCounters();
    gradzatz::gradzatz_invert_CSR(data, expected.data());
    perf.CollectDelta();
    const std::string cpu_counters = perf.PrintCounters();

    perf.ClearCounters();
    DTL::DeviceMemoryFence();
    perf.CollectCounters();
    gradzatz::gradzatz_invert_DTU(data,
        static_cast<const float*>(volumes->GetReadRegion()),
        static_cast<const float*>(gradients->GetReadRegion()), actual.data());
    perf.CollectDelta();
    const std::string dtu_counters = perf.PrintCounters();

    // A checksum is useful in the CSV, but never replaces per-zone validation.
    gradzatz::verify_results(expected, actual);
    const std::string name = "gradzatz_invert_c" +
        std::to_string(data.corner_volume.size()) + "_z" +
        std::to_string(data.zone_type.size()) + "_d" +
        std::to_string(bench_data.other.at("DISTANCE")) + "_a" +
        std::to_string(bench_data.other.at("ACTIVE_PERCENT")) + "_s" +
        std::to_string(bench_data.other.at("SEED"));
    std::cerr << name << ": verified " << actual.size()
              << " zones; DTU setup_us=" << setup_us << "\n";
    // PerfManager already emits a trailing comma. transform_cost stays zero:
    // these are demand-gather views, with setup_us reported separately above.
    return name + ",cpu,0," + cpu_counters + std::to_string(checksum(expected)) + "\n" +
           name + ",dtu,0," + dtu_counters + std::to_string(checksum(actual)) + "\n";
}
