[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# complex.js

Zero-allocation complex number arithmetic, polar form, exponential, integer powers, and principal square root for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:complex.js` ([https://github.com/rawify/Complex.js](https://github.com/rawify/Complex.js))（原库名 `complex.js` → CP 包名 `complex.js`）

| Metadata | Value |
| --- | --- |
| Package | `complex.js` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add complex.js
```

```cp
import {
    Complex,
    absF64,
    sqrtF64,
    reduceAngle,
    sinF64,
    cosF64,
    expF64,
    atanSmall,
} from "complex.js";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Complex` | `class` | Fields: `re: f64, im: f64` · Methods: `new, zero, one, iUnit, fromPolar, add, +12 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | Invokes `absF64` and returns `f64`. |
| `sqrtF64` | `(x: f64): f64` | Invokes `sqrtF64` and returns `f64`. |
| `reduceAngle` | `(rad: f64): f64` | Invokes `reduceAngle` and returns `f64`. |
| `sinF64` | `(rad: f64): f64` | Invokes `sinF64` and returns `f64`. |
| `cosF64` | `(rad: f64): f64` | Invokes `cosF64` and returns `f64`. |
| `expF64` | `(x: f64): f64` | Invokes `expF64` and returns `f64`. |
| `atanSmall` | `(x: f64): f64` | Invokes `atanSmall` and returns `f64`. |
| `atanF64` | `(x: f64): f64` | Invokes `atanF64` and returns `f64`. |
| `atan2F64` | `(y: f64, x: f64): f64` | Invokes `atan2F64` and returns `f64`. |
| `new` | `(re: f64, im: f64): Complex` | Constructs/initializes an instance via `new`. |
| `zero` | `(): Complex` | Invokes `zero` and returns `Complex`. |
| `one` | `(): Complex` | Invokes `one` and returns `Complex`. |
| `iUnit` | `(): Complex` | Invokes `iUnit` and returns `Complex`. |
| `fromPolar` | `(r: f64, theta: f64): Complex` | Invokes `fromPolar` and returns `Complex`. |
| `add` | `(a: Complex, b: Complex): Complex` | Mutates or appends state via `add`. |
| `sub` | `(a: Complex, b: Complex): Complex` | Invokes `sub` and returns `Complex`. |
| `mul` | `(a: Complex, b: Complex): Complex` | Invokes `mul` and returns `Complex`. |
| `div` | `(a: Complex, b: Complex): Complex` | Invokes `div` and returns `Complex`. |
| `conj` | `(z: Complex): Complex` | Invokes `conj` and returns `Complex`. |
| `neg` | `(z: Complex): Complex` | Invokes `neg` and returns `Complex`. |
| `scale` | `(z: Complex, s: f64): Complex` | Invokes `scale` and returns `Complex`. |
| `normSq` | `(z: Complex): f64` | Invokes `normSq` and returns `f64`. |
| `norm` | `(z: Complex): f64` | Invokes `norm` and returns `f64`. |
| `arg` | `(z: Complex): f64` | Invokes `arg` and returns `f64`. |
| `exp` | `(z: Complex): Complex` | Invokes `exp` and returns `Complex`. |
| `powInt` | `(z: Complex, n: i32): Complex` | Invokes `powInt` and returns `Complex`. |
| `sqrt` | `(z: Complex): Complex` | Invokes `sqrt` and returns `Complex`. |
| `approxEq` | `(a: Complex, b: Complex, eps: f64): bool` | Invokes `approxEq` and returns `bool`. |

## Minimal Example

```cp
import { absF64 } from "complex.js";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
