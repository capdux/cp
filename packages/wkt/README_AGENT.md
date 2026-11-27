# wkt@1.0.0 (Agent Quickref)

> Zero-heap GIS Well-Known Text (WKT) parser, formatter, and bounding-box calculator for POINT, LINESTRING, POLYGON, and MULTIPOINT geometries (clean-room port of Rust wkt) | License: MIT
> Import: `import { ... } from "wkt";` | Full types: `src/index.d.cp`

## Functions
- `export fn wktZeroCoord(): Coord2D;`
- `export fn wktEmptyGeom(kind: u8): WktGeometry;`
- `export fn wktSkipWs(s: string, pos: &mut usize);`
- `export fn wktReadWord(s: string, pos: &mut usize, out: []u8): usize;`
- `export fn wktParseNumber(s: string, pos: &mut usize, out: &mut f64): bool;`
- `export fn wktParseCoord(s: string, pos: &mut usize, out: &mut Coord2D): bool;`
- `export fn wktConsumeChar(s: string, pos: &mut usize, ch: u8): bool;`
- `export fn wktWriteF64(out: []u8, pos: usize, val: f64): usize;`
- `export fn wktWriteCoord(out: []u8, pos: usize, c: Coord2D): usize;`
- `export fn parseWkt(input: string): WktGeometry?;`
- `export fn formatWkt(g: WktGeometry, out: []u8): usize;`
- `export fn bboxWkt(g: WktGeometry): WktBbox;`
