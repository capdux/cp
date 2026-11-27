[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# s2

Zero-heap Google S2 spherical geometry: 64-bit S2CellId face/level/parent/child/contains, LatLng validation, and spherical distance (crates.io s2 equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:s2` ([https://github.com/roydbt/s2](https://github.com/roydbt/s2))

| Metadata | Value |
| --- | --- |
| Package | `s2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add s2
```

```cp
import {
    LatLng,
    S2CellId,
    s2TrailingZeros,
    s2MaskAbove,
    s2Sin,
    s2Cos,
    s2Sqrt,
    s2Asin,
} from "s2";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LatLng` | `class` | Fields: `lat_deg: f64, lng_deg: f64` · Methods: `fromDegrees, isValid, latRad, lngRad` |
| `S2CellId` | `class` | Fields: `id: u64` · Methods: `fromFace, fromFacePosLevel, face, lsb, isValid, level, +6 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `s2TrailingZeros` | `(v: u64): u8` | Invokes `s2TrailingZeros` and returns `u8`. |
| `s2MaskAbove` | `(lsb_val: u64): u64` | Invokes `s2MaskAbove` and returns `u64`. |
| `s2Sin` | `(x_in: f64): f64` | Invokes `s2Sin` and returns `f64`. |
| `s2Cos` | `(x_in: f64): f64` | Invokes `s2Cos` and returns `f64`. |
| `s2Sqrt` | `(x: f64): f64` | Invokes `s2Sqrt` and returns `f64`. |
| `s2Asin` | `(x: f64): f64` | Invokes `s2Asin` and returns `f64`. |
| `haversineMeters` | `(a: LatLng, b: LatLng): f64` | Invokes `haversineMeters` and returns `f64`. |

## Minimal Example

```cp
import { s2TrailingZeros } from "s2";

pub fn main() {
    let result = s2TrailingZeros(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
