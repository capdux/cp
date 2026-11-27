[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ccount

Zero-allocation character/substring occurrence counter and longest consecutive streak finder

| Metadata | Value |
| --- | --- |
| Package | `ccount` |
| Version | `2.0.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ccount
```

```cp
import { ccountByte, ccount, longestStreakByte, longestStreak } from "ccount";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ccountByte` | `(value: string, ch: u8): usize` | Invokes `ccountByte` and returns `usize`. |
| `ccount` | `(value: string, needle: string): usize` | Invokes `ccount` and returns `usize`. |
| `longestStreakByte` | `(value: string, ch: u8): usize` | Invokes `longestStreakByte` and returns `usize`. |
| `longestStreak` | `(value: string, needle: string): usize` | Invokes `longestStreak` and returns `usize`. |

## Minimal Example

```cp
import { ccountByte } from "ccount";

pub fn main() {
    let result = ccountByte("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
