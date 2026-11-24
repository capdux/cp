// Reference-hardened against statrs-dev/statrs (MIT) & dimforge/nalgebra (Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc Doolittle Partial-Pivoting LU Decomposition, Linear Solver, Determinant, Inverse & O(n) Thomas Tridiagonal Solver) | Created: 2026-11-22

import std from "std";

const THOMAS_PIVOT_EPS: f64 = 1.0e-15;
type LuMathNumVec4f64 = @Vector(4, f64);

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn luAbsF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn checkMatLen(buf_len: usize, n: usize): bool {
    return n > 0 && (buf_len / n) >= n;
}

/// 4-lane `@Vector(4, f64)` SIMD dot product over equal-length finite slices `a` and `b`.
/// Returns `null` if lengths differ or any element / result is non-finite.
pub fn luSimdDotF64(a: []const f64, b: []const f64): f64? {
    if (a.len != b.len) {
        return null;
    }
    const n: usize = a.len;
    let v_acc: LuMathNumVec4f64 = @splat(0.0);
    let i: usize = 0;
    while (i + 4 <= n) : (i += 4) {
        const va: LuMathNumVec4f64 = a[i .. i + 4][0..4].*;
        const vb: LuMathNumVec4f64 = b[i .. i + 4][0..4].*;
        if (!isFiniteF64(va[0]) || !isFiniteF64(va[1]) || !isFiniteF64(va[2]) || !isFiniteF64(va[3]) ||
            !isFiniteF64(vb[0]) || !isFiniteF64(vb[1]) || !isFiniteF64(vb[2]) || !isFiniteF64(vb[3]))
        {
            return null;
        }
        v_acc += va * vb;
    }
    let acc: f64 = @reduce(.Add, v_acc);
    while (i < n) : (i += 1) {
        const av: f64 = a[i];
        const bv: f64 = b[i];
        if (!isFiniteF64(av) || !isFiniteF64(bv)) {
            return null;
        }
        acc += av * bv;
    }
    if (!isFiniteF64(acc)) {
        return null;
    }
    return acc;
}

/// Metadata returned by `luFactorize` describing the row permutation and numerical conditioning.
pub struct LuInfo {
    /// Permutation parity sign: `+1.0` for even row swaps, `-1.0` for odd row swaps.
    sign: f64;
    /// Total number of row interchanges performed during partial pivoting.
    swaps: usize;
    /// Minimum pivot magnitude `min_k |U[k, k]|` encountered during factorization.
    min_pivot: f64;
}

