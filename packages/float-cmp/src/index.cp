// @cap/float-cmp — Zero-heap IEEE-754 floating-point approximate comparison, ULP diff, nextUp/nextDown & ordering.
// Clean-room MIT implementation modeled on crates.io `float-cmp` v0.10, reusing `@cap/approx`.

import {
    F64_EPSILON,
    F32_EPSILON,
    absDiffEqF64,
    absDiffEqF32,
    relativeEqF64,
    relativeEqF32,
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

fn fcmpIsInfF64(x: f64): bool {
    return !fcmpIsNanF64(x) && (x - x) != 0.0;
}

fn fcmpIsInfF32(x: f32): bool {
    return !fcmpIsNanF32(x) && (x - x) != 0.0;
}

fn f64ToOrderedI64(x: f64): i64 {
    let bits: u64 = @bitCast(x);
    let sign_mask: u64 = (1 as u64) << 63;
    if ((bits & sign_mask) != 0) {
        let mag: i64 = @bitCast(bits & (~sign_mask));
        return -mag;
    }
    let res: i64 = @bitCast(bits);
    return res;
}

fn f32ToOrderedI32(x: f32): i32 {
    let bits: u32 = @bitCast(x);
    let sign_mask: u32 = (1 as u32) << 31;
    if ((bits & sign_mask) != 0) {
        let mag: i32 = @bitCast(bits & (~sign_mask));
        return -mag;
    }
    let res: i32 = @bitCast(bits);
    return res;
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

    pub static fn fromTuple(epsilon: f64, ulps: u64): F64Margin {
        return F64Margin { epsilon: epsilon, ulps: ulps };
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

    pub static fn fromTuple(epsilon: f32, ulps: u32): F32Margin {
        return F32Margin { epsilon: epsilon, ulps: ulps };
    }

    pub fn withEpsilon(eps: f32): F32Margin {
        return F32Margin { epsilon: eps, ulps: _.ulps };
    }

    pub fn withUlps(u: u32): F32Margin {
        return F32Margin { epsilon: _.epsilon, ulps: u };
    }
}

/// Signed IEEE-754 ULP difference `a - b` for finite `f64` values (handles cross-zero cleanly).
/// Returns `null` if either operand is `NaN` or `Inf`.
pub fn ulpsDiffF64(a: f64, b: f64): i64? {
    if (fcmpIsNanF64(a) || fcmpIsNanF64(b) || fcmpIsInfF64(a) || fcmpIsInfF64(b)) {
        return null;
    }
    if (a == b) { return 0; }
    let oa: i64 = f64ToOrderedI64(a);
    let ob: i64 = f64ToOrderedI64(b);
    return oa -% ob;
}

/// Signed IEEE-754 ULP difference `a - b` for finite `f32` values (handles cross-zero cleanly).
/// Returns `null` if either operand is `NaN` or `Inf`.
pub fn ulpsDiffF32(a: f32, b: f32): i32? {
    if (fcmpIsNanF32(a) || fcmpIsNanF32(b) || fcmpIsInfF32(a) || fcmpIsInfF32(b)) {
        return null;
    }
    if (a == b) { return 0; }
    let oa: i32 = f32ToOrderedI32(a);
    let ob: i32 = f32ToOrderedI32(b);
    return oa -% ob;
}

/// Returns the least `f64` value strictly greater than `x` (IEEE-754 `nextUp`).
pub fn nextUpF64(x: f64): f64 {
    if (fcmpIsNanF64(x)) { return x; }
    let pos_inf_bits: u64 = 0x7FF0000000000000;
    let bits: u64 = @bitCast(x);
    if (bits == pos_inf_bits) { return x; }
    if (x == 0.0) {
        let min_pos: f64 = @bitCast(1 as u64);
        return min_pos;
    }
    let sign_mask: u64 = (1 as u64) << 63;
    let next_bits: u64 = if ((bits & sign_mask) == 0) { bits + 1 } else { bits - 1 };
    let res: f64 = @bitCast(next_bits);
    return res;
}

/// Returns the greatest `f64` value strictly less than `x` (IEEE-754 `nextDown`).
pub fn nextDownF64(x: f64): f64 {
    if (fcmpIsNanF64(x)) { return x; }
    let neg_inf_bits: u64 = (0xFFF00000 as u64) << 32;
    let bits: u64 = @bitCast(x);
    if (bits == neg_inf_bits) { return x; }
    if (x == 0.0) {
        let min_neg: f64 = @bitCast(((1 as u64) << 63) | (1 as u64));
        return min_neg;
    }
    let sign_mask: u64 = (1 as u64) << 63;
    let next_bits: u64 = if ((bits & sign_mask) == 0) { bits - 1 } else { bits + 1 };
    let res: f64 = @bitCast(next_bits);
    return res;
}

/// Default `f64` alias for `nextUpF64`.
pub fn nextUp(x: f64): f64 {
    return nextUpF64(x);
}

/// Default `f64` alias for `nextDownF64`.
pub fn nextDown(x: f64): f64 {
    return nextDownF64(x);
}

/// Returns the least `f32` value strictly greater than `x` (IEEE-754 `nextUp`).
pub fn nextUpF32(x: f32): f32 {
    if (fcmpIsNanF32(x)) { return x; }
    let pos_inf_bits: u32 = 0x7F800000;
    let bits: u32 = @bitCast(x);
    if (bits == pos_inf_bits) { return x; }
    if (x == 0.0) {
        let min_pos: f32 = @bitCast(1 as u32);
        return min_pos;
    }
    let sign_mask: u32 = (1 as u32) << 31;
    let next_bits: u32 = if ((bits & sign_mask) == 0) { bits + 1 } else { bits - 1 };
    let res: f32 = @bitCast(next_bits);
    return res;
}

/// Returns the greatest `f32` value strictly less than `x` (IEEE-754 `nextDown`).
pub fn nextDownF32(x: f32): f32 {
    if (fcmpIsNanF32(x)) { return x; }
    let neg_inf_bits: u32 = 0xFF800000;
    let bits: u32 = @bitCast(x);
    if (bits == neg_inf_bits) { return x; }
    if (x == 0.0) {
        let min_neg: f32 = @bitCast(((1 as u32) << 31) | (1 as u32));
        return min_neg;
    }
    let sign_mask: u32 = (1 as u32) << 31;
    let next_bits: u32 = if ((bits & sign_mask) == 0) { bits - 1 } else { bits + 1 };
    let res: f32 = @bitCast(next_bits);
    return res;
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

/// Approximate inequality for `f64`.
pub fn approxNeF64(a: f64, b: f64, margin: F64Margin): bool {
    return !approxEqF64(a, b, margin);
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

/// Approximate inequality for `f32`.
pub fn approxNeF32(a: f32, b: f32, margin: F32Margin): bool {
    return !approxEqF32(a, b, margin);
}

/// Relative ratio approximate equality for `f64`:
/// Returns `true` if `|a - b| <= max(|a|, |b|) * ratio` (delegating to `@cap/approx` `relativeEqF64`).
pub fn approxEqRatioF64(a: f64, b: f64, ratio: f64): bool {
    if (fcmpIsNanF64(ratio) || ratio < 0.0) { return false; }
    return relativeEqF64(a, b, 0.0, ratio);
}

/// Relative ratio approximate equality for `f32`.
pub fn approxEqRatioF32(a: f32, b: f32, ratio: f32): bool {
    if (fcmpIsNanF32(ratio) || ratio < 0.0) { return false; }
    return relativeEqF32(a, b, 0.0, ratio);
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

/// Approximate total ordering for `f32`, returning `-1` (`a < b`), `0` (`a ~= b`), or `1` (`a > b`).
pub fn approxOrdF32(a: f32, b: f32, margin: F32Margin): i32 {
    let a_nan: bool = fcmpIsNanF32(a);
    let b_nan: bool = fcmpIsNanF32(b);
    if (a_nan && b_nan) { return 0; }
    if (a_nan) { return 1; }
    if (b_nan) { return -1; }
    if (approxEqF32(a, b, margin)) { return 0; }
    if (a < b) { return -1; }
    return 1;
}
