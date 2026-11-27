[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# balanced-match

Zero-allocation nested balanced delimiter matcher (npm balanced-match v3 parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`balanced-match`](https://github.com/juliangruber/balanced-match)（`MIT`）（包名 `balanced-match`）。

| Metadata | Value |
| --- | --- |
| Package | `balanced-match` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add balanced-match
```

```cp
import {
    BalancedResult,
    bmMakeRange,
    bmEmptyResult,
    balancedRange,
    balanced,
    balancedAll,
} from "balanced-match";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BalancedResult` | `class` | Fields: `found: bool, start: usize, end: usize, pre: string, body: string, post: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bmMakeRange` | `(s: usize, e: usize): [usize, usize]` | Invokes `bmMakeRange` and returns `[usize, usize]`. |
| `bmEmptyResult` | `(): BalancedResult` | Invokes `bmEmptyResult` and returns `BalancedResult`. |
| `balancedRange` | `(a: string, b: string, input: string): [usize, usize]?` | Invokes `balancedRange` and returns `[usize, usize]?`. |
| `balanced` | `(a: string, b: string, input: string): BalancedResult` | Invokes `balanced` and returns `BalancedResult`. |
| `balancedAll` | `(a: string, b: string, input: string, out: []BalancedResult): usize` | Invokes `balancedAll` and returns `usize`. |

## Minimal Example

```cp
import { balancedRange } from "balanced-match";

pub fn main() {
    let result = balancedRange("example", "example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