/// Performs in-place Doolittle LU decomposition with partial pivoting (`P * A = L * U`)
/// on an `n x n` row-major matrix `lu_mat`.
///
/// On success:
/// - Strict lower triangle of `lu_mat` stores `L` (unit diagonal of `L` is implicit).
/// - Upper triangle (including diagonal) of `lu_mat` stores `U`.
/// - `piv_out[0..n]` stores the row permutation vector `P` such that `(P * A)[i, :] == A[piv[i], :]`.
///
/// Returns `null` if `n == 0`, buffers are undersized, `tol < 0` or non-finite, `lu_mat` contains
/// non-finite entries, or the matrix is singular / near-singular (`|pivot| <= tol`).
pub fn luFactorize(
    lu_mat: []f64,
    n: usize,
    piv_out: []usize,
    tol: f64,
): LuInfo? {
    if (!checkMatLen(lu_mat.len, n) || piv_out.len < n) {
        return null;
    }
    if (!isFiniteF64(tol) || tol < 0.0) {
        return null;
    }

    const total = n * n;
    let chk: usize = 0;
    while (chk < total) {
        if (!isFiniteF64(lu_mat[chk])) {
            return null;
        }
        chk += 1;
    }

    let i: usize = 0;
    while (i < n) {
        piv_out[i] = i;
        i += 1;
    }

    let sign: f64 = 1.0;
    let swaps: usize = 0;
    let min_pivot: f64 = 0.0;

    let k: usize = 0;
    while (k < n) {
        // Partial pivoting: select row r in [k, n) with maximum |lu_mat[r, k]|
        let pivot_row = k;
        let max_abs = luAbsF64(lu_mat[k * n + k]);
        let r = k + 1;
        while (r < n) {
            const cand = luAbsF64(lu_mat[r * n + k]);
            if (cand > max_abs) {
                max_abs = cand;
                pivot_row = r;
            }
            r += 1;
        }

        if (!isFiniteF64(max_abs) || max_abs <= tol) {
            return null;
        }
        if (k == 0 || max_abs < min_pivot) {
            min_pivot = max_abs;
        }

        if (pivot_row != k) {
            let c: usize = 0;
            while (c < n) {
                const idx_k = k * n + c;
                const idx_p = pivot_row * n + c;
                const tmp = lu_mat[idx_k];
                lu_mat[idx_k] = lu_mat[idx_p];
                lu_mat[idx_p] = tmp;
                c += 1;
            }
            const tmp_p = piv_out[k];
            piv_out[k] = piv_out[pivot_row];
            piv_out[pivot_row] = tmp_p;
            sign = 0.0 - sign;
            swaps += 1;
        }

        const pivot = lu_mat[k * n + k];
        const piv_off: usize = k * n;
        let row = k + 1;
        while (row < n) {
            const row_off: usize = row * n;
            const factor = lu_mat[row_off + k] / pivot;
            if (!isFiniteF64(factor)) {
                return null;
            }
            lu_mat[row_off + k] = factor;
            const v_fac: LuMathNumVec4f64 = @splat(factor);
            let col = k + 1;
            while (col + 4 <= n) : (col += 4) {
                const vr: LuMathNumVec4f64 = lu_mat[row_off + col .. row_off + col + 4][0..4].*;
                const vp: LuMathNumVec4f64 = lu_mat[piv_off + col .. piv_off + col + 4][0..4].*;
                const vu: LuMathNumVec4f64 = vr - v_fac * vp;
                if (!isFiniteF64(vu[0]) || !isFiniteF64(vu[1]) || !isFiniteF64(vu[2]) || !isFiniteF64(vu[3])) {
                    return null;
                }
                lu_mat[row_off + col .. row_off + col + 4][0..4].* = vu;
            }
            while (col < n) : (col += 1) {
                const updated = lu_mat[row_off + col] - factor * lu_mat[piv_off + col];
                if (!isFiniteF64(updated)) {
                    return null;
                }
                lu_mat[row_off + col] = updated;
            }
            row += 1;
        }

        k += 1;
    }

    return LuInfo {
        sign: sign,
        swaps: swaps,
        min_pivot: min_pivot,
    };
}

/// Solves `A * x = b` using a pre-factorized `(lu_mat, piv)` from `luFactorize`.
///
/// Does not mutate `lu_mat` or `piv`, allowing repeated solves with multiple right-hand sides `b`.
/// Returns `false` if dimensions/buffers are invalid, `piv` contains out-of-bounds indices,
/// `b` contains non-finite values, or `U` has a zero/non-finite diagonal entry.
pub fn luSolve(
    lu_mat: []const f64,
    piv: []const usize,
    n: usize,
    b: []const f64,
    x_out: []f64,
): bool {
    if (!checkMatLen(lu_mat.len, n) || piv.len < n || b.len < n || x_out.len < n) {
        return false;
    }

    // Apply row permutation: x_out[i] = b[piv[i]]
    let i: usize = 0;
    while (i < n) {
        const pr = piv[i];
        if (pr >= n || !isFiniteF64(b[pr])) {
            return false;
        }
        x_out[i] = b[pr];
        i += 1;
    }

    // Forward substitution: L * y = P * b (unit lower triangular L) with 4-lane SIMD dot product
    let fi: usize = 0;
    while (fi < n) : (fi += 1) {
        const row_off: usize = fi * n;
        let sum = x_out[fi];
        if (fi > 0) {
            const dot_opt = luSimdDotF64(lu_mat[row_off .. row_off + fi], x_out[0..fi]);
            if (dot_opt == null) return false;
            sum -= dot_opt.?;
            if (!isFiniteF64(sum)) return false;
        }
        x_out[fi] = sum;
    }

    // Back substitution: U * x = y with 4-lane SIMD dot product
    let rev = n;
    while (rev > 0) {
        rev -= 1;
        const row_off: usize = rev * n;
        let sum = x_out[rev];
        const next_col: usize = rev + 1;
        if (next_col < n) {
            const dot_opt = luSimdDotF64(lu_mat[row_off + next_col .. row_off + n], x_out[next_col..n]);
            if (dot_opt == null) return false;
            sum -= dot_opt.?;
            if (!isFiniteF64(sum)) return false;
        }
        const diag = lu_mat[row_off + rev];
        if (!isFiniteF64(diag) || diag == 0.0) {
            return false;
        }
        const val = sum / diag;
        if (!isFiniteF64(val)) {
            return false;
        }
        x_out[rev] = val;
    }

    return true;
}

