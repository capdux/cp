# s2 — AI Agent Quick Ref

Zero-heap Google S2 spherical geometry: 64-bit S2CellId face/level/parent/child/contains, LatLng validation, and spherical distance (crates.io s2 equivalent) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class LatLng { lat_deg: f64, lng_deg: f64 } // methods: fromDegrees(...) +3
class S2CellId { id: u64 } // methods: fromFace(...) +11
```

## API Signatures

```cp
fn s2TrailingZeros(v: u64): u8;
fn s2MaskAbove(lsb_val: u64): u64;
fn s2Sin(x_in: f64): f64;
fn s2Cos(x_in: f64): f64;
fn s2Sqrt(x: f64): f64;
fn s2Asin(x: f64): f64;
fn haversineMeters(a: LatLng, b: LatLng): f64;
```

## Minimal Usage

```cp
import { s2TrailingZeros } from "s2";

pub fn main() {
    let result = s2TrailingZeros(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"s2"`; install via `cap add s2`.
