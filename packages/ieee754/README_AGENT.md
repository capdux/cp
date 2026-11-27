# ieee754@1.0.0 (Agent Quickref)

> Zero-allocation IEEE-754 32-bit and 64-bit floating-point binary encoder/decoder (little-endian & big-endian) with NaN, Infinity, and subnormal fidelity (clean-room MIT implementation of npm ieee754, #293 / 173M wk) | License: MIT
> Import: `import { ... } from "ieee754";` | Full types: `src/index.d.cp`

## Functions
- `export fn readF32Le(buf: &[]u8, offset: usize): f32;`
- `export fn readF32Be(buf: &[]u8, offset: usize): f32;`
- `export fn writeF32Le(buf: []u8, offset: usize, value: f32): bool;`
- `export fn writeF32Be(buf: []u8, offset: usize, value: f32): bool;`
- `export fn readF64Le(buf: &[]u8, offset: usize): f64;`
- `export fn readF64Be(buf: &[]u8, offset: usize): f64;`
- `export fn writeF64Le(buf: []u8, offset: usize, value: f64): bool;`
- `export fn writeF64Be(buf: []u8, offset: usize, value: f64): bool;`
- `export fn readIeee754(buf: &[]u8, offset: usize, is_le: bool, n_bytes: usize): f64;`
- `export fn writeIeee754(buf: []u8, value: f64, offset: usize, is_le: bool, n_bytes: usize): bool;`
