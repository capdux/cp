// @cap/num-traits — Zero-heap overflow-safe numeric & bit-manipulation primitives.
// Clean-room MIT implementation modeled on crates.io #10 `num-traits` v0.2 (2B+ downloads).

pub const I64_MAX: i64 = 9223372036854775807;
pub const I64_MIN: i64 = -9223372036854775807 - 1;
pub const U64_MAX: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);

fn ntrtAbsI64ToU64(x: i64): u64 {
    if (x >= 0) { return x as u64; }
    if (x == I64_MIN) { return (1 as u64) << 63; }
    return (-x) as u64;
}

// ============================================================================
// 1. Checked Overflow-Safe Arithmetic
// ============================================================================

/// Checked signed 64-bit addition. Returns `null` on overflow or underflow.
pub fn checkedAddI64(a: i64, b: i64): i64? {
    if (b > 0 && a > I64_MAX - b) { return null; }
    if (b < 0 && a < I64_MIN - b) { return null; }
    return a + b;
}

/// Checked signed 64-bit subtraction. Returns `null` on overflow or underflow.
pub fn checkedSubI64(a: i64, b: i64): i64? {
    if (b < 0 && a > I64_MAX + b) { return null; }
    if (b > 0 && a < I64_MIN + b) { return null; }
    return a - b;
}

/// Checked signed 64-bit multiplication. Returns `null` on overflow or underflow.
pub fn checkedMulI64(a: i64, b: i64): i64? {
    if (a == 0 || b == 0) { return 0; }
    let neg: bool = (a < 0) != (b < 0);
    let ua: u64 = ntrtAbsI64ToU64(a);
    let ub: u64 = ntrtAbsI64ToU64(b);
    let min_mag: u64 = (1 as u64) << 63;
    let max_mag: u64 = min_mag - 1;
    let limit: u64 = if (neg) { min_mag } else { max_mag };
    if (ua > limit / ub) { return null; }
    let prod: u64 = ua * ub;
    if (neg) {
        if (prod == min_mag) { return I64_MIN; }
        return -(prod as i64);
    }
    return prod as i64;
}

/// Checked unsigned 64-bit addition. Returns `null` on overflow.
pub fn checkedAddU64(a: u64, b: u64): u64? {
    if (a > U64_MAX - b) { return null; }
    return a + b;
}

/// Checked unsigned 64-bit subtraction. Returns `null` on underflow.
pub fn checkedSubU64(a: u64, b: u64): u64? {
    if (a < b) { return null; }
    return a - b;
}

/// Checked unsigned 64-bit multiplication. Returns `null` on overflow.
pub fn checkedMulU64(a: u64, b: u64): u64? {
    if (a == 0 || b == 0) { return 0; }
    if (a > U64_MAX / b) { return null; }
    return a * b;
}

// ============================================================================
// 2. Saturating Arithmetic
// ============================================================================

/// Saturating unsigned 64-bit addition, clamping at `U64_MAX`.
pub fn saturatingAddU64(a: u64, b: u64): u64 {
    if (a > U64_MAX - b) { return U64_MAX; }
    return a + b;
}

/// Saturating unsigned 64-bit subtraction, clamping at `0`.
pub fn saturatingSubU64(a: u64, b: u64): u64 {
    if (a < b) { return 0; }
    return a - b;
}

/// Saturating unsigned 64-bit multiplication, clamping at `U64_MAX`.
pub fn saturatingMulU64(a: u64, b: u64): u64 {
    if (a == 0 || b == 0) { return 0; }
    if (a > U64_MAX / b) { return U64_MAX; }
    return a * b;
}

/// Saturating signed 64-bit addition, clamping at `[I64_MIN, I64_MAX]`.
pub fn saturatingAddI64(a: i64, b: i64): i64 {
    if (b > 0 && a > I64_MAX - b) { return I64_MAX; }
    if (b < 0 && a < I64_MIN - b) { return I64_MIN; }
    return a + b;
}

/// Saturating signed 64-bit subtraction, clamping at `[I64_MIN, I64_MAX]`.
pub fn saturatingSubI64(a: i64, b: i64): i64 {
    if (b < 0 && a > I64_MAX + b) { return I64_MAX; }
    if (b > 0 && a < I64_MIN + b) { return I64_MIN; }
    return a - b;
}

// ============================================================================
// 3. Clamp, Sign, AbsDiff & Checked Exponentiation
// ============================================================================

/// Clamps `v` into the inclusive range `[lo, hi]`.
pub fn clampI64(v: i64, lo: i64, hi: i64): i64 {
    if (v < lo) { return lo; }
    if (v > hi) { return hi; }
    return v;
}

