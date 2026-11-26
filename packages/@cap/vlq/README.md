# @cap/vlq

Zero-heap Source Map v3 Base64 VLQ (Variable-Length Quantity) signed integer sequence encoder and decoder in pure CP.

## Features

- `encodeOne(val, out)` & `decodeOne(input, start)`: exact Source Map v3 VLQ encoding with LSB sign bit and 5-bit continuation chunks
- `encodeSlice(values, out)` & `decodeSlice(input, out)`: segment-level encoding and decoding (e.g. `[0, 0, 0, 0]` <-> `"AAAA"`)
- Strict rejection of invalid Base64 characters, truncated continuation bits, and 64-bit shift overflow

## Usage

```cp
import { encodeSlice, decodeSlice } from "@cap/vlq";

const seg: [4]i64 = [0, 0, 10, 4];
let buf: [32]u8 = undefined;
const n = encodeSlice(seg[0..4], buf[0..32]);
```
