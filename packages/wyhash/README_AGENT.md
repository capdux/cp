# wyhash — AI Agent Quick Ref

Zero-allocation Wang Yi wyhash v4 64-bit hash and WyRng PRNG with overflow-free 128-bit multiply in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Mul128,
    WyRng,
    makeU64,
    mul128,
    wymum,
    read4,
    read8,
    wyhashBytes,
} from "wyhash";
```

## Core Types & Constants

```cp
class Mul128 { lo: u64, hi: u64 }
class WyRng { state: u64 } // methods: new(...) +3
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn mul128(a: u64, b: u64): Mul128;
fn wymum(a: u64, b: u64): u64;
fn read4(data: &[]u8, off: usize): u64;
fn read8(data: &[]u8, off: usize): u64;
fn wyhashBytes(data: &[]u8, seed: u64): u64;
fn wyhash(data: string, seed: u64): u64;
```

## Minimal Usage

```cp
import { wyhash } from "wyhash";

pub fn main() {
    let result = wyhash("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"wyhash"`; install via `cap add wyhash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
