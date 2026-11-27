# cliui@1.0.0 (Agent Quickref)

> Zero-allocation multi-column CLI layout engine with ANSI-aware wrapping, alignment, and padding in pure CP | License: MIT
> Import: `import { ... } from "cliui";` | Full types: `src/index.d.cp`
> Deps: `strip-ansi`, `unicode-width`, `wrap-ansi`

## Types & Constants
- `export enum Align`

## Functions
- `export fn alignToU8(a: Align): u8;`
- `export fn formatTwoCol(left: string, right: string, left_width: usize, total_width: usize, out: []u8): usize;`
