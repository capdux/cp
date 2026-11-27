[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# termtree

Zero-allocation CLI tree structure renderer with Unicode/ASCII glyph palettes and fixed-capacity arena (Rust termtree parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:termtree` ([https://github.com/rust-cli/termtree](https://github.com/rust-cli/termtree))

| Metadata | Value |
| --- | --- |
| Package | `termtree` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add termtree
```

```cp
import {
    GlyphPalette,
    TermTreeNode,
    TermTreeArena,
    ttrAppendBytes,
    ttrAppendByte,
} from "termtree";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GlyphPalette` | `class` | Fields: `middle_item: string, last_item: string, middle_skip: string, last_skip: string` · Methods: `unicode, ascii` |
| `TermTreeNode` | `class` | Fields: `label: string, first_child: i32, last_child: i32, next_sibling: i32` |
| `TermTreeArena` | `class` | Fields: `nodes: [64]TermTreeNode, count: usize` · Methods: `init, newRoot, addChild, nodeCount, maxDepth, renderWithGlyphs, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ttrAppendBytes` | `(out: []u8, pos: usize, src: string): usize?` | Invokes `ttrAppendBytes` and returns `usize?`. |
| `ttrAppendByte` | `(out: []u8, pos: usize, b: u8): usize?` | Invokes `ttrAppendByte` and returns `usize?`. |

## Minimal Example

```cp
import { ttrAppendBytes } from "termtree";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ttrAppendBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
