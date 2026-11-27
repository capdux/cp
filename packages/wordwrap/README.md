[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# wordwrap

Zero-heap column-width text word wrapper with indentation, soft/hard modes, and newline preservation in pure CP

| Metadata | Value |
| --- | --- |
| Package | `wordwrap` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add wordwrap
```

```cp
import {
    WwrpEmitRes,
    wwrpIsInlineWs,
    wwrpPushByte,
    wwrpPushSpaces,
    wwrpPushSlice,
    wwrpEmitWord,
    wrapText,
} from "wordwrap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WwrpEmitRes` | `class` | Fields: `pos: usize, line_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wwrpIsInlineWs` | `(c: u8): bool` | Invokes `wwrpIsInlineWs` and returns `bool`. |
| `wwrpPushByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `wwrpPushByte` and returns `usize`. |
| `wwrpPushSpaces` | `(out: []u8, pos: usize, count: usize): usize` | Invokes `wwrpPushSpaces` and returns `usize`. |
| `wwrpPushSlice` | `(out: []u8, pos: usize, s: string): usize` | Invokes `wwrpPushSlice` and returns `usize`. |
| `wwrpEmitWord` | `(word: string, start_col: usize, max_w: usize, hard_mode: bool, out: []u8, start_pos: usize, cur_line_len: usize): WwrpEmitRes` | Invokes `wwrpEmitWord` and returns `WwrpEmitRes`. |
| `wrapText` | `(input: string, start_col: usize, stop_col: usize, hard_mode: bool, out: []u8): usize` | Invokes `wrapText` and returns `usize`. |

## Minimal Example

```cp
import { wwrpPushSlice } from "wordwrap";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = wwrpPushSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
