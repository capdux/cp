[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# type-check

Zero-allocation runtime type expression parser and validator supporting primitives, unions (A | B), Maybe T, arrays ([T]), tuples ((A, B)), and objects (clean-room MIT implementation of npm type-check, #241 / 195M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:type-check` ([https://github.com/gkz/type-check](https://github.com/gkz/type-check)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `type-check` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add type-check
```

```cp
import {
    DynValueKind,
    DynValue,
    tckMatchAtom,
    typeCheck,
    isValidTypeExpr,
} from "type-check";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `DynValueKind` | `Undefined, Null, Boolean, Number, Int, Float, String, Array ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DynValue` | `class` | Fields: `kind: DynValueKind, elem_kind: DynValueKind, array_len: usize` · Methods: `of, arrayOf` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `tckMatchAtom` | `(atom: string, val: DynValue): bool` | Invokes `tckMatchAtom` and returns `bool`. |
| `typeCheck` | `(type_expr: string, val: DynValue): bool` | Invokes `typeCheck` and returns `bool`. |
| `isValidTypeExpr` | `(type_expr: string): bool` | Checks predicate `isValidTypeExpr` and returns `bool`. |

## Minimal Example

```cp
import { isValidTypeExpr } from "type-check";

pub fn main() {
    let result = isValidTypeExpr("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
