[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# wildmatch

Zero-heap linear-time wildcard (*, ?, custom wildcards & case-insensitive) pattern matcher (clean-room port of Rust wildmatch)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wildmatch` ([https://github.com/becheran/wildmatch](https://github.com/becheran/wildmatch))

| Metadata | Value |
| --- | --- |
| Package | `wildmatch` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add wildmatch
```

```cp
import {
    WildMatch,
    wmByteEq,
    wmMatchCore,
    wildmatch,
    wildmatchIgnoreCase,
    wildmatchCustom,
    hasWildcards,
} from "wildmatch";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WildMatch` | `class` | Fields: `pattern: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool` · Methods: `new, newWithWildcards, newIgnoreCase, newWithEscape, matches` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wmByteEq` | `(a: u8, b: u8, ignore_case: bool): bool` | Invokes `wmByteEq` and returns `bool`. |
| `wmMatchCore` | `(pattern: string, input: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool): bool` | Invokes `wmMatchCore` and returns `bool`. |
| `wildmatch` | `(pattern: string, input: string): bool` | Invokes `wildmatch` and returns `bool`. |
| `wildmatchIgnoreCase` | `(pattern: string, input: string): bool` | Invokes `wildmatchIgnoreCase` and returns `bool`. |
| `wildmatchCustom` | `(pattern: string, input: string, any_many: u8, any_one: u8): bool` | Invokes `wildmatchCustom` and returns `bool`. |
| `hasWildcards` | `(pattern: string): bool` | Checks predicate `hasWildcards` and returns `bool`. |

## Minimal Example

```cp
import { wildmatch } from "wildmatch";

pub fn main() {
    let result = wildmatch("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
