// @cap/approx — Zero-heap floating-point approximate equality modeled on Rust's `approx` crate (900M+ downloads).
// Clean-room MIT implementation providing `absDiffEq`, `relativeEq`, and `ulpsEq` for `f64` & `f32`.

pub const F64_EPSILON: f64 = 2.220446049250313e-16;
pub const F32_EPSILON: f32 = 1.1920929e-7;

fn apxAbsF64(x: f64): f64 {
    return if (x < 0.0) { -x } else { x };
}

fn apxAbsF32(x: f32): f32 {
    return if (x < 0.0) { -x } else { x };
}

fn apxIsNanF64(x: f64): bool { return x != x; }
fn apxIsNanF32(x: f32): bool { return x != x; }

fn apxIsInfF64(x: f64): bool { return !apxIsNanF64(x) && (x - x) != 0.0; }
fn apxIsInfF32(x: f32): bool { return !apxIsNanF32(x) && (x - x) != 0.0; }

/// Absolute difference equality for `f64`: `|a - b| <= epsilon`.
pub fn absDiffEqF64(a: f64, b: f64, epsilon: f64): bool {
    if (apxIsNanF64(a) || apxIsNanF64(b)) { return false; }
    if (a == b) { return true; }
    if (apxIsInfF64(a) || apxIsInfF64(b)) { return false; }
    return apxAbsF64(a - b) <= epsilon;
}

/// Absolute difference equality for `f32`: `|a - b| <= epsilon`.
pub fn absDiffEqF32(a: f32, b: f32, epsilon: f32): bool {
    if (apxIsNanF32(a) || apxIsNanF32(b)) { return false; }
    if (a == b) { return true; }
    if (apxIsInfF32(a) || apxIsInfF32(b)) { return false; }
    return apxAbsF32(a - b) <= epsilon;
}

/// Relative equality for `f64`: `|a - b| <= epsilon` OR `|a - b| <= max_relative * max(|a|, |b|)`.
pub fn relativeEqF64(a: f64, b: f64, epsilon: f64, max_relative: f64): bool {
    if (apxIsNanF64(a) || apxIsNanF64(b)) { return false; }
    if (a == b) { return true; }
    if (apxIsInfF64(a) || apxIsInfF64(b)) { return false; }
    let diff = apxAbsF64(a - b);
    if (diff <= epsilon) { return true; }
    let aa = apxAbsF64(a);
    let bb = apxAbsF64(b);
    let largest = if (aa > bb) { aa } else { bb };
    return diff <= largest * max_relative;
}

/// Relative equality for `f32`.
pub fn relativeEqF32(a: f32, b: f32, epsilon: f32, max_relative: f32): bool {
    if (apxIsNanF32(a) || apxIsNanF32(b)) { return false; }
    if (a == b) { return true; }
    if (apxIsInfF32(a) || apxIsInfF32(b)) { return false; }
    let diff = apxAbsF32(a - b);
    if (diff <= epsilon) { return true; }
    let aa = apxAbsF32(a);
    let bb = apxAbsF32(b);
    let largest = if (aa > bb) { aa } else { bb };
    return diff <= largest * max_relative;
}

/// Computes the IEEE-754 ULP (Units in the Last Place) distance between two same-sign finite `f64` values.
/// Returns `null` if either is NaN/Inf or they have opposite signs.
pub fn ulpsDistanceF64(a: f64, b: f64): u64? {
    if (apxIsNanF64(a) || apxIsNanF64(b) || apxIsInfF64(a) || apxIsInfF64(b)) { return null; }
    if (a == b) { return 0; }
    let ba: u64 = @bitCast(a);
    let bb: u64 = @bitCast(b);
    let sign_mask: u64 = (1 as u64) << 63;
    if ((ba & sign_mask) != (bb & sign_mask)) { return null; }
    return if (ba >= bb) { ba - bb } else { bb - ba };
}

/// ULP-based approximate equality for `f64`: passes if `absDiffEqF64` OR ULP distance `<= max_ulps`.
pub fn ulpsEqF64(a: f64, b: f64, epsilon: f64, max_ulps: u64): bool {
    if (absDiffEqF64(a, b, epsilon)) { return true; }
    let d = ulpsDistanceF64(a, b);
    if (d == null) { return false; }
    return (d ?? (max_ulps + 1)) <= max_ulps;
}

/// Computes the IEEE-754 ULP distance between two same-sign finite `f32` values.
pub fn ulpsDistanceF32(a: f32, b: f32): u32? {
    if (apxIsNanF32(a) || apxIsNanF32(b) || apxIsInfF32(a) || apxIsInfF32(b)) { return null; }
    if (a == b) { return 0; }
    let ba: u32 = @bitCast(a);
    let bb: u32 = @bitCast(b);
    let sign_mask: u32 = (1 as u32) << 31;
    if ((ba & sign_mask) != (bb & sign_mask)) { return null; }
    return if (ba >= bb) { ba - bb } else { bb - ba };
}

/// ULP-based approximate equality for `f32`.
pub fn ulpsEqF32(a: f32, b: f32, epsilon: f32, max_ulps: u32): bool {
    if (absDiffEqF32(a, b, epsilon)) { return true; }
    let d = ulpsDistanceF32(a, b);
    if (d == null) { return false; }
    return (d ?? (max_ulps + 1)) <= max_ulps;
}
