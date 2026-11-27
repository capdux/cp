[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# num-complex

Zero-allocation Complex64 arithmetic, Smith overflow-safe division, polar coordinates, Euler exp, principal sqrt, and formatting (Rust num-complex parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-complex` ([https://github.com/rust-num/num-complex](https://github.com/rust-num/num-complex))

| Metadata | Value |
| --- | --- |
| Package | `num-complex` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add num-complex
```

```cp
import {
    Complex64,
    fabs,
    pushByte,
    pushU64,
    pushAbsFloat,
    formatComplex,
} from "num-complex";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Complex64` | `class` | Fields: `re: f64, im: f64` · Methods: `new, zero, one, i, fromPolar, add, +14 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fabs` | `(x: f64): f64` | Invokes `fabs` and returns `f64`. |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `pushU64` | `(out: []u8, pos: usize, val: u64): usize` | Mutates or appends state via `pushU64`. |
| `pushAbsFloat` | `(out: []u8, pos: usize, v: f64): usize` | Mutates or appends state via `pushAbsFloat`. |
| `formatComplex` | `(z: Complex64, out: []u8): string` | Serializes/formats data via `formatComplex` returning `string`. |

## Minimal Example

```cp
import { formatComplex } from "num-complex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatComplex(/* z: Complex64 */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
