[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# approx

Zero-heap IEEE-754 floating-point approximate equality (`abs_diff_eq`, `relative_eq`, `ulps_eq`) for `f64`, `f32`, and slices in pure CP.

- **Upstream Alignment**: Clean-room port of Rust [`approx`](https://github.com/brendanzab/approx) (`crates.io`, `Apache-2.0`).
- **Package Version**: `1.0.0` (`MIT`).

## Installation & Import

```bash
cap add approx
```

In `cap.json`:

```json
{
  "dependencies": {
    "approx": "^1.0.0"
  }
}
```

```typescript
import {
    F64_EPSILON, F32_EPSILON, DEFAULT_MAX_ULPS_F64, DEFAULT_MAX_ULPS_F32,
    absDiffEq, absDiffNe, absDiffEqF64, absDiffNeF64, absDiffEqF32, absDiffNeF32,
    relativeEq, relativeNe, relativeEqF64, relativeNeF64, relativeEqF32, relativeNeF32,
    ulpsEq, ulpsNe, ulpsEqF64, ulpsNeF64, ulpsEqF32, ulpsNeF32,
    ulpsDistanceF64, ulpsDistanceF32,
    absDiffEqDefaultF64, absDiffEqDefaultF32,
    relativeEqDefaultF64, relativeEqDefaultF32,
    ulpsEqDefaultF64, ulpsEqDefaultF32,
    absDiffEqSliceF64, absDiffEqSliceF32,
    relativeEqSliceF64, relativeEqSliceF32,
    ulpsEqSliceF64, ulpsEqSliceF32
} from "approx";
```

## Core API Reference

### Constants

| Constant | Type | Value | Description |
| :--- | :--- | :--- | :--- |
| `F64_EPSILON` | `f64` | `2.220446049250313e-16` | Machine epsilon for IEEE-754 `f64` |
| `F32_EPSILON` | `f32` | `1.1920929e-7` | Machine epsilon for IEEE-754 `f32` |
| `DEFAULT_MAX_ULPS_F64` | `u64` | `4` | Default ULP tolerance for `ulpsEqDefaultF64` |
| `DEFAULT_MAX_ULPS_F32` | `u32` | `4` | Default ULP tolerance for `ulpsEqDefaultF32` |

### Scalar & Default Comparisons

| Family | Signatures | Description |
| :--- | :--- | :--- |
| Absolute diff (`f64` / `f32`) | `absDiffEq(a, b, epsilon)` / `absDiffEqF64` / `absDiffNeF64` / `absDiffEqF32` / `absDiffNeF32` | True iff `\|a - b\| <= epsilon` (exact infinities match; `NaN` always `false`) |
| Relative (`f64` / `f32`) | `relativeEq(a, b, epsilon, max_relative)` / `relativeEqF64` / `relativeNeF64` / `relativeEqF32` / `relativeNeF32` | Checks `absDiffEq` first, then `\|a - b\| <= max_relative * max(\|a\|, \|b\|)` |
| ULPs (`f64` / `f32`) | `ulpsEq(a, b, epsilon, max_ulps)` / `ulpsEqF64` / `ulpsNeF64` / `ulpsEqF32` / `ulpsNeF32` | Checks `absDiffEq` first, then IEEE-754 ULP distance `<= max_ulps` |
| ULP Distance | `ulpsDistanceF64(a: f64, b: f64): u64?` / `ulpsDistanceF32(a: f32, b: f32): u32?` | Exact ULP distance between same-sign finite floats, or `null` if `NaN`/Inf/opposite signs |
| Default Helpers | `absDiffEqDefaultF64`, `absDiffEqDefaultF32`, `relativeEqDefaultF64`, `relativeEqDefaultF32`, `ulpsEqDefaultF64`, `ulpsEqDefaultF32` | Convenience wrappers using `F64_EPSILON` / `F32_EPSILON` and `DEFAULT_MAX_ULPS_*` |

### Element-Wise Slice Comparisons

| Function | Signature | Description |
| :--- | :--- | :--- |
| `absDiffEqSliceF64` / `absDiffEqSliceF32` | `(a: &[]f64, b: &[]f64, epsilon: f64): bool` | Element-wise absolute difference equality (requires `a.len == b.len`) |
| `relativeEqSliceF64` / `relativeEqSliceF32` | `(a: &[]f64, b: &[]f64, epsilon: f64, max_relative: f64): bool` | Element-wise relative equality |
| `ulpsEqSliceF64` / `ulpsEqSliceF32` | `(a: &[]f64, b: &[]f64, epsilon: f64, max_ulps: u64): bool` | Element-wise ULP equality |

## Examples

### 1. Scalar Tolerance Checks (`f64` and `f32`)

```typescript
import { absDiffEq, relativeEq, ulpsEq, ulpsDistanceF64 } from "approx";

fn verifyMath(): bool {
    let ok1 = absDiffEq(0.1 + 0.2, 0.3, 1.0e-12);
    let ok2 = relativeEq(1000.0001, 1000.0002, 1.0e-9, 1.0e-6);
    let ok3 = ulpsEq(1.0, 1.0 + 2.220446049250313e-16, 0.0, 2);
    let dist = ulpsDistanceF64(1.0, 1.0) orelse 999;
    return ok1 && ok2 && ok3 && dist == 0;
}
```

### 2. Comparing Numerical Vectors with Slice Helpers

```typescript
import { relativeEqSliceF64, F64_EPSILON } from "approx";

fn checkVector(): bool {
    let expected = [1.0, 2.0, 3.0];
    let actual = [1.0 + 1.0e-12, 2.0 - 1.0e-12, 3.0 + 1.0e-12];
    return relativeEqSliceF64(&expected, &actual, F64_EPSILON, 1.0e-9);
}
```

## Differences from Upstream & CP Highlights

- **Explicit Typed Functions**: Instead of Rust trait macros (`abs_diff_eq!`), CP provides zero-overhead typed functions for `f64` (default unsuffixed aliases) and `f32`, plus slice variants (`&[]f64`, `&[]f32`).
- **Safe `ulpsDistance*` Optional Return**: Returns `u64?` / `u32?` so `NaN`, infinities, and opposite-sign comparisons are handled explicitly without overflow.
- **Zero-Heap Pure CP**: Pure bit-level IEEE-754 arithmetic with zero dependencies and zero allocations.
