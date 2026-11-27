[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cliui

Zero-allocation multi-column CLI layout engine with ANSI-aware wrapping, alignment, and padding in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cliui` ([https://github.com/yargs/cliui](https://github.com/yargs/cliui))

| Metadata | Value |
| --- | --- |
| Package | `cliui` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cliui
```

```cp
import {
    Align,
    Column,
    CliUi,
    alignToU8,
    cuWriteByte,
    cuWriteBytes,
    cuWriteSpaces,
    cuVisibleWidth,
} from "cliui";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `Align` | `Left, Right, Center` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Column` | `class` | Fields: `text: string, width: usize, alignment: u8, pad_left: usize, pad_right: usize, border: bool` · Methods: `plain, sized` |
| `CliUi` | `class` | Fields: `width: usize, len: usize, row_count: usize, in_span: bool, buf: [CU_BUF_CAP]u8` · Methods: `new, resetOutput, div, span, render` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `alignToU8` | `(a: Align): u8` | Invokes `alignToU8` and returns `u8`. |
| `cuWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `cuWriteByte` and returns `usize`. |
| `cuWriteBytes` | `(out: []u8, pos: usize, s: string): usize` | Invokes `cuWriteBytes` and returns `usize`. |
| `cuWriteSpaces` | `(out: []u8, pos: usize, count: usize): usize` | Invokes `cuWriteSpaces` and returns `usize`. |
| `cuVisibleWidth` | `(s: string): usize` | Invokes `cuVisibleWidth` and returns `usize`. |
| `cuCountLines` | `(s: string): usize` | Invokes `cuCountLines` and returns `usize`. |
| `cuGetLine` | `(s: string, target_line: usize): string` | Invokes `cuGetLine` and returns `string`. |
| `cuResolveWidths` | `(total_width: usize, cols: &[]Column, n_cols: usize, widths: []usize)` | Executes `cuResolveWidths` in-place. |
| `formatTwoCol` | `(left: string, right: string, left_width: usize, total_width: usize, out: []u8): usize` | Serializes/formats data via `formatTwoCol` returning `usize`. |

## Minimal Example

```cp
import { formatTwoCol } from "cliui";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatTwoCol("example", "example", 0, 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
