[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# webpack-sources

Zero-heap source concatenation and range replacement engine

| Metadata | Value |
| --- | --- |
| Package | `webpack-sources` |
| Version | `3.2.3` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add webpack-sources
```

```cp
import {
    SourceEdit,
    ReplaceSource,
    wsrcWriteStr,
    wsrcEmptyEdit,
    concatSources,
} from "webpack-sources";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SourceEdit` | `class` | Fields: `start: usize, end: usize, replacement: string` |
| `ReplaceSource` | `class` | Fields: `original: string, edits: [64]SourceEdit, edit_count: usize` · Methods: `init, replace, insert, remove, render` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wsrcWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `wsrcWriteStr` and returns `usize`. |
| `wsrcEmptyEdit` | `(): SourceEdit` | Invokes `wsrcEmptyEdit` and returns `SourceEdit`. |
| `concatSources` | `(parts: &[]string, sep: string, out: []u8): usize` | Invokes `concatSources` and returns `usize`. |

## Minimal Example

```cp
import { wsrcWriteStr } from "webpack-sources";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = wsrcWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
