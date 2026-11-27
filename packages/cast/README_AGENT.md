# cast@1.0.0 (Agent Quickref)

> Zero-allocation checked primitive numeric casts preventing silent overflow, underflow, and NaN/Inf truncation (clean-room MIT implementation of crates.io cast, #443 / 335M+) | License: MIT
> Import: `import { ... } from "cast";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CastError`

## Functions
- `export fn u8FromI64(v: i64): u8?;`
- `export fn u8FromU64(v: u64): u8?;`
- `export fn u16FromI64(v: i64): u16?;`
- `export fn u32FromI64(v: i64): u32?;`
- `export fn u32FromU64(v: u64): u32?;`
- `export fn i32FromU64(v: u64): i32?;`
- `export fn i32FromI64(v: i64): i32?;`
- `export fn u64FromI64(v: i64): u64?;`
- `export fn i64FromU64(v: u64): i64?;`
- `export fn usizeFromI64(v: i64): usize?;`
- `export fn u32FromF64(v: f64): u32?;`
- `export fn i32FromF64(v: f64): i32?;`
- `export fn u8FromF64(v: f64): u8?;`
- `export fn checkF64ToI32(v: f64): CastError?;`
