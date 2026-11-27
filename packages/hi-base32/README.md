[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hi-base32

Zero-allocation RFC 4648 Section 6/7 standard Base32 (A-Z2-7) and Base32hex (0-9A-V) encoder and decoder in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hi-base32` ([https://github.com/emn178/hi-base32](https://github.com/emn178/hi-base32))（原库名 `hi-base32` → CP 包名 `hi-base32`）

| Metadata | Value |
| --- | --- |
| Package | `hi-base32` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hi-base32
```

```cp
import {
    decodeStdChar,
    decodeHexChar,
    encodedLen,
    encodeWithAlphabet,
    decodeWithMode,
    encode,
    decode,
    encodeHex,
} from "hi-base32";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `decodeStdChar` | `(b: u8): i32` | Parses input via `decodeStdChar` and returns `i32`. |
| `decodeHexChar` | `(b: u8): i32` | Parses input via `decodeHexChar` and returns `i32`. |
| `encodedLen` | `(byte_len: usize, pad: bool): usize` | Serializes/formats data via `encodedLen` returning `usize`. |
| `encodeWithAlphabet` | `(data: &[]u8, pad: bool, alphabet: string, out: []u8): usize` | Serializes/formats data via `encodeWithAlphabet` returning `usize`. |
| `decodeWithMode` | `(input: string, is_hex: bool, out: []u8): i64` | Parses input via `decodeWithMode` and returns `i64`. |
| `encode` | `(data: &[]u8, pad: bool, out: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decode` | `(input: string, out: []u8): i64` | Parses input via `decode` and returns `i64`. |
| `encodeHex` | `(data: &[]u8, pad: bool, out: []u8): usize` | Serializes/formats data via `encodeHex` returning `usize`. |
| `decodeHex` | `(input: string, out: []u8): i64` | Parses input via `decodeHex` and returns `i64`. |

## Minimal Example

```cp
import { decodeWithMode } from "hi-base32";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeWithMode("example", false, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
