## Iteration 2 — 2026-10-18T13:27:48-05:00

- Reason: CreateBenchmarkConfig and CreateBenchmarkConfig2 now add the outer kernel wrapper, so stencil bodies must not contain duplicate wrappers.
- Branch: composition, explicitly requested by the user.
- Source changed: src/dtl_stencils.hpp.
- Change: Removed the opening DTL kernel declaration, matching outer closing brace, and active kernel statement from all 36 stencils (108 lines); preserved every stencil body.
- Validation: Compared all 36 bodies before/after, checked balanced braces and absence of wrappers, and passed g++ -std=c++17 -fsyntax-only on a translation unit including the updated header.
- FireSim: Not run.
- Result: Kept.
- Archive: archive/2_changes/ contains the source before, after, and diff.
