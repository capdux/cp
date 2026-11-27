[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# wkt

Zero-heap GIS Well-Known Text (WKT) parser, formatter, and bounding-box calculator for POINT, LINESTRING, POLYGON, and MULTIPOINT geometries (clean-room port of Rust wkt)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wkt` ([https://github.com/georust/wkt](https://github.com/georust/wkt))

| Metadata | Value |
| --- | --- |
| Package | `wkt` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add wkt
```

```cp
import {
    Coord2D,
    WktGeometry,
    WktBbox,
    wktZeroCoord,
    wktEmptyGeom,
    wktSkipWs,
    wktReadWord,
    wktParseNumber,
} from "wkt";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Coord2D` | `class` | Fields: `x: f64, y: f64` |
| `WktGeometry` | `class` | Fields: `kind: u8, is_empty: bool, coords: [32]Coord2D, coord_count: usize` |
| `WktBbox` | `class` | Fields: `min_x: f64, min_y: f64, max_x: f64, max_y: f64, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wktZeroCoord` | `(): Coord2D` | Invokes `wktZeroCoord` and returns `Coord2D`. |
| `wktEmptyGeom` | `(kind: u8): WktGeometry` | Invokes `wktEmptyGeom` and returns `WktGeometry`. |
| `wktSkipWs` | `(s: string, pos: &mut usize)` | Executes `wktSkipWs` in-place. |
| `wktReadWord` | `(s: string, pos: &mut usize, out: []u8): usize` | Invokes `wktReadWord` and returns `usize`. |
| `wktParseNumber` | `(s: string, pos: &mut usize, out: &mut f64): bool` | Invokes `wktParseNumber` and returns `bool`. |
| `wktParseCoord` | `(s: string, pos: &mut usize, out: &mut Coord2D): bool` | Invokes `wktParseCoord` and returns `bool`. |
| `wktConsumeChar` | `(s: string, pos: &mut usize, ch: u8): bool` | Invokes `wktConsumeChar` and returns `bool`. |
| `wktWriteF64` | `(out: []u8, pos: usize, val: f64): usize` | Invokes `wktWriteF64` and returns `usize`. |
| `wktWriteCoord` | `(out: []u8, pos: usize, c: Coord2D): usize` | Invokes `wktWriteCoord` and returns `usize`. |
| `parseWkt` | `(input: string): WktGeometry?` | Parses input via `parseWkt` and returns `WktGeometry?`. |
| `formatWkt` | `(g: WktGeometry, out: []u8): usize` | Serializes/formats data via `formatWkt` returning `usize`. |
| `bboxWkt` | `(g: WktGeometry): WktBbox` | Invokes `bboxWkt` and returns `WktBbox`. |

## Minimal Example

```cp
import { parseWkt } from "wkt";

pub fn main() {
    let result = parseWkt("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
