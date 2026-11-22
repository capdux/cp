// Origin: Pure-CP Stdlib (Zero-Alloc DWT/IDWT: Haar, Daubechies-4 db2 & JPEG 2000 Lossless CDF 5/3) | Created: 2026-11-22

const INV_SQRT2: f64 = 0.70710678118654752440;

// Orthonormal Daubechies-4 (db2) scaling (lowpass) & wavelet (highpass) coefficients
const DB4_H0: f64 = 0.48296291314453414337;
const DB4_H1: f64 = 0.83651630373780790558;
const DB4_H2: f64 = 0.22414386804201338103;
const DB4_H3: f64 = 0.0 - 0.12940952255126038117;

const DB4_G0: f64 = DB4_H3;
const DB4_G1: f64 = 0.0 - DB4_H2;
const DB4_G2: f64 = DB4_H1;
const DB4_G3: f64 = 0.0 - DB4_H0;

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn floorDiv2(x: i64): i64 {
    return x >> 1;
}

fn floorDiv4(x: i64): i64 {
    return x >> 2;
}

/// Single-level orthonormal 1D Haar forward DWT (in-place using caller `scratch`).
/// Places low-frequency approximation in `data[0..N/2]` and detail in `data[N/2..N]`.
/// Preserves Parseval signal energy. Requires even `data.len >= 2` and `scratch.len >= data.len`.
pub fn haarForward1d(data: []f64, scratch: []f64): bool {
    const n = data.len;
    if (n < 2 || (n % 2) != 0 || scratch.len < n) {
        return false;
    }
    const half = n / 2;
    let i: usize = 0;
    while (i < half) {
        const a = data[2 * i];
        const b = data[2 * i + 1];
        scratch[i] = (a + b) * INV_SQRT2;
        scratch[half + i] = (a - b) * INV_SQRT2;
        i += 1;
    }
    let k: usize = 0;
    while (k < n) {
        data[k] = scratch[k];
        k += 1;
    }
    return true;
}

/// Single-level orthonormal 1D Haar inverse DWT (in-place using caller `scratch`).
pub fn haarInverse1d(data: []f64, scratch: []f64): bool {
    const n = data.len;
    if (n < 2 || (n % 2) != 0 || scratch.len < n) {
        return false;
    }
    const half = n / 2;
    let i: usize = 0;
    while (i < half) {
        const approx = data[i];
        const detail = data[half + i];
        scratch[2 * i] = (approx + detail) * INV_SQRT2;
        scratch[2 * i + 1] = (approx - detail) * INV_SQRT2;
        i += 1;
    }
    let k: usize = 0;
    while (k < n) {
        data[k] = scratch[k];
        k += 1;
    }
    return true;
}

fn checkMultiLevels(len: usize, levels: usize): bool {
    if (levels == 0 || len < 2) {
        return false;
    }
    let cur = len;
    let lv: usize = 0;
    while (lv < levels) {
        if (cur < 2 || (cur % 2) != 0) {
            return false;
        }
        cur = cur / 2;
        lv += 1;
    }
    return true;
}

/// Multi-level 1D orthonormal Haar forward DWT (`levels >= 1`).
/// Requires `data.len` divisible by `2^levels` and `scratch.len >= data.len`.
pub fn haarForwardMulti1d(data: []f64, levels: usize, scratch: []f64): bool {
    if (!checkMultiLevels(data.len, levels) || scratch.len < data.len) {
        return false;
    }
    let cur = data.len;
    let lv: usize = 0;
    while (lv < levels) {
        if (!haarForward1d(data[0..cur], scratch)) {
            return false;
        }
        cur = cur / 2;
        lv += 1;
    }
    return true;
}

/// Multi-level 1D orthonormal Haar inverse DWT (`levels >= 1`).
pub fn haarInverseMulti1d(data: []f64, levels: usize, scratch: []f64): bool {
    if (!checkMultiLevels(data.len, levels) || scratch.len < data.len) {
        return false;
    }
    // Find coarsest transform width = data.len / 2^(levels - 1)
    let cur = data.len;
    let s: usize = 1;
    while (s < levels) {
        cur = cur / 2;
        s += 1;
    }
    let lv: usize = 0;
    while (lv < levels) {
        if (!haarInverse1d(data[0..cur], scratch)) {
            return false;
        }
        cur = cur * 2;
        lv += 1;
    }
    return true;
}

/// Single-level orthonormal 1D Daubechies-4 (`db2`) forward DWT with periodic boundary.
/// Preserves Parseval energy and has 2 vanishing moments. Requires even `data.len >= 4`.
pub fn db4Forward1d(data: []f64, scratch: []f64): bool {
    const n = data.len;
    if (n < 4 || (n % 2) != 0 || scratch.len < n) {
        return false;
    }
    const half = n / 2;
    let i: usize = 0;
    while (i < half) {
        const idx0 = 2 * i;
        const idx1 = idx0 + 1;
        const idx2 = (idx0 + 2) % n;
        const idx3 = (idx0 + 3) % n;
        const x0 = data[idx0];
        const x1 = data[idx1];
        const x2 = data[idx2];
        const x3 = data[idx3];
        scratch[i] = DB4_H0 * x0 + DB4_H1 * x1 + DB4_H2 * x2 + DB4_H3 * x3;
        scratch[half + i] = DB4_G0 * x0 + DB4_G1 * x1 + DB4_G2 * x2 + DB4_G3 * x3;
        i += 1;
    }
    let k: usize = 0;
    while (k < n) {
        data[k] = scratch[k];
        k += 1;
    }
    return true;
}

