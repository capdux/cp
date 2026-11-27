# cast — AI Agent Quick Ref

Zero-allocation checked primitive numeric casts preventing silent overflow, underflow, and NaN/Inf truncation (clean-room MIT implementation of crates.io cast, #443 / 335M+) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CastError,
    u8FromI64,
    u8FromU64,
    u16FromI64,
    u32FromI64,
    u32FromU64,
    i32FromU64,
    i32FromI64,
} from "cast";
```

## Core Types & Constants

```cp
enum CastError { Overflow, Underflow, Infinite, NaN }
```

## API Signatures

```cp
fn u8FromI64(v: i64): u8?;
fn u8FromU64(v: u64): u8?;
fn u16FromI64(v: i64): u16?;
fn u32FromI64(v: i64): u32?;
fn u32FromU64(v: u64): u32?;
fn i32FromU64(v: u64): i32?;
fn i32FromI64(v: i64): i32?;
fn u64FromI64(v: i64): u64?;
fn i64FromU64(v: u64): i64?;
fn usizeFromI64(v: i64): usize?;
fn u32FromF64(v: f64): u32?;
fn i32FromF64(v: f64): i32?;
fn u8FromF64(v: f64): u8?;
fn checkF64ToI32(v: f64): CastError?;
```

## Minimal Usage

```cp
import { u8FromI64 } from "cast";

pub fn main() {
    let result = u8FromI64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cast"`; install via `cap add cast`.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
