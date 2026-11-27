[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicode-ident

Zero-heap UAX #31 Unicode identifier classification (XID_Start / XID_Continue) and UTF-8 identifier scanner in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-ident` ([https://github.com/dtolnay/unicode-ident](https://github.com/dtolnay/unicode-ident))

| Metadata | Value |
| --- | --- |
| Package | `unicode-ident` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicode-ident
```

```cp
import {
    Utf8Codepoint,
    uidInRange,
    decodeUtf8At,
    uidIsXidStartNonAscii,
    uidIsXidContinueExtra,
    isXidStart,
    isXidContinue,
    isIdentStart,
} from "unicode-ident";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Utf8Codepoint` | `class` | Fields: `cp: u32, len: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `uidInRange` | `(cp: u32, lo: u32, hi: u32): bool` | Invokes `uidInRange` and returns `bool`. |
| `decodeUtf8At` | `(s: string, offset: usize): Utf8Codepoint` | Parses input via `decodeUtf8At` and returns `Utf8Codepoint`. |
| `uidIsXidStartNonAscii` | `(cp: u32): bool` | Invokes `uidIsXidStartNonAscii` and returns `bool`. |
| `uidIsXidContinueExtra` | `(cp: u32): bool` | Invokes `uidIsXidContinueExtra` and returns `bool`. |
| `isXidStart` | `(cp: u32): bool` | Checks predicate `isXidStart` and returns `bool`. |
| `isXidContinue` | `(cp: u32): bool` | Checks predicate `isXidContinue` and returns `bool`. |
| `isIdentStart` | `(cp: u32): bool` | Checks predicate `isIdentStart` and returns `bool`. |
| `scanIdentLen` | `(s: string): usize` | Invokes `scanIdentLen` and returns `usize`. |
| `isValidIdent` | `(s: string): bool` | Checks predicate `isValidIdent` and returns `bool`. |

## Minimal Example

```cp
import { decodeUtf8At } from "unicode-ident";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
