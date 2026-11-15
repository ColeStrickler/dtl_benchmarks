#ifndef NAS_CG_HPP
#define NAS_CG_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nas_cg {

static_assert(sizeof(float) == 4 && sizeof(int) == 4,
              "NAS CG DTU views require four-byte floats and indices");

struct Config {
    uint32_t rows = 37500;
    uint32_t nonzer = 15;
    uint32_t cg_iterations = 4;
    uint32_t iterations = 1;
    double shift = 110.0;
    double rcond = 0.1;
};

struct Data {
    Config config;
    std::vector<int> rowstr;
    std::vector<int> colidx;
    std::vector<float> a;
};

struct Output {
    std::vector<float> x, z, p, q, r;
    std::vector<double> rnorm_history, zeta_history;
};

// The provided NAS generator, randlc seed/draw order and float CSR values.
Data make_data(const Config& config);
void validate_data(const Data& data);
// Initialize x[0..N] to one and allocate scratch outside the timed interval.
Output make_output(const Data& data);

class VectorGather {
public:
    virtual ~VectorGather() = default;
    // Publish an immutable snapshot, then expose vector[colidx[edge]].
    virtual const float* publish(const float* vector, std::size_t count) = 0;
};

// Single-threaded original BASE arithmetic, including inverse-power updates.
// Both paths initialize CG scratch and overwrite all result/history entries.
void run_cpu(const Data& data, Output& output);
void run_dtu(const Data& data, VectorGather& gather, Output& output);

struct DTLProgram {
    std::string source;
    uint32_t stream_elements;
};
DTLProgram make_dtl_program(const Data& data);
void verify_results(const Data& data, const Output& expected, const Output& actual);
uint64_t checksum(const Output& output);

} // namespace nas_cg

#endif
