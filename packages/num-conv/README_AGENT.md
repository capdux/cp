# num-conv — AI Agent Quick Ref

Zero-heap lossless widening, explicit truncating, checked, and saturating integer conversions matching Rust num-conv in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CheckedU8,
    CheckedU16,
    CheckedU32,
    CheckedI8,
    CheckedI16,
    CheckedI32,
    extendU8ToU64,
    extendU16ToU64,
} from "num-conv";
```

## Core Types & Constants

```cp
class CheckedU8 { val: u8, ok: bool }
class CheckedU16 { val: u16, ok: bool }
class CheckedU32 { val: u32, ok: bool }
class CheckedI8 { val: i8, ok: bool }
class CheckedI16 { val: i16, ok: bool }
class CheckedI32 { val: i32, ok: bool }
```

## API Signatures

```cp
fn extendU8ToU64(v: u8): u64;
fn extendU16ToU64(v: u16): u64;
fn extendU32ToU64(v: u32): u64;
fn extendI8ToI64(v: i8): i64;
fn extendI16ToI64(v: i16): i64;
fn extendI32ToI64(v: i32): i64;
fn truncateU64ToU8(v: u64): u8;
fn truncateU64ToU16(v: u64): u16;
fn truncateU64ToU32(v: u64): u32;
fn tryU64ToU8(v: u64): CheckedU8;
fn tryU64ToU16(v: u64): CheckedU16;
fn tryU64ToU32(v: u64): CheckedU32;
fn tryI64ToI8(v: i64): CheckedI8;
fn tryI64ToI16(v: i64): CheckedI16;
fn tryI64ToI32(v: i64): CheckedI32;
fn tryI64ToU8(v: i64): CheckedU8;
fn tryI64ToU32(v: i64): CheckedU32;
fn saturatingI64ToU8(v: i64): u8;
fn saturatingI64ToI8(v: i64): i8;
fn saturatingI64ToI16(v: i64): i16;
fn saturatingU64ToU32(v: u64): u32;
```

## Minimal Usage

```cp
import { extendU8ToU64 } from "num-conv";

pub fn main() {
    let result = extendU8ToU64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"num-conv"`; install via `cap add num-conv`.
