# mediatype — AI Agent Quick Ref

Zero-allocation RFC 9110 / RFC 6838 HTTP Media Type (MIME) parser, structured suffix extractor, and parameter matcher for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MediaType,
    MediaParam,
    ParsedMediaType,
    mtToLower,
    mtIsOws,
    mtTrimOws,
    mtEqIgnoreCase,
    mtIsTchar,
} from "mediatype";
```

## Core Types & Constants

```cp
class MediaType { top: string, subtype: string, suffix: string, params: string }
class MediaParam { name: string, value: string }
class ParsedMediaType { valid: bool, top_level: string, sub_level: string, suffix: string, /* +2 fields */ } // methods: invalid(...) +7
```

## API Signatures

```cp
fn mtToLower(c: u8): u8;
fn mtIsOws(c: u8): bool;
fn mtTrimOws(s: &[]u8): &[]u8;
fn mtEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
fn mtIsTchar(c: u8): bool;
fn isValidToken(s: string): bool;
fn parseMediaType(input: string, out: &mut MediaType): bool;
fn getParam(mt: MediaType, name: string, out: []u8): usize;
fn formatMediaType(mt: MediaType, out: []u8): usize;
fn isJsonLike(mt: MediaType): bool;
fn isTextLike(mt: MediaType): bool;
fn essenceEq(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { isValidToken } from "mediatype";

pub fn main() {
    let result = isValidToken("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mediatype"`; install via `cap add mediatype`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
