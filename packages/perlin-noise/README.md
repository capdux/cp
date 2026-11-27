[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# perlin-noise

Zero-heap Ken Perlin improved 1D/2D/3D gradient noise, fBm, and ridge noise in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:perlin-noise` ([https://github.com/andrewrk/node-perlin-noise](https://github.com/andrewrk/node-perlin-noise))

| Metadata | Value |
| --- | --- |
| Package | `perlin-noise` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add perlin-noise
```

```cp
import {
    floorF64,
    permAt,
    lerpF64,
    grad1D,
    grad2D,
    grad3D,
    fade,
    noise1D,
} from "perlin-noise";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `floorF64` | `(x: f64): i64` | Invokes `floorF64` and returns `i64`. |
| `permAt` | `(idx: usize, seed: u32): usize` | Invokes `permAt` and returns `usize`. |
| `lerpF64` | `(a: f64, b: f64, t: f64): f64` | Invokes `lerpF64` and returns `f64`. |
| `grad1D` | `(hash: usize, x: f64): f64` | Invokes `grad1D` and returns `f64`. |
| `grad2D` | `(hash: usize, x: f64, y: f64): f64` | Invokes `grad2D` and returns `f64`. |
| `grad3D` | `(hash: usize, x: f64, y: f64, z: f64): f64` | Invokes `grad3D` and returns `f64`. |
| `fade` | `(t: f64): f64` | Invokes `fade` and returns `f64`. |
| `noise1D` | `(x: f64, seed: u32): f64` | Invokes `noise1D` and returns `f64`. |
| `noise2D` | `(x: f64, y: f64, seed: u32): f64` | Invokes `noise2D` and returns `f64`. |
| `noise3D` | `(x: f64, y: f64, z: f64, seed: u32): f64` | Invokes `noise3D` and returns `f64`. |
| `fbm2D` | `(x: f64, y: f64, seed: u32, octaves: u32, lacunarity: f64, gain: f64): f64` | Invokes `fbm2D` and returns `f64`. |
| `ridge2D` | `(x: f64, y: f64, seed: u32, octaves: u32): f64` | Invokes `ridge2D` and returns `f64`. |

## Minimal Example

```cp
import { floorF64 } from "perlin-noise";

pub fn main() {
    let result = floorF64(0.0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
