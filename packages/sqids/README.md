[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sqids

Zero-allocation YouTube-style reversible short ID generator (Sqids specification) with deterministic shuffle, min-length padding, and profanity blocklist for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sqids` ([https://github.com/sqids/sqids-javascript](https://github.com/sqids/sqids-javascript))

| Metadata | Value |
| --- | --- |
| Package | `sqids` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sqids
```

```cp
import {
    SqidsError,
    SqidsOptions,
    Sqids,
    hasDigit,
    isBlockedId,
    shuffleAlphabet,
    reverseBytes,
    appendToId,
} from "sqids";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SqidsError` | `None, AlphabetTooShort, AlphabetTooLong, AlphabetNonAscii, AlphabetDuplicateChar` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SqidsOptions` | `class` | Fields: `alphabet: string, min_length: u8` |
| `Sqids` | `class` | Fields: `alphabet: [MAX_ALPHABET_LEN]u8, alphabet_len: usize, min_length: u8, valid: bool, err: SqidsError` · Methods: `initDefault, init, isValid, encode, encodeOne, decode, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hasDigit` | `(s: string): bool` | Checks predicate `hasDigit` and returns `bool`. |
| `isBlockedId` | `(id: &[]u8): bool` | Checks predicate `isBlockedId` and returns `bool`. |
| `shuffleAlphabet` | `(buf: []u8)` | Executes `shuffleAlphabet` in-place. |
| `reverseBytes` | `(buf: []u8)` | Executes `reverseBytes` in-place. |
| `appendToId` | `(num: u64, alphabet: &[]u8, dst: []u8, pos: usize): usize?` | Invokes `appendToId` and returns `usize?`. |
| `chunkToNumber` | `(chunk: &[]u8, alphabet: &[]u8): u64?` | Invokes `chunkToNumber` and returns `u64?`. |

## Minimal Example

```cp
import { hasDigit } from "sqids";

pub fn main() {
    let result = hasDigit("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
