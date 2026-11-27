# url — AI Agent Quick Ref

Pure CP zero-allocation WHATWG URL Standard parser, normalizer, and relative URL resolver (Rust url v2 parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    WhatwgUrl,
    wuIsSchemeChar,
    wuAppendBytes,
    wuAppendByte,
    wuAppendPort,
    wuIsSpecialScheme,
    wuIsOriginScheme,
    wuIsForbiddenHostByte,
} from "url";
```

## Core Types & Constants

```cp
class WhatwgUrl { scheme: [32]u8, scheme_len: usize, username: [128]u8, username_len: usize, /* +15 fields */ } // methods: invalid(...) +12
```

## API Signatures

```cp
fn wuIsSchemeChar(ch: u8): bool;
fn wuAppendBytes(out: []u8, pos: &mut usize, src: &[]u8): bool;
fn wuAppendByte(out: []u8, pos: &mut usize, b: u8): bool;
fn wuAppendPort(out: []u8, pos: &mut usize, port: i32): bool;
fn wuIsSpecialScheme(scheme: &[]u8): bool;
fn wuIsOriginScheme(scheme: &[]u8): bool;
fn wuIsForbiddenHostByte(b: u8): bool;
fn wuIsSingleDotSegment(seg: &[]u8): bool;
fn wuIsDoubleDotSegment(seg: &[]u8): bool;
fn wuTrimBounds(input: string, out_start: &mut usize, out_end: &mut usize);
fn wuFindSchemeColon(s: &[]u8): i64;
fn wuNormalizePath(raw: &[]u8, is_special: bool, out: []u8): usize;
fn wuParsePortStr(port_str: &[]u8): i32;
fn wuParseIpv6Host(inner: &[]u8, out: []u8): i64;
fn defaultPortForScheme(scheme: string): i32;
fn wuCloneUrl(src: WhatwgUrl): WhatwgUrl;
```

## Minimal Usage

```cp
import { defaultPortForScheme } from "url";

pub fn main() {
    let result = defaultPortForScheme("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"url"`; install via `cap add url`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
