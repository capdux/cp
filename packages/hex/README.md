[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hex

Zero-allocation hexadecimal encoder/decoder, fixed-width integer hex formatter, and canonical HexDump utility for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hex` ([https://github.com/KokaKiwi/rust-hex](https://github.com/KokaKiwi/rust-hex))

| Metadata | Value |
| --- | --- |
| Package | `hex` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hex
```

```cp
import {
    appendByte,
    encodeHex,
    encodeHexUpper,
    isValidHex,
    decodeHex,
    decodeHexPrefixed,
    encodeU32Hex,
    encodeU64Hex,
} from "hex";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `encodeHex` | `(input: string, out: []u8): string` | Serializes/formats data via `encodeHex` returning `string`. |
| `encodeHexUpper` | `(input: string, out: []u8): string` | Serializes/formats data via `encodeHexUpper` returning `string`. |
| `isValidHex` | `(s: string): bool` | Checks predicate `isValidHex` and returns `bool`. |
| `decodeHex` | `(hex_str: string, out: []u8): string?` | Parses input via `decodeHex` and returns `string?`. |
| `decodeHexPrefixed` | `(hex_str: string, out: []u8): string?` | Parses input via `decodeHexPrefixed` and returns `string?`. |
| `encodeU32Hex` | `(v: u32, out: []u8): string` | Serializes/formats data via `encodeU32Hex` returning `string`. |
| `encodeU64Hex` | `(v: u64, out: []u8): string` | Serializes/formats data via `encodeU64Hex` returning `string`. |
| `hexDump` | `(data: string, out: []u8): string` | Invokes `hexDump` and returns `string`. |

## Minimal Example

```cp
import { encodeHex } from "hex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeHex("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
