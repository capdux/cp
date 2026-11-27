# ngeohash — AI Agent Quick Ref

Zero-allocation Geohash Base32 spatial coordinate encoder, decoder, and bounding box library for CP. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class GeoBounds { min_lat: f64, max_lat: f64, min_lon: f64, max_lon: f64, /* +1 fields */ }
class GeoPoint { lat: f64, lon: f64, lat_err: f64, lon_err: f64, /* +1 fields */ }
```

## API Signatures

```cp
fn ghEncodeChar(v: u8): u8;
fn ghDecodeChar(c: u8): u8;
fn isValidGeohash(hash: string): bool;
fn encode(lat: f64, lon: f64, precision: usize, out: []u8): usize;
fn decodeBounds(hash: string): GeoBounds;
fn decode(hash: string): GeoPoint;
```

## Minimal Usage

```cp
import { isValidGeohash } from "ngeohash";

pub fn main() {
    let result = isValidGeohash("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ngeohash"`; install via `cap add ngeohash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
