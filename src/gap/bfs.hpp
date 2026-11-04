// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details. Adapted from GAPBS bfs.cc.
#ifndef GAP_DTU_BFS_HPP
#define GAP_DTU_BFS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gap_bfs {
using Node = int32_t;
struct Data {
    std::vector<uint32_t> offsets;
    std::vector<Node> neighbors;
    Node source = 0;
};

// Uses the adjacent GAPBS Builder/SourcePicker, including its original seed,
// symmetrization, sorting and deduplication. A file replaces synthetic input.
Data make_data(uint32_t scale, uint32_t degree, bool uniform,
               uint32_t source, const std::string& file = "");
void validate_data(const Data& data);

struct Output {
    std::vector<Node> parent;
    std::vector<Node> queue; // Preallocated scratch, not part of the result.
    uint64_t levels = 0;
};
Output make_output(const Data& data);

// A snapshot is fixed for a level. Negative values are only a hint: the CPU
// must still check/CAS the live parent before enqueuing a vertex.
class ParentGather {
public:
    virtual ~ParentGather() = default;
    virtual void reset(const std::vector<Node>& initial_parent) = 0;
    virtual const Node* publish(const std::vector<Node>& parent,
                               const Node* frontier, size_t count) = 0;
};

// Single-threaded, level-synchronous top-down GAPBS traversal. Both include
// parent initialization and final unreachable-vertex normalization.
void bfs_cpu(const Data& data, Output& output);
void bfs_dtu(const Data& data, ParentGather& gather, Output& output);

struct DTLProgram {
    std::string source;
    uint32_t stream_elements;
};
DTLProgram make_dtl_program(const Data& data);
// Exact parent comparison AND an independent shortest-path/tree check.
void verify_results(const Data& data, const Output& cpu, const Output& dtu);
uint64_t checksum(const Output& output);
} // namespace gap_bfs
#endif
