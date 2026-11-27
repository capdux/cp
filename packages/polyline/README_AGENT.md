# polyline — AI Agent Quick Ref

Zero-heap Google Encoded Polyline (precision 5/6) GPS coordinate encoder and decoder for CP (compatible with @mapbox/polyline and Rust polyline) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class LatLng { lat: f64, lng: f64 }
```

## API Signatures

```cp
fn precisionScale(precision: u32): f64;
fn roundToI64(v: f64): i64;
fn encodeSigned(val: i64, out: []u8): usize;
fn decodeSigned(data: string, start: usize, out_val: &mut i64): usize;
fn encodePoints(points: &[]LatLng, precision: u32, out: []u8): usize;
fn decodePoints(encoded: string, precision: u32, out: []LatLng): usize;
fn countPoints(encoded: string): usize;
fn isValid(encoded: string): bool;
```

## Minimal Usage

```cp
import { decodeSigned } from "polyline";

pub fn main() {
    let result = decodeSigned("example", 0, /* out_val: &mut i64 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"polyline"`; install via `cap add polyline`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
