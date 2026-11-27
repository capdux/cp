# haversine@1.0.0 (Agent Quickref)

> Zero-allocation WGS-84 spherical Haversine great-circle distance, initial bearing, midpoint, and destination point calculator for CP. | License: MIT
> Import: `import { ... } from "haversine";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidCoord(p: LatLon): bool;`
- `export fn distanceMeters(a: LatLon, b: LatLon): f64;`
- `export fn distanceKm(a: LatLon, b: LatLon): f64;`
- `export fn distanceMiles(a: LatLon, b: LatLon): f64;`
- `export fn initialBearing(a: LatLon, b: LatLon): f64;`
- `export fn midpoint(a: LatLon, b: LatLon): LatLon;`
- `export fn destinationPoint(start: LatLon, bearing_deg: f64, distance_m: f64): LatLon;`
