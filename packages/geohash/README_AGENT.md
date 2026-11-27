# geohash@1.0.0 (Agent Quickref)

> Zero-allocation Base32 Geohash encoder, bounding-box decoder, and 8-direction neighbor finder (Rust geohash / ngeohash parity) for CP | License: MIT
> Import: `import { ... } from "geohash";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValid(hash: &[]u8): bool;`
- `export fn encode(lat: f64, lon: f64, precision: usize, dst: []u8): usize;`
- `export fn decodeBBox(hash: &[]u8): GeoBBox;`
- `export fn decode(hash: &[]u8): GeoPoint;`
- `export fn neighbor(hash: &[]u8, dlat: i32, dlon: i32, dst: []u8): usize;`
