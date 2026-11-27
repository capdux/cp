[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# debug

Zero-allocation namespace debug log filter, glob exclusion engine, deterministic ANSI coloring, and diff-ms formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:debug` ([https://github.com/debug-js/debug](https://github.com/debug-js/debug))

| Metadata | Value |
| --- | --- |
| Package | `debug` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add debug
```

```cp
import {
    DebugFilter,
    DebugLogger,
    isSep,
    globMatch,
    appendByte,
    appendStr,
    appendU64,
    isNamespaceEnabled,
} from "debug";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DebugFilter` | `class` | Fields: `raw_pattern: string` · Methods: `from, isEnabled` |
| `DebugLogger` | `class` | Fields: `namespace: string, color_code: u8, last_ms: i64, enabled: bool` · Methods: `create, formatLine` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isSep` | `(c: u8): bool` | Checks predicate `isSep` and returns `bool`. |
| `globMatch` | `(pat: string, text: string): bool` | Invokes `globMatch` and returns `bool`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `appendStr` and returns `usize`. |
| `appendU64` | `(out: []u8, pos: usize, val: u64): usize` | Invokes `appendU64` and returns `usize`. |
| `isNamespaceEnabled` | `(pattern: string, namespace: string): bool` | Checks predicate `isNamespaceEnabled` and returns `bool`. |
| `selectColor` | `(namespace: string): u8` | Invokes `selectColor` and returns `u8`. |
| `formatDiffMs` | `(diff_ms: i64, out: []u8): string` | Serializes/formats data via `formatDiffMs` returning `string`. |
| `formatDebugLine` | `(ns: string, msg: string, diff_ms: i64, use_color: bool, out: []u8): string` | Serializes/formats data via `formatDebugLine` returning `string`. |

## Minimal Example

```cp
import { isNamespaceEnabled } from "debug";

pub fn main() {
    let result = isNamespaceEnabled("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
