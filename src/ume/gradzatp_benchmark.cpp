#include "benchmarks.hpp"
#include "gradzatp_invert.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
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

void initialize_corners(DTL::DTUResourceHandle* handle, const gradzatp::Data& data,
                        uint32_t stream_elements) {
    auto* metadata = handle->m_MetaDataRegions.at("corners");
    if (!metadata->isNormalMemoryRegion() ||
        metadata->m_RegionSize < stream_elements * sizeof(int))
        throw std::runtime_error("Invalid gradzatp corner metadata region");
    auto* write = static_cast<int*>(metadata->GetMetadataWriteRegion());
    std::copy(data.p_to_c_indices.begin(), data.p_to_c_indices.end(), write);
    std::fill(write + data.p_to_c_indices.size(), write + stream_elements, 0);
}

// A plain sum would be NaN for the original empty-point inputs. Hash both
// arrays instead, canonicalizing only NaN payloads; arithmetic is unaffected.
uint64_t checksum(const gradzatp::Output& output) {
    uint64_t hash = 14695981039346656037ULL;
    for (const auto* values : {&output.point_volume, &output.point_gradient}) {
        for (float value : *values) {
            uint32_t bits;
            std::memcpy(&bits, &value, sizeof(bits));
            if (std::isnan(value))
                bits = 0x7fc00000;
            for (int byte = 0; byte < 4; ++byte) {
                hash ^= (bits >> (8 * byte)) & 0xff;
                hash *= 1099511628211ULL;
            }
        }
    }
    return hash;
}

} // namespace

std::string benchmark::bench_wrapper_gradzatp_invert(const BenchmarkData& bench_data,
                                                     DTL::API* api) {
    const auto data = gradzatp::make_data(
        bench_data.params.at("CORNERS"), bench_data.other.at("PADDING"),
        bench_data.other.at("DISTANCE"), bench_data.other.at("ACTIVE_PERCENT"),
        bench_data.other.at("SEED"));
    const auto programs = gradzatp::make_dtl_programs(data);
    const auto start = std::chrono::steady_clock::now();
    auto volume_prog = api->CompileProgram(programs.volume);
    auto surface_prog = api->CompileProgram(programs.surface);
    auto field_prog = api->CompileProgram(programs.field);
    if (volume_prog.m_Error != DTL::NO_ERROR_OK ||
        surface_prog.m_Error != DTL::NO_ERROR_OK || field_prog.m_Error != DTL::NO_ERROR_OK)
        throw std::runtime_error("Failed to compile gradzatp DTL programs");
    DTL::CompiledDTLProgram combined;
    combined.m_NeededConfigs = volume_prog.m_NeededConfigs +
        surface_prog.m_NeededConfigs + field_prog.m_NeededConfigs;
    if (!api->CheckHasAvailableResources(combined))
        throw std::runtime_error("Insufficient DTU configurations for gradzatp");
    Handle volumes(api->MapToDTUHardware(volume_prog), HandleDeleter{api});
    if (!volumes)
        throw std::runtime_error("Failed to map gradzatp volumes");
    Handle surfaces(api->MapToDTUHardware(surface_prog), HandleDeleter{api});
    if (!surfaces)
        throw std::runtime_error("Failed to map gradzatp surfaces");
    Handle fields(api->MapToDTUHardware(field_prog), HandleDeleter{api});
    if (!fields)
        throw std::runtime_error("Failed to map gradzatp fields");
    volumes->m_MainRegion->SetDataSize(sizeof(float));
    auto* zone_ids = fields->m_SubHandles.at("zone_ids");
    for (auto* handle : {volumes.get(), surfaces.get(), zone_ids})
        initialize_corners(handle, data, programs.stream_elements);
    std::memcpy(volumes->GetWriteRegion(), data.corner_volume.data(),
                data.corner_volume.size() * sizeof(float));
    std::memcpy(surfaces->GetWriteRegion(), data.csurf.data(),
                data.csurf.size() * sizeof(float));
    std::memcpy(zone_ids->GetWriteRegion(), data.c_to_z_map.data(),
                data.c_to_z_map.size() * sizeof(int));
    std::memcpy(fields->GetWriteRegion(), data.zone_field.data(),
                data.zone_field.size() * sizeof(float));
    DTL::DeviceMemoryFence();
    const auto setup_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();

    // The original clears both outputs before the ROI. Keep that work outside
    // BOTH timed intervals, with a separately initialized output per version.
    auto expected = gradzatp::make_output(data);
    auto actual = gradzatp::make_output(data);
    PerfManager perf;
    perf.ClearCounters();
    perf.CollectCounters();
    gradzatp::gradzatp_invert_CSR(data, expected);
    perf.CollectDelta();
    const std::string cpu_counters = perf.PrintCounters();

    perf.ClearCounters();
    DTL::DeviceMemoryFence();
    perf.CollectCounters();
    gradzatp::gradzatp_invert_DTU(data,
        static_cast<const float*>(volumes->GetReadRegion()),
        static_cast<const float*>(surfaces->GetReadRegion()),
        static_cast<const float*>(fields->GetReadRegion()), actual);
    perf.CollectDelta();
    const std::string dtu_counters = perf.PrintCounters();
    gradzatp::verify_results(expected, actual);

    const std::string name = "gradzatp_invert_c" +
        std::to_string(data.corner_volume.size()) + "_p" +
        std::to_string(data.point_type.size()) + "_d" +
        std::to_string(bench_data.other.at("DISTANCE")) + "_a" +
        std::to_string(bench_data.other.at("ACTIVE_PERCENT")) + "_s" +
        std::to_string(bench_data.other.at("SEED"));
    const auto nans = std::count_if(actual.point_gradient.begin(), actual.point_gradient.end(),
                                  [](float value) { return std::isnan(value); });
    std::cerr << name << ": verified both outputs for " << actual.point_volume.size()
              << " points (gradient NaNs=" << nans << "); DTU setup_us=" << setup_us << "\n";
    return name + ",cpu,0," + cpu_counters + std::to_string(checksum(expected)) + "\n" +
           name + ",dtu,0," + dtu_counters + std::to_string(checksum(actual)) + "\n";
}
