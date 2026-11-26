# @cap/geohash-pure

Zero-allocation Geohash Base32 spatial coordinate encoder, decoder, and bounding box library for CP.

## Features

- **Zero heap allocation**: encodes directly into caller-supplied `[]u8` buffers.
- **Standard Geohash Base32**: `encode`, `decodeBounds`, `decode`, and `isValidGeohash` using `"0123456789bcdefghjkmnpqrstuvwxyz"`.

## Usage

```cp
import { encode, decode } from "@cap/geohash-pure";

let buf: [12]u8 = undefined;
const n = encode(42.6 as f64, -5.6 as f64, 5, buf[0..12]); // "ezs42"
const pt = decode(buf[0..n]);
```
