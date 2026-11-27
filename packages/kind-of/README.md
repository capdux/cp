[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kind-of

Fast zero-heap value & JSON/literal type classifier (clean-room CP implementation of npm kind-of)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kind-of` ([https://github.com/jonschlinkert/kind-of](https://github.com/jonschlinkert/kind-of))

| Metadata | Value |
| --- | --- |
| Package | `kind-of` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add kind-of
```

```cp
import {
    koIsSpace,
    koIsDigit,
    koEq,
    koStartsWith,
    koIsNumber,
    koIsBigInt,
    koIsIsoDate,
    kindOf,
} from "kind-of";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `koIsSpace` | `(c: u8): bool` | Invokes `koIsSpace` and returns `bool`. |
| `koIsDigit` | `(c: u8): bool` | Invokes `koIsDigit` and returns `bool`. |
| `koEq` | `(s: string, start: usize, end: usize, lit: string): bool` | Invokes `koEq` and returns `bool`. |
| `koStartsWith` | `(s: string, start: usize, end: usize, prefix: string): bool` | Invokes `koStartsWith` and returns `bool`. |
| `koIsNumber` | `(s: string, start: usize, end: usize): bool` | Invokes `koIsNumber` and returns `bool`. |
| `koIsBigInt` | `(s: string, start: usize, end: usize): bool` | Invokes `koIsBigInt` and returns `bool`. |
| `koIsIsoDate` | `(s: string, start: usize, end: usize): bool` | Invokes `koIsIsoDate` and returns `bool`. |
| `kindOf` | `(raw: string): u8` | Invokes `kindOf` and returns `u8`. |
| `kindName` | `(kind: u8): string` | Invokes `kindName` and returns `string`. |
| `kindOfName` | `(raw: string): string` | Invokes `kindOfName` and returns `string`. |
| `isPrimitive` | `(raw: string): bool` | Checks predicate `isPrimitive` and returns `bool`. |
| `isPlainObject` | `(raw: string): bool` | Checks predicate `isPlainObject` and returns `bool`. |

## Minimal Example

```cp
import { isPrimitive } from "kind-of";

pub fn main() {
    let result = isPrimitive("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
