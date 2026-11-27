[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-number

Zero-heap strict numeric string validator (integers, decimals, scientific, hex) matching npm is-number in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-number` ([https://github.com/jonschlinkert/is-number](https://github.com/jonschlinkert/is-number))

| Metadata | Value |
| --- | --- |
| Package | `is-number` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add is-number
```

```cp
import { isNumber, isIntegerStr, isFiniteF64 } from "is-number";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isNumber` | `(s: string): bool` | Checks predicate `isNumber` and returns `bool`. |
| `isIntegerStr` | `(s: string): bool` | Checks predicate `isIntegerStr` and returns `bool`. |
| `isFiniteF64` | `(x: f64): bool` | Checks predicate `isFiniteF64` and returns `bool`. |

## Minimal Example

```cp
import { isNumber } from "is-number";

pub fn main() {
    let result = isNumber("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
