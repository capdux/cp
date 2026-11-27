[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicode-xid

Zero-allocation UAX #31 Unicode Identifier (XID_Start / XID_Continue) classification and identifier scanner for CP (clean-room MIT).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-xid` ([https://github.com/unicode-rs/unicode-xid](https://github.com/unicode-rs/unicode-xid))

| Metadata | Value |
| --- | --- |
| Package | `unicode-xid` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicode-xid
```

```cp
import {
    UxidRune,
    uxidDecodeUtf8Strict,
    uxidIsExtraStart,
    uxidBsearchStart,
    uxidBsearchCont,
    uxidIsAsciiAlpha,
    uxidIsAsciiDigit,
    isXidStart,
} from "unicode-xid";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `UxidRune` | `class` | Fields: `cp: u32, len: usize, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `uxidDecodeUtf8Strict` | `(s: string, offset: usize): UxidRune` | Invokes `uxidDecodeUtf8Strict` and returns `UxidRune`. |
| `uxidIsExtraStart` | `(cp: u32): bool` | Invokes `uxidIsExtraStart` and returns `bool`. |
| `uxidBsearchStart` | `(cp: u32): bool` | Invokes `uxidBsearchStart` and returns `bool`. |
| `uxidBsearchCont` | `(cp: u32): bool` | Invokes `uxidBsearchCont` and returns `bool`. |
| `uxidIsAsciiAlpha` | `(b: u8): bool` | Invokes `uxidIsAsciiAlpha` and returns `bool`. |
| `uxidIsAsciiDigit` | `(b: u8): bool` | Invokes `uxidIsAsciiDigit` and returns `bool`. |
| `isXidStart` | `(cp: u32): bool` | Checks predicate `isXidStart` and returns `bool`. |
| `isXidContinue` | `(cp: u32): bool` | Checks predicate `isXidContinue` and returns `bool`. |
| `isIdentStart` | `(cp: u32): bool` | Checks predicate `isIdentStart` and returns `bool`. |
| `isAsciiIdent` | `(s: string): bool` | Checks predicate `isAsciiIdent` and returns `bool`. |
| `isValidIdent` | `(s: string): bool` | Checks predicate `isValidIdent` and returns `bool`. |
| `scanIdentLen` | `(s: string, start: usize): usize` | Invokes `scanIdentLen` and returns `usize`. |

## Minimal Example

```cp
import { isAsciiIdent } from "unicode-xid";

pub fn main() {
    let result = isAsciiIdent("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
