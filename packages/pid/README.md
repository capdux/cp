[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pid

Zero-allocation industrial PID feedback controller with term limits, anti-windup, and derivative-on-measurement in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pid` ([https://github.com/braincore/pid-rs](https://github.com/braincore/pid-rs))

| Metadata | Value |
| --- | --- |
| Package | `pid` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pid
```

```cp
import { PidOutput, PidController, clampRange, clampSymmetric } from "pid";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PidOutput` | `class` | Fields: `output: f64, p: f64, i: f64, d: f64` |
| `PidController` | `class` | Fields: `setpoint: f64, kp: f64, ki: f64, kd: f64, p_limit: f64, i_limit: f64, +7 more` · Methods: `init, setLimits, setTermLimits, setSetpoint, setpointValue, integralValue, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `clampRange` | `(v: f64, lo: f64, hi: f64): f64` | Invokes `clampRange` and returns `f64`. |
| `clampSymmetric` | `(v: f64, lim: f64): f64` | Invokes `clampSymmetric` and returns `f64`. |

## Minimal Example

```cp
import { clampRange } from "pid";

pub fn main() {
    let result = clampRange(0.0, 0.0, 0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
