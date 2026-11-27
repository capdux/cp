# content-type — AI Agent Quick Ref

Zero-heap RFC 7231 HTTP Content-Type header parser and formatter with quoted-string unescaping (clean-room port of content-type) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ContentTypeParam,
    ContentType,
    ctypIsOws,
    ctypIsTokenChar,
    ctypIsValidToken,
    ctypIsQuotedAllowed,
    ctypWriteByte,
    ctypWriteSlice,
} from "content-type";
```

## Core Types & Constants

```cp
class ContentTypeParam { name: string, value: string } // methods: new(...)
class ContentType { valid: bool, media_type: string, params: [8]ContentTypeParam, param_count: usize } // methods: invalid(...) +4
```

## API Signatures

```cp
fn ctypIsOws(c: u8): bool;
fn ctypIsTokenChar(c: u8): bool;
fn ctypIsValidToken(s: string): bool;
fn ctypIsQuotedAllowed(c: u8): bool;
fn ctypWriteByte(out: []u8, pos: usize, b: u8): usize;
fn ctypWriteSlice(out: []u8, pos: usize, s: string): usize;
fn isValidMediaType(s: string): bool;
fn unescapeQuotedString(raw: string, out: []u8): usize;
fn parseContentType(header: string): ContentType;
fn formatContentType(media_type: string, params: &[]ContentTypeParam, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidMediaType } from "content-type";

pub fn main() {
    let result = isValidMediaType("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"content-type"`; install via `cap add content-type`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
