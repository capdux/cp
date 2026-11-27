[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# urlencoding

Zero-heap RFC 3986 percent-encoding and decoding in pure CP (modeled after Rust urlencoding v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:urlencoding` ([https://github.com/kornelski/rust_urlencoding](https://github.com/kornelski/rust_urlencoding))（原库名 `urlencoding` → CP 包名 `urlencoding`）

| Metadata | Value |
| --- | --- |
| Package | `urlencoding` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add urlencoding
```

```cp
import {
    urlencHexUpper,
    urlencHexVal,
    isUnreserved,
    encodedLen,
    encodeBinary,
    encode,
    decode,
    decodeLossy,
} from "urlencoding";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `urlencHexUpper` | `(nibble: u8): u8` | Invokes `urlencHexUpper` and returns `u8`. |
| `urlencHexVal` | `(ch: u8): i16` | Invokes `urlencHexVal` and returns `i16`. |
| `isUnreserved` | `(b: u8): bool` | Checks predicate `isUnreserved` and returns `bool`. |
| `encodedLen` | `(input: string): usize` | Serializes/formats data via `encodedLen` returning `usize`. |
| `encodeBinary` | `(data: &[]u8, out: []u8): usize` | Serializes/formats data via `encodeBinary` returning `usize`. |
| `encode` | `(input: string, out: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decode` | `(input: string, out: []u8): i64` | Parses input via `decode` and returns `i64`. |
| `decodeLossy` | `(input: string, out: []u8): usize` | Parses input via `decodeLossy` and returns `usize`. |

## Minimal Example

```cp
import { encodedLen } from "urlencoding";

pub fn main() {
    let result = encodedLen("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
