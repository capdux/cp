# camino@1.0.0 (Agent Quickref)

> Zero-heap strictly validated UTF-8 path engine Utf8PathBuf (crates.io camino equivalent) | License: MIT
> Import: `import { ... } from "camino";` | Full types: `src/index.d.cp`
> Deps: `utf8parse`

## Functions
- `export fn camTrimTrailingSlashes(data: &[]u8, len: usize): usize;`
- `export fn camSliceEq(a: &[]u8, a_start: usize, a_end: usize, b: &[]u8, b_start: usize, b_end: usize): bool;`
- `export fn camIsDot(data: &[]u8, start: usize, end: usize): bool;`
- `export fn camIsDotDot(data: &[]u8, start: usize, end: usize): bool;`
- `export fn camFileNameSpan(data: &[]u8, len: usize): CamSpan;`
- `export fn camFindLastDot(data: &[]u8, start: usize, end: usize): usize?;`
- `export fn camCopyToDst(data: &[]u8, start: usize, end: usize, dst: []u8): usize?;`
- `export fn camNextComponent(data: &[]u8, len: usize, cursor: usize): CamCompStep;`
- `export fn camParentLength(data: &[]u8, len: usize): usize?;`
