[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jsonpath-rust

Zero-heap RFC 9535 JSONPath expression parser and canonical serializer in pure CP

| Metadata | Value |
| --- | --- |
| Package | `jsonpath-rust` |
| Version | `0.7.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jsonpath-rust
```

```cp
import {
    JpSegKind,
    JpSegment,
    JprIntRes,
    jprIsWs,
    jprIsDigit,
    jprIsIdentStart,
    jprIsIdentCont,
    jprIsSimpleIdent,
} from "jsonpath-rust";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `JpSegKind` | `Root, Property, Index, Wildcard, RecursiveProp, RecursiveWildcard, Slice` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `JpSegment` | `class` | Fields: `kind: JpSegKind, name: string, index: i64, slice_start: i64, slice_end: i64, has_start: bool, +1 more` |
| `JprIntRes` | `class` | Fields: `val: i64, next_pos: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `jprIsWs` | `(c: u8): bool` | Invokes `jprIsWs` and returns `bool`. |
| `jprIsDigit` | `(c: u8): bool` | Invokes `jprIsDigit` and returns `bool`. |
| `jprIsIdentStart` | `(c: u8): bool` | Invokes `jprIsIdentStart` and returns `bool`. |
| `jprIsIdentCont` | `(c: u8): bool` | Invokes `jprIsIdentCont` and returns `bool`. |
| `jprIsSimpleIdent` | `(s: string): bool` | Invokes `jprIsSimpleIdent` and returns `bool`. |
| `jprMakeSeg` | `(kind: JpSegKind, name: string): JpSegment` | Invokes `jprMakeSeg` and returns `JpSegment`. |
| `jprParseInt` | `(s: string, start_pos: usize): JprIntRes` | Invokes `jprParseInt` and returns `JprIntRes`. |
| `parseJsonPath` | `(expr: string, out: []JpSegment): i64` | Parses input via `parseJsonPath` and returns `i64`. |
| `jprPushByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `jprPushByte` and returns `usize`. |
| `jprPushSlice` | `(out: []u8, pos: usize, s: string): usize` | Invokes `jprPushSlice` and returns `usize`. |
| `jprPushI64` | `(out: []u8, pos: usize, val: i64): usize` | Invokes `jprPushI64` and returns `usize`. |
| `formatJsonPath` | `(segs: &[]JpSegment, out: []u8): usize` | Serializes/formats data via `formatJsonPath` returning `usize`. |

## Minimal Example

```cp
import { parseJsonPath } from "jsonpath-rust";

pub fn main() {
    let result = parseJsonPath("example", /* out: []JpSegment */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
