# data-encoding — AI Agent Quick Ref

Zero-allocation strict RFC 4648 Base16/Hex, Base32, and Base64/Base64URL codec with canonical trailing-bit verification (Rust data-encoding v2 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DecodeResult,
    dencHexDigit,
    dencHexVal,
    dencB32Char,
    dencB32Val,
    dencB64Char,
    dencB64Val,
    dencHexEncodeImpl,
} from "data-encoding";
```

## Core Types & Constants

```cp
class DecodeResult { ok: bool, len: usize, error_pos: usize } // methods: success(...) +1
```

## API Signatures

```cp
fn dencHexDigit(v: u8, upper: bool): u8;
fn dencHexVal(c: u8): u8;
fn dencB32Char(v: u8): u8;
fn dencB32Val(c: u8): u8;
fn dencB64Char(v: u8, url_safe: bool): u8;
fn dencB64Val(c: u8, url_safe: bool): u8;
fn dencHexEncodeImpl(src: &[]u8, upper: bool, out: []u8): usize;
fn base64EncodeLen(n: usize, pad: bool): usize;
fn base32EncodeLen(n: usize, pad: bool): usize;
fn hexLowerEncode(src: &[]u8, out: []u8): usize;
fn hexUpperEncode(src: &[]u8, out: []u8): usize;
fn hexDecode(src: &[]u8, out: []u8): DecodeResult;
fn base32Encode(src: &[]u8, pad: bool, out: []u8): usize;
fn base32Decode(src: &[]u8, out: []u8): DecodeResult;
fn base64Encode(src: &[]u8, url_safe: bool, pad: bool, out: []u8): usize;
fn base64Decode(src: &[]u8, url_safe: bool, out: []u8): DecodeResult;
```

## Minimal Usage

```cp
import { dencHexDigit } from "data-encoding";

pub fn main() {
    let result = dencHexDigit(0, false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"data-encoding"`; install via `cap add data-encoding`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
