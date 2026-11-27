# ryu@1.0.0 (Agent Quickref)

> Pure CP zero-allocation fast float-to-string formatter (Rust ryu v1 parity, reuses itoa) | License: MIT
> Import: `import { ... } from "ryu";` | Full types: `src/index.d.cp`
> Deps: `itoa`

## Functions
- `export fn isNanF64(val: f64): bool;`
- `export fn isInfF64(val: f64): bool;`
- `export fn isFiniteF64(val: f64): bool;`
- `export fn formatF64(val: f64, out: []u8): usize;`
- `export fn formatF64Finite(val: f64, out: []u8): usize;`
- `export fn formatF32(val: f32, out: []u8): usize;`
- `export fn formatFixedF64(val: f64, decimals: u8, out: []u8): usize;`
- `export fn formatExpF64(val: f64, precision: u8, out: []u8): usize;`
