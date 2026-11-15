// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details. Adapted from GAPBS bfs.cc.
#include "bfs.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>

namespace gap_bfs {
void validate_data(const Data& data) {
    // DTL byte-size literals and loop bounds currently use signed int.
    constexpr uint64_t limit = (uint64_t(std::numeric_limits<int>::max()) - 4095) / 4 - 16;
    if (data.offsets.size() < 2 || data.offsets.size() - 1 > limit ||
        data.neighbors.size() > limit || data.source < 0 ||
        size_t(data.source) >= data.offsets.size() - 1 || data.offsets[0] != 0 ||
        data.offsets.back() != data.neighbors.size())
        throw std::invalid_argument("Invalid BFS dimensions/source/CSR");
    for (size_t u = 0; u + 1 < data.offsets.size(); ++u)
        if (data.offsets[u] > data.offsets[u + 1])
            throw std::invalid_argument("Nonmonotonic BFS offsets");
    for (Node v : data.neighbors)
        if (v < 0 || size_t(v) >= data.offsets.size() - 1)
            throw std::invalid_argument("Invalid BFS neighbor");
}

Output make_output(const Data& data) {
    validate_data(data);
    const size_t n = data.offsets.size() - 1;
    return {std::vector<Node>(n), std::vector<Node>(n), 0};
}

namespace {
template<bool UseDTU>
void traverse(const Data& data, ParentGather* gather, Output& output) {
    const size_t n = output.parent.size();
    auto& parent = output.parent;
    auto& queue = output.queue;
    output.levels = 0;
    for (size_t u = 0; u < n; ++u) {
        const Node degree = data.offsets[u + 1] - data.offsets[u];
        parent[u] = degree != 0 ? -degree : -1;
    }
    parent[data.source] = data.source;
    if constexpr (UseDTU)
        gather->reset(parent);
    size_t begin = 0, end = 1, tail = 1;
    queue[0] = data.source;
    while (begin < end) {
        const Node* view = nullptr;
        if constexpr (UseDTU)
            view = gather->publish(parent, queue.data() + begin, end - begin);
        ++output.levels;
        for (size_t q = begin; q < end; ++q) {
            const Node u = queue[q];
            for (uint32_t edge = data.offsets[u]; edge < data.offsets[u + 1]; ++edge) {
                // Snapshot positives are stable. Snapshot negatives can be
                // stale after an earlier claim in this same level.
                if constexpr (UseDTU)
                    if (view[edge] >= 0)
                        continue;
                const Node v = data.neighbors[edge];
                const Node curr_val = parent[v];
                if (curr_val < 0 && __sync_bool_compare_and_swap(&parent[v], curr_val, u))
                    queue[tail++] = v;
            }
        }
        begin = end;
        end = tail;
    }
    for (Node& p : parent)
        if (p < -1)
            p = -1;
}
} // namespace

void bfs_cpu(const Data& data, Output& output) { traverse<false>(data, nullptr, output); }
void bfs_dtu(const Data& data, ParentGather& gather, Output& output) {
    traverse<true>(data, &gather, output);
}

DTLProgram make_dtl_program(const Data& data) {
    validate_data(data);
    // Even an edgeless graph gets a valid page and a nonzero padded stream.
    const uint32_t stream = data.neighbors.empty() ? 16 :
        ((data.neighbors.size() + 15) / 16) * 16;
    const auto bytes = [](size_t n) { return ((n * 4 + 4095) / 4096) * 4096; };
    std::ostringstream program;
    program << "DTL<" << bytes(data.offsets.size() - 1) << "> parents = {\n"
            << "  metadata_stream<4, " << bytes(stream) << "> neighbors;\n"
            << "  for (int edge = 0; edge < " << stream
            << "; edge++) { out = neighbors[edge]; }\n}\nactive parents;\n";
    return {program.str(), stream};
}

void verify_results(const Data& data, const Output& cpu, const Output& dtu) {
    const size_t n = data.offsets.size() - 1;
    if (cpu.parent.size() != n || dtu.parent.size() != n)
        throw std::runtime_error("BFS parent size mismatch");
    for (size_t u = 0; u < n; ++u)
        if (cpu.parent[u] != dtu.parent[u])
            throw std::runtime_error("BFS parent mismatch at vertex " + std::to_string(u));
    // Independent FIFO shortest-path oracle: validate reachability, depths and
    // parent-edge existence rather than merely trusting two matching kernels.
    std::vector<int> depth(n, -1);
    std::vector<Node> queue;
    std::vector<bool> parent_edge(n, false);
    queue.reserve(n);
    depth[data.source] = 0;
    queue.push_back(data.source);
    for (size_t i = 0; i < queue.size(); ++i) {
        const Node u = queue[i];
        for (uint32_t e = data.offsets[u]; e < data.offsets[u + 1]; ++e) {
            const Node v = data.neighbors[e];
            if (dtu.parent[v] == u)
                parent_edge[v] = true;
            if (depth[v] == -1) {
                depth[v] = depth[u] + 1;
                queue.push_back(v);
            }
        }
    }
    for (size_t u = 0; u < n; ++u) {
        const Node p = dtu.parent[u];
        const bool valid = u == size_t(data.source) ? p == data.source :
            (depth[u] == -1 ? p == -1 :
             (p >= 0 && size_t(p) < n && parent_edge[u] && depth[p] == depth[u] - 1));
        if (!valid)
            throw std::runtime_error("Invalid BFS tree at vertex " + std::to_string(u));
    }
}

uint64_t checksum(const Output& output) {
    uint64_t hash = 14695981039346656037ULL;
    for (Node p : output.parent)
        for (int byte = 0; byte < 4; ++byte) {
            hash ^= (uint32_t(p) >> (8 * byte)) & 0xff;
            hash *= 1099511628211ULL;
        }
    return hash;
}
} // namespace gap_bfs
