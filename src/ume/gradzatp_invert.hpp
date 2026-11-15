#ifndef GRADZATP_INVERT_HPP
#define GRADZATP_INVERT_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace gradzatp {

struct Data {
    std::vector<int> c_to_z_map;
    std::vector<int> c_to_p_map;
    std::vector<int> p_to_c_indptr;
    std::vector<int> p_to_c_indices;
    std::vector<float> corner_volume;
    std::vector<float> csurf;
    std::vector<float> zone_field;
    std::vector<float> point_normal;
    std::vector<int> point_type;
};

struct Output {
    std::vector<float> point_volume;
    std::vector<float> point_gradient;
};

// Original all-ones inputs, rand() call order and CSR construction; use a fixed
// seed instead of time(NULL). DISTANCE applies to both corner maps, as before.
Data make_data(uint32_t corners, uint32_t padding, uint32_t distance,
               uint32_t active_percent, uint32_t seed);

// Perform the original initial zeroing before the measured kernel interval.
Output make_output(const Data& data);

// Both kernels require zero-initialized outputs from make_output(). They keep
// the original two passes, float accumulations and mixed-precision ppdot.
// Empty-point division and the resulting NaNs/Infs are deliberately preserved.
// Execution is single threaded, matching the gradzatz suite comparison.
void gradzatp_invert_CSR(const Data& data, Output& output);
void gradzatp_invert_DTU(const Data& data, const float* volume_view,
                        const float* surface_view, const float* field_view,
                        Output& output);

struct DTLPrograms {
    std::string volume;
    std::string surface;
    std::string field;
    uint32_t stream_elements;
};

DTLPrograms make_dtl_programs(const Data& data);

// Compare both outputs bit for bit, accepting NaN only when both are NaN.
// Infinity signs and signed zeros must match. NaN payloads are unspecified.
void verify_results(const Output& expected, const Output& actual);

} // namespace gradzatp

#endif
