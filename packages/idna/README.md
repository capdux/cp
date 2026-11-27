[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# idna

Zero-allocation UTS #46 Internationalized Domain Names (IDNA domainToAscii / domainToUnicode) reusing icu-normalizer for NFC and punycode for RFC 3492 Punycode (idna crate parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:idna` ([https://github.com/servo/rust-url](https://github.com/servo/rust-url))（原库名 `idna` → CP 包名 `idna`）

| Metadata | Value |
| --- | --- |
| Package | `idna` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add idna
```

```cp
import {
    idnaIsLdhLower,
    idnaIsLdhAnyCase,
    isPunycodeLabel,
    idnaMapDomain,
    idnaValidateAsciiLabelLower,
    idnaAppendBytes,
    domainToAscii,
    domainToUnicode,
} from "idna";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `idnaIsLdhLower` | `(ch: u8): bool` | Invokes `idnaIsLdhLower` and returns `bool`. |
| `idnaIsLdhAnyCase` | `(ch: u8): bool` | Invokes `idnaIsLdhAnyCase` and returns `bool`. |
| `isPunycodeLabel` | `(label: string): bool` | Checks predicate `isPunycodeLabel` and returns `bool`. |
| `idnaMapDomain` | `(domain: string, out: []u8): i64` | Invokes `idnaMapDomain` and returns `i64`. |
| `idnaValidateAsciiLabelLower` | `(label: &[]u8): i64` | Invokes `idnaValidateAsciiLabelLower` and returns `i64`. |
| `idnaAppendBytes` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `idnaAppendBytes` and returns `bool`. |
| `domainToAscii` | `(domain: string, out: []u8): i64` | Invokes `domainToAscii` and returns `i64`. |
| `domainToUnicode` | `(domain: string, out: []u8): i64` | Invokes `domainToUnicode` and returns `i64`. |
| `isValidIdnaAscii` | `(domain: string): bool` | Checks predicate `isValidIdnaAscii` and returns `bool`. |

## Minimal Example

```cp
import { isPunycodeLabel } from "idna";

pub fn main() {
    let result = isPunycodeLabel("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
