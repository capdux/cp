[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# nom

Zero-allocation streaming parser combinator primitives (tag, tagNoCase, take, takeUntil, character classes, delimited/separated sequences, integer parsers) for CP (clean-room MIT, inspired by Rust nom v7).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:nom` ([https://github.com/rust-bakery/nom](https://github.com/rust-bakery/nom))（原库名 `nom` → CP 包名 `nom`）

| Metadata | Value |
| --- | --- |
| Package | `nom` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add nom
```

```cp
import {
    ParseRes,
    IntRes,
    UintRes,
    nomFail,
    nomOk,
    nomContainsByte,
    nomIsDigit,
    nomIsHexDigit,
} from "nom";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ParseRes` | `class` | Fields: `ok: bool, rest: string, matched: string` |
| `IntRes` | `class` | Fields: `ok: bool, rest: string, value: i64` |
| `UintRes` | `class` | Fields: `ok: bool, rest: string, value: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `nomFail` | `(input: string): ParseRes` | Invokes `nomFail` and returns `ParseRes`. |
| `nomOk` | `(input: string, split_at: usize): ParseRes` | Invokes `nomOk` and returns `ParseRes`. |
| `nomContainsByte` | `(set: string, b: u8): bool` | Invokes `nomContainsByte` and returns `bool`. |
| `nomIsDigit` | `(b: u8): bool` | Invokes `nomIsDigit` and returns `bool`. |
| `nomIsHexDigit` | `(b: u8): bool` | Invokes `nomIsHexDigit` and returns `bool`. |
| `nomHexVal` | `(b: u8): u64` | Invokes `nomHexVal` and returns `u64`. |
| `nomIsAlpha` | `(b: u8): bool` | Invokes `nomIsAlpha` and returns `bool`. |
| `nomIsAlnum` | `(b: u8): bool` | Invokes `nomIsAlnum` and returns `bool`. |
| `nomIsSpace` | `(b: u8): bool` | Invokes `nomIsSpace` and returns `bool`. |
| `nomIsMultispace` | `(b: u8): bool` | Invokes `nomIsMultispace` and returns `bool`. |
| `nomSliceEq` | `(a: string, b: string): bool` | Invokes `nomSliceEq` and returns `bool`. |
| `tag` | `(input: string, expected: string): ParseRes` | Invokes `tag` and returns `ParseRes`. |
| `tagNoCase` | `(input: string, expected: string): ParseRes` | Invokes `tagNoCase` and returns `ParseRes`. |
| `take` | `(input: string, count: usize): ParseRes` | Invokes `take` and returns `ParseRes`. |
| `takeUntil` | `(input: string, needle: string): ParseRes` | Invokes `takeUntil` and returns `ParseRes`. |
| `isNot` | `(input: string, forbidden_chars: string): ParseRes` | Checks predicate `isNot` and returns `bool`. |
| `isA` | `(input: string, allowed_chars: string): ParseRes` | Checks predicate `isA` and returns `bool`. |
| `digit0` | `(input: string): ParseRes` | Invokes `digit0` and returns `ParseRes`. |
| `digit1` | `(input: string): ParseRes` | Invokes `digit1` and returns `ParseRes`. |
| `hexDigit1` | `(input: string): ParseRes` | Invokes `hexDigit1` and returns `ParseRes`. |
| `alpha0` | `(input: string): ParseRes` | Invokes `alpha0` and returns `ParseRes`. |
| `alpha1` | `(input: string): ParseRes` | Invokes `alpha1` and returns `ParseRes`. |
| `alphanumeric0` | `(input: string): ParseRes` | Invokes `alphanumeric0` and returns `ParseRes`. |
| `alphanumeric1` | `(input: string): ParseRes` | Invokes `alphanumeric1` and returns `ParseRes`. |
| `space0` | `(input: string): ParseRes` | Invokes `space0` and returns `ParseRes`. |
| `space1` | `(input: string): ParseRes` | Invokes `space1` and returns `ParseRes`. |
| `multispace0` | `(input: string): ParseRes` | Invokes `multispace0` and returns `ParseRes`. |
| `multispace1` | `(input: string): ParseRes` | Invokes `multispace1` and returns `ParseRes`. |
| `lineEnding` | `(input: string): ParseRes` | Invokes `lineEnding` and returns `ParseRes`. |
| `notLineEnding` | `(input: string): ParseRes` | Invokes `notLineEnding` and returns `ParseRes`. |
| `delimitedTag` | `(input: string, open: string, close: string): ParseRes` | Invokes `delimitedTag` and returns `ParseRes`. |
| `precededTag` | `(input: string, prefix: string): ParseRes` | Invokes `precededTag` and returns `ParseRes`. |
| `terminatedTag` | `(input: string, suffix: string): ParseRes` | Invokes `terminatedTag` and returns `ParseRes`. |
| `separatedPairTag` | `(input: string, sep: string): ParseRes` | Invokes `separatedPairTag` and returns `ParseRes`. |
| `parseU64` | `(input: string): UintRes` | Parses input via `parseU64` and returns `UintRes`. |
| `parseI64` | `(input: string): IntRes` | Parses input via `parseI64` and returns `IntRes`. |
| `parseHexU64` | `(input: string): UintRes` | Parses input via `parseHexU64` and returns `UintRes`. |

## Minimal Example

```cp
import { isNot } from "nom";

pub fn main() {
    let result = isNot("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
