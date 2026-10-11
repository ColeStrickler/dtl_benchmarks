# Iteration 1 changes

## `src/dtl_stencils.hpp`

- Before: Each map value contained a bare DTL loop or metadata-stream body.
- After: Each existing body is enclosed by `DTL kernel = {` and `}`, followed by `active kernel;`.
- Diff: Added one named-kernel declaration and one activation statement to each of the 36 raw-string kernel definitions. Existing kernel logic was not changed.

```diff
 R"(
+DTL kernel = {
 <existing kernel body>
+}
+active kernel;
 )"
```