/// Computes `det(A) = info.sign * prod_i U[i, i]` from a pre-factorized `lu_mat`.
/// Returns `0.0` if `n == 0`, `lu_mat` is undersized, or any diagonal entry is non-finite.
pub fn luDeterminant(lu_mat: []const f64, info: LuInfo, n: usize): f64 {
    if (!checkMatLen(lu_mat.len, n) || !isFiniteF64(info.sign)) {
        return 0.0;
    }
    let det = info.sign;
    let i: usize = 0;
    while (i < n) {
        const d = lu_mat[i * n + i];
        if (!isFiniteF64(d)) {
            return 0.0;
        }
        det = det * d;
        i += 1;
    }
    return if (isFiniteF64(det)) det else 0.0;
}

/// Computes `ln(|det(A)|) = sum_i ln(|U[i, i]|)` from a pre-factorized `lu_mat` without
/// intermediate overflow or underflow. Returns `null` if invalid, singular, or non-finite.
pub fn luLogDet(lu_mat: []const f64, info: LuInfo, n: usize): f64? {
    if (!checkMatLen(lu_mat.len, n) || !isFiniteF64(info.sign)) {
        return null;
    }
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const d = luAbsF64(lu_mat[i * n + i]);
        if (!isFiniteF64(d) || d <= 0.0) {
            return null;
        }
        acc += std.math.log(f64, std.math.e, d);
    }
    if (!isFiniteF64(acc)) {
        return null;
    }
    return acc;
}

/// Computes the explicit matrix inverse `A^-1` into row-major `inv_out[0..n*n]`
/// by solving `n` unit basis right-hand sides against `(lu_mat, piv)`.
///
/// Requires `rhs_scratch.len >= n` and `col_scratch.len >= n`.
pub fn luInverse(
    lu_mat: []const f64,
    piv: []const usize,
    n: usize,
    inv_out: []f64,
    rhs_scratch: []f64,
    col_scratch: []f64,
): bool {
    if (!checkMatLen(lu_mat.len, n) || !checkMatLen(inv_out.len, n)) {
        return false;
    }
    if (piv.len < n || rhs_scratch.len < n || col_scratch.len < n) {
        return false;
    }

    let col: usize = 0;
    while (col < n) {
        let r: usize = 0;
        while (r < n) {
            rhs_scratch[r] = if (r == col) 1.0 else 0.0;
            r += 1;
        }
        if (!luSolve(lu_mat, piv, n, rhs_scratch[0..n], col_scratch[0..n])) {
            return false;
        }
        r = 0;
        while (r < n) {
            inv_out[r * n + col] = col_scratch[r];
            r += 1;
        }
        col += 1;
    }

    return true;
}

