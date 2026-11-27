[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# geo-uri

Zero-allocation RFC 5870 geo: URI parser, coordinate validator, and formatter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:geo-uri` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `geo-uri` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add geo-uri
```

```cp
import {
    GeoUri,
    guParseF64,
    guWriteRaw,
    guWriteU64,
    guWriteF64,
    isValidLatLng,
    parseGeoUri,
    formatGeoUri,
} from "geo-uri";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `GeoUri` | `class` | Fields: `lat: f64, lng: f64, has_alt: bool, alt: f64, has_uncertainty: bool, uncertainty: f64, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `guParseF64` | `(s: &[]u8, out: &mut f64): bool` | Invokes `guParseF64` and returns `bool`. |
| `guWriteRaw` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `guWriteRaw` and returns `bool`. |
| `guWriteU64` | `(out: []u8, pos: &mut usize, val: u64): bool` | Invokes `guWriteU64` and returns `bool`. |
| `guWriteF64` | `(out: []u8, pos: &mut usize, val: f64): bool` | Invokes `guWriteF64` and returns `bool`. |
| `isValidLatLng` | `(lat: f64, lng: f64): bool` | Checks predicate `isValidLatLng` and returns `bool`. |
| `parseGeoUri` | `(uri: string): GeoUri` | Parses input via `parseGeoUri` and returns `GeoUri`. |
| `formatGeoUri` | `(lat: f64, lng: f64, has_alt: bool, alt: f64, has_u: bool, u: f64, out: []u8): usize` | Serializes/formats data via `formatGeoUri` returning `usize`. |

## Minimal Example

```cp
import { parseGeoUri } from "geo-uri";

pub fn main() {
    let result = parseGeoUri("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
