[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# markdown-table

Zero-heap GFM Markdown table renderer with automatic column width calculation and alignment padding

| Metadata | Value |
| --- | --- |
| Package | `markdown-table` |
| Version | `3.0.4` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add markdown-table
```

```cp
import {
    TableAlign,
    mdtbWriteByte,
    mdtbWriteStr,
    mdtbWriteRepeat,
    mdtbGetAlign,
    mdtbMinWidth,
    mdtbWriteCell,
    mdtbWriteDelim,
} from "markdown-table";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `TableAlign` | `None, Left, Center, Right` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mdtbWriteByte` | `(out: []u8, pos: usize, ch: u8): usize` | Invokes `mdtbWriteByte` and returns `usize`. |
| `mdtbWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `mdtbWriteStr` and returns `usize`. |
| `mdtbWriteRepeat` | `(out: []u8, pos: usize, ch: u8, count: usize): usize` | Invokes `mdtbWriteRepeat` and returns `usize`. |
| `mdtbGetAlign` | `(aligns: &[]TableAlign, col: usize): TableAlign` | Invokes `mdtbGetAlign` and returns `TableAlign`. |
| `mdtbMinWidth` | `(al: TableAlign): usize` | Invokes `mdtbMinWidth` and returns `usize`. |
| `mdtbWriteCell` | `(out: []u8, pos: usize, text: string, width: usize, al: TableAlign): usize` | Invokes `mdtbWriteCell` and returns `usize`. |
| `mdtbWriteDelim` | `(out: []u8, pos: usize, width: usize, al: TableAlign): usize` | Invokes `mdtbWriteDelim` and returns `usize`. |
| `mdtbColWidth` | `(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, col: usize, al: TableAlign): usize` | Invokes `mdtbColWidth` and returns `usize`. |
| `renderMarkdownTable` | `(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, aligns: &[]TableAlign, out: []u8): usize` | Invokes `renderMarkdownTable` and returns `usize`. |

## Minimal Example

```cp
import { mdtbWriteStr } from "markdown-table";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = mdtbWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
