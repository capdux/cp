# siphash — AI Agent Quick Ref

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed 64-bit HashDoS-resistant PRF for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SipState,
    shU64,
    shAdd64,
    shRotl64,
    shReadLe64,
    shRound,
    shRun,
    siphash24,
} from "siphash";
```

## Core Types & Constants

```cp
class SipState { v0: u64, v1: u64, v2: u64, v3: u64 }
```

## API Signatures

```cp
fn shU64(hi: u32, lo: u32): u64;
fn shAdd64(a: u64, b: u64): u64;
fn shRotl64(x: u64, bits: u32): u64;
fn shReadLe64(data: &[]u8, offset: usize): u64;
fn shRound(st: &mut SipState);
fn shRun(k0: u64, k1: u64, data: &[]u8, c_rounds: usize, d_rounds: usize): u64;
fn siphash24(k0: u64, k1: u64, data: &[]u8): u64;
fn siphash13(k0: u64, k1: u64, data: &[]u8): u64;
fn siphash24Key16(key: [16]u8, data: &[]u8): u64;
fn siphash13Key16(key: [16]u8, data: &[]u8): u64;
```

## Minimal Usage

```cp
import { shU64 } from "siphash";

pub fn main() {
    let result = shU64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"siphash"`; install via `cap add siphash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
