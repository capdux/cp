[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# magic-string

Zero-allocation original-offset source string editor (npm magic-string parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`magic-string`](https://github.com/Rich-Harris/magic-string)（`MIT`）（包名 `magic-string`）。

| Metadata | Value |
| --- | --- |
| Package | `magic-string` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add magic-string
```

```cp
import {
    MstrChunk,
    MstrInsert,
    MagicString,
    mstrWriteStr,
    mstrWriteLeftInserts,
    mstrWriteRightInserts,
} from "magic-string";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MstrChunk` | `class` | Fields: `start: usize, end: usize, content: string, edited: bool` |
| `MstrInsert` | `class` | Fields: `index: usize, content: string` |
| `MagicString` | `class` | Fields: `original: string, changed: bool, chunks: [64]MstrChunk, chunk_count: usize, global_intros: [16]string, global_intro_len: usize, +6 more` · Methods: `new, prepend, append, splitAt, overwrite, remove, +6 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mstrWriteStr` | `(src: string, out: []u8, pos: usize): usize` | Invokes `mstrWriteStr` and returns `usize`. |
| `mstrWriteLeftInserts` | `(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize` | Invokes `mstrWriteLeftInserts` and returns `usize`. |
| `mstrWriteRightInserts` | `(inserts: [32]MstrInsert, count: usize, target_idx: usize, out: []u8, pos: usize): usize` | Invokes `mstrWriteRightInserts` and returns `usize`. |

## Minimal Example

```cp
import { mstrWriteStr } from "magic-string";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = mstrWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
