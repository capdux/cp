# float-cmp (AI Agent Quick Ref)

Zero-heap IEEE-754 approximate equality, ULPs diff, nextUp/nextDown stepping, ratio & ordering (`f64`/`f32`).

## Import
```typescript
import {
    F64Margin, F32Margin,
    fcmpIsNanF64, fcmpIsNanF32, fcmpIsInfF64, fcmpIsInfF32,
    f64ToOrderedI64, f32ToOrderedI32,
    ulpsDiffF64, ulpsDiffF32,
    nextUpF64, nextDownF64, nextUp, nextDown, nextUpF32, nextDownF32,
    approxEqF64, approxNeF64, approxEqF32, approxNeF32,
    approxEqRatioF64, approxEqRatioF32, approxOrdF64, approxOrdF32,
} from "float-cmp";
```

## API Signatures
- `class F64Margin { epsilon: f64, ulps: u64 }` — static `defaultMargin()`, `zero()`, `fromTuple(f64, u64)`; methods `withEpsilon(f64): F64Margin`, `withUlps(u64): F64Margin`
- `class F32Margin { epsilon: f32, ulps: u32 }` — static `defaultMargin()`, `zero()`, `fromTuple(f32, u32)`; methods `withEpsilon(f32): F32Margin`, `withUlps(u32): F32Margin`
- `approxEqF64(a: f64, b: f64, margin: F64Margin): bool` / `approxNeF64(...)` / `approxEqF32(...)` / `approxNeF32(...)`
- `approxEqRatioF64(a: f64, b: f64, ratio: f64): bool` / `approxEqRatioF32(a: f32, b: f32, ratio: f32): bool`
- `approxOrdF64(a: f64, b: f64, margin: F64Margin): i32` / `approxOrdF32(...)` -> `-1 | 0 | 1`
- `ulpsDiffF64(a: f64, b: f64): i64?` / `ulpsDiffF32(a: f32, b: f32): i32?`
- `nextUpF64/nextDownF64/nextUp/nextDown(x: f64): f64`, `nextUpF32/nextDownF32(x: f32): f32`
- `fcmpIsNanF64/F32`, `fcmpIsInfF64/F32`, `f64ToOrderedI64`, `f32ToOrderedI32`

## Minimal Example
```typescript
import { F64Margin, approxEqF64, ulpsDiffF64 } from "float-cmp";

fn checkClose(a: f64, b: f64): bool {
    let m = F64Margin.defaultMargin().withUlps(4);
    return approxEqF64(a, b, m);
}
```

## Pitfalls & Borrowing Rules
- All arguments and return values are pure stack scalars/value structs; zero heap allocation, no borrow lifetimes to manage.
- `withEpsilon()` and `withUlps()` return a new `F64Margin`/`F32Margin` value (do not mutate in place).
- `ulpsDiffF64`/`ulpsDiffF32` return optional (`i64?`/`i32?`); always unwrap with `if (let d = ulpsDiffF64(a, b))` because NaN/Inf returns `null`.
- `approxEqF64` passes if EITHER `|a - b| <= margin.epsilon` OR ULP distance `<= margin.ulps`; use `F64Margin.zero().withUlps(n)` for ULPs-only checks.
- `approxOrdF64`/`approxOrdF32` return `i32` (`-1`, `0`, `1`), not an enum.
