# constant_time_eq@1.0.0 (Agent Quickref)

> Zero-allocation constant-time cryptographic comparison, branchless selection/swap, and secure memory zeroization primitives (Rust subtle + zeroize parity) for CP | License: MIT
> Import: `import { ... } from "constant_time_eq";` | Full types: `src/index.d.cp`

## Functions
- `export fn maskU8(c: Choice): u8;`
- `export fn maskU32(c: Choice): u32;`
- `export fn maskU64(c: Choice): u64;`
- `export fn ctIsZeroU8(x: u8): Choice;`
- `export fn ctEqU8(a: u8, b: u8): Choice;`
- `export fn ctEqU32(a: u32, b: u32): Choice;`
- `export fn ctEqU64(a: u64, b: u64): Choice;`
- `export fn ctLtU64(a: u64, b: u64): Choice;`
- `export fn ctGtU64(a: u64, b: u64): Choice;`
- `export fn ctSelectU8(c: Choice, when_true: u8, when_false: u8): u8;`
- `export fn ctSelectU32(c: Choice, when_true: u32, when_false: u32): u32;`
- `export fn ctSelectU64(c: Choice, when_true: u64, when_false: u64): u64;`
- `export fn ctSwapU64(c: Choice, a: &mut u64, b: &mut u64);`
- `export fn ctEqBytes(a: string, b: string): Choice;`
- `export fn ctEqFixed32(a: [32]u8, b: [32]u8): Choice;`
- `export fn ctCopyIf(c: Choice, dst: []u8, src: string);`
- `export fn zeroizeBytes(buf: []u8);`
- `export fn isAllZero(buf: string): Choice;`
