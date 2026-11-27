[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# geohash

Zero-allocation Base32 Geohash encoder, bounding-box decoder, and 8-direction neighbor finder (Rust geohash / ngeohash parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:geohash` ([https://github.com/georust/geohash](https://github.com/georust/geohash))

| Metadata | Value |
| --- | --- |
| Package | `geohash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add geohash
```

```cp
import {
    GeoPoint,
    GeoBBox,
    invalidBBox,
    base32Index,
    isValid,
    encode,
    decodeBBox,
    decode,
} from "geohash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GeoPoint` | `class` | Fields: `lat: f64, lon: f64` |
| `GeoBBox` | `class` | Fields: `min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidBBox` | `(): GeoBBox` | Invokes `invalidBBox` and returns `GeoBBox`. |
| `base32Index` | `(c: u8): i16` | Invokes `base32Index` and returns `i16`. |
| `isValid` | `(hash: &[]u8): bool` | Checks predicate `isValid` and returns `bool`. |
| `encode` | `(lat: f64, lon: f64, precision: usize, dst: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decodeBBox` | `(hash: &[]u8): GeoBBox` | Parses input via `decodeBBox` and returns `GeoBBox`. |
| `decode` | `(hash: &[]u8): GeoPoint` | Parses input via `decode` and returns `GeoPoint`. |
| `neighbor` | `(hash: &[]u8, dlat: i32, dlon: i32, dst: []u8): usize` | Invokes `neighbor` and returns `usize`. |

## Minimal Example

```cp
import { isValid } from "geohash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValid(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
