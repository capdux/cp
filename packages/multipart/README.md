[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# multipart

Zero-heap RFC 7578 multipart/form-data streaming reader, writer, and boundary extractor built on mediatype and unicase (clean-room port of Rust multipart/multer)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:multipart` ([https://github.com/abonander/multipart](https://github.com/abonander/multipart))

| Metadata | Value |
| --- | --- |
| Package | `multipart` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add multipart
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MultipartPart` | `class` | Fields: `name: string, filename: string, content_type: string, data: string, is_file: bool` |
| `MultipartReader` | `class` | Fields: `boundary: string, body: string, cursor: usize, finished: bool` · Methods: `new, nextPart, countParts` |
| `MultipartWriter` | `class` | Fields: `boundary: string` · Methods: `new, writeField, writeFile, finish` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mpartIsSpace` | `(c: u8): bool` | Invokes `mpartIsSpace` and returns `bool`. |
| `mpartMatchAt` | `(haystack: string, pos: usize, needle: string): bool` | Invokes `mpartMatchAt` and returns `bool`. |
| `mpartIsBoundaryAt` | `(body: string, pos: usize, boundary: string): bool` | Invokes `mpartIsBoundaryAt` and returns `bool`. |
| `mpartFindFirstBoundary` | `(body: string, boundary: string): usize` | Invokes `mpartFindFirstBoundary` and returns `usize`. |
| `mpartFindNextBoundary` | `(body: string, start: usize, boundary: string, data_end_out: &mut usize, next_bnd_out: &mut usize): bool` | Invokes `mpartFindNextBoundary` and returns `bool`. |
| `mpartSkipLineEnd` | `(body: string, pos: &mut usize): bool` | Invokes `mpartSkipLineEnd` and returns `bool`. |
| `mpartParseDisposition` | `(val: string, part: &mut MultipartPart)` | Executes `mpartParseDisposition` in-place. |
| `extractBoundary` | `(content_type: string): string?` | Invokes `extractBoundary` and returns `string?`. |
| `formatContentTypeHeader` | `(boundary: string, out: []u8): usize` | Serializes/formats data via `formatContentTypeHeader` returning `usize`. |

## Minimal Example

```cp
import { formatContentTypeHeader } from "multipart";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatContentTypeHeader("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
