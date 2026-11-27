[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# busboy

Zero-heap RFC 7578 multipart/form-data boundary extractor, part iterator, and Content-Disposition field/filename parser.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:busboy` ([https://github.com/mscdex/busboy](https://github.com/mscdex/busboy)) / `crates.io:multer` ([https://github.com/rousan/multer-rs](https://github.com/rousan/multer-rs))（原库名 `busboy` → CP 包名 `busboy`）

| Metadata | Value |
| --- | --- |
| Package | `busboy` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add busboy
```

```cp
import {
    MultipartPart,
    isWs,
    isBchar,
    isValidBoundary,
    extractBoundary,
    matchBoundaryAt,
    extractDispositionParam,
    nextPart,
} from "busboy";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MultipartPart` | `class` | Fields: `headers: string, body: string, name: string, filename: string, content_type: string, next_pos: usize, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isWs` | `(c: u8): bool` | Checks predicate `isWs` and returns `bool`. |
| `isBchar` | `(c: u8): bool` | Checks predicate `isBchar` and returns `bool`. |
| `isValidBoundary` | `(boundary: string): bool` | Checks predicate `isValidBoundary` and returns `bool`. |
| `extractBoundary` | `(content_type: string): string` | Invokes `extractBoundary` and returns `string`. |
| `matchBoundaryAt` | `(payload: string, pos: usize, boundary: string): bool` | Invokes `matchBoundaryAt` and returns `bool`. |
| `extractDispositionParam` | `(disp: string, key: string): string` | Invokes `extractDispositionParam` and returns `string`. |
| `nextPart` | `(payload: string, boundary: string, start: usize): MultipartPart` | Invokes `nextPart` and returns `MultipartPart`. |
| `countParts` | `(payload: string, boundary: string): usize` | Invokes `countParts` and returns `usize`. |

## Minimal Example

```cp
import { isValidBoundary } from "busboy";

pub fn main() {
    let result = isValidBoundary("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