/// Computes the infinity-norm residual `||A * x - b||_inf = max_i |(A * x)_i - b_i|`.
/// Returns `+inf` if dimensions or buffer lengths are invalid.
pub fn matVecResidualInf(
    a: []const f64,
    x: []const f64,
    b: []const f64,
    n: usize,
): f64 {
    if (!checkMatLen(a.len, n) || x.len < n || b.len < n) {
        return std.math.inf(f64);
    }
    let max_res: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const row_off: usize = i * n;
        const ax_opt = luSimdDotF64(a[row_off .. row_off + n], x[0..n]);
        if (ax_opt == null || !isFiniteF64(b[i])) {
            return std.math.inf(f64);
        }
        const err = luAbsF64(ax_opt.? - b[i]);
        if (!isFiniteF64(err)) {
            return std.math.inf(f64);
        }
        if (err > max_res) {
            max_res = err;
        }
        i += 1;
    }
    return max_res;
}

/// Solves an `n x n` tridiagonal linear system in O(n) time via the Thomas algorithm.
///
/// Diagonal layout (`n = diag.len`):
/// - `diag[0..n]`: main diagonal `b_0, ..., b_{n-1}`
/// - `lower[0..n-1]`: sub-diagonal `a_1, ..., a_{n-1}` (row `i >= 1` uses `lower[i - 1]`)
/// - `upper[0..n-1]`: super-diagonal `c_0, ..., c_{n-2}` (row `i < n - 1` uses `upper[i]`)
/// - `rhs[0..n]`: right-hand side vector
/// - `x_out[0..n]`, `c_scratch[0..n]`, `d_scratch[0..n]`: caller-owned output & scratch slices
///
/// Returns `false` if `n == 0`, any buffer is undersized, inputs contain non-finite values,
/// or a zero/near-zero pivot (`|denom| <= 1e-15`) is encountered.
pub fn solveTridiagonal(
    lower: []const f64,
    diag: []const f64,
    upper: []const f64,
    rhs: []const f64,
    x_out: []f64,
    c_scratch: []f64,
    d_scratch: []f64,
): bool {
    const n = diag.len;
    if (n == 0 || rhs.len < n || x_out.len < n || c_scratch.len < n || d_scratch.len < n) {
        return false;
    }
    if (n > 1 && (lower.len < n - 1 || upper.len < n - 1)) {
        return false;
    }

    // Validate finite inputs
    let chk: usize = 0;
    while (chk < n) {
        if (!isFiniteF64(diag[chk]) || !isFiniteF64(rhs[chk])) {
            return false;
        }
        if (chk + 1 < n) {
            if (!isFiniteF64(lower[chk]) || !isFiniteF64(upper[chk])) {
                return false;
            }
        }
        chk += 1;
    }

    const b0 = diag[0];
    if (luAbsF64(b0) <= THOMAS_PIVOT_EPS) {
        return false;
    }
    if (n == 1) {
        x_out[0] = rhs[0] / b0;
        return isFiniteF64(x_out[0]);
    }

    c_scratch[0] = upper[0] / b0;
    d_scratch[0] = rhs[0] / b0;
    if (!isFiniteF64(c_scratch[0]) || !isFiniteF64(d_scratch[0])) {
        return false;
    }

    let i: usize = 1;
    while (i < n) {
        const ai = lower[i - 1];
        const denom = diag[i] - ai * c_scratch[i - 1];
        if (!isFiniteF64(denom) || luAbsF64(denom) <= THOMAS_PIVOT_EPS) {
            return false;
        }
        if (i + 1 < n) {
            c_scratch[i] = upper[i] / denom;
            if (!isFiniteF64(c_scratch[i])) {
                return false;
            }
        } else {
            c_scratch[i] = 0.0;
        }
        d_scratch[i] = (rhs[i] - ai * d_scratch[i - 1]) / denom;
        if (!isFiniteF64(d_scratch[i])) {
            return false;
        }
        i += 1;
    }

    x_out[n - 1] = d_scratch[n - 1];
    if (!isFiniteF64(x_out[n - 1])) {
        return false;
    }
    let rev = n - 1;
    while (rev > 0) {
        rev -= 1;
        x_out[rev] = d_scratch[rev] - c_scratch[rev] * x_out[rev + 1];
        if (!isFiniteF64(x_out[rev])) {
            return false;
        }
    }

    return true;
}
