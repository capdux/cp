# approx — AI Agent Quick Ref

Zero-heap IEEE-754 floating-point approximate equality (`abs_diff_eq`, `relative_eq`, `ulps_eq`) for `f64`, `f32`, and slices.

## Import
```typescript
import {
    F64_EPSILON, F32_EPSILON, DEFAULT_MAX_ULPS_F64, DEFAULT_MAX_ULPS_F32,
    absDiffEq, absDiffNe, absDiffEqF64, absDiffEqF32,
    relativeEq, relativeNe, relativeEqF64, relativeEqF32,
    ulpsEq, ulpsNe, ulpsEqF64, ulpsEqF32, ulpsDistanceF64, ulpsDistanceF32,
    absDiffEqDefaultF64, relativeEqDefaultF64, ulpsEqDefaultF64,
    absDiffEqSliceF64, relativeEqSliceF64, ulpsEqSliceF64
} from "approx";
```

## API Signatures
```typescript
export const F64_EPSILON: f64 = 2.220446049250313e-16;
export const F32_EPSILON: f32 = 1.1920929e-7;
export const DEFAULT_MAX_ULPS_F64: u64 = 4;
export const DEFAULT_MAX_ULPS_F32: u32 = 4;

export fn absDiffEq(a: f64, b: f64, epsilon: f64): bool;        // alias of absDiffEqF64 (+ absDiffNe / F32)
export fn absDiffEqF32(a: f32, b: f32, epsilon: f32): bool;
export fn relativeEq(a: f64, b: f64, epsilon: f64, max_relative: f64): bool; // + relativeNe / F64 / F32
export fn relativeEqF32(a: f32, b: f32, epsilon: f32, max_relative: f32): bool;
export fn ulpsDistanceF64(a: f64, b: f64): u64?;
export fn ulpsDistanceF32(a: f32, b: f32): u32?;
export fn ulpsEq(a: f64, b: f64, epsilon: f64, max_ulps: u64): bool;         // + ulpsNe / F64 / F32
export fn ulpsEqF32(a: f32, b: f32, epsilon: f32, max_ulps: u32): bool;

export fn absDiffEqDefaultF64(a: f64, b: f64): bool;            // + absDiffEqDefaultF32
export fn relativeEqDefaultF64(a: f64, b: f64): bool;           // + relativeEqDefaultF32
export fn ulpsEqDefaultF64(a: f64, b: f64): bool;               // + ulpsEqDefaultF32

export fn absDiffEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64): bool;       // + F32
export fn relativeEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64, max_relative: f64): bool; // + F32
export fn ulpsEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64, max_ulps: u64): bool;         // + F32
```

## Minimal Example
```typescript
import { absDiffEq, relativeEqSliceF64, F64_EPSILON } from "approx";

fn testFloats(): bool {
    let a = [1.0, 2.0];
    let b = [1.0 + 1.0e-12, 2.0];
    return absDiffEq(0.1 + 0.2, 0.3, 1.0e-12) && relativeEqSliceF64(&a, &b, F64_EPSILON, 1.0e-9);
}
```

## Pitfalls & Rules
- Unsuffixed `absDiffEq`, `relativeEq`, `ulpsEq` (and `*Ne`) operate on `f64`; use `*F32` for `f32`.
- `NaN` always compares `false` in `*Eq` and returns `null` from `ulpsDistance*`.
- `ulpsDistance*` returns `null` for opposite-sign non-zero values (pass a non-zero `epsilon` to `ulpsEq` for near-zero comparisons).
- Slice helpers take read-only `&[]f64` / `&[]f32` and return `false` immediately if `a.len != b.len`.
