// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Doolittle Partial-Pivoting LU Decomposition, Linear Solver, Determinant, Inverse & O(n) Thomas Tridiagonal Solver)

import std from "std";

const THOMAS_PIVOT_EPS: f64 = 1.0e-15;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn checkMatLen(buf_len: usize, n: usize): bool {
    return n > 0 && (buf_len / n) >= n;
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
        let max_abs = absF64(lu_mat[k * n + k]);
        let r = k + 1;
        while (r < n) {
            const cand = absF64(lu_mat[r * n + k]);
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
        let row = k + 1;
        while (row < n) {
            const factor = lu_mat[row * n + k] / pivot;
            if (!isFiniteF64(factor)) {
                return null;
            }
            lu_mat[row * n + k] = factor;
            let col = k + 1;
            while (col < n) {
                lu_mat[row * n + col] = lu_mat[row * n + col] - factor * lu_mat[k * n + col];
                col += 1;
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

    // Forward substitution: L * y = P * b (unit lower triangular L)
    i = 0;
    while (i < n) {
        let sum = x_out[i];
        let j: usize = 0;
        while (j < i) {
            sum = sum - lu_mat[i * n + j] * x_out[j];
            j += 1;
        }
        x_out[i] = sum;
        i += 1;
    }

    // Back substitution: U * x = y
    let rev = n;
    while (rev > 0) {
        rev -= 1;
        let sum = x_out[rev];
        let j = rev + 1;
        while (j < n) {
            sum = sum - lu_mat[rev * n + j] * x_out[j];
            j += 1;
        }
        const diag = lu_mat[rev * n + rev];
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
/// Returns `0.0` if `n == 0` or `lu_mat` is undersized.
pub fn luDeterminant(lu_mat: []const f64, info: LuInfo, n: usize): f64 {
    if (!checkMatLen(lu_mat.len, n)) {
        return 0.0;
    }
    let det = info.sign;
    let i: usize = 0;
    while (i < n) {
        det = det * lu_mat[i * n + i];
        i += 1;
    }
    return det;
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
        let ax: f64 = 0.0;
        let j: usize = 0;
        while (j < n) {
            ax = ax + a[i * n + j] * x[j];
            j += 1;
        }
        const err = absF64(ax - b[i]);
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
    if (absF64(b0) <= THOMAS_PIVOT_EPS) {
        return false;
    }
    if (n == 1) {
        x_out[0] = rhs[0] / b0;
        return isFiniteF64(x_out[0]);
    }

    c_scratch[0] = upper[0] / b0;
    d_scratch[0] = rhs[0] / b0;

    let i: usize = 1;
    while (i < n) {
        const ai = lower[i - 1];
        const denom = diag[i] - ai * c_scratch[i - 1];
        if (!isFiniteF64(denom) || absF64(denom) <= THOMAS_PIVOT_EPS) {
            return false;
        }
        if (i + 1 < n) {
            c_scratch[i] = upper[i] / denom;
        } else {
            c_scratch[i] = 0.0;
        }
        d_scratch[i] = (rhs[i] - ai * d_scratch[i - 1]) / denom;
        i += 1;
    }

    x_out[n - 1] = d_scratch[n - 1];
    let rev = n - 1;
    while (rev > 0) {
        rev -= 1;
        x_out[rev] = d_scratch[rev] - c_scratch[rev] * x_out[rev + 1];
    }

    return true;
}
