BFS is integrated into the same CPU/DTU CSV runner as GZZI and GZZP:

```sh
make -j2
./dtl_bench --bfs
./dtl_bench --bfs-file /path/to/graph.el
```

Run the executable on the DTU-equipped target. `--bfs` runs GAPBS R-MAT scales
12, 16 and 20, each with edge factor 16. The adjacent GAPBS tree supplies its
original Builder and SourcePicker; `GAPBS_DIR` in the Makefile defaults to
`../gapbs`. Graph generation retains GAPBS's fixed seed, permutation,
symmetrization, sorting and duplicate/self-edge removal. Actual vertex/edge
counts and the chosen source appear in each CSV name. `--bfs-file` uses the
original GAPBS reader and the file's directionality. It runs one CPU/DTU pair;
it does not symmetrize a directed edge list. Synthetic sizes, uniform versus
R-MAT input and an explicit source can be changed in `benchmark::bfsData`.
`SOURCE=UINT32_MAX` uses the original deterministic SourcePicker (source zero
for an edgeless graph).

This port runs **single-threaded, top-down BFS**, matching the DX100 traversal
variant discussed in the conversation. The local GAPBS `bfs.cc` normally uses
direction optimization; this port deliberately omits its bottom-up phase.
Consequently, these timings must not be labeled as acceleration of the
unmodified direction-optimizing algorithm. CSR neighbor order, level boundaries,
negative-degree parent initialization, CPU CAS claims and final normalization
of unreachable parents follow GAPBS's top-down path. Output parents exactly
match the original single-threaded TDStep traversal. Parallel BFS can choose
other equally valid parents.

The DTU uses one configuration:

```text
parent_view[edge] = parent_snapshot[neighbors[edge]]
```

For example, five vertices and 17 edges produce this full program:

```text
DTL<4096> parents = {
  metadata_stream<4, 4096> neighbors;
  for (int edge = 0; edge < 32; edge++) { out = neighbors[edge]; }
}
active parents;
```

The wrapper uses CompileProgram, CheckHasAvailableResources, MapTODTUHardware,
4-byte data, metadata/write mappings and fences, as in the UME wrappers.
Metadata contains the original outgoing CSR neighbors; partial 64-byte tails
are filled with valid vertex-zero indices. A root handle owns the mapping and
is freed through the API.

The CPU owns the live parent array and the discovery queue. At each level's
start, the snapshot equals the live parents. A nonnegative gathered parent
therefore identifies an already visited vertex. Negative snapshot entries can
become stale during a level, so the CPU always reads and CAS-checks the live
parent before enqueueing; duplicate edges cannot produce duplicate discoveries.
All prior snapshot positives remain valid. Only the new frontier's parent
entries are written into the snapshot between levels, followed by a fence,
checked EphemeralRegion::Sync and a second fence. Sync supplies fresh shadow
physical addresses to avoid reusing gathered cache lines from an earlier level.
No atomics execute against a DTU shadow mapping, and no DTU conditional stores
or ragged-loop expansion are assumed.

Both timed intervals include parent initialization, traversal, CAS operations,
queue writes and unreachable-parent normalization. The DTU interval additionally
includes the initial full snapshot copy, all sparse snapshot publications,
fences and one Sync per level. Allocations, graph construction, compilation,
mapping, neighbor metadata initialization, verification, hashing and teardown
are outside the intervals. CSV transform_cost remains zero, like the UME
wrappers; separate setup_us and timed snapshot/Sync counts are reported on
stderr. Each result includes an FNV-1a hash of the complete parent array.

Verification checks exact CPU/DTU parent equality for every vertex, then checks
reachability, shortest-path depth and parent-edge existence using an independent
serial traversal. Software tests compare against the original GAPBS TDStep and
run the original DOBFS verifier too. Device mapping tests emulate gathers in
RAM; they do not validate actual RTL requests, device remapping or performance.
A target run is still required. Large speedups are not assumed: small parent
arrays can fit in CPU cache, this design still performs CPU CAS for candidate
discoveries, and high-diameter graphs pay a Sync for every level.

The original `../gapbs/src/bfs.cc` is unchanged. Adapted code retains its Regents
copyright notice; the GAPBS license is included as LICENSE.txt.