/// Clamps `v` into the inclusive range `[lo, hi]`, propagating `NaN`.
pub fn clampF64(v: f64, lo: f64, hi: f64): f64 {
    if (v != v) { return v; }
    if (v < lo) { return lo; }
    if (v > hi) { return hi; }
    return v;
}

/// Returns `-1` if `v < 0`, `0` if `v == 0`, and `1` if `v > 0`.
pub fn signumI64(v: i64): i64 {
    if (v > 0) { return 1; }
    if (v < 0) { return -1; }
    return 0;
}

/// Computes `|a - b|` without underflow for unsigned 64-bit integers.
pub fn absDiffU64(a: u64, b: u64): u64 {
    return if (a >= b) { a - b } else { b - a };
}

/// Checked unsigned 64-bit exponentiation `base^exp`. Returns `null` on overflow.
pub fn checkedPowU64(base: u64, exp: u32): u64? {
    if (exp == 0) { return 1; }
    let acc: u64 = 1;
    let b: u64 = base;
    let e: u32 = exp;
    while (e > 0) {
        if ((e & 1) == 1) {
            let next_acc = checkedMulU64(acc, b);
            if (next_acc == null) { return null; }
            acc = next_acc ?? 0;
        }
        e = e >> 1;
        if (e > 0) {
            let next_b = checkedMulU64(b, b);
            if (next_b == null) { return null; }
            b = next_b ?? 0;
        }
    }
    return acc;
}

// ============================================================================
// 4. Bit Manipulation & Powers of Two
// ============================================================================

/// Returns `true` if `n` is an exact power of two (`n > 0` and `(n & (n - 1)) == 0`).
pub fn isPowerOfTwoU64(n: u64): bool {
    return n != 0 && (n & (n - 1)) == 0;
}

/// Returns the smallest power of two greater than or equal to `n`, or `null` if it exceeds `U64_MAX`.
pub fn nextPowerOfTwoU64(n: u64): u64? {
    if (n <= 1) { return 1; }
    let highest_pow2: u64 = (1 as u64) << 63;
    if (n > highest_pow2) { return null; }
    let x: u64 = n - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    x = x | (x >> 32);
    return x + 1;
}

/// Returns the number of set (`1`) bits in `n` (popcount).
pub fn countOnesU64(n: u64): u32 {
    let x: u64 = n;
    x = x - ((x >> 1) & 0x5555555555555555);
    x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F;
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32);
    return (x & 0x7F) as u32;
}

/// Returns the number of leading zero bits in `n` (`64` when `n == 0`).
pub fn leadingZerosU64(n: u64): u32 {
    if (n == 0) { return 64; }
    let x: u64 = n;
    let count: u32 = 0;
    if ((x >> 32) == 0) { count = count + 32; x = x << 32; }
    if ((x >> 48) == 0) { count = count + 16; x = x << 16; }
    if ((x >> 56) == 0) { count = count + 8; x = x << 8; }
    if ((x >> 60) == 0) { count = count + 4; x = x << 4; }
    if ((x >> 62) == 0) { count = count + 2; x = x << 2; }
    if ((x >> 63) == 0) { count = count + 1; }
    return count;
}

/// Returns the number of trailing zero bits in `n` (`64` when `n == 0`).
pub fn trailingZerosU64(n: u64): u32 {
    if (n == 0) { return 64; }
    let x: u64 = n;
    let count: u32 = 0;
    if ((x & 0x00000000FFFFFFFF) == 0) { count = count + 32; x = x >> 32; }
    if ((x & 0x000000000000FFFF) == 0) { count = count + 16; x = x >> 16; }
    if ((x & 0x00000000000000FF) == 0) { count = count + 8; x = x >> 8; }
    if ((x & 0x000000000000000F) == 0) { count = count + 4; x = x >> 4; }
    if ((x & 0x0000000000000003) == 0) { count = count + 2; x = x >> 2; }
    if ((x & 0x0000000000000001) == 0) { count = count + 1; }
    return count;
}

/// Rotates the bits of `n` left by `k` positions (`k` is taken modulo 64).
pub fn rotateLeftU64(n: u64, k: u32): u64 {
    let shift: u32 = k & 63;
    if (shift == 0) { return n; }
    return (n << shift) | (n >> (64 - shift));
}

/// Rotates the bits of `n` right by `k` positions (`k` is taken modulo 64).
pub fn rotateRightU64(n: u64, k: u32): u64 {
    let shift: u32 = k & 63;
    if (shift == 0) { return n; }
    return (n >> shift) | (n << (64 - shift));
}
