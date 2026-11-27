[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# base16ct

Pure CP zero-allocation constant-time (table-free) Base16 / Hex encoder and strict decoder (RustCrypto base16ct parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:base16ct` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

| Metadata | Value |
| --- | --- |
| Package | `base16ct` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add base16ct
```

```cp
import {
    HexDecodeResult,
    b16InRange,
    b16EncodeNibbleLower,
    b16EncodeNibbleUpper,
    b16DecodeNibble,
    b16ZeroOut,
    encodedLen,
    encodeLower,
} from "base16ct";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HexDecodeResult` | `class` | Fields: `len: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `b16InRange` | `(c: u32, lo: u32, hi: u32): u32` | Invokes `b16InRange` and returns `u32`. |
| `b16EncodeNibbleLower` | `(nib: u8): u8` | Invokes `b16EncodeNibbleLower` and returns `u8`. |
| `b16EncodeNibbleUpper` | `(nib: u8): u8` | Invokes `b16EncodeNibbleUpper` and returns `u8`. |
| `b16DecodeNibble` | `(ch: u8, allow_lower: bool, allow_upper: bool): u32` | Invokes `b16DecodeNibble` and returns `u32`. |
| `b16ZeroOut` | `(out: []u8, n: usize)` | Executes `b16ZeroOut` in-place. |
| `encodedLen` | `(n: usize): usize` | Serializes/formats data via `encodedLen` returning `usize`. |
| `encodeLower` | `(src: []u8, out: []u8): usize` | Serializes/formats data via `encodeLower` returning `usize`. |
| `encodeUpper` | `(src: []u8, out: []u8): usize` | Serializes/formats data via `encodeUpper` returning `usize`. |
| `b16DecodeCore` | `(hex: string, out: []u8, allow_lower: bool, allow_upper: bool): HexDecodeResult` | Invokes `b16DecodeCore` and returns `HexDecodeResult`. |
| `decodeLower` | `(hex: string, out: []u8): HexDecodeResult` | Parses input via `decodeLower` and returns `HexDecodeResult`. |
| `decodeUpper` | `(hex: string, out: []u8): HexDecodeResult` | Parses input via `decodeUpper` and returns `HexDecodeResult`. |
| `decodeMixed` | `(hex: string, out: []u8): HexDecodeResult` | Parses input via `decodeMixed` and returns `HexDecodeResult`. |

## Minimal Example

```cp
import { decodeLower } from "base16ct";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeLower("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
