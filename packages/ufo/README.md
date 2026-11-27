[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ufo

Zero-heap URL protocol detection, slash normalization, and safe URL joining utilities

| Metadata | Value |
| --- | --- |
| Package | `ufo` |
| Version | `1.5.4` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ufo
```

```cp
import {
    ufoIsAlpha,
    ufoIsSchemeChar,
    ufoProtocolEnd,
    ufoCopyBytes,
    hasProtocol,
    isRelative,
    withoutLeadingSlash,
    withoutTrailingSlash,
} from "ufo";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ufoIsAlpha` | `(c: u8): bool` | Invokes `ufoIsAlpha` and returns `bool`. |
| `ufoIsSchemeChar` | `(c: u8): bool` | Invokes `ufoIsSchemeChar` and returns `bool`. |
| `ufoProtocolEnd` | `(url: string): usize` | Invokes `ufoProtocolEnd` and returns `usize`. |
| `ufoCopyBytes` | `(src: string, out: []u8, offset: usize): usize` | Invokes `ufoCopyBytes` and returns `usize`. |
| `hasProtocol` | `(url: string): bool` | Checks predicate `hasProtocol` and returns `bool`. |
| `isRelative` | `(url: string): bool` | Checks predicate `isRelative` and returns `bool`. |
| `withoutLeadingSlash` | `(s: string): string` | Invokes `withoutLeadingSlash` and returns `string`. |
| `withoutTrailingSlash` | `(s: string): string` | Invokes `withoutTrailingSlash` and returns `string`. |
| `withLeadingSlash` | `(s: string, out: []u8): usize` | Invokes `withLeadingSlash` and returns `usize`. |
| `withTrailingSlash` | `(s: string, out: []u8): usize` | Invokes `withTrailingSlash` and returns `usize`. |
| `cleanDoubleSlashes` | `(url: string, out: []u8): usize` | Invokes `cleanDoubleSlashes` and returns `usize`. |
| `joinUrl` | `(base: string, segment: string, out: []u8): usize` | Invokes `joinUrl` and returns `usize`. |

## Minimal Example

```cp
import { isRelative } from "ufo";

pub fn main() {
    let result = isRelative("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
