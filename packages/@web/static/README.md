[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/static

Official zero-GC static file serving plugin for cap:web (path traversal guard, extension + magic-number MIME detection, FNV-1a ETag & 304 Not Modified)

| Metadata | Value |
| --- | --- |
| Package | `@web/static` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/static
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StaticConfig` | `class` | Fields: `root_dir: string, url_prefix: string, index_file: string, cache_control: string, enable_etag: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `stAsciiLower` | `(c: u8): u8` | Invokes `stAsciiLower` and returns `u8`. |
| `stAsciiEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `stAsciiEqlIgnoreCase` and returns `bool`. |
| `stTrimAsciiSpaces` | `(s: string): string` | Invokes `stTrimAsciiSpaces` and returns `string`. |
| `stHexVal` | `(c: u8): u8?` | Invokes `stHexVal` and returns `u8?`. |
| `stHexDigitLower` | `(n: u8): u8` | Invokes `stHexDigitLower` and returns `u8`. |
| `stWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | Invokes `stWriteByte` and returns `bool`. |
| `stWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | Invokes `stWriteRaw` and returns `bool`. |
| `sanitizeRelPath` | `(raw: string, out_buf: []u8): usize?` | Invokes `sanitizeRelPath` and returns `usize?`. |
| `detectMimeType` | `(path: string, content: string): string` | Invokes `detectMimeType` and returns `string`. |
| `computeStrongEtag` | `(content: string, out_buf: []u8): usize?` | Invokes `computeStrongEtag` and returns `usize?`. |
| `etagMatches` | `(if_none_match: string, etag: string): bool` | Invokes `etagMatches` and returns `bool`. |
| `stReadRegularFileInto` | `(full_path: string, out_buf: []u8): usize?` | Invokes `stReadRegularFileInto` and returns `usize?`. |
| `stJoinFsPath` | `(root_dir: string, rel_path: string, out_buf: []u8): usize?` | Invokes `stJoinFsPath` and returns `usize?`. |
| `stServeWithEtagBuf` | `(cfg: StaticConfig, rel_path: string, if_none_match: string?, out_buf: []u8, etag_buf: []u8): Response` | Invokes `stServeWithEtagBuf` and returns `Response`. |
| `serveStaticRequest` | `(cfg: StaticConfig, rel_path: string, if_none_match: string?, out_buf: []u8): Response` | Invokes `serveStaticRequest` and returns `Response`. |
| `serveFileFromDir` | `(root_dir: string, rel_path: string, out_buf: []u8): Response` | Invokes `serveFileFromDir` and returns `Response`. |
| `serveStaticIntoArena` | `(ctx: &mut Context, cfg: StaticConfig, rel_path: string): Response` | Invokes `serveStaticIntoArena` and returns `Response`. |
| `stRouteHandler` | `(ctx: &mut Context): Response` | Invokes `stRouteHandler` and returns `Response`. |
| `registerStatic` | `(app: &mut App, cfg: StaticConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { stAsciiEqlIgnoreCase } from "@web/static";

pub fn main() {
    let result = stAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
