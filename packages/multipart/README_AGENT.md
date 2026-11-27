# multipart — AI Agent Quick Ref

Zero-heap RFC 7578 multipart/form-data streaming reader, writer, and boundary extractor built on mediatype and unicase (clean-room port of Rust multipart/multer) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MultipartPart,
    MultipartReader,
    MultipartWriter,
    mpartIsSpace,
    mpartMatchAt,
    mpartIsBoundaryAt,
    mpartFindFirstBoundary,
    mpartFindNextBoundary,
} from "multipart";
```

## Core Types & Constants

```cp
class MultipartPart { name: string, filename: string, content_type: string, data: string, /* +1 fields */ }
class MultipartReader { boundary: string, body: string, cursor: usize, finished: bool } // methods: new(...) +2
class MultipartWriter { boundary: string } // methods: new(...) +3
```

## API Signatures

```cp
fn mpartIsSpace(c: u8): bool;
fn mpartMatchAt(haystack: string, pos: usize, needle: string): bool;
fn mpartIsBoundaryAt(body: string, pos: usize, boundary: string): bool;
fn mpartFindFirstBoundary(body: string, boundary: string): usize;
fn mpartFindNextBoundary(body: string, start: usize, boundary: string, data_end_out: &mut usize, next_bnd_out: &mut usize): bool;
fn mpartSkipLineEnd(body: string, pos: &mut usize): bool;
fn mpartParseDisposition(val: string, part: &mut MultipartPart);
fn extractBoundary(content_type: string): string?;
fn formatContentTypeHeader(boundary: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatContentTypeHeader } from "multipart";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatContentTypeHeader("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"multipart"`; install via `cap add multipart`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
