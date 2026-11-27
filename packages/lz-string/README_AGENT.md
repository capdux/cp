# lz-string — AI Agent Quick Ref

Zero-heap LZW byte/string compression with Base64 and URI-safe encoding for CP (clean-room MIT implementation of npm lz-string). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    lzsHash,
    lzsClearTable,
    lzsFind,
    lzsInsert,
    compressBytes,
    decompressBytes,
    lzsEncode64,
    lzsDecodeChar,
} from "lz-string";
```

## API Signatures

```cp
fn lzsHash(key: u32): usize;
fn lzsClearTable(ht_key: []u32);
fn lzsFind(ht_key: &[]u32, ht_val: &[]u16, key: u32): i32;
fn lzsInsert(ht_key: []u32, ht_val: []u16, key: u32, code: u16);
fn compressBytes(input: &[]u8, out: []u8): usize;
fn decompressBytes(compressed: &[]u8, out: []u8): i64;
fn lzsEncode64(raw: &[]u8, alphabet: string, pad: bool, out: []u8): usize;
fn lzsDecodeChar(ch: u8, uri_mode: bool): i32;
fn lzsDecode64(text: string, uri_mode: bool, out: []u8): i64;
fn compressToBase64(input: string, out: []u8): usize;
fn decompressFromBase64(b64: string, out: []u8): i64;
fn compressToEncodedURIComponent(input: string, out: []u8): usize;
fn decompressFromEncodedURIComponent(uri: string, out: []u8): i64;
```

## Minimal Usage

```cp
import { lzsDecode64 } from "lz-string";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lzsDecode64("example", false, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lz-string"`; install via `cap add lz-string`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
