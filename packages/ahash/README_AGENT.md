# ahash — AI Agent Quick Ref

Zero-allocation randomized high-speed non-cryptographic hasher with 4-key RandomState, folded multiply, and rotation rounds (Rust ahash parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    AHasher,
    RandomState,
    makeU64,
    addWrap64,
    mulWrap64,
    pow2_64,
    rotl64,
    foldedMultiply,
} from "ahash";
```

## Core Types & Constants

```cp
class AHasher { buffer: u64, pad: u64, extra_keys: [2]u64, tail: [8]u8, /* +2 fields */ } // methods: init(...) +5
class RandomState { k0: u64, k1: u64, k2: u64, k3: u64 } // methods: withSeeds(...) +4
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn addWrap64(a: u64, b: u64): u64;
fn mulWrap64(a: u64, b: u64): u64;
fn pow2_64(n: u32): u64;
fn rotl64(x: u64, r_in: u32): u64;
fn foldedMultiply(x: u64, y: u64): u64;
fn multipleConst(): u64;
fn pi0(): u64;
fn pi1(): u64;
fn pi2(): u64;
fn pi3(): u64;
fn packTail(tail: [8]u8, tail_len: usize): u64;
fn hashBytes(data: &[]u8, k0: u64, k1: u64): u64;
fn hashU64(v: u64, k0: u64, k1: u64): u64;
```

## Minimal Usage

```cp
import { hashBytes } from "ahash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytes(out_buf[0..256], 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ahash"`; install via `cap add ahash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
