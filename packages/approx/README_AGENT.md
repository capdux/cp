# approx — AI Agent Quick Ref

- **Import**: `import { F64_EPSILON, F32_EPSILON, DEFAULT_MAX_ULPS_F64, DEFAULT_MAX_ULPS_F32, absDiffEq, absDiffNe, relativeEq, relativeNe, ulpsEq, ulpsNe, ulpsDistanceF64, ulpsDistanceF32, absDiffEqSliceF64, relativeEqSliceF64, ulpsEqSliceF64 } from "approx";`
- **Source**: crates.io `approx` (`v1.0.0`, zero-heap)

## Constants
```cp
F64_EPSILON: f64 = 2.220446049250313e-16
F32_EPSILON: f32 = 1.1920929e-7
DEFAULT_MAX_ULPS_F64: u64 = 4
DEFAULT_MAX_ULPS_F32: u32 = 4
```

## Core Signatures
```cp
// Helpers
apxAbsF64(x: f64): f64;            apxAbsF32(x: f32): f32;
apxIsNanF64(x: f64): bool;         apxIsNanF32(x: f32): bool;
apxIsInfF64(x: f64): bool;         apxIsInfF32(x: f32): bool;

// Absolute difference (f64 aliases: absDiffEq, absDiffNe)
absDiffEqF64(a: f64, b: f64, epsilon: f64): bool;
absDiffNeF64(a: f64, b: f64, epsilon: f64): bool;
absDiffEqF32(a: f32, b: f32, epsilon: f32): bool;
absDiffNeF32(a: f32, b: f32, epsilon: f32): bool;

// Relative difference (f64 aliases: relativeEq, relativeNe)
relativeEqF64(a: f64, b: f64, epsilon: f64, max_relative: f64): bool;
relativeNeF64(a: f64, b: f64, epsilon: f64, max_relative: f64): bool;
relativeEqF32(a: f32, b: f32, epsilon: f32, max_relative: f32): bool;
relativeNeF32(a: f32, b: f32, epsilon: f32, max_relative: f32): bool;

// ULPs distance & equality (f64 aliases: ulpsEq, ulpsNe)
ulpsDistanceF64(a: f64, b: f64): u64?;
ulpsDistanceF32(a: f32, b: f32): u32?;
ulpsEqF64(a: f64, b: f64, epsilon: f64, max_ulps: u64): bool;
ulpsNeF64(a: f64, b: f64, epsilon: f64, max_ulps: u64): bool;
ulpsEqF32(a: f32, b: f32, epsilon: f32, max_ulps: u32): bool;
ulpsNeF32(a: f32, b: f32, epsilon: f32, max_ulps: u32): bool;

// Default-tolerance shorthands
absDiffEqDefaultF64(a: f64, b: f64): bool;   absDiffEqDefaultF32(a: f32, b: f32): bool;
relativeEqDefaultF64(a: f64, b: f64): bool;  relativeEqDefaultF32(a: f32, b: f32): bool;
ulpsEqDefaultF64(a: f64, b: f64): bool;      ulpsEqDefaultF32(a: f32, b: f32): bool;

// Slice comparisons
absDiffEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64): bool;
absDiffEqSliceF32(a: &[]f32, b: &[]f32, epsilon: f32): bool;
relativeEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64, max_relative: f64): bool;
relativeEqSliceF32(a: &[]f32, b: &[]f32, epsilon: f32, max_relative: f32): bool;
ulpsEqSliceF64(a: &[]f64, b: &[]f64, epsilon: f64, max_ulps: u64): bool;
ulpsEqSliceF32(a: &[]f32, b: &[]f32, epsilon: f32, max_ulps: u32): bool;
```

## Pitfalls
- `ulpsDistanceF64`/`F32` returns `null` if either operand is NaN, Inf, or has opposite sign.
- Near zero, `ulpsEq` relies on `epsilon` first (`|a - b| <= epsilon`) because ULP steps across `±0.0` are huge.
- Slice comparisons return `false` immediately when `a.len != b.len`.
