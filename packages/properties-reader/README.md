[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# properties-reader

Zero-heap Java / Spring Boot / i18n .properties parser and formatter with =/:/space separators, backslash line continuations, comments, and \uXXXX Unicode escapes.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:properties-reader` ([https://github.com/steveukx/properties](https://github.com/steveukx/properties))（原库名 `properties-reader` → CP 包名 `properties-reader`）

| Metadata | Value |
| --- | --- |
| Package | `properties-reader` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add properties-reader
```

```cp
import {
    PropEntry,
    EscDecodeResult,
    Properties,
    isLineWs,
    decodeEscapeInto,
    pushByte,
    pushEscaped,
    formatEntry,
} from "properties-reader";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PropEntry` | `class` | Fields: `key_start: usize, key_len: usize, val_start: usize, val_len: usize` |
| `EscDecodeResult` | `class` | Fields: `next_pos: usize, written: usize` |
| `Properties` | `class` | Fields: `entries: [128]PropEntry, count: usize, val_buf: [2048]u8, buf_used: usize` · Methods: `parse, len, findIndex, has, get` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isLineWs` | `(c: u8): bool` | Checks predicate `isLineWs` and returns `bool`. |
| `decodeEscapeInto` | `(src: &[]u8, esc_pos: usize, dst: []u8, dst_pos: usize): EscDecodeResult` | Parses input via `decodeEscapeInto` and returns `EscDecodeResult`. |
| `pushByte` | `(dst: []u8, pos: usize, b: u8): i64` | Mutates or appends state via `pushByte`. |
| `pushEscaped` | `(dst: []u8, pos: usize, ch: u8): i64` | Mutates or appends state via `pushEscaped`. |
| `formatEntry` | `(key: &[]u8, value: &[]u8, dst: []u8): i64` | Serializes/formats data via `formatEntry` returning `i64`. |

## Minimal Example

```cp
import { isLineWs } from "properties-reader";

pub fn main() {
    let result = isLineWs(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
