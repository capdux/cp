# base64ct — AI Agent Quick Ref

Pure CP zero-allocation constant-time (table-free) Base64 encoder and strict canonical decoder (RustCrypto base64ct parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DecodeResult,
    b64ctInRange,
    b64ctEq,
    b64ctEncode6Std,
    b64ctEncode6Url,
    b64ctDecode6Std,
    b64ctDecode6Url,
    encodedLen,
} from "base64ct";
```

## Core Types & Constants

```cp
class DecodeResult { len: usize, ok: bool }
```

## API Signatures

```cp
fn b64ctInRange(c: u32, lo: u32, hi: u32): u32;
fn b64ctEq(c: u32, target: u32): u32;
fn b64ctEncode6Std(x: u8): u8;
fn b64ctEncode6Url(x: u8): u8;
fn b64ctDecode6Std(ch: u8): u32;
fn b64ctDecode6Url(ch: u8): u32;
fn encodedLen(n: usize, padded: bool): usize;
fn b64ctEncodeCore(src: []u8, out: []u8, url_safe: bool, padded: bool): usize;
fn encodeStd(src: []u8, out: []u8): usize;
fn encodeStdUnpadded(src: []u8, out: []u8): usize;
fn encodeUrl(src: []u8, out: []u8): usize;
fn encodeUrlUnpadded(src: []u8, out: []u8): usize;
fn b64ctZeroOut(out: []u8, n: usize);
fn decodeStd(src: string, out: []u8): DecodeResult;
fn decodeUrlUnpadded(src: string, out: []u8): DecodeResult;
```

## Minimal Usage

```cp
import { decodeStd } from "base64ct";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeStd("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"base64ct"`; install via `cap add base64ct`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
