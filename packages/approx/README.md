[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# approx

Zero-heap IEEE-754 floating-point approximate equality for `f64` and `f32`: absolute difference (`absDiffEq`), relative tolerance (`relativeEq`), ULPs distance (`ulpsEq`), default-tolerance helpers, and slice comparisons. Clean-room port of Rust `approx`.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `approx` (`brendanzab/approx`) |
| **CP Package** | `approx` (`v1.0.0`) |
| **License** | MIT (upstream Apache-2.0) |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: approx)
use approx::{abs_diff_eq, relative_eq, ulps_eq};
assert!(relative_eq!(1.0_f64, 1.0 + 1e-12, epsilon = 1e-12, max_relative = 1e-12));
```

```typescript
// CP (same package name: "approx")
import { absDiffEq, relativeEq, ulpsEq } from "approx";
let ok = relativeEq(1.0, 1.0 + 1e-12, 1e-12, 1e-12);
```

## Installation

```bash
cap add approx
```

## Quick Start & Practical Examples

### 1. Absolute, Relative & ULPs Comparisons (`f64` and `f32`)

```typescript
import {
    F64_EPSILON,
    F32_EPSILON,
    DEFAULT_MAX_ULPS_F64,
    absDiffEq,
    absDiffNe,
    absDiffEqF32,
    relativeEq,
    relativeEqF32,
    ulpsEq,
    ulpsDistanceF64,
    ulpsEqF32,
    ulpsDistanceF32,
} from "approx";

fn compareScalars(): void {
    // Absolute difference: |a - b| <= epsilon
    let a_ok = absDiffEq(0.1 + 0.2, 0.3, 1e-12);
    let a_ne = absDiffNe(1.0, 2.0, 1e-6);

    // Relative tolerance: |a - b| <= epsilon || |a - b| <= max_relative * max(|a|, |b|)
    let r_ok = relativeEq(1000.0, 1000.0001, F64_EPSILON, 1e-6);
    let r32_ok = relativeEqF32(10.0 as f32, 10.00001 as f32, F32_EPSILON, 1e-5 as f32);

    // ULPs comparison: IEEE-754 representable float distance
    let u_ok = ulpsEq(1.0, 1.0 + F64_EPSILON, F64_EPSILON, DEFAULT_MAX_ULPS_F64);
    if (let dist = ulpsDistanceF64(1.0, 1.0 + F64_EPSILON)) {
        // dist == 1
    }
    let dist32 = ulpsDistanceF32(1.0 as f32, (1.0 as f32) + F32_EPSILON);
    let u32_ok = ulpsEqF32(1.0 as f32, 1.0 as f32, F32_EPSILON, 4);
}
```

### 2. Default-Epsilon Shorthands & NaN/Inf Guards

```typescript
import {
    absDiffEqDefaultF64,
    absDiffEqDefaultF32,
    relativeEqDefaultF64,
    relativeEqDefaultF32,
    ulpsEqDefaultF64,
    ulpsEqDefaultF32,
    apxAbsF64,
    apxIsNanF64,
    apxIsInfF64,
} from "approx";

fn checkDefaults(x: f64, y: f64): bool {
    if (apxIsNanF64(x) || apxIsInfF64(x)) {
        return false;
    }
    let mag = apxAbsF64(x);
    return absDiffEqDefaultF64(x, y) || relativeEqDefaultF64(x, y) || ulpsEqDefaultF64(x, y);
}
```

### 3. Element-Wise Slice Comparisons

```typescript
import {
    absDiffEqSliceF64,
    relativeEqSliceF64,
    ulpsEqSliceF64,
    absDiffEqSliceF32,
} from "approx";

fn compareVectors(): bool {
    let a: [3]f64 = [1.0, 2.0, 3.0];
    let b: [3]f64 = [1.0 + 1e-13, 2.0 - 1e-13, 3.0];
    return absDiffEqSliceF64(a[0..3], b[0..3], 1e-12)
        && relativeEqSliceF64(a[0..3], b[0..3], 1e-12, 1e-12)
        && ulpsEqSliceF64(a[0..3], b[0..3], 1e-12, 4);
}
```

## API Reference

### Constants
- `F64_EPSILON: f64` (`2.220446049250313e-16`), `F32_EPSILON: f32` (`1.1920929e-7`)
- `DEFAULT_MAX_ULPS_F64: u64` (`4`), `DEFAULT_MAX_ULPS_F32: u32` (`4`)

### Primitives
- `apxAbsF64(x: f64): f64`, `apxAbsF32(x: f32): f32`
- `apxIsNanF64(x: f64): bool`, `apxIsNanF32(x: f32): bool`
- `apxIsInfF64(x: f64): bool`, `apxIsInfF32(x: f32): bool`

### Scalar Comparisons (`f64` aliases omit `F64` suffix)
- `absDiffEq` / `absDiffEqF64`, `absDiffNe` / `absDiffNeF64`, `absDiffEqF32`, `absDiffNeF32`
- `relativeEq` / `relativeEqF64`, `relativeNe` / `relativeNeF64`, `relativeEqF32`, `relativeNeF32`
- `ulpsDistanceF64(a: f64, b: f64): u64?`, `ulpsDistanceF32(a: f32, b: f32): u32?`
- `ulpsEq` / `ulpsEqF64`, `ulpsNe` / `ulpsNeF64`, `ulpsEqF32`, `ulpsNeF32`
- Default helpers: `absDiffEqDefaultF64`, `absDiffEqDefaultF32`, `relativeEqDefaultF64`, `relativeEqDefaultF32`, `ulpsEqDefaultF64`, `ulpsEqDefaultF32`

### Slice Comparisons
- `absDiffEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64): bool`, `absDiffEqSliceF32`
- `relativeEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64, max_relative: f64): bool`, `relativeEqSliceF32`
- `ulpsEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64, max_ulps: u64): bool`, `ulpsEqSliceF32`
