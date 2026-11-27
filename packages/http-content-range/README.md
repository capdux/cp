[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# http-content-range

Zero-allocation RFC 9110 HTTP Content-Range header parser and formatter (clean-room MIT implementation of crates.io http-content-range)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:http-content-range` ([https://github.com/briansmith/http-content-range](https://github.com/briansmith/http-content-range)) | 许可：`MIT OR Apache-2.0`

| Metadata | Value |
| --- | --- |
| Package | `http-content-range` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add http-content-range
```

```cp
import {
    ContentRangeKind,
    ContentRange,
    hcrParseU64,
    hcrPush,
    hcrPushU64,
    parseContentRange,
    formatContentRangeHeader,
} from "http-content-range";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `ContentRangeKind` | `Invalid, Bytes, BytesUnknownLength, Unsatisfied` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ContentRange` | `class` | Fields: `kind: ContentRangeKind, first_byte: u64, last_byte: u64, complete_length: u64` · Methods: `isValid, rangeLen` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hcrParseU64` | `(s: string): u64?` | Invokes `hcrParseU64` and returns `u64?`. |
| `hcrPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `hcrPush` and returns `usize`. |
| `hcrPushU64` | `(out: []u8, pos: usize, val: u64): usize` | Invokes `hcrPushU64` and returns `usize`. |
| `parseContentRange` | `(header: string): ContentRange` | Parses input via `parseContentRange` and returns `ContentRange`. |
| `formatContentRangeHeader` | `(cr: ContentRange, out: []u8): usize` | Serializes/formats data via `formatContentRangeHeader` returning `usize`. |

## Minimal Example

```cp
import { parseContentRange } from "http-content-range";

pub fn main() {
    let result = parseContentRange("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
