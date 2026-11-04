# gradzatp invert CPU/DTU benchmark

Build with `make`, then run the RISC-V binary on the DTU target:

```sh
./dtl_bench --gradzatp-invert
```

This runs CPU and DTU versions at 4096, 65536, 262144, 2000000, 10000000 and
20000000 corners. The sizes mirror the current gradzatz benchmark entries.
`--gradzatz-invert` and the no-argument composed benchmark remain available.

The input generator retains the original all-ones corner volumes, corner
surfaces, zone fields and point normals, padded point/zone counts, rand() draw
order, point types and corner ordering in the inverse CSR map. A fixed seed
replaces time-based seeding. The default padding is 90000, offset radius 85000,
interior-point percentage 95 and seed 1234. As in the original, both maps use
the 85000 radius; its unused DISTANCE_P2C constant did not affect the inputs.

The scalar reference retains the original two passes:

1. For every point, accumulate float point_volume and point_gradient in the
   original CSR order, including boundary and other point types.
2. Normalize interior points and apply the original boundary expression only
   for point_type == -1. Other types retain their unnormalized accumulation.

The DTU version changes only the three input accesses in the first pass:

```cpp
volume_view[i]  = corner_volume[p_to_c_indices[i]];
surface_view[i] = csurf[p_to_c_indices[i]];
field_view[i]   = zone_field[c_to_z_map[p_to_c_indices[i]]];
```

Volumes and surfaces use one configuration each. The zone-field gather uses
two composed configurations, zone_ids and fields. All four are checked for
availability before mapping. Backing and corner metadata are initialized via
write mappings, with 4-byte elements and a publication fence. Stream tails are
padded to complete 64-byte cache lines using valid corner-zero indices.

Both versions execute on one CPU thread, matching the current gradzatz
comparison. OpenMP scheduling and optional verification atomics are omitted;
the arithmetic order within each point and the separate passes are preserved.
Both output arrays are zeroed before their measured kernel interval, as in
the original ROI. The DTU consumer still performs both reductions and all
normalization arithmetic. In particular, ppdot is double, but its right-hand
float multiplication is evaluated before promotion, matching the original.

The original divides by zero for empty interior/boundary points. This behavior
is preserved: matching NaNs are accepted at the same indices in verification.
Both point_volume and point_gradient are compared bit for bit for other values,
including infinity signs and signed zeros. NaN payload differences are ignored.
The CSV hash combines both output arrays and canonicalizes NaNs only for
hashing, so empty points do not reduce the checksum to a plain NaN sum.

The wrapper reports CPU/DTU PerfManager counters, checks both outputs and prints
the gradient NaN count and separate compile/map/copy setup_us on stderr.
transform_cost is zero for these demand-gather views. Initialization,
verification, hashing and teardown are outside the measured intervals.

Validation is archived in archive/18_changes: exact comparisons against the
extracted original scalar function for 56 cases (including nonuniform values,
empty rows and both boundary branches), ASan/UBSan checks, optimized native
checks with the original using four OpenMP threads, and real compiler/API
mapping checks through 20 million corners. The numerical original-generator
comparison covers sizes through two million corners. API checks use device
mocks; FireSim/FPGA execution and speedups have not been tested for this port.

The complete original file is preserved at
archive/18_changes/before/src/ume/gradzatp_invert.cpp.
