// Origin: Pure-CP Stdlib (Unbounded Zero-Heap Dense Matrix Engine: SIMD GEMM, LU, Cholesky, Householder QR, Jacobi Eigen & One-Sided Jacobi Thin SVD) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, caller-slice-driven dense linear algebra engine:
//   - Row-major unbounded views (`MatrixViewF64`, `MatrixMutF64`, `MatrixViewF32`, `MatrixMutF32`)
//   - Transposed-B SIMD `@dot` GEMM (`matmulF32Simd`, `matvecF32Simd`, `matmulF64`, `matvecF64`)
//   - Partial-pivoting LU (`luDecomposeInPlace`, `denseLuSolve`, `denseLuDeterminant`, `luInvert`)
//   - SPD Cholesky (`choleskyDecomposeInPlace`, `denseCholeskySolve`, `denseCholeskyLogDet`)
//   - Householder QR & Overdetermined Least Squares (`qrDecomposeHouseholder`, `denseQrSolveLeastSquares`)
//   - Cyclic Jacobi Symmetric Eigenvalue Decomposition (`eigenSymmetricJacobi`)
//   - High-precision One-Sided Jacobi (Hestenes) Thin SVD (`svdThin`, `svdThinPinv`, `svdCondNumber`)

const std = @import("std");

// ============================================================================
// 1. Error Set, Helpers & Zero-Heap Matrix Views (F64 / F32)
// ============================================================================

/// Error set returned by dense matrix operations and decompositions.
pub err MatrixError {
    InvalidDimensions,
    DimensionMismatch,
    NotSquare,
    BufferTooSmall,
    SingularMatrix,
    NotPositiveDefinite,
    DidNotConverge,
    NonFiniteValue,
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Read-only row-major dense `f64` matrix view backed by caller's slice.
pub struct MatrixViewF64 {
    data: []const f64 = [];
    rows: usize = 0;
    cols: usize = 0;

    /// Constructs a validated `MatrixViewF64` checking `rows > 0`, `cols > 0`, and `data.len >= rows * cols`.
    static fn init(data: []const f64, rows: usize, cols: usize): MatrixViewF64 ! MatrixError {
        if (rows == 0 || cols == 0) return MatrixError.InvalidDimensions;
        if (data.len < rows * cols) return MatrixError.BufferTooSmall;
        return MatrixViewF64{ data: data[0..rows * cols], rows: rows, cols: cols };
    }

    fn at(r: usize, c: usize): f64 {
        return data[r * cols + c];
    }

    fn row(r: usize): []const f64 {
        const off: usize = r * cols;
        return data[off..off + cols];
    }

    fn isSquare(): bool {
        return rows == cols;
    }
}

/// Mutable row-major dense `f64` matrix view backed by caller's slice.
pub struct MatrixMutF64 {
    data: []f64 = [];
    rows: usize = 0;
    cols: usize = 0;

    /// Constructs a validated `MatrixMutF64` checking `rows > 0`, `cols > 0`, and `data.len >= rows * cols`.
    static fn init(data: []f64, rows: usize, cols: usize): MatrixMutF64 ! MatrixError {
        if (rows == 0 || cols == 0) return MatrixError.InvalidDimensions;
        if (data.len < rows * cols) return MatrixError.BufferTooSmall;
        return MatrixMutF64{ data: data[0..rows * cols], rows: rows, cols: cols };
    }

    fn asView(): MatrixViewF64 {
        return MatrixViewF64{ data: data, rows: rows, cols: cols };
    }

    fn at(r: usize, c: usize): f64 {
        return data[r * cols + c];
    }

    fn set(*_, r: usize, c: usize, val: f64) {
        _.data[r * _.cols + c] = val;
    }

    fn isSquare(): bool {
        return rows == cols;
    }

    fn fill(*_, val: f64) {
        let i: usize = 0;
        while (i < _.data.len) : (i += 1) {
            _.data[i] = val;
        }
    }
}

/// Read-only row-major dense `f32` matrix view backed by caller's slice.
pub struct MatrixViewF32 {
    data: []const f32 = [];
    rows: usize = 0;
    cols: usize = 0;

    static fn init(data: []const f32, rows: usize, cols: usize): MatrixViewF32 ! MatrixError {
        if (rows == 0 || cols == 0) return MatrixError.InvalidDimensions;
        if (data.len < rows * cols) return MatrixError.BufferTooSmall;
        return MatrixViewF32{ data: data[0..rows * cols], rows: rows, cols: cols };
    }

    fn at(r: usize, c: usize): f32 {
        return data[r * cols + c];
    }

    fn row(r: usize): []const f32 {
        const off: usize = r * cols;
        return data[off..off + cols];
    }
}

/// Mutable row-major dense `f32` matrix view backed by caller's slice.
pub struct MatrixMutF32 {
    data: []f32 = [];
    rows: usize = 0;
    cols: usize = 0;

    static fn init(data: []f32, rows: usize, cols: usize): MatrixMutF32 ! MatrixError {
        if (rows == 0 || cols == 0) return MatrixError.InvalidDimensions;
        if (data.len < rows * cols) return MatrixError.BufferTooSmall;
        return MatrixMutF32{ data: data[0..rows * cols], rows: rows, cols: cols };
    }

    fn asView(): MatrixViewF32 {
        return MatrixViewF32{ data: data, rows: rows, cols: cols };
    }

    fn at(r: usize, c: usize): f32 {
        return data[r * cols + c];
    }

    fn set(*_, r: usize, c: usize, val: f32) {
        _.data[r * _.cols + c] = val;
    }
}

// ============================================================================
// 2. Basic Matrix Utilities & SIMD `@dot` GEMM / GEMV
// ============================================================================

/// Sets a square `MatrixMutF64` to the identity matrix `I_n`.
pub fn matrixIdentityF64(m: *MatrixMutF64): void ! MatrixError {
    if (!m.isSquare()) return MatrixError.NotSquare;
    m.fill(0.0);
    let i: usize = 0;
    while (i < m.rows) : (i += 1) {
        m.set(i, i, 1.0);
    }
}

/// Computes `dst = src^T` for `f64` matrices.
pub fn transposeMatrixF64(src: MatrixViewF64, dst: *MatrixMutF64): void ! MatrixError {
    if (src.rows != dst.cols || src.cols != dst.rows) return MatrixError.DimensionMismatch;
    let r: usize = 0;
    while (r < src.rows) : (r += 1) {
        let c: usize = 0;
        while (c < src.cols) : (c += 1) {
            dst.set(c, r, src.at(r, c));
        }
    }
}

/// Computes `dst = src^T` for `f32` matrices.
pub fn transposeMatrixF32(src: MatrixViewF32, dst: *MatrixMutF32): void ! MatrixError {
    if (src.rows != dst.cols || src.cols != dst.rows) return MatrixError.DimensionMismatch;
    let r: usize = 0;
    while (r < src.rows) : (r += 1) {
        let c: usize = 0;
        while (c < src.cols) : (c += 1) {
            dst.set(c, r, src.at(r, c));
        }
    }
}

/// Returns the trace `sum_i A[i, i]` of a square `f64` matrix.
pub fn matrixTraceF64(a: MatrixViewF64): f64 ! MatrixError {
    if (!a.isSquare()) return MatrixError.NotSquare;
    let tr: f64 = 0.0;
    let i: usize = 0;
    while (i < a.rows) : (i += 1) {
        tr += a.at(i, i);
    }
    return tr;
}

/// Computes the Frobenius norm `sqrt(sum_{r,c} A[r,c]^2)` of a `MatrixViewF64`.
pub fn frobeniusNormF64(a: MatrixViewF64): f64 {
    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < a.data.len) : (i += 1) {
        const v: f64 = a.data[i];
        sum_sq += v * v;
    }
    return std.math.sqrt(sum_sq);
}

