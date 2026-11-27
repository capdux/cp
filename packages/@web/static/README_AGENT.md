# @web/static — AI Agent Quick Ref

Official zero-GC static file serving plugin for cap:web (path traversal guard, extension + magic-number MIME detection, FNV-1a ETag & 304 Not Modified) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StaticConfig,
    stAsciiLower,
    stAsciiEqlIgnoreCase,
    stTrimAsciiSpaces,
    stHexVal,
    stHexDigitLower,
    stWriteByte,
    stWriteRaw,
} from "@web/static";
```

## Core Types & Constants

```cp
class StaticConfig { root_dir: string, url_prefix: string, index_file: string, cache_control: string, /* +1 fields */ }
```

## API Signatures

```cp
fn stAsciiLower(c: u8): u8;
fn stAsciiEqlIgnoreCase(a: string, b: string): bool;
fn stTrimAsciiSpaces(s: string): string;
fn stHexVal(c: u8): u8?;
fn stHexDigitLower(n: u8): u8;
fn stWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn stWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn sanitizeRelPath(raw: string, out_buf: []u8): usize?;
fn detectMimeType(path: string, content: string): string;
fn computeStrongEtag(content: string, out_buf: []u8): usize?;
fn etagMatches(if_none_match: string, etag: string): bool;
fn stReadRegularFileInto(full_path: string, out_buf: []u8): usize?;
fn stJoinFsPath(root_dir: string, rel_path: string, out_buf: []u8): usize?;
fn stServeWithEtagBuf(cfg: StaticConfig, rel_path: string, if_none_match: string?, out_buf: []u8, etag_buf: []u8): Response;
fn serveStaticRequest(cfg: StaticConfig, rel_path: string, if_none_match: string?, out_buf: []u8): Response;
fn serveFileFromDir(root_dir: string, rel_path: string, out_buf: []u8): Response;
fn serveStaticIntoArena(ctx: &mut Context, cfg: StaticConfig, rel_path: string): Response;
fn stRouteHandler(ctx: &mut Context): Response;
fn registerStatic(app: &mut App, cfg: StaticConfig): &mut App;
```

## Minimal Usage

```cp
import { stAsciiEqlIgnoreCase } from "@web/static";

pub fn main() {
    let result = stAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/static"`; install via `cap add @web/static`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
