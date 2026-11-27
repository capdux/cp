# bwt@1.0.0 (Agent Quickref)

> Zero-allocation Burrows-Wheeler Transform (BWT) and Move-To-Front (MTF) reversible compression preprocessing in pure CP | License: MIT
> Import: `import { ... } from "bwt";` | Full types: `src/index.d.cp`

## Functions
- `export fn cmpRotations(src: &[]u8, a: usize, b: usize): i32;`
- `export fn bwtForward(src: &[]u8, dst: []u8): BwtResult;`
- `export fn bwtInverse(src: &[]u8, primary_index: usize, dst: []u8): bool;`
- `export fn mtfEncode(src: &[]u8, dst: []u8): bool;`
- `export fn mtfDecode(src: &[]u8, dst: []u8): bool;`