/// Computes the Frobenius norm of a `MatrixViewF32` using hardware SIMD `@dot`.
pub fn frobeniusNormF32(a: MatrixViewF32): f64 {
    if (a.data.len == 0) return 0.0;
    const sum_sq: f64 = @as(f64, @dot(a.data, a.data));
    return std.math.sqrt(if (sum_sq > 0.0) sum_sq else 0.0);
}

/// Computes `||A - B||_F` for two `f64` matrices of identical dimensions.
pub fn matrixDiffFrobeniusF64(a: MatrixViewF64, b: MatrixViewF64): f64 ! MatrixError {
    if (a.rows != b.rows || a.cols != b.cols) return MatrixError.DimensionMismatch;
    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < a.data.len) : (i += 1) {
        const d: f64 = a.data[i] - b.data[i];
        sum_sq += d * d;
    }
    return std.math.sqrt(sum_sq);
}

/// Computes `y = alpha * A * x + beta * y` for `f32` using hardware SIMD `@dot` per row.
pub fn matvecF32Simd(
    a: MatrixViewF32,
    x: []const f32,
    alpha: f32,
    beta: f32,
    y: *[]f32,
): void ! MatrixError {
    if (x.len < a.cols || y.len < a.rows) return MatrixError.DimensionMismatch;
    const x_s: []const f32 = x[0..a.cols];
    let r: usize = 0;
    while (r < a.rows) : (r += 1) {
        const dot_val: f32 = @dot(a.row(r), x_s);
        y[r] = if (beta == 0.0) (alpha * dot_val) else (alpha * dot_val + beta * y[r]);
    }
}

/// Computes `y = alpha * A * x + beta * y` for `f64`.
pub fn matvecF64(
    a: MatrixViewF64,
    x: []const f64,
    alpha: f64,
    beta: f64,
    y: *[]f64,
): void ! MatrixError {
    if (x.len < a.cols || y.len < a.rows) return MatrixError.DimensionMismatch;
    let r: usize = 0;
    while (r < a.rows) : (r += 1) {
        const row_off: usize = r * a.cols;
        let sum: f64 = 0.0;
        let c: usize = 0;
        while (c < a.cols) : (c += 1) {
            sum += a.data[row_off + c] * x[c];
        }
        y[r] = if (beta == 0.0) (alpha * sum) else (alpha * sum + beta * y[r]);
    }
}

