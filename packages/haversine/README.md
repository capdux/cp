[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# haversine

Zero-allocation WGS-84 spherical Haversine great-circle distance, initial bearing, midpoint, and destination point calculator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:haversine` ([https://github.com/njj/haversine](https://github.com/njj/haversine)) / `crates.io:haversine` ([https://github.com/gavinuhma/haversine](https://github.com/gavinuhma/haversine))（原库名 `haversine` → CP 包名 `haversine`）

| Metadata | Value |
| --- | --- |
| Package | `haversine` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add haversine
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LatLon` | `class` | Fields: `lat: f64, lon: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hgAbs` | `(x: f64): f64` | Invokes `hgAbs` and returns `f64`. |
| `hgClamp` | `(x: f64, lo: f64, hi: f64): f64` | Invokes `hgClamp` and returns `f64`. |
| `hgSqrt` | `(x: f64): f64` | Invokes `hgSqrt` and returns `f64`. |
| `hgWrapPi` | `(rad: f64): f64` | Invokes `hgWrapPi` and returns `f64`. |
| `hgSin` | `(rad: f64): f64` | Invokes `hgSin` and returns `f64`. |
| `hgCos` | `(rad: f64): f64` | Invokes `hgCos` and returns `f64`. |
| `hgAtan` | `(z: f64): f64` | Invokes `hgAtan` and returns `f64`. |
| `hgAtan2` | `(y: f64, x: f64): f64` | Invokes `hgAtan2` and returns `f64`. |
| `hgAsin` | `(x: f64): f64` | Invokes `hgAsin` and returns `f64`. |
| `hgDegToRad` | `(deg: f64): f64` | Invokes `hgDegToRad` and returns `f64`. |
| `hgRadToDeg` | `(rad: f64): f64` | Invokes `hgRadToDeg` and returns `f64`. |
| `hgNormLon` | `(lon: f64): f64` | Invokes `hgNormLon` and returns `f64`. |
| `isValidCoord` | `(p: LatLon): bool` | Checks predicate `isValidCoord` and returns `bool`. |
| `distanceMeters` | `(a: LatLon, b: LatLon): f64` | Invokes `distanceMeters` and returns `f64`. |
| `distanceKm` | `(a: LatLon, b: LatLon): f64` | Invokes `distanceKm` and returns `f64`. |
| `distanceMiles` | `(a: LatLon, b: LatLon): f64` | Invokes `distanceMiles` and returns `f64`. |
| `initialBearing` | `(a: LatLon, b: LatLon): f64` | Constructs/initializes an instance via `initialBearing`. |
| `midpoint` | `(a: LatLon, b: LatLon): LatLon` | Invokes `midpoint` and returns `LatLon`. |
| `destinationPoint` | `(start: LatLon, bearing_deg: f64, distance_m: f64): LatLon` | Invokes `destinationPoint` and returns `LatLon`. |

## Minimal Example

```cp
import { isValidCoord } from "haversine";

pub fn main() {
    let result = isValidCoord(/* p: LatLon */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
