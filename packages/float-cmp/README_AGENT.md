# float-cmp — AI Agent Quick Ref

- **Import**: `import { F64Margin, F32Margin, approxEqF64, approxNeF64, approxEqF32, approxNeF32, approxEqRatioF64, approxEqRatioF32, approxOrdF64, approxOrdF32, ulpsDiffF64, ulpsDiffF32, nextUp, nextDown, nextUpF64, nextDownF64, nextUpF32, nextDownF32, f64ToOrderedI64, f32ToOrderedI32, fcmpIsNanF64, fcmpIsNanF32, fcmpIsInfF64, fcmpIsInfF32 } from "float-cmp";`
- **Source**: crates.io `float-cmp` (`v1.0.0`, zero-heap, depends on `approx`)

## Core Signatures
```cp
class F64Margin {
    epsilon: f64, ulps: u64,
    static defaultMargin(): F64Margin;
    static zero(): F64Margin;
    static fromTuple(epsilon: f64, ulps: u64): F64Margin;
    static withEpsilon(eps: f64): F64Margin;
    static withUlps(u: u64): F64Margin;
}
class F32Margin {
    epsilon: f32, ulps: u32,
    static defaultMargin(): F32Margin;
    static zero(): F32Margin;
    static fromTuple(epsilon: f32, ulps: u32): F32Margin;
    static withEpsilon(eps: f32): F32Margin;
    static withUlps(u: u32): F32Margin;
}

approxEqF64(a: f64, b: f64, margin: F64Margin): bool;
approxNeF64(a: f64, b: f64, margin: F64Margin): bool;
approxEqF32(a: f32, b: f32, margin: F32Margin): bool;
approxNeF32(a: f32, b: f32, margin: F32Margin): bool;
approxEqRatioF64(a: f64, b: f64, ratio: f64): bool;
approxEqRatioF32(a: f32, b: f32, ratio: f32): bool;
approxOrdF64(a: f64, b: f64, margin: F64Margin): i32; // -1, 0, +1
approxOrdF32(a: f32, b: f32, margin: F32Margin): i32; // -1, 0, +1
ulpsDiffF64(a: f64, b: f64): i64?;
ulpsDiffF32(a: f32, b: f32): i32?;
nextUp(x: f64): f64;       nextDown(x: f64): f64;
nextUpF64(x: f64): f64;    nextDownF64(x: f64): f64;
nextUpF32(x: f32): f32;    nextDownF32(x: f32): f32;
f64ToOrderedI64(x: f64): i64;
f32ToOrderedI32(x: f32): i32;
fcmpIsNanF64(x: f64): bool; fcmpIsNanF32(x: f32): bool;
fcmpIsInfF64(x: f64): bool; fcmpIsInfF32(x: f32): bool;
```

## Pitfalls
- `ulpsDiffF64`/`F32` returns `null` when either input is NaN or Inf (signed difference `a - b` in ULPs otherwise).
- `approxEqF64` succeeds if **either** `|a - b| <= margin.epsilon` **or** ULP distance `<= margin.ulps`.
- `approxOrdF64`/`F32` returns `0` when approximately equal; guard against `NaN` with `fcmpIsNanF64` before sorting.
- Use `F64Margin.fromTuple(eps, ulps)` instead of literal struct construction if you want a clean portable call.
