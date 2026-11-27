[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# data-encoding

Zero-allocation strict RFC 4648 Base16/Hex, Base32, and Base64/Base64URL codec with canonical trailing-bit verification (Rust data-encoding v2 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:data-encoding` ([https://github.com/ia0/data-encoding](https://github.com/ia0/data-encoding))

| Metadata | Value |
| --- | --- |
| Package | `data-encoding` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add data-encoding
```

```cp
import {
    DecodeResult,
    dencHexDigit,
    dencHexVal,
    dencB32Char,
    dencB32Val,
    dencB64Char,
    dencB64Val,
    dencHexEncodeImpl,
} from "data-encoding";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DecodeResult` | `class` | Fields: `ok: bool, len: usize, error_pos: usize` · Methods: `success, fail` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `dencHexDigit` | `(v: u8, upper: bool): u8` | Invokes `dencHexDigit` and returns `u8`. |
| `dencHexVal` | `(c: u8): u8` | Invokes `dencHexVal` and returns `u8`. |
| `dencB32Char` | `(v: u8): u8` | Invokes `dencB32Char` and returns `u8`. |
| `dencB32Val` | `(c: u8): u8` | Invokes `dencB32Val` and returns `u8`. |
| `dencB64Char` | `(v: u8, url_safe: bool): u8` | Invokes `dencB64Char` and returns `u8`. |
| `dencB64Val` | `(c: u8, url_safe: bool): u8` | Invokes `dencB64Val` and returns `u8`. |
| `dencHexEncodeImpl` | `(src: &[]u8, upper: bool, out: []u8): usize` | Invokes `dencHexEncodeImpl` and returns `usize`. |
| `base64EncodeLen` | `(n: usize, pad: bool): usize` | Invokes `base64EncodeLen` and returns `usize`. |
| `base32EncodeLen` | `(n: usize, pad: bool): usize` | Invokes `base32EncodeLen` and returns `usize`. |
| `hexLowerEncode` | `(src: &[]u8, out: []u8): usize` | Invokes `hexLowerEncode` and returns `usize`. |
| `hexUpperEncode` | `(src: &[]u8, out: []u8): usize` | Invokes `hexUpperEncode` and returns `usize`. |
| `hexDecode` | `(src: &[]u8, out: []u8): DecodeResult` | Invokes `hexDecode` and returns `DecodeResult`. |
| `base32Encode` | `(src: &[]u8, pad: bool, out: []u8): usize` | Invokes `base32Encode` and returns `usize`. |
| `base32Decode` | `(src: &[]u8, out: []u8): DecodeResult` | Invokes `base32Decode` and returns `DecodeResult`. |
| `base64Encode` | `(src: &[]u8, url_safe: bool, pad: bool, out: []u8): usize` | Invokes `base64Encode` and returns `usize`. |
| `base64Decode` | `(src: &[]u8, url_safe: bool, out: []u8): DecodeResult` | Invokes `base64Decode` and returns `DecodeResult`. |

## Minimal Example

```cp
import { dencHexDigit } from "data-encoding";

pub fn main() {
    let result = dencHexDigit(0, false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
