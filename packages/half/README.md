[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# half

Zero-heap IEEE 754 binary16 (f16) and Google Brain Float 16 (bf16) codec and arithmetic in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:half` ([https://github.com/VoidStarKat/half-rs](https://github.com/VoidStarKat/half-rs)) / `npm:@petamoriken/float16` ([https://github.com/petamoriken/float16](https://github.com/petamoriken/float16))（原库名 `half` → CP 包名 `half`）

| Metadata | Value |
| --- | --- |
| Package | `half` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add half
```

```cp
import {
    F16,
    Bf16,
    f32ToBits,
    bitsToF32,
    f32SliceToF16,
    f16SliceToF32,
} from "half";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `F16` | `class` | Fields: `bits: u16` · Methods: `fromBits, fromLeBytes, fromF32, toF32, isNan, isInfinite, +4 more` |
| `Bf16` | `class` | Fields: `bits: u16` · Methods: `fromBits, fromF32, toF32, isNan, isInfinite, neg, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `f32ToBits` | `(x: f32): u32` | Invokes `f32ToBits` and returns `u32`. |
| `bitsToF32` | `(b: u32): f32` | Invokes `bitsToF32` and returns `f32`. |
| `f32SliceToF16` | `(src: &[]f32, dst: []u16): usize` | Invokes `f32SliceToF16` and returns `usize`. |
| `f16SliceToF32` | `(src: &[]u16, dst: []f32): usize` | Invokes `f16SliceToF32` and returns `usize`. |

## Minimal Example

```cp
import { f32ToBits } from "half";

pub fn main() {
    let result = f32ToBits(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
