## Iteration 3 — 2026-10-18T20:03:38-05:00

- Hypothesis: The first record-sort run may stall during remapping, MMIO programming, backing/metadata initialization, ordinary view reads during sorting, or metadata-dependent view reads during validation. Existing buffered output cannot distinguish these stages.
- User authorization: Add C++ printf diagnostics only; verify the metadata address passed to ProgramHardware is used. No RTL edits or FireSim runs.
- Branches preserved: benchmark composition; nested DTL composability. Existing modifications preserved.
- Files: src/sort.hpp; src/dtl/src/debug_trace.hpp; src/dtl/src/dtl_api.cpp; src/dtl/src/resource_alloc_pass.cpp.
- Changes: Flushed stderr checkpoints around each stage, remap ioctl, config writes, initial-state register groups, routing writes, reset writes/polling, sampled sort loads and validation progress. Prints the input binding, metadata variable/index names, resolved address, and a volatile readback of the actual metadata-address MMIO register.
- Semantics: Existing addresses, values, mappings, sort order, and validation comparison remain unchanged. One diagnostic register read is added per metadata slot during initialization. Progress logging/counters add overhead to measured intervals; these are diagnostic performance results.
- Validation: RISC-V syntax checks passed for both modified implementation files and explicit RecordSort instantiations 3, 7, 15, 31, and 63 (pre-existing warnings remain). A native host harness with wrapped device calls and RAM-backed MMIO confirmed x=0x178000000 reaches the register, and changing only the supplied argument to 0x168000000 changes that register/readback too. Original-address final AGU/DTU register images match the earlier current-compiler experiment byte-for-byte.
- Limits: Host checks do not validate the real driver, physical MMIO, cache behavior, or FireSim. The added volatile readback will check the real register on the next target run.
- Build: Rebuild the nested DTL library as well as the benchmark so the API/allocator diagnostics are linked. The benchmark Makefile does not track sort.hpp dependencies, so rebuild its objects too.
- FireSim: Not run. RTL/configuration files unchanged. No commits.
- Result: Kept.
- Archive: before/, after/, changes.diff preserve this iteration independently of pre-existing changes.
