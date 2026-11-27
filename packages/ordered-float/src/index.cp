// @cap/ordered-float — Zero-heap IEEE-754 total ordering & canonical hashing for f64 / f32.
// Clean-room MIT port of crates.io #85 `ordered-float` (1.2B+ downloads); reuses `@cap/fnv-hash`.
// Total order: -Inf < finite negatives < (-0.0 == +0.0) < finite positives < +Inf < NaN (all NaNs equal).

import { fnv1a64 } from "../../@sindresorhus/fnv1a/src/index.cp";

/// Returns true if `x` is NaN (`x != x`).
pub fn isNanF64(x: f64): bool {
    return x != x;
}

/// Returns true if `x` is positive or negative infinity.
pub fn isInfiniteF64(x: f64): bool {
    if (x != x) { return false; }
    return (x - x) != 0.0;
}

/// Returns true if `x` is finite (neither NaN nor +/-Inf).
pub fn isFiniteF64(x: f64): bool {
    return !isNanF64(x) && !isInfiniteF64(x);
}

/// Total ordering comparison for `f64` (`-1`, `0`, `1`):
/// All NaNs compare equal to each other and greater than `+Inf`; `-0.0 == +0.0`.
pub fn cmpOrderedF64(a: f64, b: f64): i32 {
    let a_nan = isNanF64(a);
    let b_nan = isNanF64(b);
    if (a_nan && b_nan) { return 0; }
    if (a_nan) { return 1; }
    if (b_nan) { return -1; }
    if (a < b) { return -1; }
    if (a > b) { return 1; }
    return 0;
}

/// Total equality for `f64` (`NaN == NaN` is true, `-0.0 == +0.0` is true).
pub fn eqOrderedF64(a: f64, b: f64): bool {
    return cmpOrderedF64(a, b) == 0;
}

/// Canonical 64-bit hash for `f64` consistent with `eqOrderedF64` (normalizes -0.0 -> +0.0 and all NaNs).
pub fn hashOrderedF64(x: f64): u64 {
    let bits: u64 = if (isNanF64(x)) {
        // Canonical quiet NaN bit pattern 0x7ff8000000000000
        (0x7ff80000 as u64) << 32
    } else if (x == 0.0) {
        // Normalize -0.0 and +0.0 to 0
        0
    } else {
        @bitCast(x)
    };
    let buf: [8]u8 = undefined;
    let i: usize = 0;
    while (i < 8) {
        let sh: u6 = (i * 8) as u6;
        buf[i] = ((bits >> sh) & 0xff) as u8;
        i = i + 1;
    }
    return fnv1a64(buf[0..8]);
}

/// Total-order wrapper struct (`ordered_float::OrderedFloat<f64>`).
pub struct OrderedF64 {
    pub val: f64,

    pub static fn new(v: f64): OrderedF64 {
        return OrderedF64 { val: v };
    }

    pub fn intoInner(): f64 { return _.val; }
    pub fn isNan(): bool { return isNanF64(_.val); }
    pub fn isFinite(): bool { return isFiniteF64(_.val); }
    pub fn cmp(other: OrderedF64): i32 { return cmpOrderedF64(_.val, other.val); }
    pub fn eq(other: OrderedF64): bool { return eqOrderedF64(_.val, other.val); }
    pub fn hash(): u64 { return hashOrderedF64(_.val); }

    pub fn min(other: OrderedF64): OrderedF64 {
        return if (_.cmp(other) <= 0) { OrderedF64 { val: _.val } } else { other };
    }

    pub fn max(other: OrderedF64): OrderedF64 {
        return if (_.cmp(other) >= 0) { OrderedF64 { val: _.val } } else { other };
    }

    pub fn clamp(lo: OrderedF64, hi: OrderedF64): OrderedF64 {
        if (_.cmp(lo) < 0) { return lo; }
        if (_.cmp(hi) > 0) { return hi; }
        return OrderedF64 { val: _.val };
    }
}

/// Non-NaN wrapper (`ordered_float::NotNan<f64>`): rejects NaN at construction.
pub struct NotNanF64 {
    pub val: f64,

    pub static fn new(v: f64): NotNanF64? {
        if (isNanF64(v)) { return null; }
        return NotNanF64 { val: v };
    }

    pub fn intoInner(): f64 { return _.val; }
    pub fn cmp(other: NotNanF64): i32 { return cmpOrderedF64(_.val, other.val); }
    pub fn eq(other: NotNanF64): bool { return eqOrderedF64(_.val, other.val); }
    pub fn hash(): u64 { return hashOrderedF64(_.val); }
}

/// In-place insertion sort of an `f64` slice using `cmpOrderedF64` (NaNs sorted to end).
pub fn sortOrderedF64(arr: []f64) {
    let i: usize = 1;
    while (i < arr.len) {
        let cur = arr[i];
        let j = i;
        while (j > 0 && cmpOrderedF64(arr[j - 1], cur) > 0) {
            arr[j] = arr[j - 1];
            j = j - 1;
        }
        arr[j] = cur;
        i = i + 1;
    }
}
