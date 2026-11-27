[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# geo

Zero-allocation spherical geometry engine: Haversine distance, initial bearing, destination point, midpoint, bounding box, and point-in-polygon (Rust geo / haversine / geolib parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:geo` ([https://github.com/georust/geo](https://github.com/georust/geo))（原库名 `geo` → CP 包名 `geo`）

| Metadata | Value |
| --- | --- |
| Package | `geo` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add geo
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Coord` | `class` | Fields: `lat: f64, lon: f64` |
| `Bounds` | `class` | Fields: `min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `toRad` | `(deg: f64): f64` | Invokes `toRad` and returns `f64`. |
| `toDeg` | `(rad: f64): f64` | Invokes `toDeg` and returns `f64`. |
| `normalizeLon` | `(lon_in: f64): f64` | Invokes `normalizeLon` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `sinF64` | `(x_in: f64): f64` | Invokes `sinF64` and returns `f64`. |
| `cosF64` | `(x: f64): f64` | Invokes `cosF64` and returns `f64`. |
| `atanF64` | `(x_in: f64): f64` | Invokes `atanF64` and returns `f64`. |
| `atan2F64` | `(y: f64, x: f64): f64` | Invokes `atan2F64` and returns `f64`. |
| `asinF64` | `(x: f64): f64` | Invokes `asinF64` and returns `f64`. |
| `haversineMeters` | `(a: Coord, b: Coord): f64` | Invokes `haversineMeters` and returns `f64`. |
| `initialBearing` | `(a: Coord, b: Coord): f64` | Constructs/initializes an instance via `initialBearing`. |
| `destinationPoint` | `(origin: Coord, bearing_deg: f64, distance_m: f64): Coord` | Invokes `destinationPoint` and returns `Coord`. |
| `midpoint` | `(a: Coord, b: Coord): Coord` | Invokes `midpoint` and returns `Coord`. |
| `boundingBox` | `(center: Coord, radius_m: f64): Bounds` | Invokes `boundingBox` and returns `Bounds`. |
| `pointInPolygon` | `(pt: Coord, poly: &[]Coord): bool` | Invokes `pointInPolygon` and returns `bool`. |

## Minimal Example

```cp
import { toRad } from "geo";

pub fn main() {
    let result = toRad(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
