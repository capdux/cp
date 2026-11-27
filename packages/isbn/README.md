[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# isbn

Zero-allocation ISO 2108 ISBN-10, ISBN-13, ISSN, and EAN-13 validator and converter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:isbn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `isbn` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add isbn
```

```cp
import {
    isbnToUpper,
    isbnIsDigit,
    stripHyphens,
    isbnNormalizeExact,
    computeIsbn13CheckDigit,
    isValidIsbn10,
    isValidEan13,
    isValidIsbn13,
} from "isbn";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isbnToUpper` | `(c: u8): u8` | Checks predicate `isbnToUpper` and returns `bool`. |
| `isbnIsDigit` | `(c: u8): bool` | Checks predicate `isbnIsDigit` and returns `bool`. |
| `stripHyphens` | `(input: string, out: []u8): usize` | Invokes `stripHyphens` and returns `usize`. |
| `isbnNormalizeExact` | `(input: string, expected_len: usize, out: []u8): bool` | Checks predicate `isbnNormalizeExact` and returns `bool`. |
| `computeIsbn13CheckDigit` | `(prefix12: string): u8` | Invokes `computeIsbn13CheckDigit` and returns `u8`. |
| `isValidIsbn10` | `(input: string): bool` | Checks predicate `isValidIsbn10` and returns `bool`. |
| `isValidEan13` | `(input: string): bool` | Checks predicate `isValidEan13` and returns `bool`. |
| `isValidIsbn13` | `(input: string): bool` | Checks predicate `isValidIsbn13` and returns `bool`. |
| `isValidIssn` | `(input: string): bool` | Checks predicate `isValidIssn` and returns `bool`. |
| `isValidIsbn` | `(input: string): bool` | Checks predicate `isValidIsbn` and returns `bool`. |
| `isbn10To13` | `(isbn10: string, out: []u8): usize` | Checks predicate `isbn10To13` and returns `bool`. |
| `isbn13To10` | `(isbn13: string, out: []u8): usize` | Checks predicate `isbn13To10` and returns `bool`. |

## Minimal Example

```cp
import { isbnNormalizeExact } from "isbn";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isbnNormalizeExact("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
