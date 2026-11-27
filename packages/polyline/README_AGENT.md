# polyline@1.0.0 (Agent Quickref)

> Zero-heap Google Encoded Polyline (precision 5/6) GPS coordinate encoder and decoder for CP (compatible with @mapbox/polyline and Rust polyline) | License: MIT
> Import: `import { ... } from "polyline";` | Full types: `src/index.d.cp`

## Functions
- `export fn precisionScale(precision: u32): f64;`
- `export fn roundToI64(v: f64): i64;`
- `export fn encodeSigned(val: i64, out: []u8): usize;`
- `export fn decodeSigned(data: string, start: usize, out_val: &mut i64): usize;`
- `export fn encodePoints(points: &[]LatLng, precision: u32, out: []u8): usize;`
- `export fn decodePoints(encoded: string, precision: u32, out: []LatLng): usize;`
- `export fn countPoints(encoded: string): usize;`
- `export fn isValid(encoded: string): bool;`
