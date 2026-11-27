[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jsonpointer

Zero-allocation RFC 6901 JSON Pointer validator, escaper, segment navigator, array index parser, and iterator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsonpointer` ([https://github.com/janl/node-jsonpointer](https://github.com/janl/node-jsonpointer))（原库名 `jsonpointer` → CP 包名 `jsonpointer`）

| Metadata | Value |
| --- | --- |
| Package | `jsonpointer` |
| Version | `0.1.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jsonpointer
```

```cp
import {
    PointerIter,
    isValid,
    escapeToken,
    unescapeToken,
    countSegments,
    hasSegment,
    getSegment,
    parseArrayIndex,
} from "jsonpointer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PointerIter` | `class` | Fields: `ptr: string, pos: usize, valid: bool` · Methods: `init, next` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isValid` | `(ptr: string): bool` | Checks predicate `isValid` and returns `bool`. |
| `escapeToken` | `(token: string, out: []u8): usize` | Invokes `escapeToken` and returns `usize`. |
| `unescapeToken` | `(token: string, out: []u8): usize` | Invokes `unescapeToken` and returns `usize`. |
| `countSegments` | `(ptr: string): usize` | Invokes `countSegments` and returns `usize`. |
| `hasSegment` | `(ptr: string, index: usize): bool` | Checks predicate `hasSegment` and returns `bool`. |
| `getSegment` | `(ptr: string, index: usize, out: []u8): usize` | Queries/retrieves data via `getSegment` returning `usize`. |
| `parseArrayIndex` | `(token: string): i64` | Parses input via `parseArrayIndex` and returns `i64`. |
| `appendSegment` | `(base: string, token: string, out: []u8): usize` | Invokes `appendSegment` and returns `usize`. |
| `parentPointer` | `(ptr: string): string` | Invokes `parentPointer` and returns `string`. |

## Minimal Example

```cp
import { isValid } from "jsonpointer";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
