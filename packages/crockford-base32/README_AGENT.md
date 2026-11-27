# crockford-base32 — AI Agent Quick Ref

Zero-allocation Douglas Crockford Base32 encoder/decoder for 64-bit integers (with optional mod-37 check symbol), arbitrary byte streams, and grouped formatting in CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    bcBuildDecode32Lut,
    bcBuildDecode37Lut,
    bcEncodeDigit32,
    bcEncodeCheck37,
    bcDecodeDigit32,
    bcDecodeCheck37,
    encodeU64,
    decodeU64,
} from "crockford-base32";
```

## API Signatures

```cp
fn bcBuildDecode32Lut(): [256]u8;
fn bcBuildDecode37Lut(): [256]u8;
fn bcEncodeDigit32(v: u8): u8;
fn bcEncodeCheck37(v: u8): u8;
fn bcDecodeDigit32(c: u8): u8;
fn bcDecodeCheck37(c: u8): u8;
fn encodeU64(val: u64, with_check: bool, out: []u8): usize;
fn decodeU64(input: string, with_check: bool, out_val: &mut u64): bool;
fn encodeBytes(data: &[]u8, out: []u8): usize;
fn decodeBytes(input: string, out: []u8): i64;
fn formatGrouped(raw: string, group_size: usize, out: []u8): usize;
```

## Minimal Usage

```cp
import { decodeU64 } from "crockford-base32";

pub fn main() {
    let result = decodeU64("example", false, /* out_val: &mut u64 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"crockford-base32"`; install via `cap add crockford-base32`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
