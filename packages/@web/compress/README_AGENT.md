# @web/compress — AI Agent Quick Ref

Official HTTP response compression plugin for cap:web with pure-CP zero-heap RFC 1952 Gzip encoder (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EncodingKind,
    CompressConfig,
    cmpAsciiLower,
    cmpEqlIgnoreCase,
    cmpTrimSpaces,
    cmpParseQValue,
    negotiateEncoding,
    cmpCrc32,
} from "@web/compress";
```

## Core Types & Constants

```cp
enum EncodingKind { None, Gzip, Deflate }
class CompressConfig { min_bytes: usize, prefer_gzip: bool }
```

## API Signatures

```cp
fn cmpAsciiLower(c: u8): u8;
fn cmpEqlIgnoreCase(a: string, b: string): bool;
fn cmpTrimSpaces(s: string): string;
fn cmpParseQValue(raw: string): i32;
fn negotiateEncoding(accept_encoding: string, prefer_gzip: bool): EncodingKind;
fn cmpCrc32(data: string): u32;
fn cmpWriteU16Le(out_buf: []u8, pos: usize, val: u16);
fn cmpWriteU32Le(out_buf: []u8, pos: usize, val: u32);
fn encodeGzipStored(src: string, out_buf: []u8): usize?;
fn compressResponseHook(ctx: &mut Context, resp: &mut Response);
fn createCompressPlugin(cfg: CompressConfig): Plugin;
fn registerCompress(app: &mut App, cfg: CompressConfig): &mut App;
```

## Minimal Usage

```cp
import { encodeGzipStored } from "@web/compress";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeGzipStored("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/compress"`; install via `cap add @web/compress`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
