# DTL composition diagrams

## Dependency DAG compilation: paper figure

`dtl-dependency-dag.pdf` is a compact vector figure intended for one column
of an ACM paper. It contains three panels: a five-kernel dependency DAG, one
valid topological compilation order, and shadow-mapping propagation into the
next consuming kernel. The design uses black lines, 8-point
annotations and 9-point kernel labels at its natural width of approximately
8.2 cm, including the PDF border. Shared dependencies and the active kernel
are identifiable without color. The caption is outside the artwork.

- `dtl-dependency-dag.tikz`: editable TikZ artwork.
- `dtl-dependency-dag.tex`: standalone PDF wrapper.
- `dtl-dependency-dag-acm.tex`: ready-to-include `figure` fragment with a caption,
  label and ACM `\Description` text. Include it from the paper root, adjusting
  its graphic path as needed. It uses `\columnwidth`, not a two-column float.

Build the PDF from this directory:

```sh
pdflatex -halt-on-error dtl-dependency-dag.tex
```

For editable in-document rendering, load TikZ with `arrows.meta` and
`positioning`, then replace the fragment's `\includegraphics` with
`\input{docs/figures/dtl-dependency-dag.tikz}`. The TikZ body uses the enclosing
paper's normal font family. The standalone PDF uses embedded Latin Modern
fonts. Check the final size in your venue's selected `acmart` variant; the local
installation does not contain `acmart.cls`, so the full ACM document was not
compiled here.

The graph has edges K0 -> K1, K0 -> K2, K1 -> K3, K2 -> K3, and K3 -> K4,
pointing from dependencies to consumers. K4 is active. The compilation-order
arrows in panel (b) indicate sequence, not additional dependency edges.
Panel (c) shows a dependency's shadow mapping `S_i` being propagated into a
mapping slot drawn inside its next consumer `K_j`. This follows a dependency
edge from panel (a); adjacent independent kernels in panel (b) do not exchange
mappings. The panel describes placement, not allocation of a physical address
during compilation; concrete shadow addresses are bound when mapping to hardware.
General DAG support is assumed, as requested. Each reachable kernel is compiled
once; K1 and K2 share K0 and can appear in either order. No compiler changes
are included. Prior presentation-style artwork is preserved in archive/39_changes.

The caption and accessible description are separate, following ACM's guidance:
https://www.acm.org/publications/taps/describing-figures/

## Runtime composition

`dtl-runtime-composition.pdf` illustrates metadata composition using the repository's
`doubleIndirect` stencil: `A[B[C[i]]]`. The upper row shows runtime configuration;
the lower diagram shows the logical reads and returned values for one CPU load.
It does not prescribe bus transaction order, cache behavior, or implementation latency.

Build the standalone figure from this directory:

```sh
pdflatex -halt-on-error dtl-runtime-composition.tex
```

For inclusion in a paper, use the PDF with `\includegraphics`, or load TikZ,
`amsmath`, and the `arrows.meta`, `calc`, and `positioning` libraries and input
`dtl-runtime-composition.tikz` directly. The diagram uses vector text and shapes.

The drawing follows these implementation points:

- `src/dtl_stencils.hpp`, `doubleIndirect`: `base` reads ordinary metadata `C`
  and gathers backing `B`; `kernel` uses `base` as metadata to gather backing `A`.
- `src/dtl/src/dtl_api.cpp`, `MapToDTUHardware`: allocate views in dependency
  order; record subhandles; alias dependent metadata read/write mappings;
  use the dependency's shadow physical address when programming metadata.
- `src/dtl/src/dtl_api.hpp`, `DTUResourceHandle` and `MetadataRegion`: expose
  logical read regions, positional write regions, and metadata aliases.
- `src/benchmarks.cpp`, `bench_DoubleIndirect`: populate metadata and both
  backing arrays, then read the active view sequentially.

This is the implemented metadata-composition case, not a claim that every
possible composition shares this exact topology. Each view has a configuration;
the diagram does not imply a separate physical DTU engine per view.
