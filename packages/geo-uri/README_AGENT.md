# geo-uri — AI Agent Quick Ref

Zero-allocation RFC 5870 geo: URI parser, coordinate validator, and formatter for CP. (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class GeoUri { lat: f64, lng: f64, has_alt: bool, alt: f64, /* +4 fields */ }
```

## API Signatures

```cp
fn guParseF64(s: &[]u8, out: &mut f64): bool;
fn guWriteRaw(out: []u8, pos: &mut usize, src: &[]u8): bool;
fn guWriteU64(out: []u8, pos: &mut usize, val: u64): bool;
fn guWriteF64(out: []u8, pos: &mut usize, val: f64): bool;
fn isValidLatLng(lat: f64, lng: f64): bool;
fn parseGeoUri(uri: string): GeoUri;
fn formatGeoUri(lat: f64, lng: f64, has_alt: bool, alt: f64, has_u: bool, u: f64, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseGeoUri } from "geo-uri";

pub fn main() {
    let result = parseGeoUri("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"geo-uri"`; install via `cap add geo-uri`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