/// Single-level orthonormal 1D Daubechies-4 (`db2`) inverse DWT with periodic boundary.
pub fn db4Inverse1d(data: []f64, scratch: []f64): bool {
    const n = data.len;
    if (n < 4 || (n % 2) != 0 || scratch.len < n) {
        return false;
    }
    let z: usize = 0;
    while (z < n) {
        scratch[z] = 0.0;
        z += 1;
    }
    const half = n / 2;
    let i: usize = 0;
    while (i < half) {
        const a = data[i];
        const d = data[half + i];
        const idx0 = 2 * i;
        const idx1 = idx0 + 1;
        const idx2 = (idx0 + 2) % n;
        const idx3 = (idx0 + 3) % n;
        scratch[idx0] = scratch[idx0] + DB4_H0 * a + DB4_G0 * d;
        scratch[idx1] = scratch[idx1] + DB4_H1 * a + DB4_G1 * d;
        scratch[idx2] = scratch[idx2] + DB4_H2 * a + DB4_G2 * d;
        scratch[idx3] = scratch[idx3] + DB4_H3 * a + DB4_G3 * d;
        i += 1;
    }
    let k: usize = 0;
    while (k < n) {
        data[k] = scratch[k];
        k += 1;
    }
    return true;
}

/// JPEG 2000 lossless integer-to-integer Le Gall CDF 5/3 lifting forward DWT.
/// Places low-frequency `s` in `data[0..N/2]` and high-frequency `d` in `data[N/2..N]`.
/// Bit-exact reversible via `cdf53InverseI32`. Requires even `data.len >= 2` and `scratch.len >= data.len`.
pub fn cdf53ForwardI32(data: []i32, scratch: []i32): bool {
    const n = data.len;
    if (n < 2 || (n % 2) != 0 || scratch.len < n) {
        return false;
    }
    const half = n / 2;

    // Split even -> scratch[0..half], odd -> scratch[half..n]
    let i: usize = 0;
    while (i < half) {
        scratch[i] = data[2 * i];
        scratch[half + i] = data[2 * i + 1];
        i += 1;
    }

    // Predict: d[i] = odd[i] - floor((s[i] + s[i+1]) / 2)
    let p: usize = 0;
    while (p < half) {
        const s_cur = scratch[p] as i64;
        const s_nxt = (if (p + 1 < half) scratch[p + 1] else scratch[p]) as i64;
        const d_val = (scratch[half + p] as i64) - floorDiv2(s_cur + s_nxt);
        scratch[half + p] = d_val as i32;
        p += 1;
    }

    // Update: s[i] = even[i] + floor((d[i-1] + d[i] + 2) / 4)
    let u: usize = 0;
    while (u < half) {
        const d_prv = (if (u > 0) scratch[half + u - 1] else scratch[half]) as i64;
        const d_cur = scratch[half + u] as i64;
        const s_val = (scratch[u] as i64) + floorDiv4(d_prv + d_cur + 2);
        scratch[u] = s_val as i32;
        u += 1;
    }

    let k: usize = 0;
    while (k < n) {
        data[k] = scratch[k];
        k += 1;
    }
    return true;
}

/// JPEG 2000 lossless integer-to-integer Le Gall CDF 5/3 lifting inverse DWT.
/// Bit-exact inverse of `cdf53ForwardI32`.
pub fn cdf53InverseI32(data: []i32, scratch: []i32): bool {
    const n = data.len;
    if (n < 2 || (n % 2) != 0 || scratch.len < n) {
        return false;
    }
    const half = n / 2;

    let c: usize = 0;
    while (c < n) {
        scratch[c] = data[c];
        c += 1;
    }

    // Undo Update: s[i] = s[i] - floor((d[i-1] + d[i] + 2) / 4)
    let u: usize = 0;
    while (u < half) {
        const d_prv = (if (u > 0) scratch[half + u - 1] else scratch[half]) as i64;
        const d_cur = scratch[half + u] as i64;
        const s_val = (scratch[u] as i64) - floorDiv4(d_prv + d_cur + 2);
        scratch[u] = s_val as i32;
        u += 1;
    }

    // Undo Predict: d[i] = d[i] + floor((s[i] + s[i+1]) / 2)
    let p: usize = 0;
    while (p < half) {
        const s_cur = scratch[p] as i64;
        const s_nxt = (if (p + 1 < half) scratch[p + 1] else scratch[p]) as i64;
        const d_val = (scratch[half + p] as i64) + floorDiv2(s_cur + s_nxt);
        scratch[half + p] = d_val as i32;
        p += 1;
    }

    // Merge back into interleaved signal
    let i: usize = 0;
    while (i < half) {
        data[2 * i] = scratch[i];
        data[2 * i + 1] = scratch[half + i];
        i += 1;
    }
    return true;
}

/// Applies Donoho-Johnstone soft thresholding shrinkage in-place (`threshold > 0`).
pub fn softThreshold(data: []f64, threshold: f64) {
    if (threshold <= 0.0) {
        return;
    }
    let i: usize = 0;
    while (i < data.len) {
        const v = data[i];
        if (v > threshold) {
            data[i] = v - threshold;
        } else if (v < 0.0 - threshold) {
            data[i] = v + threshold;
        } else {
            data[i] = 0.0;
        }
        i += 1;
    }
}

/// Applies hard thresholding in-place (`|x| < threshold` -> `0.0`).
pub fn hardThreshold(data: []f64, threshold: f64) {
    if (threshold <= 0.0) {
        return;
    }
    let i: usize = 0;
    while (i < data.len) {
        if (absF64(data[i]) < threshold) {
            data[i] = 0.0;
        }
        i += 1;
    }
}

/// Computes discrete Parseval signal energy `sum(x_i^2)`.
pub fn signalEnergy(data: []const f64): f64 {
    let e: f64 = 0.0;
    let i: usize = 0;
    while (i < data.len) {
        e = e + data[i] * data[i];
        i += 1;
    }
    return e;
}
