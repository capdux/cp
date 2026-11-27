[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# simdutf8

Zero-heap strict RFC 3629 UTF-8 validator with 16-lane hardware SIMD ASCII fast-path for CP (Rust `simdutf8` parity).

Validates UTF-8 byte buffers at SIMD speed (`@Vector(16, u8)`), rejecting overlong encodings, UTF-16 surrogates (`U+D800..U+DFFF`), codepoints above `U+10FFFF`, and truncated sequences — with both fast boolean (`basic`) and detailed error-location (`compat`: `valid_up_to`, `has_error_len`, `error_len`) APIs.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `simdutf8` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/rusticstuff/simdutf8` |
| License | MIT (upstream: MIT OR Apache-2.0) |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: simdutf8)
let ok = simdutf8::basic::from_utf8(bytes).is_ok();
if let Err(e) = simdutf8::compat::from_utf8(bytes) {
    let valid_up_to = e.valid_up_to();
    let error_len = e.error_len();
}
```

```cp
// CP (cap: simdutf8) — 16-lane SIMD ASCII fast-path, zero heap
import { basicIsValidUtf8, validateUtf8Compat } from "simdutf8";
const ok = basicIsValidUtf8(bytes);
const res = validateUtf8Compat(bytes);
if (!res.valid) {
    // res.valid_up_to, res.has_error_len, res.error_len
}
```

## Installation

```bash
cap add simdutf8
```

## Usage

### Basic Usage: Fast UTF-8 & ASCII Validation and Codepoint Counting

```cp
import { isAscii, basicIsValidUtf8, countCodepoints } from "simdutf8";

let ascii: [11]u8 = ['h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'];
const pureAscii = isAscii(ascii[0..11]);          // true (16-lane SIMD)
const valid = basicIsValidUtf8(ascii[0..11]);     // true

// UTF-8 string "你好" (6 bytes -> 2 Unicode scalar values)
let zh: [6]u8 = [0xE4, 0xBD, 0xA0, 0xE5, 0xA5, 0xBD];
if (n := countCodepoints(zh[0..6])) {
    // n == 2
}
```

### Security & Edge Cases: Streaming Truncation vs Malformed Sequence Detection

```cp
import { validateUtf8Compat } from "simdutf8";

// 1. Truncated 3-byte sequence at chunk boundary ("hi" + 0xE4, 0xBD)
let partial: [4]u8 = ['h', 'i', 0xE4, 0xBD];
const r1 = validateUtf8Compat(partial[0..4]);
// r1.valid == false, r1.valid_up_to == 2, r1.has_error_len == false (incomplete tail, wait for next chunk!)

// 2. Rejects UTF-16 surrogate U+D800 (ED A0 80) and overlong encodings (C0 AF)
let surrogate: [3]u8 = [0xED, 0xA0, 0x80];
const r2 = validateUtf8Compat(surrogate[0..3]);
// r2.valid == false, r2.valid_up_to == 0, r2.has_error_len == true, r2.error_len == 1
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `Utf8Error` | `class { valid_up_to: usize, has_error_len: bool, error_len: usize }` | Structured error descriptor matching Rust `Utf8Error`. |
| `Utf8CompatResult` | `class { valid: bool, valid_up_to: usize, has_error_len: bool, error_len: usize }` | Full validation result with exact failure offset & length. |
| `isAscii` | `fn(buf: []u8): bool` | 16-lane SIMD check whether all bytes `< 0x80`. |
| `basicIsValidUtf8` | `fn(buf: []u8): bool` | Fast strict RFC 3629 UTF-8 validity check. |
| `validateUtf8Compat` | `fn(buf: []u8): Utf8CompatResult` | Strict validation returning `valid_up_to` and optional `error_len`. |
| `countCodepoints` | `fn(buf: []u8): usize?` | Validates UTF-8 and returns Unicode scalar count via 16-lane SIMD, or `null` if invalid. |

## Security & Performance

- **16-lane SIMD ASCII fast-path**: Skips 16 pure-ASCII bytes per cycle via `@Vector(16, u8)` high-bit mask (`0x80`).
- **Strict RFC 3629 compliance**: Rejects overlong 2/3/4-byte encodings (`C0..C1`, `E0 80..9F`, `F0 80..8F`), UTF-16 surrogates (`ED A0..BF`), and values `> U+10FFFF` (`F4 90..BF`, `F5..FF`).

## License

MIT — Clean-room CP implementation with Rust `simdutf8` parity.