/// Computes `C = alpha * A * B + beta * C` (`M x K` by `K x N` -> `M x N`) using transposed-B
/// layout in `b_trans_work` (`len >= K * N`) and native SIMD `@dot` inner products.
pub fn matmulF32Simd(
    a: MatrixViewF32,
    b: MatrixViewF32,
    alpha: f32,
    beta: f32,
    c: *MatrixMutF32,
    b_trans_work: *[]f32,
): void ! MatrixError {
    if (a.cols != b.rows || a.rows != c.rows || b.cols != c.cols) {
        return MatrixError.DimensionMismatch;
    }
    const m: usize = a.rows;
    const k: usize = a.cols;
    const n: usize = b.cols;
    if (b_trans_work.len < k * n) return MatrixError.BufferTooSmall;

    let bt: MatrixMutF32 = try MatrixMutF32.init(b_trans_work.*, n, k);
    try transposeMatrixF32(b, *bt);
    const bt_view: MatrixViewF32 = bt.asView();

    let i: usize = 0;
    while (i < m) : (i += 1) {
        const a_row: []const f32 = a.row(i);
        const c_off: usize = i * n;
        let j: usize = 0;
        while (j < n) : (j += 1) {
            const dot_val: f32 = @dot(a_row, bt_view.row(j));
            c.data[c_off + j] = if (beta == 0.0)
                (alpha * dot_val)
            else
                (alpha * dot_val + beta * c.data[c_off + j]);
        }
    }
}

/// Computes `C = alpha * A * B + beta * C` (`f64`) using transposed-B layout in `b_trans_work`
/// (`len >= K * N`) with 4-way unrolled inner accumulation.
pub fn matmulF64(
    a: MatrixViewF64,
    b: MatrixViewF64,
    alpha: f64,
    beta: f64,
    c: *MatrixMutF64,
    b_trans_work: *[]f64,
): void ! MatrixError {
    if (a.cols != b.rows || a.rows != c.rows || b.cols != c.cols) {
        return MatrixError.DimensionMismatch;
    }
    const m: usize = a.rows;
    const k: usize = a.cols;
    const n: usize = b.cols;
    if (b_trans_work.len < k * n) return MatrixError.BufferTooSmall;

    let bt: MatrixMutF64 = try MatrixMutF64.init(b_trans_work.*, n, k);
    try transposeMatrixF64(b, *bt);

    let i: usize = 0;
    while (i < m) : (i += 1) {
        const a_off: usize = i * k;
        const c_off: usize = i * n;
        let j: usize = 0;
        while (j < n) : (j += 1) {
            const bt_off: usize = j * k;
            let s0: f64 = 0.0;
            let s1: f64 = 0.0;
            let s2: f64 = 0.0;
            let s3: f64 = 0.0;
            let t: usize = 0;
            while (t + 4 <= k) : (t += 4) {
                s0 += a.data[a_off + t + 0] * bt.data[bt_off + t + 0];
                s1 += a.data[a_off + t + 1] * bt.data[bt_off + t + 1];
                s2 += a.data[a_off + t + 2] * bt.data[bt_off + t + 2];
                s3 += a.data[a_off + t + 3] * bt.data[bt_off + t + 3];
            }
            let sum: f64 = (s0 + s1) + (s2 + s3);
            while (t < k) : (t += 1) {
                sum += a.data[a_off + t] * bt.data[bt_off + t];
            }
            c.data[c_off + j] = if (beta == 0.0)
                (alpha * sum)
            else
                (alpha * sum + beta * c.data[c_off + j]);
        }
    }
}

// ============================================================================
// 3. Partial-Pivoting LU Factorization (PA = LU), Linear Solver, Det & Inverse
// ============================================================================

/// Summary returned by `luDecomposeInPlace`.
pub struct LuFactorResult {
    sign: i32 = 1;
    swaps: usize = 0;
}

/// Computes in-place Doolittle LU factorization `P * A = L * U` with partial pivoting.
/// On success, `L` (unit diagonal implicit) is stored in the strict lower triangle of `a`
/// and `U` in the upper triangle; `piv[0..n]` stores the row permutation.
pub fn luDecomposeInPlace(a: *MatrixMutF64, piv: *[]usize): LuFactorResult ! MatrixError {
    if (!a.isSquare()) return MatrixError.NotSquare;
    const n: usize = a.rows;
    if (piv.len < n) return MatrixError.BufferTooSmall;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        piv[i] = i;
    }

    let sign: i32 = 1;
    let swaps: usize = 0;

    let k: usize = 0;
    while (k < n) : (k += 1) {
        // Select largest pivot in column k on or below diagonal
        let piv_row: usize = k;
        let max_abs: f64 = absF64(a.at(k, k));
        let r: usize = k + 1;
        while (r < n) : (r += 1) {
            const v: f64 = absF64(a.at(r, k));
            if (v > max_abs) {
                max_abs = v;
                piv_row = r;
            }
        }
        if (!isFiniteF64(max_abs) || max_abs <= 1.0e-14) {
            return MatrixError.SingularMatrix;
        }

        if (piv_row != k) {
            // Swap rows k and piv_row across all columns
            let c: usize = 0;
            while (c < n) : (c += 1) {
                const tmp: f64 = a.at(k, c);
                a.set(k, c, a.at(piv_row, c));
                a.set(piv_row, c, tmp);
            }
            const tmp_p: usize = piv[k];
            piv[k] = piv[piv_row];
            piv[piv_row] = tmp_p;
            sign = -sign;
            swaps += 1;
        }

        const diag: f64 = a.at(k, k);
        r = k + 1;
        while (r < n) : (r += 1) {
            const mult: f64 = a.at(r, k) / diag;
            a.set(r, k, mult);
            let c: usize = k + 1;
            while (c < n) : (c += 1) {
                a.set(r, c, a.at(r, c) - mult * a.at(k, c));
            }
        }
    }

    return LuFactorResult{ sign: sign, swaps: swaps };
}

