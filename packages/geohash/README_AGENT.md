# geohash — AI Agent Quick Ref

Zero-allocation Base32 Geohash encoder, bounding-box decoder, and 8-direction neighbor finder (Rust geohash / ngeohash parity) for CP (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class GeoPoint { lat: f64, lon: f64 }
class GeoBBox { min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64, /* +1 fields */ }
```

## API Signatures

```cp
fn invalidBBox(): GeoBBox;
fn base32Index(c: u8): i16;
fn isValid(hash: &[]u8): bool;
fn encode(lat: f64, lon: f64, precision: usize, dst: []u8): usize;
fn decodeBBox(hash: &[]u8): GeoBBox;
fn decode(hash: &[]u8): GeoPoint;
fn neighbor(hash: &[]u8, dlat: i32, dlon: i32, dst: []u8): usize;
```

## Minimal Usage

```cp
import { isValid } from "geohash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValid(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"geohash"`; install via `cap add geohash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
