# content-disposition — AI Agent Quick Ref

Zero-allocation RFC 6266 / RFC 5987 HTTP Content-Disposition header parser, formatter, filename* decoder, and filename sanitizer for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ContentDisposition,
    cdspIsOws,
    cdspIsTokenChar,
    cdspSkipOws,
    cdspHasNonAscii,
    cdspWriteByte,
    cdspWriteStr,
    cdspWriteAsciiFallback,
} from "content-disposition";
```

## Core Types & Constants

```cp
class ContentDisposition { valid: bool, disp_type: string, filename: string, filename_star: string } // methods: invalid(...) +3
```

## API Signatures

```cp
fn cdspIsOws(b: u8): bool;
fn cdspIsTokenChar(b: u8): bool;
fn cdspSkipOws(s: string, start: usize): usize;
fn cdspHasNonAscii(s: string): bool;
fn cdspWriteByte(out: []u8, pos: usize, b: u8): usize;
fn cdspWriteStr(out: []u8, pos: usize, s: string): usize;
fn cdspWriteAsciiFallback(out: []u8, pos: usize, s: string): usize;
fn cdspRfc5987Set(): AsciiSet;
fn cdspIsControl(b: u8): bool;
fn parseContentDisposition(header: string): ContentDisposition;
fn sanitizeFilename(raw: string, out: []u8): usize;
fn decodeFilenameStar(raw_star: string, out: []u8): usize;
fn formatContentDisposition(disp_type: string, filename: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseContentDisposition } from "content-disposition";

pub fn main() {
    let result = parseContentDisposition("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"content-disposition"`; install via `cap add content-disposition`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