/// Solves `A * x = b` using an LU factorization produced by `luDecomposeInPlace`.
pub fn denseLuSolve(
    lu: MatrixViewF64,
    piv: []const usize,
    b: []const f64,
    x: *[]f64,
): void ! MatrixError {
    if (!lu.isSquare()) return MatrixError.NotSquare;
    const n: usize = lu.rows;
    if (piv.len < n || b.len < n || x.len < n) return MatrixError.DimensionMismatch;

    // Forward substitution: L * y = P * b (stored directly in x)
    let i: usize = 0;
    while (i < n) : (i += 1) {
        let sum: f64 = b[piv[i]];
        let j: usize = 0;
        while (j < i) : (j += 1) {
            sum -= lu.at(i, j) * x[j];
        }
        x[i] = sum;
    }

    // Back substitution: U * x = y
    let idx: usize = n;
    while (idx > 0) {
        idx -= 1;
        let sum: f64 = x[idx];
        let j: usize = idx + 1;
        while (j < n) : (j += 1) {
            sum -= lu.at(idx, j) * x[j];
        }
        const diag: f64 = lu.at(idx, idx);
        if (absF64(diag) <= 1.0e-14) return MatrixError.SingularMatrix;
        x[idx] = sum / diag;
    }
}

/// Computes `det(A)` from an LU factorization and its permutation `sign`.
pub fn denseLuDeterminant(lu: MatrixViewF64, sign: i32): f64 ! MatrixError {
    if (!lu.isSquare()) return MatrixError.NotSquare;
    let det: f64 = @as(f64, @floatFromInt(sign));
    let i: usize = 0;
    while (i < lu.rows) : (i += 1) {
        det *= lu.at(i, i);
    }
    return det;
}

/// Computes the explicit matrix inverse `A^{-1}` into `inv_out` using LU factorization
/// and caller-supplied scratch `col_work` (`len >= 2 * n`).
pub fn luInvert(
    lu: MatrixViewF64,
    piv: []const usize,
    inv_out: *MatrixMutF64,
    col_work: *[]f64,
): void ! MatrixError {
    if (!lu.isSquare() || !inv_out.isSquare()) return MatrixError.NotSquare;
    const n: usize = lu.rows;
    if (inv_out.rows != n || piv.len < n) return MatrixError.DimensionMismatch;
    if (col_work.len < 2 * n) return MatrixError.BufferTooSmall;

    let rhs: []f64 = col_work.*[0..n];
    let sol: []f64 = col_work.*[n..2 * n];

    let col: usize = 0;
    while (col < n) : (col += 1) {
        let r: usize = 0;
        while (r < n) : (r += 1) {
            rhs[r] = if (r == col) 1.0 else 0.0;
        }
        try denseLuSolve(lu, piv, rhs, *sol);
        r = 0;
        while (r < n) : (r += 1) {
            inv_out.set(r, col, sol[r]);
        }
    }
}

// ============================================================================
// 4. Cholesky Factorization (A = L * L^T) for Symmetric Positive Definite Matrices
// ============================================================================

/// Computes in-place Cholesky factorization `A = L * L^T` for an SPD matrix.
/// Zeroes the strict upper triangle; returns `MatrixError.NotPositiveDefinite` if any pivot <= 1e-14.
pub fn choleskyDecomposeInPlace(a: *MatrixMutF64): void ! MatrixError {
    if (!a.isSquare()) return MatrixError.NotSquare;
    const n: usize = a.rows;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        let j: usize = 0;
        while (j <= i) : (j += 1) {
            let sum: f64 = a.at(i, j);
            let k: usize = 0;
            while (k < j) : (k += 1) {
                sum -= a.at(i, k) * a.at(j, k);
            }
            if (i == j) {
                if (!isFiniteF64(sum) || sum <= 1.0e-14) {
                    return MatrixError.NotPositiveDefinite;
                }
                a.set(i, i, std.math.sqrt(sum));
            } else {
                const ljj: f64 = a.at(j, j);
                a.set(i, j, sum / ljj);
            }
        }
        // Clear strict upper triangle
        let u: usize = i + 1;
        while (u < n) : (u += 1) {
            a.set(i, u, 0.0);
        }
    }
}

/// Solves `L * L^T * x = b` using a lower-triangular Cholesky factor `l_mat`.
pub fn denseCholeskySolve(l_mat: MatrixViewF64, b: []const f64, x: *[]f64): void ! MatrixError {
    if (!l_mat.isSquare()) return MatrixError.NotSquare;
    const n: usize = l_mat.rows;
    if (b.len < n || x.len < n) return MatrixError.DimensionMismatch;

    // Forward solve: L * y = b
    let i: usize = 0;
    while (i < n) : (i += 1) {
        let sum: f64 = b[i];
        let j: usize = 0;
        while (j < i) : (j += 1) {
            sum -= l_mat.at(i, j) * x[j];
        }
        const diag: f64 = l_mat.at(i, i);
        if (diag <= 1.0e-14) return MatrixError.NotPositiveDefinite;
        x[i] = sum / diag;
    }

    // Backward solve: L^T * x = y
    let idx: usize = n;
    while (idx > 0) {
        idx -= 1;
        let sum: f64 = x[idx];
        let j: usize = idx + 1;
        while (j < n) : (j += 1) {
            sum -= l_mat.at(j, idx) * x[j];
        }
        x[idx] = sum / l_mat.at(idx, idx);
    }
}

