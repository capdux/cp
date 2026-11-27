[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# libm

Pure CP zero-allocation IEEE 754 f64 math library (libm parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:libm` ([https://github.com/rust-lang/libm](https://github.com/rust-lang/libm))

| Metadata | Value |
| --- | --- |
| Package | `libm` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add libm
```

```cp
import {
    LmTrigRed,
    lmNan,
    lmInf,
    lmScalePow2,
    fabs,
    trunc,
    floor,
    ceil,
} from "libm";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LmTrigRed` | `class` | Fields: `r: f64, quad: i64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lmNan` | `(): f64` | Invokes `lmNan` and returns `f64`. |
| `lmInf` | `(): f64` | Invokes `lmInf` and returns `f64`. |
| `lmScalePow2` | `(val: f64, exp2: i64): f64` | Invokes `lmScalePow2` and returns `f64`. |
| `fabs` | `(x: f64): f64` | Invokes `fabs` and returns `f64`. |
| `trunc` | `(x: f64): f64` | Invokes `trunc` and returns `f64`. |
| `floor` | `(x: f64): f64` | Invokes `floor` and returns `f64`. |
| `ceil` | `(x: f64): f64` | Invokes `ceil` and returns `f64`. |
| `round` | `(x: f64): f64` | Invokes `round` and returns `f64`. |
| `fmod` | `(x: f64, y: f64): f64` | Invokes `fmod` and returns `f64`. |
| `copysign` | `(x: f64, y: f64): f64` | Invokes `copysign` and returns `f64`. |
| `fmin` | `(x: f64, y: f64): f64` | Invokes `fmin` and returns `f64`. |
| `fmax` | `(x: f64, y: f64): f64` | Invokes `fmax` and returns `f64`. |
| `sqrt` | `(x: f64): f64` | Invokes `sqrt` and returns `f64`. |
| `cbrt` | `(x: f64): f64` | Invokes `cbrt` and returns `f64`. |
| `hypot` | `(x: f64, y: f64): f64` | Invokes `hypot` and returns `f64`. |
| `exp` | `(x: f64): f64` | Invokes `exp` and returns `f64`. |
| `ln` | `(x: f64): f64` | Invokes `ln` and returns `f64`. |
| `log2` | `(x: f64): f64` | Invokes `log2` and returns `f64`. |
| `log10` | `(x: f64): f64` | Invokes `log10` and returns `f64`. |
| `pow` | `(x: f64, y: f64): f64` | Invokes `pow` and returns `f64`. |
| `lmSinCore` | `(r: f64): f64` | Invokes `lmSinCore` and returns `f64`. |
| `lmCosCore` | `(r: f64): f64` | Invokes `lmCosCore` and returns `f64`. |
| `lmReduceTrig` | `(x: f64): LmTrigRed` | Invokes `lmReduceTrig` and returns `LmTrigRed`. |
| `sin` | `(x: f64): f64` | Invokes `sin` and returns `f64`. |
| `cos` | `(x: f64): f64` | Invokes `cos` and returns `f64`. |
| `tan` | `(x: f64): f64` | Invokes `tan` and returns `f64`. |
| `lmAtanUnit` | `(u: f64): f64` | Invokes `lmAtanUnit` and returns `f64`. |
| `atan` | `(x: f64): f64` | Invokes `atan` and returns `f64`. |
| `atan2` | `(y: f64, x: f64): f64` | Invokes `atan2` and returns `f64`. |
| `asin` | `(x: f64): f64` | Invokes `asin` and returns `f64`. |
| `acos` | `(x: f64): f64` | Invokes `acos` and returns `f64`. |
| `sinh` | `(x: f64): f64` | Invokes `sinh` and returns `f64`. |
| `cosh` | `(x: f64): f64` | Invokes `cosh` and returns `f64`. |
| `tanh` | `(x: f64): f64` | Invokes `tanh` and returns `f64`. |

## Minimal Example

```cp
import { lmNan } from "libm";

pub fn main() {
    let result = lmNan();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
