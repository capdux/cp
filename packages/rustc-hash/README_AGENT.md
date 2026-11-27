# rustc-hash@1.0.0 (Agent Quickref)

> Zero-heap pure-CP fast non-cryptographic FxHasher (64-bit & 32-bit, crates.io rustc-hash equivalent) | License: MIT
> Import: `import { ... } from "rustc-hash";` | Full types: `src/index.d.cp`

## Functions
- `export fn fxhMul64(a: u64, b: u64): u64;`
- `export fn fxhMul32(a: u32, b: u32): u32;`
- `export fn fxhRotl64By5(x: u64): u64;`
- `export fn fxhRotl32By5(x: u32): u32;`
- `export fn fxhAddWord64(hash: u64, word: u64): u64;`
- `export fn fxhAddWord32(hash: u32, word: u32): u32;`
- `export fn fxhHashSlice64(hash: u64, data: &[]u8): u64;`
- `export fn fxhHashSlice32(hash: u32, data: &[]u8): u32;`
- `export fn fxHash64(data: []u8): u64;`
- `export fn fxHash64Str(s: string): u64;`
- `export fn fxHash32(data: []u8): u32;`
