# gradzatz invert

Build the suite with `make`, then run on the DTU target:

```sh
./dtl_bench --gradzatz-invert
```

The no-argument invocation retains the existing composed benchmark. Cases are
registered in `gradzatzInvertData` in `src/benchmarks.hpp`: 4096, 65536 and 262144
corners, 90000 padding on either side, an 85000 random-offset radius around corner + 85000 and
95% active zones. A fixed seed makes both implementations use identical inputs.
Volumes are positive nonuniform floats; point gradients include signed values.

`gradzatz_invert.cpp` provides data generation, the scalar reference, the DTU
consumer loop and complete DTL program strings. `gradzatz_benchmark.cpp` wraps
them in the benchmark API. The original standalone main, global data and MAA
implementation have been replaced by these callable CPU/DTU implementations.

The DTU presents two logical arrays in CSR corner order:

```cpp
volume_view[i]   = corner_volume[z_to_c_indices[i]];
gradient_view[i] = point_gradient[c_to_p_map[z_to_c_indices[i]]];
```

The volume view uses one configuration. The gradient program uses two composed
configurations: `point_ids` gathers corner-to-point indices, and `gradients`
uses its shadow addresses as metadata. Three configurations are live together.
Each program gets its own copy of the corner metadata. Backing/metadata are
initialized through write mappings and fenced before the DTU is read. Partial
64-byte stream tails use valid corner-zero indices; backing and metadata sizes
are page aligned.

The CPU retains zone-type checks, CSR row boundaries, the volume reduction and
the weighted gradient reduction. Both implementations use double intermediates
and float output accumulation, matching the original scalar kernel. Execution
is single threaded. Initialization, allocation, DTL compilation/mapping, copies,
verification and teardown are outside the measured kernel intervals.

Successful runs return `cpu` and `dtu` CSV rows using `PerfManager`. The checksum
column is a sum of zone gradients; correctness is checked for every zone with
`abs(cpu-dtu) <= 1e-5 + 1e-4*abs(cpu)`, rejecting nonfinite values. Compile,
resource and verification failures produce an error and a nonzero exit status.
`transform_cost` is zero because views gather on demand; combined DTU
compile/map/copy time is reported separately as `setup_us` on stderr.

Local numerical and compiler/API checks are recorded in `archive/17_changes/`.
The API checks use RAM-backed device/driver mocks and do not establish hardware
execution or speedup. No FireSim/FPGA run was performed for this port.
