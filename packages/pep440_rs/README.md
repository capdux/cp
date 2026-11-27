[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pep440_rs

Zero-heap PEP 440 Python version parser, total-order comparator, and specifier matcher in pure CP (modeled after Astral pep440_rs)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pep440_rs` ([https://github.com/astral-sh/uv](https://github.com/astral-sh/uv))（原库名 `pep440_rs` → CP 包名 `pep440_rs`）

| Metadata | Value |
| --- | --- |
| Package | `pep440_rs` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pep440_rs
```

```cp
import {
    PreKind,
    Pep440Version,
    PepNumRes,
    pepInvalid,
    pepIsDigit,
    pepIsSpace,
    pepToLower,
    pepIsSep,
} from "pep440_rs";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `PreKind` | `None, Alpha, Beta, Rc` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Pep440Version` | `class` | Fields: `valid: bool, epoch: u32, release: [4]u32, release_len: u8, pre_kind: PreKind, pre_num: u32, +4 more` |
| `PepNumRes` | `class` | Fields: `ok: bool, val: u32, next: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pepInvalid` | `(): Pep440Version` | Invokes `pepInvalid` and returns `Pep440Version`. |
| `pepIsDigit` | `(c: u8): bool` | Invokes `pepIsDigit` and returns `bool`. |
| `pepIsSpace` | `(c: u8): bool` | Invokes `pepIsSpace` and returns `bool`. |
| `pepToLower` | `(c: u8): u8` | Invokes `pepToLower` and returns `u8`. |
| `pepIsSep` | `(c: u8): bool` | Invokes `pepIsSep` and returns `bool`. |
| `pepMatchWord` | `(s: string, pos: usize, end: usize, word: string): bool` | Invokes `pepMatchWord` and returns `bool`. |
| `pepReadU32` | `(s: string, pos: usize, end: usize): PepNumRes` | Invokes `pepReadU32` and returns `PepNumRes`. |
| `parsePep440` | `(s: string): Pep440Version` | Parses input via `parsePep440` and returns `Pep440Version`. |
| `pepCmpU32` | `(a: u32, b: u32): i32` | Invokes `pepCmpU32` and returns `i32`. |
| `pepPreRank` | `(k: PreKind): i32` | Invokes `pepPreRank` and returns `i32`. |
| `pepStageRank` | `(v: Pep440Version): i32` | Invokes `pepStageRank` and returns `i32`. |
| `comparePep440` | `(a: Pep440Version, b: Pep440Version): i32` | Invokes `comparePep440` and returns `i32`. |
| `pepPrefixMatch` | `(v: Pep440Version, target: Pep440Version, prefix_len: u8): bool` | Invokes `pepPrefixMatch` and returns `bool`. |
| `pepMatchSingle` | `(v: Pep440Version, raw: string): bool` | Invokes `pepMatchSingle` and returns `bool`. |
| `matchesSpecifier` | `(ver: string, spec: string): bool` | Invokes `matchesSpecifier` and returns `bool`. |

## Minimal Example

```cp
import { parsePep440 } from "pep440_rs";

pub fn main() {
    let result = parsePep440("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
