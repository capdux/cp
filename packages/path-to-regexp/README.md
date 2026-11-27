[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# path-to-regexp

Zero-heap Express/Router URL path template matching, parameter extraction, and reverse route compilation (clean-room port of path-to-regexp v8)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:path-to-regexp` ([https://github.com/pillarjs/path-to-regexp](https://github.com/pillarjs/path-to-regexp)) / `npm:path-to-regexp` ([https://github.com/pillarjs/path-to-regexp](https://github.com/pillarjs/path-to-regexp))

| Metadata | Value |
| --- | --- |
| Package | `path-to-regexp` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add path-to-regexp
```

```cp
import {
    RouteParam,
    RouteMatch,
    ptrStrEq,
    ptrSegEq,
    ptrTrimSlashes,
    ptrFindSegEnd,
    ptrSkipSlashes,
    ptrWriteByte,
} from "path-to-regexp";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RouteParam` | `class` | Fields: `name: string, value: string` · Methods: `new` |
| `RouteMatch` | `class` | Fields: `matched: bool, params: [16]RouteParam, param_count: usize` · Methods: `noMatch, getParam` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ptrStrEq` | `(a: string, b: string): bool` | Invokes `ptrStrEq` and returns `bool`. |
| `ptrSegEq` | `(a: string, b: string, case_sensitive: bool): bool` | Invokes `ptrSegEq` and returns `bool`. |
| `ptrTrimSlashes` | `(s: string): string` | Invokes `ptrTrimSlashes` and returns `string`. |
| `ptrFindSegEnd` | `(s: string, pos: usize): usize` | Invokes `ptrFindSegEnd` and returns `usize`. |
| `ptrSkipSlashes` | `(s: string, pos: usize): usize` | Invokes `ptrSkipSlashes` and returns `usize`. |
| `ptrWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `ptrWriteByte` and returns `usize`. |
| `ptrWriteSlice` | `(out: []u8, pos: usize, s: string): usize` | Invokes `ptrWriteSlice` and returns `usize`. |
| `ptrFindParam` | `(params: &[]RouteParam, name: string): string?` | Invokes `ptrFindParam` and returns `string?`. |
| `matchPath` | `(pattern: string, path: string, case_sensitive: bool): RouteMatch` | Invokes `matchPath` and returns `RouteMatch`. |
| `compilePath` | `(pattern: string, params: &[]RouteParam, out: []u8): usize` | Invokes `compilePath` and returns `usize`. |
| `countRouteParams` | `(pattern: string): usize` | Invokes `countRouteParams` and returns `usize`. |

## Minimal Example

```cp
import { ptrStrEq } from "path-to-regexp";

pub fn main() {
    let result = ptrStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