/// Computes `ln(det(A)) = 2 * sum_i ln(L[i, i])` from a Cholesky factor `l_mat`.
pub fn denseCholeskyLogDet(l_mat: MatrixViewF64): f64 ! MatrixError {
    if (!l_mat.isSquare()) return MatrixError.NotSquare;
    let sum_ln: f64 = 0.0;
    let i: usize = 0;
    while (i < l_mat.rows) : (i += 1) {
        const d: f64 = l_mat.at(i, i);
        if (d <= 0.0 || !isFiniteF64(d)) return MatrixError.NotPositiveDefinite;
        sum_ln += std.math.log(f64, std.math.e, d);
    }
    return 2.0 * sum_ln;
}

// ============================================================================
// 5. Householder QR Decomposition & Linear Least Squares Solver
// ============================================================================

/// Computes full Householder QR decomposition `A = Q * R` (`A: M x N` with `M >= N`,
/// `Q: M x M` orthogonal, `R: M x N` upper triangular) using caller scratch `v_work` (`len >= M`).
pub fn qrDecomposeHouseholder(
    a: MatrixViewF64,
    q_out: *MatrixMutF64,
    r_out: *MatrixMutF64,
    v_work: *[]f64,
): void ! MatrixError {
    const m: usize = a.rows;
    const n: usize = a.cols;
    if (m < n) return MatrixError.InvalidDimensions;
    if (q_out.rows != m || q_out.cols != m || r_out.rows != m || r_out.cols != n) {
        return MatrixError.DimensionMismatch;
    }
    if (v_work.len < m) return MatrixError.BufferTooSmall;

    // Copy A -> R, initialize Q = I_m
    let i: usize = 0;
    while (i < a.data.len) : (i += 1) {
        r_out.data[i] = a.data[i];
    }
    try matrixIdentityF64(q_out);

    let k: usize = 0;
    while (k < n) : (k += 1) {
        // Compute norm of column k from row k..m
        let norm_sq: f64 = 0.0;
        let r: usize = k;
        while (r < m) : (r += 1) {
            const val: f64 = r_out.at(r, k);
            v_work[r] = val;
            norm_sq += val * val;
        }
        const norm_x: f64 = std.math.sqrt(norm_sq);
        if (norm_x <= 1.0e-15) continue;

        const x0: f64 = v_work[k];
        const alpha: f64 = if (x0 >= 0.0) -norm_x else norm_x;
        v_work[k] = x0 - alpha;

        let v_norm_sq: f64 = 0.0;
        r = k;
        while (r < m) : (r += 1) {
            v_norm_sq += v_work[r] * v_work[r];
        }
        if (v_norm_sq <= 1.0e-30) continue;
        const beta: f64 = 2.0 / v_norm_sq;

        // Apply H = I - beta * v * v^T from the left to R[k..m, k..n]
        let c: usize = k;
        while (c < n) : (c += 1) {
            let dot_vc: f64 = 0.0;
            r = k;
            while (r < m) : (r += 1) {
                dot_vc += v_work[r] * r_out.at(r, c);
            }
            const scale: f64 = beta * dot_vc;
            r = k;
            while (r < m) : (r += 1) {
                r_out.set(r, c, r_out.at(r, c) - scale * v_work[r]);
            }
        }
        // Clean sub-diagonal noise in column k
        r = k + 1;
        while (r < m) : (r += 1) {
            r_out.set(r, k, 0.0);
        }

        // Accumulate Q <- Q * H on columns k..m
        let qr: usize = 0;
        while (qr < m) : (qr += 1) {
            let dot_qv: f64 = 0.0;
            let qc: usize = k;
            while (qc < m) : (qc += 1) {
                dot_qv += q_out.at(qr, qc) * v_work[qc];
            }
            const scale: f64 = beta * dot_qv;
            qc = k;
            while (qc < m) : (qc += 1) {
                q_out.set(qr, qc, q_out.at(qr, qc) - scale * v_work[qc]);
            }
        }
    }
}

/// Solves the overdetermined linear least squares problem `min_x ||A*x - b||_2`
/// given `Q` (`M x M`) and `R` (`M x N`) from `qrDecomposeHouseholder`, using `qtb_work` (`len >= M`).
pub fn denseQrSolveLeastSquares(
    q: MatrixViewF64,
    r: MatrixViewF64,
    b: []const f64,
    x_out: *[]f64,
    qtb_work: *[]f64,
): void ! MatrixError {
    const m: usize = r.rows;
    const n: usize = r.cols;
    if (m < n || q.rows != m || q.cols != m) return MatrixError.DimensionMismatch;
    if (b.len < m || x_out.len < n || qtb_work.len < m) return MatrixError.BufferTooSmall;

    // y = Q^T * b (only first n entries needed for R[0..n, 0..n] * x = y[0..n])
    let j: usize = 0;
    while (j < n) : (j += 1) {
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < m) : (i += 1) {
            sum += q.at(i, j) * b[i];
        }
        qtb_work[j] = sum;
    }

    // Back substitution on upper-triangular R[0..n, 0..n]
    let idx: usize = n;
    while (idx > 0) {
        idx -= 1;
        let sum: f64 = qtb_work[idx];
        let c: usize = idx + 1;
        while (c < n) : (c += 1) {
            sum -= r.at(idx, c) * x_out[c];
        }
        const diag: f64 = r.at(idx, idx);
        if (absF64(diag) <= 1.0e-13) return MatrixError.SingularMatrix;
        x_out[idx] = sum / diag;
    }
}

