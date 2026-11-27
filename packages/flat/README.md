[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# flat

Zero-allocation JSON flattening (a.b.0.c), dot-prop path lookup & flat-to-JSON reconstruction engine (flat / dot-prop parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:flat` ([https://github.com/hughsk/flat](https://github.com/hughsk/flat))（原库名 `flat` → CP 包名 `flat`）

| Metadata | Value |
| --- | --- |
| Package | `flat` |
| Version | `1.0.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add flat
```

```cp
import {
    FlatResult,
    strEq,
    isWs,
    skipWs,
    parseJsonStringEnd,
    formatUsize,
    parseValue,
    flattenJsonWith,
} from "flat";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FlatResult` | `class` | Fields: `pool: [MAX_FLAT_POOL]u8, pool_len: usize, key_starts: [MAX_FLAT_ENTRIES]usize, key_lens: [MAX_FLAT_ENTRIES]usize, val_starts: [MAX_FLAT_ENTRIES]usize, val_lens: [MAX_FLAT_ENTRIES]usize, +2 more` · Methods: `empty, pushEntry, keyAt, valAt, get, has` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `strEq` | `(a: string, b: string): bool` | Invokes `strEq` and returns `bool`. |
| `isWs` | `(c: u8): bool` | Checks predicate `isWs` and returns `bool`. |
| `skipWs` | `(s: string, pos: usize): usize` | Invokes `skipWs` and returns `usize`. |
| `parseJsonStringEnd` | `(s: string, open_quote: usize): usize` | Parses input via `parseJsonStringEnd` and returns `usize`. |
| `formatUsize` | `(v: usize, buf: []u8): string` | Serializes/formats data via `formatUsize` returning `string`. |
| `parseValue` | `(s: string, start_pos: usize, prefix: []u8, prefix_len: usize, sep: u8, depth: usize, res: &mut FlatResult): usize` | Parses input via `parseValue` and returns `usize`. |
| `flattenJsonWith` | `(json_str: string, sep: u8): FlatResult` | Invokes `flattenJsonWith` and returns `FlatResult`. |
| `flattenJson` | `(json_str: string): FlatResult` | Invokes `flattenJson` and returns `FlatResult`. |
| `getByPath` | `(json_str: string, dot_path: string, out: []u8): string?` | Queries/retrieves data via `getByPath` returning `string?`. |
| `hasPath` | `(json_str: string, dot_path: string): bool` | Checks predicate `hasPath` and returns `bool`. |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `appendStr` and returns `usize`. |
| `formatFlatJsonObject` | `(flat: FlatResult, out: []u8): string` | Serializes/formats data via `formatFlatJsonObject` returning `string`. |

## Minimal Example

```cp
import { parseJsonStringEnd } from "flat";

pub fn main() {
    let result = parseJsonStringEnd("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
