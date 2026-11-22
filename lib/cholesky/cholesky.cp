// Origin: Pure-CP Stdlib (Zero-Alloc SPD Cholesky LL^T & Sqrt-Free LDL^T Decomposition, Solver, LogDet & Inverse) | Created: 2026-11-22

import std from "std";

const LN2: f64 = 0.69314718055994530942;
const PIVOT_EPS: f64 = 1.0e-15;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn checkMatLen(buf_len: usize, n: usize): bool {
    return n > 0 && (buf_len / n) >= n;
}

/// Pure-CP double-precision natural logarithm for `x > 0` via range reduction to `[0.75, 1.5]`
/// and atanh series `ln(m) = 2 * sum_{k=0..11} t^(2k+1)/(2k+1)` (accuracy ~1e-16).
fn lnF64(x: f64): f64 {
    let m = x;
    let k: i32 = 0;
    while (m >= 1.5) {
        m = m * 0.5;
        k += 1;
    }
    while (m < 0.75) {
        m = m * 2.0;
        k -= 1;
    }
    const t = (m - 1.0) / (m + 1.0);
    const t2 = t * t;
    let term = t;
    let sum: f64 = 0.0;
    let denom: f64 = 1.0;
    let iter: usize = 0;
    while (iter < 12) {
        sum = sum + term / denom;
        term = term * t2;
        denom = denom + 2.0;
        iter += 1;
    }
    return 2.0 * sum + (k as f64) * LN2;
}

/// Computes the Cholesky decomposition `A = L * L^T` of a symmetric positive-definite `n x n` matrix.
/// - Reads only the lower triangle of row-major `a` (`a.len >= n * n`).
/// - Writes lower-triangular factor `L` into `l_out` and zeroes its strict upper triangle.
/// - Returns `false` if `n == 0`, buffers are too small, or `A` is not positive-definite / contains NaN.
pub fn choleskyDecompose(a: []const f64, n: usize, l_out: []f64): bool {
    if (!checkMatLen(a.len, n) || !checkMatLen(l_out.len, n)) {
        return false;
    }
    const total = n * n;
    let z: usize = 0;
    while (z < total) {
        l_out[z] = 0.0;
        z += 1;
    }

    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j <= i) {
            let sum = a[i * n + j];
            if (!isFiniteF64(sum)) {
                return false;
            }
            let k: usize = 0;
            while (k < j) {
                sum = sum - l_out[i * n + k] * l_out[j * n + k];
                k += 1;
            }
            if (i == j) {
                if (!isFiniteF64(sum) || sum <= 0.0) {
                    return false;
                }
                l_out[i * n + i] = std.math.sqrt(sum);
            } else {
                const diag = l_out[j * n + j];
                if (!isFiniteF64(diag) || diag <= 0.0) {
                    return false;
                }
                l_out[i * n + j] = sum / diag;
            }
            j += 1;
        }
        i += 1;
    }
    return true;
}

/// Solves lower-triangular linear system `L * y = b` via forward substitution.
pub fn forwardSubstitute(l: []const f64, n: usize, b: []const f64, y_out: []f64): bool {
    if (!checkMatLen(l.len, n) || b.len < n || y_out.len < n) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        let sum = b[i];
        if (!isFiniteF64(sum)) {
            return false;
        }
        let j: usize = 0;
        while (j < i) {
            sum = sum - l[i * n + j] * y_out[j];
            j += 1;
        }
        const diag = l[i * n + i];
        if (!isFiniteF64(diag) || absF64(diag) <= PIVOT_EPS) {
            return false;
        }
        y_out[i] = sum / diag;
        i += 1;
    }
    return true;
}

/// Solves transposed lower-triangular linear system `L^T * x = y` via backward substitution.
pub fn backwardSubstituteT(l: []const f64, n: usize, y: []const f64, x_out: []f64): bool {
    if (!checkMatLen(l.len, n) || y.len < n || x_out.len < n) {
        return false;
    }
    let step: usize = 0;
    while (step < n) {
        const i = n - 1 - step;
        let sum = y[i];
        if (!isFiniteF64(sum)) {
            return false;
        }
        let j = i + 1;
        while (j < n) {
            sum = sum - l[j * n + i] * x_out[j];
            j += 1;
        }
        const diag = l[i * n + i];
        if (!isFiniteF64(diag) || absF64(diag) <= PIVOT_EPS) {
            return false;
        }
        x_out[i] = sum / diag;
        step += 1;
    }
    return true;
}

/// Solves `A * x = L * L^T * x = b` using precomputed Cholesky factor `l` and caller-owned `scratch_y`.
pub fn choleskySolve(l: []const f64, n: usize, b: []const f64, x_out: []f64, scratch_y: []f64): bool {
    if (!forwardSubstitute(l, n, b, scratch_y)) {
        return false;
    }
    return backwardSubstituteT(l, n, scratch_y[0..n], x_out);
}

