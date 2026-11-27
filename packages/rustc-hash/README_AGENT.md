# rustc-hash — AI Agent Quick Ref

Zero-heap pure-CP fast non-cryptographic FxHasher (64-bit & 32-bit, crates.io rustc-hash equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FxHasher64,
    FxHasher32,
    fxhMul64,
    fxhMul32,
    fxhRotl64By5,
    fxhRotl32By5,
    fxhAddWord64,
    fxhAddWord32,
} from "rustc-hash";
```

## Core Types & Constants

```cp
class FxHasher64 { hash: u64 } // methods: new(...) +8
class FxHasher32 { hash: u32 } // methods: new(...) +6
```

## API Signatures

```cp
fn fxhMul64(a: u64, b: u64): u64;
fn fxhMul32(a: u32, b: u32): u32;
fn fxhRotl64By5(x: u64): u64;
fn fxhRotl32By5(x: u32): u32;
fn fxhAddWord64(hash: u64, word: u64): u64;
fn fxhAddWord32(hash: u32, word: u32): u32;
fn fxhHashSlice64(hash: u64, data: &[]u8): u64;
fn fxhHashSlice32(hash: u32, data: &[]u8): u32;
fn fxHash64(data: []u8): u64;
fn fxHash64Str(s: string): u64;
fn fxHash32(data: []u8): u32;
```

## Minimal Usage

```cp
import { fxHash64Str } from "rustc-hash";

pub fn main() {
    let result = fxHash64Str("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rustc-hash"`; install via `cap add rustc-hash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
