# bumpalo@1.0.0 (Agent Quickref)

> Zero-allocation alignment-aware bump arena allocator (BumpArena, BumpSlice) with checkpoint/rewind and overflow protection (Rust bumpalo v3 parity) for CP | License: MIT
> Import: `import { ... } from "bumpalo";` | Full types: `src/index.d.cp`

## Functions
- `export fn bmpIsValidAlign(alignment: usize): bool;`
- `export fn bmpAlignUp(offset: usize, alignment: usize): usize?;`
- `export fn bmpHandleValid(handle: BumpSlice, cursor: usize): bool;`
