#include "gradzatz_invert.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>

namespace gradzatz {

static_assert(sizeof(int) == 4 && sizeof(float) == 4,
              "gradzatz DTL views require 4-byte indices and values");

Data make_data(uint32_t corners, uint32_t padding, uint32_t distance,
               uint32_t active_percent, uint32_t seed) {
    const uint64_t size = uint64_t(corners) + 2ULL * padding;
    // The DTL frontend uses signed 32-bit sizes and indices.
    if (corners == 0 || distance > padding || active_percent > 100 ||
        size > uint64_t(std::numeric_limits<int>::max()) / sizeof(float) - 1024) {
        throw std::invalid_argument("Invalid gradzatz dimensions/locality/active percentage");
    }
    const int count = static_cast<int>(size);
    Data data;
    data.c_to_p_map.resize(corners);
    data.corner_volume.resize(corners);
    data.point_gradient.resize(count);
    data.zone_type.resize(count);
    data.z_to_c_indptr.assign(count + 1, 0);
    data.z_to_c_indices.resize(corners);
    std::vector<int> corner_zone(corners);
    std::mt19937 rng(seed);
    for (int i = 0; i < count; ++i) {
        data.zone_type[i] = rng() % 100 < active_percent ? 1 : -1;
        // Nonuniform, exactly representable inputs expose incorrect gathers.
        data.point_gradient[i] = (static_cast<int>(rng() % 2049) - 1024) / 64.0f;
    }
    for (uint32_t c = 0; c < corners; ++c) {
        const int center = static_cast<int>(c + distance);
        const auto offset = [&]() {
            return static_cast<int>(rng() % (2ULL * distance + 1)) -
                   static_cast<int>(distance);
        };
        data.c_to_p_map[c] = center + offset();
        corner_zone[c] = center + offset();
        data.corner_volume[c] = (1 + rng() % 64) / 8.0f;
        ++data.z_to_c_indptr[corner_zone[c] + 1];
    }
    for (int z = 0; z < count; ++z)
        data.z_to_c_indptr[z + 1] += data.z_to_c_indptr[z];
    auto cursor = data.z_to_c_indptr;
    for (uint32_t c = 0; c < corners; ++c)
        data.z_to_c_indices[cursor[corner_zone[c]]++] = static_cast<int>(c);
    return data;
}

void gradzatz_invert_CSR(const Data& data, float* output) {
    for (size_t z = 0; z < data.zone_type.size(); ++z) {
        output[z] = 0.0f;
        if (data.zone_type[z] < 1)
            continue;
        const int begin = data.z_to_c_indptr[z];
        const int end = data.z_to_c_indptr[z + 1];
        double zone_vol = 0.0;
        for (int k = begin; k < end; ++k)
            zone_vol += data.corner_volume[data.z_to_c_indices[k]];
        // Empty zones execute neither inner loop, so no division by zero.
        for (int k = begin; k < end; ++k) {
            const int c = data.z_to_c_indices[k];
            const int p = data.c_to_p_map[c];
            const double ratio = data.corner_volume[c] / zone_vol;
            output[z] += data.point_gradient[p] * ratio;
        }
    }
}

void gradzatz_invert_DTU(const Data& data, const float* volume_view,
                         const float* gradient_view, float* output) {
    for (size_t z = 0; z < data.zone_type.size(); ++z) {
        output[z] = 0.0f;
        if (data.zone_type[z] < 1)
            continue;
        const int begin = data.z_to_c_indptr[z];
        const int end = data.z_to_c_indptr[z + 1];
        double zone_vol = 0.0;
        for (int k = begin; k < end; ++k)
            zone_vol += volume_view[k];
        for (int k = begin; k < end; ++k) {
            const double ratio = volume_view[k] / zone_vol;
            output[z] += gradient_view[k] * ratio;
        }
    }
}

DTLPrograms make_dtl_programs(const Data& data) {
    // Byte sizes are page aligned for mmap; loop counts are element counts,
    // matching composedTest. DTL's out expression selects a 4-byte element.
    const auto bytes = [](size_t elements) {
        return ((elements * sizeof(float) + 4095) / 4096) * 4096;
    };
    const uint32_t n = static_cast<uint32_t>(data.z_to_c_indices.size());
    const uint32_t stream = ((n + 15) / 16) * 16;
    const auto corner_bytes = bytes(data.corner_volume.size());
    const auto point_bytes = bytes(data.point_gradient.size());
    const auto metadata_bytes = bytes(stream);
    std::ostringstream volume;
    volume << "DTL<" << corner_bytes << "> volumes = {\n"
           << "  metadata_stream<4, " << metadata_bytes << "> corners;\n"
           << "  for (int i = 0; i < " << stream << "; i++) { out = corners[i]; }\n"
           << "}\nactive volumes;\n";
    std::ostringstream gradient;
    gradient << "DTL<" << corner_bytes << "> point_ids = {\n"
             << "  metadata_stream<4, " << metadata_bytes << "> corners;\n"
             << "  for (int i = 0; i < " << stream << "; i++) { out = corners[i]; }\n"
             << "}\nDTL<" << point_bytes << "> gradients : point_ids = {\n"
             << "  metadata_stream<4, point_ids> points;\n"
             << "  for (int i = 0; i < " << stream << "; i++) { out = points[i]; }\n"
             << "}\nactive gradients;\n";
    return {volume.str(), gradient.str(), stream};
}

void verify_results(const std::vector<float>& expected,
                    const std::vector<float>& actual) {
    if (expected.size() != actual.size())
        throw std::runtime_error("gradzatz output size mismatch");
    for (size_t z = 0; z < expected.size(); ++z) {
        if (!std::isfinite(expected[z]) || !std::isfinite(actual[z]) ||
            std::fabs(expected[z] - actual[z]) > 1e-5f + 1e-4f * std::fabs(expected[z])) {
            throw std::runtime_error("gradzatz mismatch at zone " + std::to_string(z) +
                                     ": cpu=" + std::to_string(expected[z]) +
                                     " dtu=" + std::to_string(actual[z]));
        }
    }
}

} // namespace gradzatz
