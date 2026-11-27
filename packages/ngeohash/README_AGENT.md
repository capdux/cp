# ngeohash@1.0.0 (Agent Quickref)

> Zero-allocation Geohash Base32 spatial coordinate encoder, decoder, and bounding box library for CP. | License: MIT
> Import: `import { ... } from "ngeohash";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidGeohash(hash: string): bool;`
- `export fn encode(lat: f64, lon: f64, precision: usize, out: []u8): usize;`
- `export fn decodeBounds(hash: string): GeoBounds;`
- `export fn decode(hash: string): GeoPoint;`
