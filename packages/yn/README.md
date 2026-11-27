[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# yn

Zero-allocation parser for yes/no boolean strings (y/yes/true/1/on/n/no/false/0/off) with lenient mode

| Metadata | Value |
| --- | --- |
| Package | `yn` |
| Version | `5.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add yn
```

```cp
import { yn, ynLenient, ynDefault } from "yn";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `yn` | `(input: string): bool?` | Invokes `yn` and returns `bool?`. |
| `ynLenient` | `(input: string): bool?` | Invokes `ynLenient` and returns `bool?`. |
| `ynDefault` | `(input: string, default_val: bool): bool` | Invokes `ynDefault` and returns `bool`. |

## Minimal Example

```cp
import { yn } from "yn";

pub fn main() {
    let result = yn("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
