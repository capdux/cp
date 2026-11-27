[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bs58

Zero-allocation Base58 encoder/decoder supporting Bitcoin, Ripple, and Flickr alphabets with leading-zero preservation (Rust bs58 / npm bs58 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bs58` ([https://github.com/Nullus157/bs58-rs](https://github.com/Nullus157/bs58-rs))

| Metadata | Value |
| --- | --- |
| Package | `bs58` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bs58
```

```cp
import {
    Base58Alphabet,
    alphabetChars,
    charIndex,
    isValidBase58,
    encodeBase58With,
    decodeBase58With,
    encodeBase58,
    decodeBase58,
} from "bs58";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `Base58Alphabet` | `Bitcoin, Ripple, Flickr` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `alphabetChars` | `(alpha: Base58Alphabet): string` | Invokes `alphabetChars` and returns `string`. |
| `charIndex` | `(alphabet: string, c: u8): i8` | Invokes `charIndex` and returns `i8`. |
| `isValidBase58` | `(encoded: string, alpha: Base58Alphabet): bool` | Checks predicate `isValidBase58` and returns `bool`. |
| `encodeBase58With` | `(input: string, alpha: Base58Alphabet, out: []u8): string?` | Serializes/formats data via `encodeBase58With` returning `string?`. |
| `decodeBase58With` | `(encoded: string, alpha: Base58Alphabet, out: []u8): string?` | Parses input via `decodeBase58With` and returns `string?`. |
| `encodeBase58` | `(input: string, out: []u8): string?` | Serializes/formats data via `encodeBase58` returning `string?`. |
| `decodeBase58` | `(encoded: string, out: []u8): string?` | Parses input via `decodeBase58` and returns `string?`. |

## Minimal Example

```cp
import { isValidBase58 } from "bs58";

pub fn main() {
    let result = isValidBase58("example", /* alpha: Base58Alphabet */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
