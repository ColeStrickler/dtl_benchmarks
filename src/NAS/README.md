The supplied `cg.cpp` is converted into a single-threaded CPU/DTU benchmark:

```sh
make -j2
./dtl_bench --nas-cg
```

Run the executable on the usual DTU-equipped RISC-V target. Each case emits
matching `cpu` and `dtu` CSV rows through `PerfManager`, with whole-result FNV-1a
hashes. Verification compares every element of `x`, `z`, `p`, `q`, `r`, residual
norms and zeta histories exactly, rejects nonfinite results, and independently
recomputes the final `A*z` product. Errors abort the benchmark.

`benchmark::nasCgData` in `src/benchmarks.hpp` supplies 4,096-, 37,500- and
150,000-row cases. Parameters are `ROWS`, `NONZER`, `CGITMAX`, `NITER` and `SHIFT`.
Defaults preserve the supplied variant: `NONZER=15`, `CGITMAX=4`, `NITER=1`,
`SHIFT=110`, `RCOND=0.1`, the NAS `randlc` seed/draw sequence, and float matrix,
vector and reduction arithmetic. Matrix generation occurs outside timing.

This is the provided shortened **FP32 CG variant**, using serial `BASE` operation
order. It includes the final explicit residual calculation, inverse-power zeta
calculation and vector normalization. It does not report an official NAS Class C
score: the supplied source reduced the iteration counts, used floats and scaled
the row count with `NUM_CORES`. Here row count is independent of threading.
The former MAA/gem5/OpenMP dependencies and accelerator arithmetic are removed;
the original supplied source is preserved in `archive/31_changes/before/src/NAS/`.
Its MIT notice remains in the adapted `cg.cpp`.

The DTU uses one static metadata stream:

```text
view[edge] = vector[colidx[edge]]
```

The CPU still traverses CSR rows, loads `a[edge]`, multiplies and reduces in the
original order. The DTU replaces only the indirect vector read in `A*p` and the
final `A*z`. Ragged row bounds and reductions remain on the CPU. Metadata is
initialized once outside timing, padded to a full 64-byte line with valid
column-zero indices. Compilation explicitly passes `sizeof(float)` and hardware
element width is set to four bytes. The wrapper owns and frees the resource
handle using the UME/GAP convention.

Both measured intervals include CG scratch initialization, all inner/outer
iterations, residuals, scalar reductions and inverse-power normalization. Before
each DTU SpMV, the complete current `p` or `z` vector is copied into backing
memory, fenced, refreshed with a checked `Sync`, and fenced again. These copies
and Syncs are **inside the DTU timing**. A fresh shadow physical address prevents
reusing gathered cache lines from a previous vector. The default case therefore
performs five timed publications/Syncs, totaling `5*ROWS` floats. No pre-gathered
vector is built in software on the target.

Input generation, output allocation, initial all-ones `x`, compilation, mapping,
immutable metadata initialization, verification, hashing and cleanup are outside
the measured intervals. `transform_cost` remains zero, matching UME/GAP; DTU
setup microseconds, timed publication counts, residual norm and zeta are printed
on stderr. `-fno-tree-vectorize -ffp-contract=off` applies to both CG paths.

Host regression sources and build scripts are preserved under
`archive/31_changes/`. They compare against the original generator and serial
BASE routine, and exercise the actual compiler/API and vector publisher with
RAM-emulated gathers. These checks cannot establish RTL correctness or speedup;
target performance remains to be measured. The timed full-vector publication and
per-SpMV Sync can outweigh savings from the indirect loads.
