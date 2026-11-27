// @cap/cast — Zero-allocation checked primitive numeric casts in pure CP
// Clean-room MIT implementation of crates.io `cast` (#443, 335M+).

pub enum CastError {
    Overflow,
    Underflow,
    Infinite,
    NaN,
}

/// Checked cast `i64 -> u8` (returns `null` on negative or > 255).
pub fn u8FromI64(v: i64): u8? {
    if (v < 0 || v > 255) {
        return null;
    }
    return v as u8;
}

/// Checked cast `u64 -> u8`.
pub fn u8FromU64(v: u64): u8? {
    if (v > 255) {
        return null;
    }
    return v as u8;
}

/// Checked cast `i64 -> u16`.
pub fn u16FromI64(v: i64): u16? {
    if (v < 0 || v > 65535) {
        return null;
    }
    return v as u16;
}

/// Checked cast `i64 -> u32`.
pub fn u32FromI64(v: i64): u32? {
    if (v < 0 || v > 4294967295) {
        return null;
    }
    return v as u32;
}

/// Checked cast `u64 -> u32`.
pub fn u32FromU64(v: u64): u32? {
    if (v > 4294967295) {
        return null;
    }
    return v as u32;
}

/// Checked cast `u64 -> i32`.
pub fn i32FromU64(v: u64): i32? {
    if (v > 2147483647) {
        return null;
    }
    return v as i32;
}

/// Checked cast `i64 -> i32`.
pub fn i32FromI64(v: i64): i32? {
    if (v < -2147483648 || v > 2147483647) {
        return null;
    }
    return v as i32;
}

/// Checked cast `i64 -> u64`.
pub fn u64FromI64(v: i64): u64? {
    if (v < 0) {
        return null;
    }
    return v as u64;
}

/// Checked cast `u64 -> i64`.
pub fn i64FromU64(v: u64): i64? {
    const max_i64: u64 = ((0x7FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    if (v > max_i64) {
        return null;
    }
    return v as i64;
}

/// Checked cast `i64 -> usize`.
pub fn usizeFromI64(v: i64): usize? {
    if (v < 0) {
        return null;
    }
    return v as usize;
}

/// Checked cast `f64 -> u32` (rejects NaN, Inf, negative, and > u32::MAX).
pub fn u32FromF64(v: f64): u32? {
    if (v != v) {
        return null; // NaN
    }
    if (v < 0.0 || v > 4294967295.0) {
        return null;
    }
    return v as u32;
}

/// Checked cast `f64 -> i32` (rejects NaN, Inf, < i32::MIN, > i32::MAX).
pub fn i32FromF64(v: f64): i32? {
    if (v != v) {
        return null; // NaN
    }
    if (v < -2147483648.0 || v > 2147483647.0) {
        return null;
    }
    return v as i32;
}

/// Checked cast `f64 -> u8`.
pub fn u8FromF64(v: f64): u8? {
    if (v != v || v < 0.0 || v > 255.0) {
        return null;
    }
    return v as u8;
}

/// Diagnoses exact `CastError` for `f64 -> i32` conversion.
pub fn checkF64ToI32(v: f64): CastError? {
    if (v != v) {
        return .NaN;
    }
    const pos_inf: f64 = std.math.inf(f64);
    if (v == pos_inf || v == -pos_inf) {
        return .Infinite;
    }
    if (v < -2147483648.0) {
        return .Underflow;
    }
    if (v > 2147483647.0) {
        return .Overflow;
    }
    return null;
}

export {
    CastError,
    u8FromI64,
    u8FromU64,
    u16FromI64,
    u32FromI64,
    u32FromU64,
    i32FromU64,
    i32FromI64,
    u64FromI64,
    i64FromU64,
    usizeFromI64,
    u32FromF64,
    i32FromF64,
    u8FromF64,
    checkF64ToI32,
};
