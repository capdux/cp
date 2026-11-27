[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# noise

Zero-heap 2D/3D Improved Perlin Noise, Value Noise, and Fractal Brownian Motion (fBm) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:noise` ([https://github.com/Razaekel/noise-rs](https://github.com/Razaekel/noise-rs))

| Metadata | Value |
| --- | --- |
| Package | `noise` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add noise
```

```cp
import {
    PerlinNoise,
    floorInt,
    fade,
    lerp,
    grad2,
    grad3,
    splitmix64,
} from "noise";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PerlinNoise` | `class` | Fields: `perm: [512]u8` · Methods: `init, noise2D, noise3D, fbm2D, ridged2D` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `floorInt` | `(x: f64): i64` | Invokes `floorInt` and returns `i64`. |
| `fade` | `(t: f64): f64` | Invokes `fade` and returns `f64`. |
| `lerp` | `(a: f64, b: f64, t: f64): f64` | Invokes `lerp` and returns `f64`. |
| `grad2` | `(hash: u8, x: f64, y: f64): f64` | Invokes `grad2` and returns `f64`. |
| `grad3` | `(hash: u8, x: f64, y: f64, z: f64): f64` | Invokes `grad3` and returns `f64`. |
| `splitmix64` | `(state: &mut u64): u64` | Invokes `splitmix64` and returns `u64`. |

## Minimal Example

```cp
import { floorInt } from "noise";

pub fn main() {
    let result = floorInt(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
