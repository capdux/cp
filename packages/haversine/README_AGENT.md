# haversine — AI Agent Quick Ref

Zero-allocation WGS-84 spherical Haversine great-circle distance, initial bearing, midpoint, and destination point calculator for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    LatLon,
    hgAbs,
    hgClamp,
    hgSqrt,
    hgWrapPi,
    hgSin,
    hgCos,
    hgAtan,
} from "haversine";
```

## Core Types & Constants

```cp
class LatLon { lat: f64, lon: f64 }
```

## API Signatures

```cp
fn hgAbs(x: f64): f64;
fn hgClamp(x: f64, lo: f64, hi: f64): f64;
fn hgSqrt(x: f64): f64;
fn hgWrapPi(rad: f64): f64;
fn hgSin(rad: f64): f64;
fn hgCos(rad: f64): f64;
fn hgAtan(z: f64): f64;
fn hgAtan2(y: f64, x: f64): f64;
fn hgAsin(x: f64): f64;
fn hgDegToRad(deg: f64): f64;
fn hgRadToDeg(rad: f64): f64;
fn hgNormLon(lon: f64): f64;
fn isValidCoord(p: LatLon): bool;
fn distanceMeters(a: LatLon, b: LatLon): f64;
fn distanceKm(a: LatLon, b: LatLon): f64;
fn distanceMiles(a: LatLon, b: LatLon): f64;
fn initialBearing(a: LatLon, b: LatLon): f64;
fn midpoint(a: LatLon, b: LatLon): LatLon;
fn destinationPoint(start: LatLon, bearing_deg: f64, distance_m: f64): LatLon;
```

## Minimal Usage

```cp
import { isValidCoord } from "haversine";

pub fn main() {
    let result = isValidCoord(/* p: LatLon */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"haversine"`; install via `cap add haversine`.
