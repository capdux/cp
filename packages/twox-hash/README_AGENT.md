# twox-hash@1.0.0 (Agent Quickref)

> Zero-heap pure-CP XXHash32 and XXHash64 fast non-cryptographic streaming hashers (crates.io twox-hash equivalent) | License: MIT
> Import: `import { ... } from "twox-hash";` | Full types: `src/index.d.cp`

## Functions
- `export fn xxhU64(hi: u32, lo: u32): u64;`
- `export fn xxhAdd32(a: u32, b: u32): u32;`
- `export fn xxhSub32(a: u32, b: u32): u32;`
- `export fn xxhMul32(a: u32, b: u32): u32;`
- `export fn xxhRol32(x: u32, r: u32): u32;`
- `export fn xxhAdd64(a: u64, b: u64): u64;`
- `export fn xxhSub64(a: u64, b: u64): u64;`
- `export fn xxhMul64(a: u64, b: u64): u64;`
- `export fn xxhRol64(x: u64, r: u64): u64;`
- `export fn xxhReadU32Le(buf: []u8, off: usize): u32;`
- `export fn xxhReadU64Le(buf: []u8, off: usize): u64;`
- `export fn xxh32Round(acc: u32, lane: u32): u32;`
- `export fn xxh64Prime1(): u64;`
- `export fn xxh64Prime2(): u64;`
- `export fn xxh64Prime3(): u64;`
- `export fn xxh64Prime4(): u64;`
- `export fn xxh64Prime5(): u64;`
- `export fn xxh64Round(acc: u64, lane: u64): u64;`
- `export fn xxh64MergeRound(acc: u64, val: u64): u64;`
