[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# punycode

Zero-allocation RFC 3492 Punycode / IDNA (xn--...) encoder and decoder for internationalized domain names in CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:punycode` ([https://github.com/mathiasbynens/punycode.js](https://github.com/mathiasbynens/punycode.js)) / `crates.io:punycode` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `punycode` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add punycode
```

```cp
import {
    pcToLower,
    pcIsXnPrefix,
    isAsciiOnly,
    pcAdapt,
    pcEncodeDigit,
    pcDecodeDigit,
    pcThreshold,
    pcDecodeUtf8,
} from "punycode";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pcToLower` | `(c: u8): u8` | Invokes `pcToLower` and returns `u8`. |
| `pcIsXnPrefix` | `(s: &[]u8): bool` | Invokes `pcIsXnPrefix` and returns `bool`. |
| `isAsciiOnly` | `(s: string): bool` | Checks predicate `isAsciiOnly` and returns `bool`. |
| `pcAdapt` | `(delta_in: u32, numpoints: u32, firsttime: bool): u32` | Invokes `pcAdapt` and returns `u32`. |
| `pcEncodeDigit` | `(d: u32): u8` | Invokes `pcEncodeDigit` and returns `u8`. |
| `pcDecodeDigit` | `(c: u8): u32` | Invokes `pcDecodeDigit` and returns `u32`. |
| `pcThreshold` | `(k: u32, bias: u32): u32` | Invokes `pcThreshold` and returns `u32`. |
| `pcDecodeUtf8` | `(s: &[]u8, cps: []u32): i64` | Invokes `pcDecodeUtf8` and returns `i64`. |
| `pcEncodeUtf8` | `(cps: &[]u32, out: []u8): i64` | Invokes `pcEncodeUtf8` and returns `i64`. |
| `encodeLabel` | `(utf8_label: string, out: []u8): i64` | Serializes/formats data via `encodeLabel` returning `i64`. |
| `decodeLabel` | `(ascii_label: string, out_utf8: []u8): i64` | Parses input via `decodeLabel` and returns `i64`. |
| `pcWriteSlice` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `pcWriteSlice` and returns `bool`. |
| `toAsciiDomain` | `(utf8_domain: string, out: []u8): i64` | Invokes `toAsciiDomain` and returns `i64`. |
| `toUnicodeDomain` | `(ascii_domain: string, out: []u8): i64` | Invokes `toUnicodeDomain` and returns `i64`. |

## Minimal Example

```cp
import { isAsciiOnly } from "punycode";

pub fn main() {
    let result = isAsciiOnly("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
