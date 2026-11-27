# foldhash — AI Agent Quick Ref

Zero-allocation 64x64->128-bit folded-multiply non-cryptographic hasher with Fast and Quality modes (Rust foldhash parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FoldHasher,
    makeU64,
    addWrap64,
    foldedMultiply,
    byteShift64,
    initFoldSeed,
    initAccum,
    hashBytesFast,
} from "foldhash";
```

## Core Types & Constants

```cp
class FoldHasher { accum: u64, sponge: u64, sponge_len: u8, fold_seed: u64, /* +2 fields */ } // methods: initFast(...) +6
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn addWrap64(a: u64, b: u64): u64;
fn foldedMultiply(x: u64, y: u64): u64;
fn byteShift64(idx: u8): u64;
fn initFoldSeed(seed: u64): u64;
fn initAccum(seed: u64): u64;
fn hashBytesFast(data: &[]u8, seed: u64): u64;
fn hashBytesQuality(data: &[]u8, seed: u64): u64;
fn hashU64(v: u64, seed: u64): u64;
```

## Minimal Usage

```cp
import { hashBytesFast } from "foldhash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytesFast(out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"foldhash"`; install via `cap add foldhash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
