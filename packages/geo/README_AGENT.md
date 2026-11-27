# geo — AI Agent Quick Ref

Zero-allocation spherical geometry engine: Haversine distance, initial bearing, destination point, midpoint, bounding box, and point-in-polygon (Rust geo / haversine / geolib parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Coord,
    Bounds,
    toRad,
    toDeg,
    normalizeLon,
    sqrtF64,
    sinF64,
    cosF64,
} from "geo";
```

## Core Types & Constants

```cp
class Coord { lat: f64, lon: f64 }
class Bounds { min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64 }
```

## API Signatures

```cp
fn toRad(deg: f64): f64;
fn toDeg(rad: f64): f64;
fn normalizeLon(lon_in: f64): f64;
fn sqrtF64(x: f64): f64;
fn sinF64(x_in: f64): f64;
fn cosF64(x: f64): f64;
fn atanF64(x_in: f64): f64;
fn atan2F64(y: f64, x: f64): f64;
fn asinF64(x: f64): f64;
fn haversineMeters(a: Coord, b: Coord): f64;
fn initialBearing(a: Coord, b: Coord): f64;
fn destinationPoint(origin: Coord, bearing_deg: f64, distance_m: f64): Coord;
fn midpoint(a: Coord, b: Coord): Coord;
fn boundingBox(center: Coord, radius_m: f64): Bounds;
fn pointInPolygon(pt: Coord, poly: &[]Coord): bool;
```

## Minimal Usage

```cp
import { toRad } from "geo";

pub fn main() {
    let result = toRad(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"geo"`; install via `cap add geo`.
