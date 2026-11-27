[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `base36`

> **Source Attribution**: `npm:base36` ([base36](https://www.npmjs.com/package/base36)) & `crates.io:radix_fmt` ([davimiku/radix_fmt](https://github.com/davimiku/radix_fmt)) — License: MIT

Zero-heap Base36 and arbitrary radix (`2..36`) unsigned 64-bit integer encoder, overflow-safe decoder, byte-buffer codec, and validator in pure CP.

## Zero-Cost Migration (TS / Rust → CP)

Keep the exact upstream package name (`base36`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: base36)
import base36 from "base36";
const encoded = base36.base36encode(123456789);
const decoded = base36.base36decode(encoded);
```

```rust
// Rust (crates.io: radix_fmt)
use radix_fmt::radix_36;
let encoded = format!("{}", radix_36(123456789u64));
```

```cp
// CP (zero-heap, direct buffer output)
import { encodeBase36, decodeBase36 } from "base36";

let buf: [32]u8 = undefined;
let n = encodeBase36(123456789, buf[0..32]);
let encoded = buf[0..n] as string; // "21i3v9"
let res = decodeBase36(encoded);   // res.valid == true, res.value == 123456789
```

## Installation

Add `base36` to your `cap.json`:

```json
{
  "dependencies": {
    "base36": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `ParseU64Result` | `class { value: u64, valid: bool }` | Overflow-safe integer decode result |
| `DecodeBytesResult` | `class { len: usize, valid: bool }` | Arbitrary-byte Base36 decode result |
| `encodeBase36` | `(val: u64, out: []u8): usize` | Encode `u64` to lowercase Base36 |
| `decodeBase36` | `(input: string): ParseU64Result` | Decode case-insensitive Base36 to `u64` |
| `encodeU64` | `(val: u64, out: []u8): usize` | Alias for lowercase Base36 `u64` encoding |
| `encodeU64Upper` | `(val: u64, out: []u8): usize` | Encode `u64` to uppercase Base36 |
| `decodeU64` | `(input: string): ParseU64Result` | Alias for `decodeBase36` |
| `encodeRadix` | `(val: u64, radix: u8, uppercase: bool, out: []u8): usize` | Encode `u64` in any radix `2..=36` |
| `decodeRadix` | `(input: string, radix: u8): ParseU64Result` | Decode string in any radix `2..=36` with overflow guard |
| `encodedLenU64` | `(val: u64, radix: u8): usize` | Compute exact output byte length before encoding |
| `isValidBase36` | `(input: string): bool` | Validate non-empty Base36 string |
| `isValidBase36String` | `(input: string): bool` | Alias for `isValidBase36` |
| `encodeBytes` | `(data: &[]u8, uppercase: bool, out: []u8): usize` | Encode arbitrary byte slice into Base36 |
| `decodeBytes` | `(input: string, out: []u8): DecodeBytesResult` | Decode Base36 string back into raw bytes |
| `digitChar` / `digitVal` | `(v: u8, uppercase: bool): u8` / `(c: u8): i32` | Low-level digit conversion helpers |

## Practical Examples

### 1. Encoding & Decoding `u64` (Base36 & Arbitrary Radix)

```cp
import {
    encodeU64,
    encodeU64Upper,
    decodeU64,
    encodeRadix,
    decodeRadix,
    encodedLenU64,
    isValidBase36,
} from "base36";

let out: [64]u8 = undefined;

// Base36 lowercase & uppercase
let n1 = encodeU64(46655, out[0..64]);       // "zzz" (n1 == 3)
let n2 = encodeU64Upper(46655, out[0..64]);  // "ZZZ" (n2 == 3)
let parsed = decodeU64("ZZZ");               // parsed.valid == true, parsed.value == 46655

// Arbitrary radix (e.g. binary, hex, base32)
let need = encodedLenU64(255, 16);           // 2
let n3 = encodeRadix(255, 16, true, out[0..64]); // "FF"
let hex = decodeRadix("ff", 16);             // hex.valid == true, hex.value == 255

// Validation
let ok = isValidBase36("a1B2z9");            // true
let bad = isValidBase36("not-valid!");       // false
```

### 2. Encoding & Decoding Arbitrary Byte Buffers

```cp
import { encodeBytes, decodeBytes } from "base36";

let raw: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
let encoded_buf: [64]u8 = undefined;
let enc_len = encodeBytes(&raw[0..4], false, encoded_buf[0..64]);
let enc_str = encoded_buf[0..enc_len] as string;

let decoded_buf: [16]u8 = undefined;
let dec = decodeBytes(enc_str, decoded_buf[0..16]);
if (dec.valid) {
    // decoded_buf[0..dec.len] matches raw
}
```

## Notes & Edge Cases

- **Buffer sizing**: A `u64` requires at most 13 bytes in Base36 and at most 64 bytes in Base2. Use `encodedLenU64(val, radix)` to pre-check capacity.
- **Overflow protection**: `decodeRadix` / `decodeBase36` / `decodeU64` return `ParseU64Result { value: 0, valid: false }` on invalid characters, empty input, out-of-range radix, or `u64` overflow.
- **Zero heap allocations**: All encoders and decoders write directly into caller-provided `[]u8` slices.
