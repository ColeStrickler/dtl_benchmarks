#ifndef GRADZATZ_INVERT_HPP
#define GRADZATZ_INVERT_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace gradzatz {

struct Data {
    std::vector<int> z_to_c_indptr;
    std::vector<int> z_to_c_indices;
    std::vector<int> c_to_p_map;
    std::vector<int> zone_type;
    std::vector<float> corner_volume;
    std::vector<float> point_gradient;
};

// Retain the original padded, bounded-distance corner-to-zone/point topology.
Data make_data(uint32_t corners, uint32_t padding, uint32_t distance,
               uint32_t active_percent, uint32_t seed);

// Both kernels overwrite every output zone; setup and allocation are external.
void gradzatz_invert_CSR(const Data& data, float* output);
void gradzatz_invert_DTU(const Data& data, const float* volume_view,
                         const float* gradient_view, float* output);

struct DTLPrograms {
    std::string volume;
    std::string gradient;
    uint32_t stream_elements;
};

// Stream tails are padded to a complete 64-byte line with valid corner zero.
DTLPrograms make_dtl_programs(const Data& data);

// Throws with the first mismatching zone; rejects nonfinite values as well.
void verify_results(const std::vector<float>& expected,
                    const std::vector<float>& actual);

} // namespace gradzatz

#endif
