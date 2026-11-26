// @cap/ordered-float — Zero-allocation IEEE 754 total-order float engine for CP
// Clean-room MIT implementation inspired by Rust `ordered-float` (crates.io #292, 571M+ downloads).
//
// Ordering contract (strict total order):
//   -Inf < finite numbers (-0.0 == +0.0) < +Inf < NaN (all NaNs compare equal)
//
// Features:
// - Classification: `isNanF64`, `isInfF64`, `isFiniteF64`.
// - Total-order comparison: `orderedCmpF64`, `orderedEqF64`, `orderedMinF64`, `orderedMaxF64`, `clampF64`.
// - `NotNanF64`: checked non-NaN wrapper (`new`, `value`, `cmp`).
// - NaN/Inf-safe zero-allocation in-place sorting (`sortF64`), `isSortedF64`, and `binarySearchF64`.

const std = @import("std");

/// Returns `true` if `x` is IEEE 754 NaN.
pub fn isNanF64(x: f64) -> bool {
    return x != x;
}

/// Returns `true` if `x` is `+Inf` or `-Inf`.
pub fn isInfF64(x: f64) -> bool {
    return std.math.isInf(x);
}

/// Returns `true` if `x` is neither NaN nor infinite.
pub fn isFiniteF64(x: f64) -> bool {
    return !isNanF64(x) && !isInfF64(x);
}

/// Strict total-order comparison for `f64`: `-Inf < finite (-0.0 == +0.0) < +Inf < NaN`.
/// Returns `-1` if `a < b`, `0` if `a == b` (including two NaNs or `-0.0` vs `+0.0`), `1` if `a > b`.
pub fn orderedCmpF64(a: f64, b: f64) -> i32 {
    const na = isNanF64(a);
    const nb = isNanF64(b);
    if (na && nb) {
        return 0;
    }
    if (na) {
        return 1;
    }
    if (nb) {
        return -1;
    }
    if (a < b) {
        return -1;
    }
    if (a > b) {
        return 1;
    }
    return 0;
}

/// Total-order equality: `NaN == NaN` is `true`, and `-0.0 == +0.0` is `true`.
pub fn orderedEqF64(a: f64, b: f64) -> bool {
    return orderedCmpF64(a, b) == 0;
}

/// Returns the smaller of `a` and `b` under `orderedCmpF64` (finite beats NaN).
pub fn orderedMinF64(a: f64, b: f64) -> f64 {
    if (orderedCmpF64(a, b) <= 0) {
        return a;
    }
    return b;
}

/// Returns the larger of `a` and `b` under `orderedCmpF64`.
pub fn orderedMaxF64(a: f64, b: f64) -> f64 {
    if (orderedCmpF64(a, b) >= 0) {
        return a;
    }
    return b;
}

/// Clamps `x` into `[lo, hi]` using total-order comparison.
pub fn clampF64(x: f64, lo: f64, hi: f64) -> f64 {
    if (orderedCmpF64(x, lo) < 0) {
        return lo;
    }
    if (orderedCmpF64(x, hi) > 0) {
        return hi;
    }
    return x;
}

/// Checked floating-point wrapper guaranteed never to contain NaN.
pub struct NotNanF64 {
    pub inner: f64,

    pub static fn new(v: f64) -> NotNanF64? {
        if (isNanF64(v)) {
            return null;
        }
        return NotNanF64{ inner: v };
    }

    pub fn value() -> f64 {
        return _.inner;
    }

    pub fn cmp(other: NotNanF64) -> i32 {
        return orderedCmpF64(_.inner, other.inner);
    }
}

fn siftDownF64(slice: []f64, start: usize, end: usize) {
    let root = start;
    while (true) {
        const left = root * 2 + 1;
        if (left >= end) {
            break;
        }
        let best = root;
        if (orderedCmpF64(slice[best], slice[left]) < 0) {
            best = left;
        }
        const right = left + 1;
        if (right < end && orderedCmpF64(slice[best], slice[right]) < 0) {
            best = right;
        }
        if (best == root) {
            break;
        }
        const tmp = slice[root];
        slice[root] = slice[best];
        slice[best] = tmp;
        root = best;
    }
}

/// Zero-allocation in-place ascending sort for `[]f64` under strict total order.
/// Safely handles `-Inf`, `+Inf`, `-0.0`, and `NaN` (all `NaN`s placed at the end).
pub fn sortF64(slice: []f64) {
    const n = slice.len;
    if (n <= 1) {
        return;
    }

    if (n <= 16) {
        let i: usize = 1;
        while (i < n) {
            const key = slice[i];
            let j = i;
            while (j > 0 && orderedCmpF64(key, slice[j - 1]) < 0) {
                slice[j] = slice[j - 1];
                j -= 1;
            }
            slice[j] = key;
            i += 1;
        }
        return;
    }

    let start = n / 2;
    while (start > 0) {
        start -= 1;
        siftDownF64(slice, start, n);
    }

    let end = n;
    while (end > 1) {
        end -= 1;
        const tmp = slice[0];
        slice[0] = slice[end];
        slice[end] = tmp;
        siftDownF64(slice, 0, end);
    }
}

/// Returns `true` if `slice` is sorted in non-decreasing total order.
pub fn isSortedF64(slice: []const f64) -> bool {
    if (slice.len <= 1) {
        return true;
    }
    let i: usize = 0;
    while (i + 1 < slice.len) {
        if (orderedCmpF64(slice[i], slice[i + 1]) > 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Binary searches a total-order sorted `[]const f64` for `target` (supports finite values, `±Inf`, and `NaN`).
pub fn binarySearchF64(slice: []const f64, target: f64) -> usize? {
    let lo: usize = 0;
    let hi: usize = slice.len;
    while (lo < hi) {
        const mid = lo + ((hi - lo) >> 1);
        const c = orderedCmpF64(slice[mid], target);
        if (c == 0) {
            return mid;
        }
        if (c < 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return null;
}
