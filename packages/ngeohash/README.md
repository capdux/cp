[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ngeohash

Zero-allocation Geohash Base32 spatial coordinate encoder, decoder, and bounding box library for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ngeohash` ([https://github.com/sunng87/node-geohash](https://github.com/sunng87/node-geohash))（原库名 `ngeohash` → CP 包名 `ngeohash`）

| Metadata | Value |
| --- | --- |
| Package | `ngeohash` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ngeohash
```

```cp
import {
    GeoBounds,
    GeoPoint,
    ghEncodeChar,
    ghDecodeChar,
    isValidGeohash,
    encode,
    decodeBounds,
    decode,
} from "ngeohash";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GeoBounds` | `class` | Fields: `min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64, valid: bool` |
| `GeoPoint` | `class` | Fields: `lat: f64, lon: f64, lat_err: f64, lon_err: f64, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ghEncodeChar` | `(v: u8): u8` | Invokes `ghEncodeChar` and returns `u8`. |
| `ghDecodeChar` | `(c: u8): u8` | Invokes `ghDecodeChar` and returns `u8`. |
| `isValidGeohash` | `(hash: string): bool` | Checks predicate `isValidGeohash` and returns `bool`. |
| `encode` | `(lat: f64, lon: f64, precision: usize, out: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decodeBounds` | `(hash: string): GeoBounds` | Parses input via `decodeBounds` and returns `GeoBounds`. |
| `decode` | `(hash: string): GeoPoint` | Parses input via `decode` and returns `GeoPoint`. |

## Minimal Example

```cp
import { isValidGeohash } from "ngeohash";

pub fn main() {
    let result = isValidGeohash("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
