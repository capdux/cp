# geojson — AI Agent Quick Ref

Zero-heap RFC 7946 GeoJSON geometry & feature parser, formatter, and WKT interop built on wkt (clean-room port of Rust geojson) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class GeoJsonGeometry { kind: u8, coords: [32]Coord2D, coord_count: usize } // methods: fromWkt(...) +1
class GeoJsonFeature { id: string, geom: WktGeometry, has_bbox: bool, bbox: WktBbox } // methods: init(...)
```

## API Signatures

```cp
fn gjZeroCoord(): Coord2D;
fn gjEmptyGeom(kind: u8): WktGeometry;
fn gjIsSpace(c: u8): bool;
fn gjSkipWs(s: string, pos: &mut usize);
fn gjConsumeChar(s: string, pos: &mut usize, ch: u8): bool;
fn gjWriteByte(out: []u8, pos: usize, b: u8): usize;
fn gjWriteJsonEscaped(out: []u8, pos: usize, s: string): usize;
fn gjWriteF64(out: []u8, pos: usize, val: f64): usize;
fn gjWriteCoord(out: []u8, pos: usize, c: Coord2D): usize;
fn gjParseNumber(s: string, pos: &mut usize, out: &mut f64): bool;
fn gjParseStringSlice(s: string, pos: &mut usize, start_out: &mut usize, end_out: &mut usize): bool;
fn gjSkipValue(s: string, pos: &mut usize, depth: u32): bool;
fn gjParsePosition(s: string, pos: &mut usize, out: &mut Coord2D): bool;
fn gjParsePosList(s: string, pos: &mut usize, geom: &mut WktGeometry): bool;
fn formatGeoJsonGeometry(g: WktGeometry, out: []u8): usize;
fn formatGeoJsonFeature(f: GeoJsonFeature, out: []u8): usize;
fn parseGeoJsonGeometry(json: string): WktGeometry?;
fn parseGeoJsonPoint(json: string): WktGeometry?;
fn wktToGeoJson(wkt_str: string, out: []u8): usize;
fn geoJsonToWkt(json: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseGeoJsonGeometry } from "geojson";

pub fn main() {
    let result = parseGeoJsonGeometry("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"geojson"`; install via `cap add geojson`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
