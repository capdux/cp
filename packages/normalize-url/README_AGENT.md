# normalize-url — AI Agent Quick Ref

Zero-allocation URL normalization utility (npm normalize-url parity) composing prepend-http for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    NormalizeUrlOptions,
    NurlSegment,
    defaultNormalizeUrlOptions,
    nurlWriteByte,
    nurlWriteLower,
    nurlFindSchemeSep,
    nurlIsDefaultPort,
    nurlNormalizePath,
} from "normalize-url";
```

## Core Types & Constants

```cp
class NormalizeUrlOptions { default_https: bool, strip_www: bool, strip_hash: bool, remove_trailing_slash: bool, /* +2 fields */ }
class NurlSegment { start: usize, end: usize }
```

## API Signatures

```cp
fn defaultNormalizeUrlOptions(): NormalizeUrlOptions;
fn nurlWriteByte(out: []u8, pos: usize, b: u8): usize;
fn nurlWriteLower(out: []u8, pos: usize, s: string): usize;
fn nurlFindSchemeSep(s: string): usize;
fn nurlIsDefaultPort(scheme: string, port: string): bool;
fn nurlNormalizePath(raw_path: string, remove_trailing_slash: bool, has_query_or_hash: bool, out: []u8, start_pos: usize): usize;
fn normalizeUrlWithOptions(url: string, opts: NormalizeUrlOptions, out: []u8): usize;
fn normalizeUrl(url: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { nurlWriteLower } from "normalize-url";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = nurlWriteLower(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"normalize-url"`; install via `cap add normalize-url`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
