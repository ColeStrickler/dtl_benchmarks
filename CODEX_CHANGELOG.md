# Codex Changelog

## 2026-10-11 — Iteration 1

- Hypothesis/reason: Every DTL stencil string must use the named-kernel syntax required by the updated DTL parser.
- Files modified: `src/dtl_stencils.hpp`
- Summary: Wrapped all 36 existing kernel bodies in `DTL kernel = { ... }` and appended `active kernel;` inside each raw string.
- Tests performed: Confirmed 36 raw strings, 36 `DTL kernel` declarations, and 36 `active kernel` statements; `git diff --check` passed; `g++ -std=c++17 -include string -fsyntax-only -x c++ src/dtl_stencils.hpp` passed. A standalone check without `-include string` exposed the pre-existing missing direct `<string>` include.
- FireSim result: Not run; this was a syntax-only workload-source update and did not request an experiment.
- Disposition: Kept.
