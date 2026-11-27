[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pcg-random

Zero-heap Melissa O'Neill PCG-XSH-RR 32-bit deterministic PRNG with stream separation, unbiased bounded sampling, O(log n) advance, and byte filling.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pcg-random` ([https://github.com/ThomWright/pcg-random](https://github.com/ThomWright/pcg-random))（原库名 `pcg-random` → CP 包名 `pcg-random`）

| Metadata | Value |
| --- | --- |
| Package | `pcg-random` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pcg-random
```

```cp
import {
    Pcg32,
    pcgMult,
    defaultStream,
    add64,
    mul64,
    rotr32,
} from "pcg-random";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Pcg32` | `class` | Fields: `state: u64, inc: u64` · Methods: `new, fromSeed, nextU32, nextU64, nextF64, nextBounded, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pcgMult` | `(): u64` | Invokes `pcgMult` and returns `u64`. |
| `defaultStream` | `(): u64` | Invokes `defaultStream` and returns `u64`. |
| `add64` | `(a: u64, b: u64): u64` | Mutates or appends state via `add64`. |
| `mul64` | `(a: u64, b: u64): u64` | Invokes `mul64` and returns `u64`. |
| `rotr32` | `(x: u32, rot: u32): u32` | Invokes `rotr32` and returns `u32`. |

## Minimal Example

```cp
import { pcgMult } from "pcg-random";

pub fn main() {
    let result = pcgMult();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
