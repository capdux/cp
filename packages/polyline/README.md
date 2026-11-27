[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# polyline

Zero-heap Google Encoded Polyline (precision 5/6) GPS coordinate encoder and decoder for CP (compatible with @mapbox/polyline and Rust polyline)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:polyline` ([https://github.com/georust/polyline](https://github.com/georust/polyline))

| Metadata | Value |
| --- | --- |
| Package | `polyline` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add polyline
```

```cp
import {
    LatLng,
    precisionScale,
    roundToI64,
    encodeSigned,
    decodeSigned,
    encodePoints,
    decodePoints,
    countPoints,
} from "polyline";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LatLng` | `class` | Fields: `lat: f64, lng: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `precisionScale` | `(precision: u32): f64` | Invokes `precisionScale` and returns `f64`. |
| `roundToI64` | `(v: f64): i64` | Invokes `roundToI64` and returns `i64`. |
| `encodeSigned` | `(val: i64, out: []u8): usize` | Serializes/formats data via `encodeSigned` returning `usize`. |
| `decodeSigned` | `(data: string, start: usize, out_val: &mut i64): usize` | Parses input via `decodeSigned` and returns `usize`. |
| `encodePoints` | `(points: &[]LatLng, precision: u32, out: []u8): usize` | Serializes/formats data via `encodePoints` returning `usize`. |
| `decodePoints` | `(encoded: string, precision: u32, out: []LatLng): usize` | Parses input via `decodePoints` and returns `usize`. |
| `countPoints` | `(encoded: string): usize` | Invokes `countPoints` and returns `usize`. |
| `isValid` | `(encoded: string): bool` | Checks predicate `isValid` and returns `bool`. |

## Minimal Example

```cp
import { decodeSigned } from "polyline";

pub fn main() {
    let result = decodeSigned("example", 0, /* out_val: &mut i64 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
