[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bech32

Zero-allocation BIP-173 Bech32 and BIP-350 Bech32m encoder, decoder, 8-to-5 bit converter, and BCH checksum verifier for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bech32` ([https://github.com/rust-bitcoin/rust-bech32](https://github.com/rust-bitcoin/rust-bech32))

| Metadata | Value |
| --- | --- |
| Package | `bech32` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bech32
```

```cp
import {
    Bech32Variant,
    Bech32Decoded,
    ParsedBech32Meta,
    charsetIndex,
    polymodStep,
    polymodHrp,
    variantConst,
    isValidHrp,
} from "bech32";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `Bech32Variant` | `Bech32, Bech32m` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Bech32Decoded` | `class` | Fields: `valid: bool, variant: Bech32Variant, hrp: string, data: string` · Methods: `invalid` |
| `ParsedBech32Meta` | `class` | Fields: `valid: bool, variant: Bech32Variant, sep: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `charsetIndex` | `(c: u8): u8?` | Invokes `charsetIndex` and returns `u8?`. |
| `polymodStep` | `(chk: u32, v: u8): u32` | Invokes `polymodStep` and returns `u32`. |
| `polymodHrp` | `(hrp: string): u32` | Invokes `polymodHrp` and returns `u32`. |
| `variantConst` | `(v: Bech32Variant): u32` | Invokes `variantConst` and returns `u32`. |
| `isValidHrp` | `(hrp: string): bool` | Checks predicate `isValidHrp` and returns `bool`. |
| `to5BitWords` | `(bytes: string, out: []u8): usize?` | Invokes `to5BitWords` and returns `usize?`. |
| `from5BitWords` | `(words: &[]u8, out: []u8): usize?` | Invokes `from5BitWords` and returns `usize?`. |
| `encodeBech32` | `(hrp: string, payload_bytes: string, variant: Bech32Variant, out: []u8): string?` | Serializes/formats data via `encodeBech32` returning `string?`. |
| `validateStructureAndChecksum` | `(encoded: string): ParsedBech32Meta` | Validates input with `validateStructureAndChecksum` returning `ParsedBech32Meta`. |
| `verifyBech32` | `(encoded: string): bool` | Validates input with `verifyBech32` returning `bool`. |
| `decodeBech32` | `(encoded: string, hrp_out: []u8, data_out: []u8): Bech32Decoded` | Parses input via `decodeBech32` and returns `Bech32Decoded`. |

## Minimal Example

```cp
import { isValidHrp } from "bech32";

pub fn main() {
    let result = isValidHrp("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
