// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details.
#include "bfs.hpp"
#include "benchmark.h" // GAPBS Builder, Graph and SourcePicker

#include <limits>
#include <stdexcept>

namespace gap_bfs {
Data make_data(uint32_t scale, uint32_t degree, bool uniform,
               uint32_t source, const std::string& file) {
    if (file.empty() && (scale == 0 || scale > 24 || degree == 0 || degree > 1024 ||
        (uint64_t(1) << scale) * degree > 250000000))
        throw std::invalid_argument("Invalid/oversized BFS synthetic graph");
    // Configure the original builder without getopt's process-global state.
    // Synthetic GAPBS graphs are always symmetrized by CLBase::ParseArgs.
    CLBase cli(0, nullptr, "DTU BFS");
    auto scale_arg = std::to_string(scale);
    auto degree_arg = std::to_string(degree);
    auto file_arg = file;
    if (file.empty()) {
        cli.HandleArg(uniform ? 'u' : 'g', scale_arg.data());
        cli.HandleArg('k', degree_arg.data());
        cli.HandleArg('s', nullptr);
    } else {
        cli.HandleArg('f', file_arg.data());
    }
    Builder builder(cli);
    Graph graph = builder.MakeGraph();
    const auto nodes = graph.num_nodes();
    const auto edges = graph.num_edges_directed();
    constexpr uint64_t limit = (uint64_t(std::numeric_limits<int>::max()) - 4095) / 4 - 16;
    if (nodes <= 0 || uint64_t(nodes) > limit || edges < 0 || uint64_t(edges) > limit)
        throw std::invalid_argument("BFS graph exceeds DTL limits");
    Data data;
    data.offsets.reserve(nodes + 1);
    data.neighbors.reserve(edges);
    for (Node u = 0; u < nodes; ++u) {
        data.offsets.push_back(data.neighbors.size());
        for (Node v : graph.out_neigh(u))
            data.neighbors.push_back(v);
    }
    data.offsets.push_back(data.neighbors.size());
    if (source == std::numeric_limits<uint32_t>::max()) {
        // Avoid SourcePicker's infinite search on an edgeless graph.
        data.source = edges == 0 ? 0 : SourcePicker<Graph>(graph).PickNext();
    } else {
        if (source >= uint64_t(nodes))
            throw std::invalid_argument("BFS source is outside graph");
        data.source = source;
    }
    validate_data(data);
    return data;
}
} // namespace gap_bfs
