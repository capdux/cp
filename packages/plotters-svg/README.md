[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# plotters-svg

Zero-heap pure CP SVG vector graphics drawing backend (plotters-svg parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:plotters-svg` ([https://github.com/plotters-rs/plotters](https://github.com/plotters-rs/plotters)) — `MIT`

| Metadata | Value |
| --- | --- |
| Package | `plotters-svg` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add plotters-svg
```

```cp
import {
    SvgColor,
    SvgCanvas,
    psvgWriteByte,
    psvgWriteStr,
    psvgWriteU32,
    psvgWriteI32,
    psvgWriteRgb,
    psvgWriteXmlEscaped,
} from "plotters-svg";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SvgColor` | `class` | Fields: `r: u8, g: u8, b: u8` · Methods: `rgb, black, white, red, green, blue` |
| `SvgCanvas` | `class` | Fields: `buf: []u8, pos: usize, overflowed: bool` · Methods: `new, written, hasOverflow, slice, begin, end, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `psvgWriteByte` | `(buf: []u8, pos: usize, b: u8): usize` | Invokes `psvgWriteByte` and returns `usize`. |
| `psvgWriteStr` | `(buf: []u8, pos: usize, s: string): usize` | Invokes `psvgWriteStr` and returns `usize`. |
| `psvgWriteU32` | `(buf: []u8, pos: usize, val: u32): usize` | Invokes `psvgWriteU32` and returns `usize`. |
| `psvgWriteI32` | `(buf: []u8, pos: usize, val: i32): usize` | Invokes `psvgWriteI32` and returns `usize`. |
| `psvgWriteRgb` | `(buf: []u8, pos: usize, c: SvgColor): usize` | Invokes `psvgWriteRgb` and returns `usize`. |
| `psvgWriteXmlEscaped` | `(buf: []u8, pos: usize, text: string): usize` | Invokes `psvgWriteXmlEscaped` and returns `usize`. |

## Minimal Example

```cp
import { psvgWriteStr } from "plotters-svg";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = psvgWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
