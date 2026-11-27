[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# d3-shape

Pure CP SVG shape and pie chart path generator (line, step, arc, pie)

| Metadata | Value |
| --- | --- |
| Package | `d3-shape` |
| Version | `3.2.0` |
| License | `ISC` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add d3-shape
```

```cp
import {
    Point2D,
    PieSlice,
    d3shWriteByte,
    d3shWriteStr,
    d3shWriteU64,
    d3shWriteFloat,
    d3shWritePair,
    computePie,
} from "d3-shape";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Point2D` | `class` | Fields: `x: f64, y: f64` |
| `PieSlice` | `class` | Fields: `value: f64, index: usize, start_angle: f64, end_angle: f64, pad_angle: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `d3shWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `d3shWriteByte` and returns `usize`. |
| `d3shWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `d3shWriteStr` and returns `usize`. |
| `d3shWriteU64` | `(out: []u8, pos: usize, val: u64): usize` | Invokes `d3shWriteU64` and returns `usize`. |
| `d3shWriteFloat` | `(out: []u8, pos: usize, val: f64): usize` | Invokes `d3shWriteFloat` and returns `usize`. |
| `d3shWritePair` | `(out: []u8, pos: usize, x: f64, y: f64): usize` | Invokes `d3shWritePair` and returns `usize`. |
| `computePie` | `(values: &[]f64, start_angle: f64, end_angle: f64, pad_angle: f64, out: []PieSlice): usize` | Invokes `computePie` and returns `usize`. |
| `linePath` | `(pts: &[]Point2D, out: []u8): usize` | Invokes `linePath` and returns `usize`. |
| `stepLinePath` | `(pts: &[]Point2D, out: []u8): usize` | Invokes `stepLinePath` and returns `usize`. |
| `arcPath` | `(inner_r: f64, outer_r: f64, start_angle: f64, end_angle: f64, out: []u8): usize` | Invokes `arcPath` and returns `usize`. |

## Minimal Example

```cp
import { d3shWriteStr } from "d3-shape";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = d3shWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
