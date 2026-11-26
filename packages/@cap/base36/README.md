# @cap/base36

Zero-heap Base36 (`0-9a-z` / `0-9A-Z`) and arbitrary radix `2..36` 64-bit unsigned integer encoder and overflow-safe decoder in pure CP.

## Features

- `encodeRadix(val, radix, uppercase, out)` & `decodeRadix(input, radix)`: supports any radix `2..36` with exact 64-bit overflow detection
- `encodeBase36(val, out)`, `decodeBase36(input)`, `isValidBase36(input)`

## Usage

```cp
import { encodeBase36, decodeBase36 } from "@cap/base36";

let buf: [16]u8 = undefined;
const n = encodeBase36(1234567890, buf[0..16]); // "kf12oi"
const r = decodeBase36("KF12OI");               // 1234567890
```
