[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cache-control-parser

Zero-allocation RFC 9111 HTTP Cache-Control parser, formatter, and shared/private TTL calculator (cache-control-parser / http-cache-semantics parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cache-control-parser` ([https://github.com/etienne-martin/cache-control-parser](https://github.com/etienne-martin/cache-control-parser))（原库名 `cache-control-parser` → CP 包名 `cache-control-parser`）

| Metadata | Value |
| --- | --- |
| Package | `cache-control-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cache-control-parser
```

```cp
import {
    CacheControl,
    unquote,
    parseNonNegI64,
    writeStrChecked,
    writeU64Checked,
    appendDirectiveFlag,
    appendDirectiveKv,
} from "cache-control-parser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CacheControl` | `class` | Fields: `max_age: i64, s_maxage: i64, stale_while_revalidate: i64, stale_if_error: i64, no_cache: bool, no_store: bool, +5 more` · Methods: `parse, isCacheableShared, effectiveTtl, format` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `unquote` | `(s_in: &[]u8): &[]u8` | Invokes `unquote` and returns `&[]u8`. |
| `parseNonNegI64` | `(s_in: &[]u8): i64` | Parses input via `parseNonNegI64` and returns `i64`. |
| `writeStrChecked` | `(dst: []u8, pos: usize, s: string): i64` | Invokes `writeStrChecked` and returns `i64`. |
| `writeU64Checked` | `(dst: []u8, pos: usize, v_in: u64): i64` | Invokes `writeU64Checked` and returns `i64`. |
| `appendDirectiveFlag` | `(dst: []u8, pos_in: usize, written: &mut usize, name: string): i64` | Invokes `appendDirectiveFlag` and returns `i64`. |
| `appendDirectiveKv` | `(dst: []u8, pos_in: usize, written: &mut usize, name: string, val: i64): i64` | Invokes `appendDirectiveKv` and returns `i64`. |

## Minimal Example

```cp
import { writeStrChecked } from "cache-control-parser";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = writeStrChecked(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
