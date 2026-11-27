[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# nth-check

Zero-allocation CSS :nth-child(An+B) microsyntax parser and index matcher

| Metadata | Value |
| --- | --- |
| Package | `nth-check` |
| Version | `2.1.1` |
| License | `BSD-2-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add nth-check
```

```cp
import { NthRule, parseNth, nthCheck } from "nth-check";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `NthRule` | `class` | Fields: `a: i32, b: i32, valid: bool` · Methods: `matches` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseNth` | `(formula: string): NthRule` | Parses input via `parseNth` and returns `NthRule`. |
| `nthCheck` | `(formula: string, index: usize): bool` | Invokes `nthCheck` and returns `bool`. |

## Minimal Example

```cp
import { parseNth } from "nth-check";

pub fn main() {
    let result = parseNth("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
