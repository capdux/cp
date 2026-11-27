[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# esutils

Zero-allocation ECMAScript keyword, reserved word, identifier, whitespace, and line terminator utilities (clean-room MIT implementation of npm esutils, #201 / 215M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:esutils` ([https://github.com/estools/esutils](https://github.com/estools/esutils)) | 许可：`BSD-2-Clause`

| Metadata | Value |
| --- | --- |
| Package | `esutils` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add esutils
```

```cp
import {
    isDecimalDigit,
    isHexDigit,
    isOctalDigit,
    isWhiteSpace,
    isLineTerminator,
    isIdentifierStartES6,
    isIdentifierPartES6,
    isKeywordES6,
} from "esutils";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isDecimalDigit` | `(cp: u32): bool` | Checks predicate `isDecimalDigit` and returns `bool`. |
| `isHexDigit` | `(cp: u32): bool` | Checks predicate `isHexDigit` and returns `bool`. |
| `isOctalDigit` | `(cp: u32): bool` | Checks predicate `isOctalDigit` and returns `bool`. |
| `isWhiteSpace` | `(cp: u32): bool` | Checks predicate `isWhiteSpace` and returns `bool`. |
| `isLineTerminator` | `(cp: u32): bool` | Checks predicate `isLineTerminator` and returns `bool`. |
| `isIdentifierStartES6` | `(cp: u32): bool` | Checks predicate `isIdentifierStartES6` and returns `bool`. |
| `isIdentifierPartES6` | `(cp: u32): bool` | Checks predicate `isIdentifierPartES6` and returns `bool`. |
| `isKeywordES6` | `(id: string, strict: bool): bool` | Checks predicate `isKeywordES6` and returns `bool`. |
| `isReservedWordES6` | `(id: string, strict: bool): bool` | Checks predicate `isReservedWordES6` and returns `bool`. |
| `isRestrictedWord` | `(id: string): bool` | Checks predicate `isRestrictedWord` and returns `bool`. |
| `isIdentifierNameES6` | `(id: string): bool` | Checks predicate `isIdentifierNameES6` and returns `bool`. |
| `isIdentifierES6` | `(id: string, strict: bool): bool` | Checks predicate `isIdentifierES6` and returns `bool`. |

## Minimal Example

```cp
import { isKeywordES6 } from "esutils";

pub fn main() {
    let result = isKeywordES6("example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
