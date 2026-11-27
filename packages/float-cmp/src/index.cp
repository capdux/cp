// @cap/float-cmp — Zero-heap IEEE-754 floating-point approximate comparison & ordering.
// Clean-room MIT implementation modeled on crates.io `float-cmp` v0.10, reusing `@cap/approx`.

import {
    F64_EPSILON,
    F32_EPSILON,
    absDiffEqF64,
    absDiffEqF32,
    relativeEqF64,
    ulpsDistanceF64,
    ulpsEqF64,
    ulpsDistanceF32,
    ulpsEqF32
} from "../../approx/src/index.cp";

fn fcmpIsNanF64(x: f64): bool {
    return x != x;
}

fn fcmpIsNanF32(x: f32): bool {
    return x != x;
}

/// Dual-threshold tolerance margin for `f64` approximate comparisons.
pub struct F64Margin {
    pub epsilon: f64,
    pub ulps: u64,

    pub static fn defaultMargin(): F64Margin {
        return F64Margin { epsilon: F64_EPSILON, ulps: 4 };
    }

    pub static fn zero(): F64Margin {
        return F64Margin { epsilon: 0.0, ulps: 0 };
    }

    pub fn withEpsilon(eps: f64): F64Margin {
        return F64Margin { epsilon: eps, ulps: _.ulps };
    }

    pub fn withUlps(u: u64): F64Margin {
        return F64Margin { epsilon: _.epsilon, ulps: u };
    }
}

/// Dual-threshold tolerance margin for `f32` approximate comparisons.
pub struct F32Margin {
    pub epsilon: f32,
    pub ulps: u32,

    pub static fn defaultMargin(): F32Margin {
        return F32Margin { epsilon: F32_EPSILON, ulps: 4 };
    }

    pub static fn zero(): F32Margin {
        return F32Margin { epsilon: 0.0, ulps: 0 };
    }

    pub fn withEpsilon(eps: f32): F32Margin {
        return F32Margin { epsilon: eps, ulps: _.ulps };
    }

    pub fn withUlps(u: u32): F32Margin {
        return F32Margin { epsilon: _.epsilon, ulps: u };
    }
}

/// Approximate equality for `f64` using dual epsilon and ULPs thresholds:
/// 1. Returns `false` if either operand is `NaN`.
/// 2. Returns `true` if `a == b` (including `-0.0 == +0.0`, `+Inf == +Inf`, `-Inf == -Inf`).
/// 3. Returns `true` if `|a - b| <= margin.epsilon` (via `@cap/approx` `absDiffEqF64`).
/// 4. Returns `true` if same-sign IEEE-754 ULP distance `<= margin.ulps` (via `@cap/approx` `ulpsEqF64`).
pub fn approxEqF64(a: f64, b: f64, margin: F64Margin): bool {
    if (fcmpIsNanF64(a) || fcmpIsNanF64(b)) { return false; }
    if (a == b) { return true; }
    if (absDiffEqF64(a, b, margin.epsilon)) { return true; }
    let d = ulpsDistanceF64(a, b);
    if (d == null) { return false; }
    return ulpsEqF64(a, b, margin.epsilon, margin.ulps);
}

/// Approximate equality for `f32` using dual epsilon and ULPs thresholds.
pub fn approxEqF32(a: f32, b: f32, margin: F32Margin): bool {
    if (fcmpIsNanF32(a) || fcmpIsNanF32(b)) { return false; }
    if (a == b) { return true; }
    if (absDiffEqF32(a, b, margin.epsilon)) { return true; }
    let d = ulpsDistanceF32(a, b);
    if (d == null) { return false; }
    return ulpsEqF32(a, b, margin.epsilon, margin.ulps);
}

/// Relative ratio approximate equality for `f64`:
/// Returns `true` if `|a - b| <= max(|a|, |b|) * ratio` (delegating to `@cap/approx` `relativeEqF64`).
pub fn approxEqRatioF64(a: f64, b: f64, ratio: f64): bool {
    if (fcmpIsNanF64(ratio) || ratio < 0.0) { return false; }
    return relativeEqF64(a, b, 0.0, ratio);
}

/// Approximate total ordering for `f64`, returning `-1` (`a < b`), `0` (`a ~= b`), or `1` (`a > b`).
/// Values within `margin` compare as `0`; `NaN` sorts after all non-`NaN` values (`NaN == NaN -> 0`).
pub fn approxOrdF64(a: f64, b: f64, margin: F64Margin): i32 {
    let a_nan: bool = fcmpIsNanF64(a);
    let b_nan: bool = fcmpIsNanF64(b);
    if (a_nan && b_nan) { return 0; }
    if (a_nan) { return 1; }
    if (b_nan) { return -1; }
    if (approxEqF64(a, b, margin)) { return 0; }
    if (a < b) { return -1; }
    return 1;
}