/// Computes `ln(det(A)) = 2 * sum(ln(L_ii))` from Cholesky factor `l`, preventing overflow for large `n`.
/// Returns `null` if `l` is invalid or has non-positive diagonal entries.
pub fn choleskyLogDet(l: []const f64, n: usize): f64? {
    if (!checkMatLen(l.len, n)) {
        return null;
    }
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const diag = l[i * n + i];
        if (!isFiniteF64(diag) || diag <= 0.0) {
            return null;
        }
        sum = sum + lnF64(diag);
        i += 1;
    }
    return 2.0 * sum;
}

/// Computes `det(A) = prod(L_ii^2)` from Cholesky factor `l`.
pub fn choleskyDet(l: []const f64, n: usize): f64? {
    if (!checkMatLen(l.len, n)) {
        return null;
    }
    let prod: f64 = 1.0;
    let i: usize = 0;
    while (i < n) {
        const diag = l[i * n + i];
        if (!isFiniteF64(diag) || diag <= 0.0) {
            return null;
        }
        prod = prod * diag * diag;
        i += 1;
    }
    return prod;
}

/// Computes the explicit inverse `A^-1` of an SPD matrix from its Cholesky factor `l`.
/// Requires `inv_out.len >= n * n`, `scratch_col.len >= n`, and `scratch_y.len >= n`.
pub fn choleskyInvert(l: []const f64, n: usize, inv_out: []f64, scratch_col: []f64, scratch_y: []f64): bool {
    if (!checkMatLen(l.len, n) || !checkMatLen(inv_out.len, n) || scratch_col.len < n || scratch_y.len < n) {
        return false;
    }
    let col: usize = 0;
    while (col < n) {
        // Build unit basis vector e_col in scratch_col
        let r: usize = 0;
        while (r < n) {
            scratch_col[r] = if (r == col) 1.0 else 0.0;
            r += 1;
        }
        if (!forwardSubstitute(l, n, scratch_col[0..n], scratch_y)) {
            return false;
        }
        if (!backwardSubstituteT(l, n, scratch_y[0..n], scratch_col)) {
            return false;
        }
        let row: usize = 0;
        while (row < n) {
            inv_out[row * n + col] = scratch_col[row];
            row += 1;
        }
        col += 1;
    }
    return true;
}

/// Computes the square-root-free `A = L * D * L^T` decomposition of a symmetric non-singular matrix.
/// - `l_out` receives unit lower-triangular `L` (`L_ii = 1.0`, strict upper triangle zeroed).
/// - `d_out` receives the `n` diagonal pivots (may be negative for indefinite symmetric matrices).
/// - Returns `false` on zero pivot, NaN, or insufficient buffer size.
pub fn ldltDecompose(a: []const f64, n: usize, l_out: []f64, d_out: []f64): bool {
    if (!checkMatLen(a.len, n) || !checkMatLen(l_out.len, n) || d_out.len < n) {
        return false;
    }
    const total = n * n;
    let z: usize = 0;
    while (z < total) {
        l_out[z] = 0.0;
        z += 1;
    }

    let j: usize = 0;
    while (j < n) {
        let dj = a[j * n + j];
        if (!isFiniteF64(dj)) {
            return false;
        }
        let k: usize = 0;
        while (k < j) {
            const ljk = l_out[j * n + k];
            dj = dj - ljk * ljk * d_out[k];
            k += 1;
        }
        if (!isFiniteF64(dj) || absF64(dj) <= PIVOT_EPS) {
            return false;
        }
        d_out[j] = dj;
        l_out[j * n + j] = 1.0;

        let i = j + 1;
        while (i < n) {
            let sum = a[i * n + j];
            if (!isFiniteF64(sum)) {
                return false;
            }
            let m: usize = 0;
            while (m < j) {
                sum = sum - l_out[i * n + m] * l_out[j * n + m] * d_out[m];
                m += 1;
            }
            l_out[i * n + j] = sum / dj;
            i += 1;
        }
        j += 1;
    }
    return true;
}

/// Solves `L * D * L^T * x = b` using precomputed `ldltDecompose` factors `l` and `d`.
pub fn ldltSolve(l: []const f64, d: []const f64, n: usize, b: []const f64, x_out: []f64, scratch_y: []f64): bool {
    if (d.len < n || !forwardSubstitute(l, n, b, scratch_y)) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        if (!isFiniteF64(d[i]) || absF64(d[i]) <= PIVOT_EPS) {
            return false;
        }
        scratch_y[i] = scratch_y[i] / d[i];
        i += 1;
    }
    return backwardSubstituteT(l, n, scratch_y[0..n], x_out);
}
