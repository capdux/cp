# @web/multipart — AI Agent Quick Ref

Official multipart/form-data parser and payload guard plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MultipartConfig,
    MultipartPart,
    MultipartForm,
    mpAsciiLower,
    mpStartsWithIgnoreCase,
    mpEqlIgnoreCase,
    mpTrimSpaces,
    mpParseUsize,
} from "@web/multipart";
```

## Core Types & Constants

```cp
class MultipartConfig { max_parts: usize, max_file_bytes: usize, max_total_bytes: usize, allowed_mime_prefix: string }
class MultipartPart { name: string, filename: string, content_type: string, data: string, /* +1 fields */ }
class MultipartForm { parts: [16]MultipartPart, count: usize, total_bytes: usize }
```

## API Signatures

```cp
fn mpAsciiLower(c: u8): u8;
fn mpStartsWithIgnoreCase(s: string, prefix: string): bool;
fn mpEqlIgnoreCase(a: string, b: string): bool;
fn mpTrimSpaces(s: string): string;
fn mpParseUsize(s: string): usize?;
fn mpMatchAt(haystack: string, pos: usize, needle: string): bool;
fn mpFindBoundary(body: string, start_pos: usize, boundary: string): usize?;
fn mpFindHeaderEnd(part: string, body_start_out: &mut usize): usize?;
fn mpExtractParam(header_val: string, key: string): string?;
fn parseMultipartBody(boundary: string, body: string, cfg: MultipartConfig, out_form: &mut MultipartForm): i32;
fn multipartGuardHook(ctx: &mut Context): bool;
fn createMultipartPlugin(cfg: MultipartConfig): Plugin;
fn registerMultipart(app: &mut App, cfg: MultipartConfig): &mut App;
```

## Minimal Usage

```cp
import { parseMultipartBody } from "@web/multipart";

pub fn main() {
    let result = parseMultipartBody("example", "example", /* cfg: MultipartConfig */, /* out_form: &mut MultipartForm */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/multipart"`; install via `cap add @web/multipart`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
