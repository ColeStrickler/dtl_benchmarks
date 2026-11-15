#include "gradzatp_invert.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace gradzatp {

static_assert(sizeof(int) == 4 && sizeof(float) == 4,
              "gradzatp DTL views require 4-byte indices and values");

Data make_data(uint32_t corners, uint32_t padding, uint32_t distance,
               uint32_t active_percent, uint32_t seed) {
    const uint64_t count = uint64_t(corners) + 2ULL * padding;
    if (corners == 0 || distance > padding || active_percent > 100 ||
        count > uint64_t(std::numeric_limits<int>::max()) / sizeof(float) - 1024)
        throw std::invalid_argument("Invalid gradzatp dimensions/locality/active percentage");
    const int num_points = static_cast<int>(count);
    Data data;
    data.c_to_z_map.resize(corners);
    data.c_to_p_map.resize(corners);
    data.p_to_c_indptr.assign(num_points + 1, 0);
    data.p_to_c_indices.resize(corners);
    data.corner_volume.assign(corners, 1.0f);
    data.csurf.assign(corners, 1.0f);
    data.zone_field.assign(num_points, 1.0f);
    data.point_normal.assign(num_points, 1.0f);
    data.point_type.resize(num_points);

    // Match the original input generator, including its rand() draw order.
    std::srand(seed);
    for (int i = 0; i < num_points; ++i)
        data.point_type[i] = (std::rand() % 100 < static_cast<int>(active_percent)) ? 1 : -1;
    const int span = static_cast<int>(2 * distance + 1);
    for (uint32_t idx = 0; idx < corners; ++idx) {
        const int c = static_cast<int>(idx + distance);
        int rand_offset = std::rand() % span - static_cast<int>(distance);
        data.c_to_p_map[idx] = c + rand_offset;
        rand_offset = std::rand() % span - static_cast<int>(distance);
        data.c_to_z_map[idx] = c + rand_offset;
        ++data.p_to_c_indptr[data.c_to_p_map[idx] + 1];
    }
    for (int p = 0; p < num_points; ++p)
        data.p_to_c_indptr[p + 1] += data.p_to_c_indptr[p];
    auto cursor = data.p_to_c_indptr;
    for (uint32_t c = 0; c < corners; ++c)
        data.p_to_c_indices[cursor[data.c_to_p_map[c]]++] = static_cast<int>(c);
    return data;
}

Output make_output(const Data& data) {
    return {std::vector<float>(data.point_type.size(), 0.0f),
            std::vector<float>(data.point_type.size(), 0.0f)};
}

void gradzatp_invert_CSR(const Data& data, Output& output) {
    const int num_points = static_cast<int>(output.point_volume.size());
    // Original first pass: accumulate ALL points, regardless of point_type.
    for (int p = 0; p < num_points; ++p) {
        for (int idx = data.p_to_c_indptr[p]; idx < data.p_to_c_indptr[p + 1]; ++idx) {
            int c = data.p_to_c_indices[idx];
            int z = data.c_to_z_map[c];
            output.point_volume[p] += data.corner_volume[c];
            output.point_gradient[p] += data.csurf[c] * data.zone_field[z];
        }
    }
    // Original separate second pass, including zero-volume division behavior.
    for (int p = 0; p < num_points; ++p) {
        if (data.point_type[p] > 0) {
            output.point_gradient[p] /= output.point_volume[p];
        } else if (data.point_type[p] == -1) {
            double ppdot = output.point_gradient[p] * data.point_normal[p];
            output.point_gradient[p] = (output.point_gradient[p] - data.point_normal[p] * ppdot) / output.point_volume[p];
        }
    }
}

void gradzatp_invert_DTU(const Data& data, const float* volume_view,
                        const float* surface_view, const float* field_view,
                        Output& output) {
    const int num_points = static_cast<int>(output.point_volume.size());
    for (int p = 0; p < num_points; ++p) {
        for (int idx = data.p_to_c_indptr[p]; idx < data.p_to_c_indptr[p + 1]; ++idx) {
            // Only these three input reads change; CSR order and float
            // multiply/add accumulation are identical to the reference.
            output.point_volume[p] += volume_view[idx];
            output.point_gradient[p] += surface_view[idx] * field_view[idx];
        }
    }
    for (int p = 0; p < num_points; ++p) {
        if (data.point_type[p] > 0) {
            output.point_gradient[p] /= output.point_volume[p];
        } else if (data.point_type[p] == -1) {
            // The multiply on the RHS is float, then promoted to double.
            double ppdot = output.point_gradient[p] * data.point_normal[p];
            output.point_gradient[p] = (output.point_gradient[p] - data.point_normal[p] * ppdot) / output.point_volume[p];
        }
    }
}

DTLPrograms make_dtl_programs(const Data& data) {
    const auto bytes = [](size_t n) { return ((n * 4 + 4095) / 4096) * 4096; };
    const uint32_t n = static_cast<uint32_t>(data.p_to_c_indices.size());
    const uint32_t stream = ((n + 15) / 16) * 16;
    const auto corner_bytes = bytes(data.corner_volume.size());
    const auto metadata_bytes = bytes(stream);
    const auto gather = [&](const std::string& name) {
        std::ostringstream program;
        program << "DTL<" << corner_bytes << "> " << name << " = {\n"
                << "  metadata_stream<4, " << metadata_bytes << "> corners;\n"
                << "  for (int i = 0; i < " << stream << "; i++) { out = corners[i]; }\n"
                << "}\nactive " << name << ";\n";
        return program.str();
    };
    std::ostringstream field;
    field << "DTL<" << corner_bytes << "> zone_ids = {\n"
          << "  metadata_stream<4, " << metadata_bytes << "> corners;\n"
          << "  for (int i = 0; i < " << stream << "; i++) { out = corners[i]; }\n"
          << "}\nDTL<" << bytes(data.zone_field.size()) << "> fields : zone_ids = {\n"
          << "  metadata_stream<4, zone_ids> zones;\n"
          << "  for (int i = 0; i < " << stream << "; i++) { out = zones[i]; }\n"
          << "}\nactive fields;\n";
    return {gather("volumes"), gather("surfaces"), field.str(), stream};
}

void verify_results(const Output& expected, const Output& actual) {
    const auto compare = [](const std::vector<float>& reference,
                            const std::vector<float>& result, const char* name) {
        if (reference.size() != result.size())
            throw std::runtime_error(std::string("gradzatp size mismatch: ") + name);
        for (size_t p = 0; p < reference.size(); ++p) {
            if (std::isnan(reference[p]) && std::isnan(result[p]))
                continue;
            uint32_t lhs, rhs;
            std::memcpy(&lhs, &reference[p], sizeof(lhs));
            std::memcpy(&rhs, &result[p], sizeof(rhs));
            if (lhs != rhs)
                throw std::runtime_error(std::string("gradzatp mismatch in ") + name +
                    " at point " + std::to_string(p) + ": cpu=" +
                    std::to_string(reference[p]) + " dtu=" + std::to_string(result[p]));
        }
    };
    compare(expected.point_volume, actual.point_volume, "point_volume");
    compare(expected.point_gradient, actual.point_gradient, "point_gradient");
}

} // namespace gradzatp
