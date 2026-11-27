# rand_pcg@1.0.0 (Agent Quickref)

> Zero-heap PCG32 (XSH-RR) pseudorandom number generator with independent streams and O(log N) jump-ahead (inspired by Rust rand_pcg) | License: MIT
> Import: `import { ... } from "rand_pcg";` | Full types: `src/index.d.cp`

## Functions
- `export fn wrapAdd64(a: u64, b: u64): u64;`
- `export fn wrapMul64(a: u64, b: u64): u64;`
- `export fn mul64Full(a: u64, b: u64): U128Pair;`
- `export fn wrapMul128(a_hi: u64, a_lo: u64, b_hi: u64, b_lo: u64): U128Pair;`
- `export fn rotr32(x: u32, r: u32): u32;`
- `export fn rotr64(x: u64, r: u32): u64;`
- `export fn splitmix64Next(x_in: u64): u64;`
