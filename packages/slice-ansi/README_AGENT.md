# slice-ansi@1.0.0 (Agent Quickref)

> Zero-allocation ANSI-preserving terminal column slicer and truncator with CJK double-width support in pure CP | License: MIT
> Import: `import { ... } from "slice-ansi";` | Full types: `src/index.d.cp`
> Deps: `anstyle-parse`, `unicode-width`

## Functions
- `export fn ansiVisibleWidth(input: string): usize;`
- `export fn sliceAnsi(input: string, begin: usize, end: usize, out: []u8): usize;`
- `export fn sliceAnsiFrom(input: string, begin: usize, out: []u8): usize;`
- `export fn truncateAnsi(input: string, max_cols: usize, ellipsis: string, out: []u8): usize;`
