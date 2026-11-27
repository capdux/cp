# ryu — AI Agent Quick Ref

Pure CP zero-allocation fast float-to-string formatter (Rust ryu v1 parity, reuses itoa) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RyuDecimal,
    ryuPow10U64,
    ryuScalePow10,
    ryuStripTrailingZeros,
    ryuRoundSig,
    ryuExtractShortestF64,
    ryuExtractShortestF32,
    ryuCopySlice,
} from "ryu";
```

## Core Types & Constants

```cp
class RyuDecimal { sig: u64, exp10_msb: i32 }
```

## API Signatures

```cp
fn ryuPow10U64(n: u8): u64;
fn ryuScalePow10(x: f64, p_in: i32): f64;
fn ryuStripTrailingZeros(sig_in: u64): u64;
fn ryuRoundSig(m17: u64, drop_digits: u8): u64;
fn ryuExtractShortestF64(abs_val: f64): RyuDecimal;
fn ryuExtractShortestF32(abs_f32: f32): RyuDecimal;
fn ryuCopySlice(dst: []u8, pos: &mut usize, src: &[]u8): bool;
fn ryuPushByte(dst: []u8, pos: &mut usize, b: u8): bool;
fn ryuRenderDecimal(neg: bool, dec_val: RyuDecimal, out: []u8): usize;
fn ryuRoundAsciiDigits(digits: &[]u8, keep: usize, rounded: []u8): bool;
fn isNanF64(val: f64): bool;
fn isInfF64(val: f64): bool;
fn isFiniteF64(val: f64): bool;
fn formatF64(val: f64, out: []u8): usize;
fn formatF64Finite(val: f64, out: []u8): usize;
fn formatF32(val: f32, out: []u8): usize;
fn formatFixedF64(val: f64, decimals: u8, out: []u8): usize;
fn formatExpF64(val: f64, precision: u8, out: []u8): usize;
```

## Minimal Usage

```cp
import { isNanF64 } from "ryu";

pub fn main() {
    let result = isNanF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ryu"`; install via `cap add ryu`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
