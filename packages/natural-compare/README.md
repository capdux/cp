[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# natural-compare

Zero-allocation natural / alphanumeric string comparison and in-place sorting (natural-compare / natsort / natord parity, arbitrary-length integer segments without overflow) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:natural-compare` ([https://github.com/litejs/natural-compare-lite](https://github.com/litejs/natural-compare-lite))（原库名 `natural-compare` → CP 包名 `natural-compare`）

| Metadata | Value |
| --- | --- |
| Package | `natural-compare` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add natural-compare
```

```cp
import {
    compareRawBytes,
    naturalCompareImpl,
    naturalCompare,
    naturalCompareIgnoreCase,
    naturalLess,
    naturalLessIgnoreCase,
    naturalEq,
    comesBefore,
} from "natural-compare";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `compareRawBytes` | `(a: string, b: string): i8` | Invokes `compareRawBytes` and returns `i8`. |
| `naturalCompareImpl` | `(a: string, b: string, ignore_case: bool): i8` | Invokes `naturalCompareImpl` and returns `i8`. |
| `naturalCompare` | `(a: string, b: string): i8` | Invokes `naturalCompare` and returns `i8`. |
| `naturalCompareIgnoreCase` | `(a: string, b: string): i8` | Invokes `naturalCompareIgnoreCase` and returns `i8`. |
| `naturalLess` | `(a: string, b: string): bool` | Invokes `naturalLess` and returns `bool`. |
| `naturalLessIgnoreCase` | `(a: string, b: string): bool` | Invokes `naturalLessIgnoreCase` and returns `bool`. |
| `naturalEq` | `(a: string, b: string): bool` | Invokes `naturalEq` and returns `bool`. |
| `comesBefore` | `(a: string, b: string, desc: bool, ignore_case: bool): bool` | Invokes `comesBefore` and returns `bool`. |
| `siftDown` | `(items: []string, start: usize, end: usize, desc: bool, ignore_case: bool)` | Executes `siftDown` in-place. |
| `sortInPlace` | `(items: []string, desc: bool, ignore_case: bool)` | Executes `sortInPlace` in-place. |
| `sortNatural` | `(items: []string)` | Executes `sortNatural` in-place. |
| `sortNaturalDesc` | `(items: []string)` | Executes `sortNaturalDesc` in-place. |
| `sortNaturalIgnoreCase` | `(items: []string)` | Executes `sortNaturalIgnoreCase` in-place. |
| `isSortedNatural` | `(items: []string): bool` | Checks predicate `isSortedNatural` and returns `bool`. |

## Minimal Example

```cp
import { isSortedNatural } from "natural-compare";

pub fn main() {
    let result = isSortedNatural(/* items: []string */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
