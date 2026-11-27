[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# fixed

Zero-allocation signed 64-bit Q32.32 deterministic fixed-point arithmetic engine with overflow-free 128-bit split mul/div, Newton sqrt, lerp, and clamp (Rust fixed parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fixed` ([https://gitlab.com/tspiteri/fixed](https://gitlab.com/tspiteri/fixed))（原库名 `fixed` → CP 包名 `fixed`）

| Metadata | Value |
| --- | --- |
| Package | `fixed` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add fixed
```

```cp
import { Fixed32, absU64, divShift32U64 } from "fixed";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Fixed32` | `class` | Fields: `raw: i64` · Methods: `fromInt, fromRaw, fromRatio, fromF64, rawValue, toF64, +14 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absU64` | `(v: i64): u64` | Invokes `absU64` and returns `u64`. |
| `divShift32U64` | `(a: u64, b: u64): u64` | Invokes `divShift32U64` and returns `u64`. |

## Minimal Example

```cp
import { absU64 } from "fixed";

pub fn main() {
    let result = absU64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
