# wkt — AI Agent Quick Ref

Zero-heap GIS Well-Known Text (WKT) parser, formatter, and bounding-box calculator for POINT, LINESTRING, POLYGON, and MULTIPOINT geometries (clean-room port of Rust wkt) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class Coord2D { x: f64, y: f64 }
class WktGeometry { kind: u8, is_empty: bool, coords: [32]Coord2D, coord_count: usize }
class WktBbox { min_x: f64, min_y: f64, max_x: f64, max_y: f64, /* +1 fields */ }
```

## API Signatures

```cp
fn wktZeroCoord(): Coord2D;
fn wktEmptyGeom(kind: u8): WktGeometry;
fn wktSkipWs(s: string, pos: &mut usize);
fn wktReadWord(s: string, pos: &mut usize, out: []u8): usize;
fn wktParseNumber(s: string, pos: &mut usize, out: &mut f64): bool;
fn wktParseCoord(s: string, pos: &mut usize, out: &mut Coord2D): bool;
fn wktConsumeChar(s: string, pos: &mut usize, ch: u8): bool;
fn wktWriteF64(out: []u8, pos: usize, val: f64): usize;
fn wktWriteCoord(out: []u8, pos: usize, c: Coord2D): usize;
fn parseWkt(input: string): WktGeometry?;
fn formatWkt(g: WktGeometry, out: []u8): usize;
fn bboxWkt(g: WktGeometry): WktBbox;
```

## Minimal Usage

```cp
import { parseWkt } from "wkt";

pub fn main() {
    let result = parseWkt("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"wkt"`; install via `cap add wkt`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
