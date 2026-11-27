[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# geojson

Zero-heap RFC 7946 GeoJSON geometry & feature parser, formatter, and WKT interop built on wkt (clean-room port of Rust geojson)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:geojson` ([https://github.com/georust/geojson](https://github.com/georust/geojson))

| Metadata | Value |
| --- | --- |
| Package | `geojson` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add geojson
```

```cp
import {
    GeoJsonGeometry,
    GeoJsonFeature,
    gjZeroCoord,
    gjEmptyGeom,
    gjIsSpace,
    gjSkipWs,
    gjConsumeChar,
    gjWriteByte,
} from "geojson";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GeoJsonGeometry` | `class` | Fields: `kind: u8, coords: [32]Coord2D, coord_count: usize` · Methods: `fromWkt, toWkt` |
| `GeoJsonFeature` | `class` | Fields: `id: string, geom: WktGeometry, has_bbox: bool, bbox: WktBbox` · Methods: `init` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `gjZeroCoord` | `(): Coord2D` | Invokes `gjZeroCoord` and returns `Coord2D`. |
| `gjEmptyGeom` | `(kind: u8): WktGeometry` | Invokes `gjEmptyGeom` and returns `WktGeometry`. |
| `gjIsSpace` | `(c: u8): bool` | Invokes `gjIsSpace` and returns `bool`. |
| `gjSkipWs` | `(s: string, pos: &mut usize)` | Executes `gjSkipWs` in-place. |
| `gjConsumeChar` | `(s: string, pos: &mut usize, ch: u8): bool` | Invokes `gjConsumeChar` and returns `bool`. |
| `gjWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `gjWriteByte` and returns `usize`. |
| `gjWriteJsonEscaped` | `(out: []u8, pos: usize, s: string): usize` | Invokes `gjWriteJsonEscaped` and returns `usize`. |
| `gjWriteF64` | `(out: []u8, pos: usize, val: f64): usize` | Invokes `gjWriteF64` and returns `usize`. |
| `gjWriteCoord` | `(out: []u8, pos: usize, c: Coord2D): usize` | Invokes `gjWriteCoord` and returns `usize`. |
| `gjParseNumber` | `(s: string, pos: &mut usize, out: &mut f64): bool` | Invokes `gjParseNumber` and returns `bool`. |
| `gjParseStringSlice` | `(s: string, pos: &mut usize, start_out: &mut usize, end_out: &mut usize): bool` | Invokes `gjParseStringSlice` and returns `bool`. |
| `gjSkipValue` | `(s: string, pos: &mut usize, depth: u32): bool` | Invokes `gjSkipValue` and returns `bool`. |
| `gjParsePosition` | `(s: string, pos: &mut usize, out: &mut Coord2D): bool` | Invokes `gjParsePosition` and returns `bool`. |
| `gjParsePosList` | `(s: string, pos: &mut usize, geom: &mut WktGeometry): bool` | Invokes `gjParsePosList` and returns `bool`. |
| `formatGeoJsonGeometry` | `(g: WktGeometry, out: []u8): usize` | Serializes/formats data via `formatGeoJsonGeometry` returning `usize`. |
| `formatGeoJsonFeature` | `(f: GeoJsonFeature, out: []u8): usize` | Serializes/formats data via `formatGeoJsonFeature` returning `usize`. |
| `parseGeoJsonGeometry` | `(json: string): WktGeometry?` | Parses input via `parseGeoJsonGeometry` and returns `WktGeometry?`. |
| `parseGeoJsonPoint` | `(json: string): WktGeometry?` | Parses input via `parseGeoJsonPoint` and returns `WktGeometry?`. |
| `wktToGeoJson` | `(wkt_str: string, out: []u8): usize` | Invokes `wktToGeoJson` and returns `usize`. |
| `geoJsonToWkt` | `(json: string, out: []u8): usize` | Invokes `geoJsonToWkt` and returns `usize`. |

## Minimal Example

```cp
import { parseGeoJsonGeometry } from "geojson";

pub fn main() {
    let result = parseGeoJsonGeometry("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
