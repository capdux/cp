[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# iban_validate

Zero-heap ISO 13616 IBAN validator, ISO 7064 Mod-97-10 checksum engine, check-digit generator, and electronic/friendly formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:iban_validate` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `iban_validate` → CP 包名 `iban_validate`）

| Metadata | Value |
| --- | --- |
| Package | `iban_validate` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add iban_validate
```

```cp
import {
    countryLength,
    electronicFormat,
    friendlyFormat,
    mod97,
    isValid,
    computeCheckDigits,
} from "iban_validate";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `countryLength` | `(c0: u8, c1: u8): usize` | Invokes `countryLength` and returns `usize`. |
| `electronicFormat` | `(input: string, out: []u8): usize` | Invokes `electronicFormat` and returns `usize`. |
| `friendlyFormat` | `(input: string, out: []u8): usize` | Invokes `friendlyFormat` and returns `usize`. |
| `mod97` | `(electronic: string): u32` | Invokes `mod97` and returns `u32`. |
| `isValid` | `(input: string): bool` | Checks predicate `isValid` and returns `bool`. |
| `computeCheckDigits` | `(country0: u8, country1: u8, bban: string, out_d0: &mut u8, out_d1: &mut u8): bool` | Invokes `computeCheckDigits` and returns `bool`. |

## Minimal Example

```cp
import { isValid } from "iban_validate";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
