[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# etag

Zero-allocation RFC 9110 HTTP ETag generator, parser, and If-None-Match / If-Match evaluator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:etag` ([https://github.com/jshttp/etag](https://github.com/jshttp/etag))

| Metadata | Value |
| --- | --- |
| Package | `etag` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add etag
```

```cp
import {
    EntityTag,
    etgIsSpace,
    etgIsEtagChar,
    etgStrEq,
    parseEtag,
    formatEtag,
    strongEqual,
    weakEqual,
} from "etag";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EntityTag` | `class` | Fields: `weak: bool, tag: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `etgIsSpace` | `(b: u8): bool` | Invokes `etgIsSpace` and returns `bool`. |
| `etgIsEtagChar` | `(b: u8): bool` | Invokes `etgIsEtagChar` and returns `bool`. |
| `etgStrEq` | `(a: string, b: string): bool` | Invokes `etgStrEq` and returns `bool`. |
| `parseEtag` | `(input: string, out: &mut EntityTag): bool` | Parses input via `parseEtag` and returns `bool`. |
| `formatEtag` | `(weak: bool, tag: string, out: []u8): usize` | Serializes/formats data via `formatEtag` returning `usize`. |
| `strongEqual` | `(a: EntityTag, b: EntityTag): bool` | Invokes `strongEqual` and returns `bool`. |
| `weakEqual` | `(a: EntityTag, b: EntityTag): bool` | Invokes `weakEqual` and returns `bool`. |
| `etgHexLower` | `(nibble: u8): u8` | Invokes `etgHexLower` and returns `u8`. |
| `etgWriteHexU64` | `(val: u64, out: []u8, start: usize): usize` | Invokes `etgWriteHexU64` and returns `usize`. |
| `etgHexLenU64` | `(val: u64): usize` | Invokes `etgHexLenU64` and returns `usize`. |
| `generateStatEtag` | `(size: u64, mtime_ms: u64, out: []u8): usize` | Invokes `generateStatEtag` and returns `usize`. |
| `etgFnv1a64` | `(data: &[]u8): u64` | Invokes `etgFnv1a64` and returns `u64`. |
| `generateContentEtag` | `(data: &[]u8, weak: bool, out: []u8): usize` | Invokes `generateContentEtag` and returns `usize`. |
| `etgMatchHeaderList` | `(header: string, current: EntityTag, require_strong: bool): bool` | Invokes `etgMatchHeaderList` and returns `bool`. |
| `matchIfNoneMatch` | `(header: string, current: EntityTag): bool` | Invokes `matchIfNoneMatch` and returns `bool`. |
| `matchIfMatch` | `(header: string, current: EntityTag): bool` | Invokes `matchIfMatch` and returns `bool`. |
| `compareStrong` | `(a: string, b: string): bool` | Invokes `compareStrong` and returns `bool`. |
| `compareWeak` | `(a: string, b: string): bool` | Invokes `compareWeak` and returns `bool`. |

## Minimal Example

```cp
import { parseEtag } from "etag";

pub fn main() {
    let result = parseEtag("example", /* out: &mut EntityTag */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