// ============================================================================
// 6. Symmetric Cyclic Jacobi Eigenvalue Decomposition (A * V = V * Lambda)
// ============================================================================

/// Convergence summary for `eigenSymmetricJacobi`.
pub struct EigenJacobiResult {
    sweeps: usize = 0;
    rotations: usize = 0;
    max_offdiag: f64 = 0.0;
    converged: bool = false;
}

/// Computes all eigenvalues (sorted descending) and orthonormal eigenvectors (columns of `eigvecs`)
/// of a real symmetric matrix `a` (`N x N`) via cyclic Jacobi Givens rotations.
/// Requires `eigvals.len >= N`, `eigvecs: N x N`, and `a_work: N x N`.
pub fn eigenSymmetricJacobi(
    a: MatrixViewF64,
    max_sweeps: usize,
    tol: f64,
    eigvals: *[]f64,
    eigvecs: *MatrixMutF64,
    a_work: *MatrixMutF64,
): EigenJacobiResult ! MatrixError {
    if (!a.isSquare()) return MatrixError.NotSquare;
    const n: usize = a.rows;
    if (eigvals.len < n || eigvecs.rows != n || eigvecs.cols != n || a_work.rows != n || a_work.cols != n) {
        return MatrixError.DimensionMismatch;
    }

    // Copy symmetrized A into a_work and initialize V = I_n
    let r: usize = 0;
    while (r < n) : (r += 1) {
        let c: usize = 0;
        while (c < n) : (c += 1) {
            a_work.set(r, c, 0.5 * (a.at(r, c) + a.at(c, r)));
        }
    }
    try matrixIdentityF64(eigvecs);

    const eff_sweeps: usize = if (max_sweeps == 0) 64 else max_sweeps;
    const eff_tol: f64 = if (tol <= 0.0) 1.0e-12 else tol;
    let total_rots: usize = 0;
    let sweep: usize = 0;
    let max_off: f64 = 0.0;
    let converged: bool = false;

    while (sweep < eff_sweeps) : (sweep += 1) {
        max_off = 0.0;
        let p: usize = 0;
        while (p < n) : (p += 1) {
            let q: usize = p + 1;
            while (q < n) : (q += 1) {
                const apq: f64 = a_work.at(p, q);
                const a_abs: f64 = absF64(apq);
                if (a_abs > max_off) max_off = a_abs;
                if (a_abs <= eff_tol) continue;

                const app: f64 = a_work.at(p, p);
                const aqq: f64 = a_work.at(q, q);
                const tau: f64 = (aqq - app) / (2.0 * apq);
                const t: f64 = if (tau >= 0.0)
                    1.0 / (tau + std.math.sqrt(1.0 + tau * tau))
                else
                    -1.0 / (-tau + std.math.sqrt(1.0 + tau * tau));
                const c: f64 = 1.0 / std.math.sqrt(1.0 + t * t);
                const s: f64 = t * c;

                a_work.set(p, p, app - t * apq);
                a_work.set(q, q, aqq + t * apq);
                a_work.set(p, q, 0.0);
                a_work.set(q, p, 0.0);

                let k: usize = 0;
                while (k < n) : (k += 1) {
                    if (k == p || k == q) continue;
                    const akp: f64 = a_work.at(k, p);
                    const akq: f64 = a_work.at(k, q);
                    const nkp: f64 = c * akp - s * akq;
                    const nkq: f64 = s * akp + c * akq;
                    a_work.set(k, p, nkp);
                    a_work.set(p, k, nkp);
                    a_work.set(k, q, nkq);
                    a_work.set(q, k, nkq);
                }

                k = 0;
                while (k < n) : (k += 1) {
                    const vkp: f64 = eigvecs.at(k, p);
                    const vkq: f64 = eigvecs.at(k, q);
                    eigvecs.set(k, p, c * vkp - s * vkq);
                    eigvecs.set(k, q, s * vkp + c * vkq);
                }
                total_rots += 1;
            }
        }
        if (max_off <= eff_tol) {
            converged = true;
            sweep += 1;
            break;
        }
    }

    // Extract eigenvalues and sort descending (swapping corresponding eigenvector columns)
    let ei: usize = 0;
    while (ei < n) : (ei += 1) {
        eigvals[ei] = a_work.at(ei, ei);
    }
    let si: usize = 0;
    while (si < n) : (si += 1) {
        let best: usize = si;
        let sj: usize = si + 1;
        while (sj < n) : (sj += 1) {
            if (eigvals[sj] > eigvals[best]) best = sj;
        }
        if (best != si) {
            const tv: f64 = eigvals[si];
            eigvals[si] = eigvals[best];
            eigvals[best] = tv;
            let row: usize = 0;
            while (row < n) : (row += 1) {
                const vc: f64 = eigvecs.at(row, si);
                eigvecs.set(row, si, eigvecs.at(row, best));
                eigvecs.set(row, best, vc);
            }
        }
    }

    return EigenJacobiResult{
        sweeps: sweep,
        rotations: total_rots,
        max_offdiag: max_off,
        converged: converged,
    };
}

