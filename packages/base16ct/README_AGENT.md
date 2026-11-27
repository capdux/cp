# base16ct — AI Agent Quick Ref

Pure CP zero-allocation constant-time (table-free) Base16 / Hex encoder and strict decoder (RustCrypto base16ct parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HexDecodeResult,
    b16InRange,
    b16EncodeNibbleLower,
    b16EncodeNibbleUpper,
    b16DecodeNibble,
    b16ZeroOut,
    encodedLen,
    encodeLower,
} from "base16ct";
```

## Core Types & Constants

```cp
class HexDecodeResult { len: usize, ok: bool }
```

## API Signatures

```cp
fn b16InRange(c: u32, lo: u32, hi: u32): u32;
fn b16EncodeNibbleLower(nib: u8): u8;
fn b16EncodeNibbleUpper(nib: u8): u8;
fn b16DecodeNibble(ch: u8, allow_lower: bool, allow_upper: bool): u32;
fn b16ZeroOut(out: []u8, n: usize);
fn encodedLen(n: usize): usize;
fn encodeLower(src: []u8, out: []u8): usize;
fn encodeUpper(src: []u8, out: []u8): usize;
fn b16DecodeCore(hex: string, out: []u8, allow_lower: bool, allow_upper: bool): HexDecodeResult;
fn decodeLower(hex: string, out: []u8): HexDecodeResult;
fn decodeUpper(hex: string, out: []u8): HexDecodeResult;
fn decodeMixed(hex: string, out: []u8): HexDecodeResult;
```

## Minimal Usage

```cp
import { decodeLower } from "base16ct";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeLower("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"base16ct"`; install via `cap add base16ct`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
