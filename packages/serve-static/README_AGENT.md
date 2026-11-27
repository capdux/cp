# serve-static — AI Agent Quick Ref

Zero-heap HTTP static file request resolver, dotfile policy evaluator, Cache-Control builder, and MIME/ETag helper (clean-room port of npm serve-static) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ServeStaticOptions,
    StaticResolution,
    sstPush,
    sstIsGetOrHead,
    hasDotfileSegment,
    resolveStaticRequest,
    formatCacheControl,
    formatStaticEtag,
} from "serve-static";
```

## Core Types & Constants

```cp
class ServeStaticOptions { index_file: string, dotfiles: u8, max_age_secs: u64, immutable: bool, /* +1 fields */ } // methods: defaultOpts(...)
class StaticResolution { status: u16, rel_path_len: usize, mime_type: string }
```

## API Signatures

```cp
fn sstPush(out: []u8, pos: usize, s: string): usize;
fn sstIsGetOrHead(method: string): bool;
fn hasDotfileSegment(pathname: string): bool;
fn resolveStaticRequest(method: string, raw_url: string, opts: ServeStaticOptions, path_out: []u8): StaticResolution;
fn formatCacheControl(max_age_secs: u64, immutable: bool, out: []u8): usize;
fn formatStaticEtag(size_bytes: u64, mtime_ms: u64, out: []u8): usize;
fn formatStaticContentType(file_path: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatStaticContentType } from "serve-static";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatStaticContentType("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"serve-static"`; install via `cap add serve-static`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