// ============================================================================
// 7. One-Sided Jacobi (Hestenes) Thin SVD, Pseudoinverse & Condition Number
// ============================================================================

/// Convergence summary for `svdThin`.
pub struct SvdThinResult {
    sweeps: usize = 0;
    rotations: usize = 0;
    converged: bool = false;
    rank: usize = 0;
}

/// Computes the high-precision Thin SVD `A = U * diag(S) * V^T` (`A: M x N` with `M >= N`,
/// `U: M x N` orthonormal columns, `S: [N]f64` descending singular values, `V: N x N` orthogonal)
/// via One-Sided Jacobi (Hestenes) column orthogonalization (avoids squaring condition number).
pub fn svdThin(
    a: MatrixViewF64,
    max_sweeps: usize,
    tol: f64,
    u_out: *MatrixMutF64,
    s_out: *[]f64,
    v_out: *MatrixMutF64,
): SvdThinResult ! MatrixError {
    const m: usize = a.rows;
    const n: usize = a.cols;
    if (m < n) return MatrixError.InvalidDimensions;
    if (u_out.rows != m || u_out.cols != n || s_out.len < n || v_out.rows != n || v_out.cols != n) {
        return MatrixError.DimensionMismatch;
    }

    // Initialize U = A, V = I_n
    let i: usize = 0;
    while (i < a.data.len) : (i += 1) {
        u_out.data[i] = a.data[i];
    }
    try matrixIdentityF64(v_out);

    const eff_sweeps: usize = if (max_sweeps == 0) 64 else max_sweeps;
    const eff_tol: f64 = if (tol <= 0.0) 1.0e-12 else tol;
    let total_rots: usize = 0;
    let sweep: usize = 0;
    let converged: bool = false;

    while (sweep < eff_sweeps) : (sweep += 1) {
        let max_cos: f64 = 0.0;
        let p: usize = 0;
        while (p < n) : (p += 1) {
            let q: usize = p + 1;
            while (q < n) : (q += 1) {
                let app: f64 = 0.0;
                let aqq: f64 = 0.0;
                let apq: f64 = 0.0;
                let r: usize = 0;
                while (r < m) : (r += 1) {
                    const up: f64 = u_out.at(r, p);
                    const uq: f64 = u_out.at(r, q);
                    app += up * up;
                    aqq += uq * uq;
                    apq += up * uq;
                }
                const denom: f64 = std.math.sqrt(app * aqq);
                if (denom <= 1.0e-30) continue;
                const cos_ang: f64 = absF64(apq) / denom;
                if (cos_ang > max_cos) max_cos = cos_ang;
                if (cos_ang <= eff_tol) continue;

                const tau: f64 = (aqq - app) / (2.0 * apq);
                const t: f64 = if (tau >= 0.0)
                    1.0 / (tau + std.math.sqrt(1.0 + tau * tau))
                else
                    -1.0 / (-tau + std.math.sqrt(1.0 + tau * tau));
                const c: f64 = 1.0 / std.math.sqrt(1.0 + t * t);
                const s: f64 = t * c;

                // Rotate columns p and q of U
                r = 0;
                while (r < m) : (r += 1) {
                    const up: f64 = u_out.at(r, p);
                    const uq: f64 = u_out.at(r, q);
                    u_out.set(r, p, c * up - s * uq);
                    u_out.set(r, q, s * up + c * uq);
                }
                // Rotate columns p and q of V
                let vr: usize = 0;
                while (vr < n) : (vr += 1) {
                    const vp: f64 = v_out.at(vr, p);
                    const vq: f64 = v_out.at(vr, q);
                    v_out.set(vr, p, c * vp - s * vq);
                    v_out.set(vr, q, s * vp + c * vq);
                }
                total_rots += 1;
            }
        }
        if (max_cos <= eff_tol) {
            converged = true;
            sweep += 1;
            break;
        }
    }

    // Extract singular values (column norms of U) and normalize U columns
    let j: usize = 0;
    while (j < n) : (j += 1) {
        let norm_sq: f64 = 0.0;
        let r: usize = 0;
        while (r < m) : (r += 1) {
            const v: f64 = u_out.at(r, j);
            norm_sq += v * v;
        }
        const sigma: f64 = std.math.sqrt(norm_sq);
        s_out[j] = sigma;
        if (sigma > 1.0e-15) {
            const inv_s: f64 = 1.0 / sigma;
            r = 0;
            while (r < m) : (r += 1) {
                u_out.set(r, j, u_out.at(r, j) * inv_s);
            }
        } else {
            r = 0;
            while (r < m) : (r += 1) {
                u_out.set(r, j, 0.0);
            }
        }
    }

    // Sort singular values descending (swapping corresponding U and V columns)
    let sj: usize = 0;
    while (sj < n) : (sj += 1) {
        let best: usize = sj;
        let sk: usize = sj + 1;
        while (sk < n) : (sk += 1) {
            if (s_out[sk] > s_out[best]) best = sk;
        }
        if (best != sj) {
            const ts: f64 = s_out[sj];
            s_out[sj] = s_out[best];
            s_out[best] = ts;
            let r_u: usize = 0;
            while (r_u < m) : (r_u += 1) {
                const tu: f64 = u_out.at(r_u, sj);
                u_out.set(r_u, sj, u_out.at(r_u, best));
                u_out.set(r_u, best, tu);
            }
            let r_v: usize = 0;
            while (r_v < n) : (r_v += 1) {
                const tv: f64 = v_out.at(r_v, sj);
                v_out.set(r_v, sj, v_out.at(r_v, best));
                v_out.set(r_v, best, tv);
            }
        }
    }

    const s_max: f64 = if (n > 0) s_out[0] else 0.0;
    const rank_tol: f64 = s_max * 1.0e-12;
    let rank: usize = 0;
    j = 0;
    while (j < n) : (j += 1) {
        if (s_out[j] > rank_tol) rank += 1;
    }

    return SvdThinResult{
        sweeps: sweep,
        rotations: total_rots,
        converged: converged,
        rank: rank,
    };
}

