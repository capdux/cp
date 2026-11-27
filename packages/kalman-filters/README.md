[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kalman-filters

Zero-allocation 1D scalar and 2D constant-velocity Kalman filters with Joseph-form covariance update (Rust kalman-filters / kfilter parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kalman-filters` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `kalman-filters` → CP 包名 `kalman-filters`）

| Metadata | Value |
| --- | --- |
| Package | `kalman-filters` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add kalman-filters
```

```cp
import { Kalman1D, Kalman2D } from "kalman-filters";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Kalman1D` | `class` | Fields: `x: f64, p: f64, q: f64, r: f64, k: f64` · Methods: `init, predict, update, step, estimate, variance, +2 more` |
| `Kalman2D` | `class` | Fields: `pos: f64, vel: f64, p00: f64, p01: f64, p10: f64, p11: f64, +3 more` · Methods: `init, predict, updatePosition, position, velocity` |

## Minimal Example

```cp
import { Kalman1D } from "kalman-filters";

pub fn main() {
    // Refer to src/index.d.cp for Kalman1D fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
