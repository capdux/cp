[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bn.js

Zero-allocation 256-bit unsigned big integer arithmetic (4x u64 limbs) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bn.js` ([https://github.com/indutny/bn.js](https://github.com/indutny/bn.js)) / `crates.io:primitive-types` ([https://github.com/paritytech/parity-common](https://github.com/paritytech/parity-common)) / `crates.io:ruint` ([https://github.com/recmo/uint](https://github.com/recmo/uint))（原库名 `bn.js` → CP 包名 `bn.js`）

| Metadata | Value |
| --- | --- |
| Package | `bn.js` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bn.js
```

```cp
import {
    Add64Res,
    Sub64Res,
    Mul64Res,
    DivRem256,
    U256,
    addWithCarry64,
    subWithBorrow64,
    mulWide64,
} from "bn.js";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Add64Res` | `class` | Fields: `sum: u64, carry: u64` |
| `Sub64Res` | `class` | Fields: `diff: u64, borrow: u64` |
| `Mul64Res` | `class` | Fields: `lo: u64, hi: u64` |
| `DivRem256` | `class` | Fields: `quot: U256, rem: U256` |
| `U256` | `class` | Fields: `limbs: [4]u64` · Methods: `zero, one, fromU64, fromU128, maxVal, fromHex, +15 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `addWithCarry64` | `(a: u64, b: u64, carry_in: u64): Add64Res` | Mutates or appends state via `addWithCarry64`. |
| `subWithBorrow64` | `(a: u64, b: u64, borrow_in: u64): Sub64Res` | Invokes `subWithBorrow64` and returns `Sub64Res`. |
| `mulWide64` | `(a: u64, b: u64): Mul64Res` | Invokes `mulWide64` and returns `Mul64Res`. |
| `clz64` | `(x: u64): u32` | Invokes `clz64` and returns `u32`. |
| `zero` | `(): U256` | Invokes `zero` and returns `U256`. |
| `one` | `(): U256` | Invokes `one` and returns `U256`. |
| `fromU64` | `(v: u64): U256` | Invokes `fromU64` and returns `U256`. |
| `fromU128` | `(lo: u64, hi: u64): U256` | Invokes `fromU128` and returns `U256`. |
| `maxVal` | `(): U256` | Invokes `maxVal` and returns `U256`. |
| `fromHex` | `(s: string): U256` | Invokes `fromHex` and returns `U256`. |

## Minimal Example

```cp
import { fromHex } from "bn.js";

pub fn main() {
    let result = fromHex("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