/// Computes the 2-norm condition number `sigma_max / sigma_min` from descending singular values `s`.
/// Returns `std.math.inf(f64)` if `s` is empty or `sigma_min <= 1e-15`.
pub fn svdCondNumber(s: []const f64): f64 {
    if (s.len == 0) return std.math.inf(f64);
    const s_max: f64 = s[0];
    const s_min: f64 = s[s.len - 1];
    if (s_min <= 1.0e-15) return std.math.inf(f64);
    return s_max / s_min;
}

/// Computes the Moore-Penrose pseudoinverse `A^+ = V * Sigma^+ * U^T` (`N x M`)
/// from Thin SVD factors (`U: M x N`, `s: [N]`, `V: N x N`).
pub fn svdThinPinv(
    u: MatrixViewF64,
    s: []const f64,
    v: MatrixViewF64,
    rcond: f64,
    pinv_out: *MatrixMutF64,
): void ! MatrixError {
    const m: usize = u.rows;
    const n: usize = u.cols;
    if (s.len < n || v.rows != n || v.cols != n || pinv_out.rows != n || pinv_out.cols != m) {
        return MatrixError.DimensionMismatch;
    }
    const s_max: f64 = if (n > 0) s[0] else 0.0;
    const cutoff: f64 = s_max * (if (rcond > 0.0) rcond else 1.0e-12);

    let r: usize = 0;
    while (r < n) : (r += 1) {
        let c: usize = 0;
        while (c < m) : (c += 1) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) : (k += 1) {
                if (s[k] > cutoff) {
                    sum += v.at(r, k) * (1.0 / s[k]) * u.at(c, k);
                }
            }
            pinv_out.set(r, c, sum);
        }
    }
}

// ============================================================================
// 8. SIMD Gram Matrix (`A^T * A`) & Binary Exponentiation (`A^exp`)
// ============================================================================

/// Computes the symmetric Gram matrix `G = A^T * A` (`N x N` from `A: M x N`)
/// using transposed `A^T` in `at_work` (`len >= M * N`) and hardware SIMD `@dot`.
/// Exploit symmetry (`G[i, j] = G[j, i]`) to halve dot-product computations.
pub fn gramMatrixF32Simd(
    a: MatrixViewF32,
    gram_out: *MatrixMutF32,
    at_work: *[]f32,
): void ! MatrixError {
    const m: usize = a.rows;
    const n: usize = a.cols;
    if (gram_out.rows != n || gram_out.cols != n) return MatrixError.DimensionMismatch;
    if (at_work.len < m * n) return MatrixError.BufferTooSmall;

    let at: MatrixMutF32 = try MatrixMutF32.init(at_work.*, n, m);
    try transposeMatrixF32(a, *at);
    const at_view: MatrixViewF32 = at.asView();

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const row_i: []const f32 = at_view.row(i);
        let j: usize = i;
        while (j < n) : (j += 1) {
            const d: f32 = @dot(row_i, at_view.row(j));
            gram_out.set(i, j, d);
            if (j != i) gram_out.set(j, i, d);
        }
    }
}

/// Computes `out = A^exp` for a square `N x N` matrix via O(log exp) binary exponentiation
/// using caller-provided square scratch matrices `base_work`, `prod_work`, and `bt_work`.
pub fn matrixPowerF64(
    a: MatrixViewF64,
    exp: u64,
    out: *MatrixMutF64,
    base_work: *MatrixMutF64,
    prod_work: *MatrixMutF64,
    bt_work: *[]f64,
): void ! MatrixError {
    if (!a.isSquare() || !out.isSquare() || !base_work.isSquare() || !prod_work.isSquare()) {
        return MatrixError.NotSquare;
    }
    const n: usize = a.rows;
    if (out.rows != n || base_work.rows != n || prod_work.rows != n) {
        return MatrixError.DimensionMismatch;
    }
    if (bt_work.len < n * n) return MatrixError.BufferTooSmall;

    try matrixIdentityF64(out);
    if (exp == 0) return;

    let i: usize = 0;
    while (i < a.data.len) : (i += 1) {
        base_work.data[i] = a.data[i];
    }

    let e: u64 = exp;
    while (e > 0) {
        if ((e & 1) != 0) {
            try matmulF64(out.asView(), base_work.asView(), 1.0, 0.0, prod_work, bt_work);
            let k: usize = 0;
            while (k < out.data.len) : (k += 1) out.data[k] = prod_work.data[k];
        }
        e >>= 1;
        if (e > 0) {
            try matmulF64(base_work.asView(), base_work.asView(), 1.0, 0.0, prod_work, bt_work);
            let k: usize = 0;
            while (k < base_work.data.len) : (k += 1) base_work.data[k] = prod_work.data[k];
        }
    }
}
