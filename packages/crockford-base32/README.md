[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# crockford-base32

Zero-allocation Douglas Crockford Base32 encoder/decoder for 64-bit integers (with optional mod-37 check symbol), arbitrary byte streams, and grouped formatting in CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:crockford-base32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `crockford-base32` → CP 包名 `crockford-base32`）

| Metadata | Value |
| --- | --- |
| Package | `crockford-base32` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add crockford-base32
```

```cp
import {
    bcBuildDecode32Lut,
    bcBuildDecode37Lut,
    bcEncodeDigit32,
    bcEncodeCheck37,
    bcDecodeDigit32,
    bcDecodeCheck37,
    encodeU64,
    decodeU64,
} from "crockford-base32";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bcBuildDecode32Lut` | `(): [256]u8` | Invokes `bcBuildDecode32Lut` and returns `[256]u8`. |
| `bcBuildDecode37Lut` | `(): [256]u8` | Invokes `bcBuildDecode37Lut` and returns `[256]u8`. |
| `bcEncodeDigit32` | `(v: u8): u8` | Invokes `bcEncodeDigit32` and returns `u8`. |
| `bcEncodeCheck37` | `(v: u8): u8` | Invokes `bcEncodeCheck37` and returns `u8`. |
| `bcDecodeDigit32` | `(c: u8): u8` | Invokes `bcDecodeDigit32` and returns `u8`. |
| `bcDecodeCheck37` | `(c: u8): u8` | Invokes `bcDecodeCheck37` and returns `u8`. |
| `encodeU64` | `(val: u64, with_check: bool, out: []u8): usize` | Serializes/formats data via `encodeU64` returning `usize`. |
| `decodeU64` | `(input: string, with_check: bool, out_val: &mut u64): bool` | Parses input via `decodeU64` and returns `bool`. |
| `encodeBytes` | `(data: &[]u8, out: []u8): usize` | Serializes/formats data via `encodeBytes` returning `usize`. |
| `decodeBytes` | `(input: string, out: []u8): i64` | Parses input via `decodeBytes` and returns `i64`. |
| `formatGrouped` | `(raw: string, group_size: usize, out: []u8): usize` | Serializes/formats data via `formatGrouped` returning `usize`. |

## Minimal Example

```cp
import { decodeU64 } from "crockford-base32";

pub fn main() {
    let result = decodeU64("example", false, /* out_val: &mut u64 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
