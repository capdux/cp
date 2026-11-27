# geo@1.0.0 (Agent Quickref)

> Zero-allocation spherical geometry engine: Haversine distance, initial bearing, destination point, midpoint, bounding box, and point-in-polygon (Rust geo / haversine / geolib parity) for CP | License: MIT
> Import: `import { ... } from "geo";` | Full types: `src/index.d.cp`

## Functions
- `export fn haversineMeters(a: Coord, b: Coord): f64;`
- `export fn initialBearing(a: Coord, b: Coord): f64;`
- `export fn destinationPoint(origin: Coord, bearing_deg: f64, distance_m: f64): Coord;`
- `export fn midpoint(a: Coord, b: Coord): Coord;`
- `export fn boundingBox(center: Coord, radius_m: f64): Bounds;`
- `export fn pointInPolygon(pt: Coord, poly: &[]Coord): bool;`
