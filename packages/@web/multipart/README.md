[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/multipart

Official multipart/form-data parser and payload guard plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/multipart` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/multipart
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MultipartConfig` | `class` | Fields: `max_parts: usize, max_file_bytes: usize, max_total_bytes: usize, allowed_mime_prefix: string` |
| `MultipartPart` | `class` | Fields: `name: string, filename: string, content_type: string, data: string, is_file: bool` |
| `MultipartForm` | `class` | Fields: `parts: [16]MultipartPart, count: usize, total_bytes: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mpAsciiLower` | `(c: u8): u8` | Invokes `mpAsciiLower` and returns `u8`. |
| `mpStartsWithIgnoreCase` | `(s: string, prefix: string): bool` | Invokes `mpStartsWithIgnoreCase` and returns `bool`. |
| `mpEqlIgnoreCase` | `(a: string, b: string): bool` | Invokes `mpEqlIgnoreCase` and returns `bool`. |
| `mpTrimSpaces` | `(s: string): string` | Invokes `mpTrimSpaces` and returns `string`. |
| `mpParseUsize` | `(s: string): usize?` | Invokes `mpParseUsize` and returns `usize?`. |
| `mpMatchAt` | `(haystack: string, pos: usize, needle: string): bool` | Invokes `mpMatchAt` and returns `bool`. |
| `mpFindBoundary` | `(body: string, start_pos: usize, boundary: string): usize?` | Invokes `mpFindBoundary` and returns `usize?`. |
| `mpFindHeaderEnd` | `(part: string, body_start_out: &mut usize): usize?` | Invokes `mpFindHeaderEnd` and returns `usize?`. |
| `mpExtractParam` | `(header_val: string, key: string): string?` | Invokes `mpExtractParam` and returns `string?`. |
| `parseMultipartBody` | `(boundary: string, body: string, cfg: MultipartConfig, out_form: &mut MultipartForm): i32` | Parses input via `parseMultipartBody` and returns `i32`. |
| `multipartGuardHook` | `(ctx: &mut Context): bool` | Invokes `multipartGuardHook` and returns `bool`. |
| `createMultipartPlugin` | `(cfg: MultipartConfig): Plugin` | Constructs/initializes an instance via `createMultipartPlugin`. |
| `registerMultipart` | `(app: &mut App, cfg: MultipartConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { parseMultipartBody } from "@web/multipart";

pub fn main() {
    let result = parseMultipartBody("example", "example", /* cfg: MultipartConfig */, /* out_form: &mut MultipartForm */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
