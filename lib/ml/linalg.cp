// Source: cp/lib/linalg/matrix.cp (Arbitrary-Dimension Dense Matrix & Numerical Linear Algebra)
// Pure CP, zero-C-dependency dense matrix arithmetic, LU/Partial-Pivoting solver, determinant, inverse, and Householder QR least squares.

const std = @import("std");

const PIVOT_EPS: f64 = 0.000000000001;

/// Error set for numerical linear algebra operations.
pub err LinalgError {
    DimensionMismatch,
    NotSquare,
    SingularMatrix,
    InvalidDimensions,
    OutOfMemory,
}

/// Computes the Euclidean dot product of two equal-length vectors.
pub fn dot(a: []const f64, b: []const f64): f64 ! LinalgError {
    return Matrix.dot(a, b);
}

/// Computes the L1 norm (sum of absolute values) of a vector.
pub fn vecNorm1(v: []const f64): f64 {
    let sum: f64 = 0.0;
    for (x of v) {
        sum += @abs(x);
    }
    return sum;
}

/// Computes the L2 (Euclidean) norm of a vector.
pub fn vecNorm2(v: []const f64): f64 {
    let sum: f64 = 0.0;
    for (x of v) {
        sum += x * x;
    }
    return @sqrt(sum);
}

/// Computes the L-infinity norm (maximum absolute value) of a vector.
pub fn vecNormInf(v: []const f64): f64 {
    let best: f64 = 0.0;
    for (x of v) {
        const ax = @abs(x);
        if (ax > best) best = ax;
    }
    return best;
}

/// Row-major contiguous dense matrix of `f64` elements.
pub struct Matrix {
    rows: usize;
    cols: usize;
    data: []f64;
    alloc: std.mem.Allocator;

    /// Allocates a `rows x cols` matrix initialized to 0.0.
    static fn zeros(alloc: std.mem.Allocator, rows: usize, cols: usize): Matrix ! LinalgError {
        if (rows == 0 || cols == 0) return LinalgError.InvalidDimensions;
        const total = std.math.mul(usize, rows, cols) catch return LinalgError.InvalidDimensions;
        const buf = alloc.alloc(f64, total) catch return LinalgError.OutOfMemory;
        @memset(buf, 0.0);
        return Matrix {
            rows: rows,
            cols: cols,
            data: buf,
            alloc: alloc,
        };
    }

    /// Allocates an `n x n` identity matrix.
    static fn identity(alloc: std.mem.Allocator, n: usize): Matrix ! LinalgError {
        let m = Matrix.zeros(alloc, n, n)?;
        for (i of 0..n) {
            m.data[i * n + i] = 1.0;
        }
        return m;
    }

    /// Allocates a `rows x cols` matrix by copying row-major data from `slice`.
    static fn fromSlice(alloc: std.mem.Allocator, rows: usize, cols: usize, slice: []const f64): Matrix ! LinalgError {
        if (rows == 0 || cols == 0) return LinalgError.InvalidDimensions;
        const total = std.math.mul(usize, rows, cols) catch return LinalgError.InvalidDimensions;
        if (slice.len != total) return LinalgError.DimensionMismatch;
        const buf = alloc.alloc(f64, total) catch return LinalgError.OutOfMemory;
        @memcpy(buf, slice);
        return Matrix {
            rows: rows,
            cols: cols,
            data: buf,
            alloc: alloc,
        };
    }

    /// Returns a deep copy of the matrix allocated with `alloc`.
    fn clone(alloc: std.mem.Allocator): Matrix ! LinalgError {
        const buf = alloc.alloc(f64, _.data.len) catch return LinalgError.OutOfMemory;
        @memcpy(buf, _.data);
        return Matrix {
            rows: _.rows,
            cols: _.cols,
            data: buf,
            alloc: alloc,
        };
    }

    /// Frees the underlying matrix storage.
    fn deinit(*_) {
        if (_.data.len > 0) {
            _.alloc.free(_.data);
            _.data = [];
        }
        _.rows = 0;
        _.cols = 0;
    }

    /// Reads the element at `(r, c)`.
    fn get(r: usize, c: usize): f64 {
        return _.data[r * _.cols + c];
    }

    /// Writes `val` to the element at `(r, c)`.
    fn set(*_, r: usize, c: usize, val: f64) {
        _.data[r * _.cols + c] = val;
    }

    /// Computes the trace (sum of main diagonal elements) of a square matrix.
    fn trace(): f64 ! LinalgError {
        if (_.rows != _.cols) return LinalgError.NotSquare;
        let sum: f64 = 0.0;
        for (i of 0.._.rows) {
            sum += _.data[i * _.cols + i];
        }
        return sum;
    }

    /// Computes the Frobenius norm `sqrt(sum(A[i,j]^2))`.
    fn frobeniusNorm(): f64 {
        let sum: f64 = 0.0;
        for (x of _.data) {
            sum += x * x;
        }
        return @sqrt(sum);
    }

    /// Computes the maximum absolute element-wise difference between `_` and `other`.
    fn maxAbsDiff(other: Matrix): f64 ! LinalgError {
        if (_.rows != other.rows || _.cols != other.cols) return LinalgError.DimensionMismatch;
        let best: f64 = 0.0;
        for (i of 0.._.data.len) {
            const d = @abs(_.data[i] - other.data[i]);
            if (d > best) best = d;
        }
        return best;
    }

    /// Element-wise matrix addition `A + B`.
    fn add(alloc: std.mem.Allocator, other: Matrix): Matrix ! LinalgError {
        if (_.rows != other.rows || _.cols != other.cols) return LinalgError.DimensionMismatch;
        let res = Matrix.zeros(alloc, _.rows, _.cols)?;
        for (i of 0.._.data.len) {
            res.data[i] = _.data[i] + other.data[i];
        }
        return res;
    }

    /// Element-wise matrix subtraction `A - B`.
    fn sub(alloc: std.mem.Allocator, other: Matrix): Matrix ! LinalgError {
        if (_.rows != other.rows || _.cols != other.cols) return LinalgError.DimensionMismatch;
        let res = Matrix.zeros(alloc, _.rows, _.cols)?;
        for (i of 0.._.data.len) {
            res.data[i] = _.data[i] - other.data[i];
        }
        return res;
    }

    /// Multiplies every element by `factor`.
    fn scale(alloc: std.mem.Allocator, factor: f64): Matrix ! LinalgError {
        let res = Matrix.zeros(alloc, _.rows, _.cols)?;
        for (i of 0.._.data.len) {
            res.data[i] = _.data[i] * factor;
        }
        return res;
    }

    /// Returns the transposed matrix `A^T`.
    fn transpose(alloc: std.mem.Allocator): Matrix ! LinalgError {
        let res = Matrix.zeros(alloc, _.cols, _.rows)?;
        for (r of 0.._.rows) {
            for (c of 0.._.cols) {
                res.data[c * _.rows + r] = _.data[r * _.cols + c];
            }
        }
        return res;
    }

    /// Matrix multiplication `A * B`.
    fn mul(alloc: std.mem.Allocator, other: Matrix): Matrix ! LinalgError {
        if (_.cols != other.rows) return LinalgError.DimensionMismatch;
        let res = Matrix.zeros(alloc, _.rows, other.cols)?;
        for (i of 0.._.rows) {
            const row_a = i * _.cols;
            const row_c = i * other.cols;
            for (k of 0.._.cols) {
                const a_ik = _.data[row_a + k];
                if (a_ik == 0.0) continue;
                const row_b = k * other.cols;
                for (j of 0..other.cols) {
                    res.data[row_c + j] += a_ik * other.data[row_b + j];
                }
            }
        }
        return res;
    }

    /// Matrix-vector product `A * x`.
    fn mulVec(alloc: std.mem.Allocator, x: []const f64): []f64 ! LinalgError {
        if (_.cols != x.len) return LinalgError.DimensionMismatch;
        let y = alloc.alloc(f64, _.rows) catch return LinalgError.OutOfMemory;
        for (i of 0.._.rows) {
            let sum: f64 = 0.0;
            const row_off = i * _.cols;
            for (j of 0.._.cols) {
                sum += _.data[row_off + j] * x[j];
            }
            y[i] = sum;
        }
        return y;
    }

    /// Static dot product of two equal-length vectors.
    static fn dot(a: []const f64, b: []const f64): f64 ! LinalgError {
        if (a.len != b.len) return LinalgError.DimensionMismatch;
        let sum: f64 = 0.0;
        for (i of 0..a.len) {
            sum += a[i] * b[i];
        }
        return sum;
    }

    /// Internal partial-pivoting LU factorization (`PA = LU` stored in-place in `lu`).
    fn luFactor(alloc: std.mem.Allocator, lu: []f64, piv: []usize): usize ! LinalgError {
        * = alloc;
        const n = _.rows;
        @memcpy(lu, _.data);
        for (i of 0..n) {
            piv[i] = i;
        }

        let swaps: usize = 0;
        for (k of 0..n) {
            let max_row = k;
            let max_val = @abs(lu[k * n + k]);
            for (r of (k + 1)..n) {
                const v = @abs(lu[r * n + k]);
                if (v > max_val) {
                    max_val = v;
                    max_row = r;
                }
            }

            if (max_val < PIVOT_EPS) return LinalgError.SingularMatrix;

            if (max_row != k) {
                const row_k = k * n;
                const row_p = max_row * n;
                for (c of 0..n) {
                    const tmp = lu[row_k + c];
                    lu[row_k + c] = lu[row_p + c];
                    lu[row_p + c] = tmp;
                }
                const tp = piv[k];
                piv[k] = piv[max_row];
                piv[max_row] = tp;
                swaps += 1;
            }

            const diag = lu[k * n + k];
            for (i of (k + 1)..n) {
                const factor = lu[i * n + k] / diag;
                lu[i * n + k] = factor;
                const row_i = i * n;
                const row_k = k * n;
                for (j of (k + 1)..n) {
                    lu[row_i + j] -= factor * lu[row_k + j];
                }
            }
        }
        return swaps;
    }

    /// Computes the determinant `det(A)` using partial-pivoting LU decomposition.
    fn determinant(alloc: std.mem.Allocator): f64 ! LinalgError {
        if (_.rows != _.cols) return LinalgError.NotSquare;
        const n = _.rows;
        let lu = alloc.alloc(f64, n * n) catch return LinalgError.OutOfMemory;
        defer alloc.free(lu);
        let piv = alloc.alloc(usize, n) catch return LinalgError.OutOfMemory;
        defer alloc.free(piv);

        const swaps = _.luFactor(alloc, lu, piv) catch e => {
            if (e == LinalgError.SingularMatrix) return 0.0;
            return e;
        };

        let det: f64 = if (swaps % 2 == 0) 1.0 else -1.0;
        for (i of 0..n) {
            det *= lu[i * n + i];
        }
        return det;
    }

    /// Solves the linear system `Ax = b` using partial-pivoting LU decomposition.
    fn solve(alloc: std.mem.Allocator, b: []const f64): []f64 ! LinalgError {
        if (_.rows != _.cols) return LinalgError.NotSquare;
        const n = _.rows;
        if (b.len != n) return LinalgError.DimensionMismatch;

        let lu = alloc.alloc(f64, n * n) catch return LinalgError.OutOfMemory;
        defer alloc.free(lu);
        let piv = alloc.alloc(usize, n) catch return LinalgError.OutOfMemory;
        defer alloc.free(piv);

        * = _.luFactor(alloc, lu, piv)?;

        let x = alloc.alloc(f64, n) catch return LinalgError.OutOfMemory;
        errdefer alloc.free(x);

        // Permute b: x = P * b
        for (i of 0..n) {
            x[i] = b[piv[i]];
        }

        // Forward substitution: L * y = P * b (unit lower triangular)
        for (i of 0..n) {
            let sum = x[i];
            const row_i = i * n;
            for (j of 0..i) {
                sum -= lu[row_i + j] * x[j];
            }
            x[i] = sum;
        }

        // Backward substitution: U * x = y
        let i_rev: usize = n;
        while (i_rev > 0) {
            i_rev -= 1;
            let sum = x[i_rev];
            const row_i = i_rev * n;
            for (j of (i_rev + 1)..n) {
                sum -= lu[row_i + j] * x[j];
            }
            const diag = lu[row_i + i_rev];
            if (@abs(diag) < PIVOT_EPS) return LinalgError.SingularMatrix;
            x[i_rev] = sum / diag;
        }

        return x;
    }

    /// Computes the matrix inverse `A^-1` using partial-pivoting LU decomposition.
    fn inverse(alloc: std.mem.Allocator): Matrix ! LinalgError {
        if (_.rows != _.cols) return LinalgError.NotSquare;
        const n = _.rows;

        let lu = alloc.alloc(f64, n * n) catch return LinalgError.OutOfMemory;
        defer alloc.free(lu);
        let piv = alloc.alloc(usize, n) catch return LinalgError.OutOfMemory;
        defer alloc.free(piv);

        * = _.luFactor(alloc, lu, piv)?;

        let inv = Matrix.zeros(alloc, n, n)?;
        errdefer inv.deinit();

        let col = alloc.alloc(f64, n) catch return LinalgError.OutOfMemory;
        defer alloc.free(col);

        for (c of 0..n) {
            for (i of 0..n) {
                col[i] = if (piv[i] == c) 1.0 else 0.0;
            }

            // Forward substitution
            for (i of 0..n) {
                let sum = col[i];
                const row_i = i * n;
                for (j of 0..i) {
                    sum -= lu[row_i + j] * col[j];
                }
                col[i] = sum;
            }

            // Backward substitution
            let i_rev: usize = n;
            while (i_rev > 0) {
                i_rev -= 1;
                let sum = col[i_rev];
                const row_i = i_rev * n;
                for (j of (i_rev + 1)..n) {
                    sum -= lu[row_i + j] * col[j];
                }
                const diag = lu[row_i + i_rev];
                if (@abs(diag) < PIVOT_EPS) return LinalgError.SingularMatrix;
                col[i_rev] = sum / diag;
            }

            for (i of 0..n) {
                inv.data[i * n + c] = col[i];
            }
        }

        return inv;
    }

    /// Solves the linear least-squares problem `min ||Ax - b||_2` for `m >= n` via Householder QR factorization.
    static fn leastSquares(alloc: std.mem.Allocator, A: Matrix, b: []const f64): []f64 ! LinalgError {
        const m = A.rows;
        const n = A.cols;
        if (m < n) return LinalgError.DimensionMismatch;
        if (b.len != m) return LinalgError.DimensionMismatch;

        let r = alloc.alloc(f64, m * n) catch return LinalgError.OutOfMemory;
        defer alloc.free(r);
        @memcpy(r, A.data);

        let qty = alloc.alloc(f64, m) catch return LinalgError.OutOfMemory;
        defer alloc.free(qty);
        @memcpy(qty, b);

        let v = alloc.alloc(f64, m) catch return LinalgError.OutOfMemory;
        defer alloc.free(v);

        for (k of 0..n) {
            let tail_sq: f64 = 0.0;
            for (i of (k + 1)..m) {
                const val = r[i * n + k];
                tail_sq += val * val;
            }
            const r_kk = r[k * n + k];
            const norm_x = @sqrt(r_kk * r_kk + tail_sq);
            if (norm_x < PIVOT_EPS) return LinalgError.SingularMatrix;

            const alpha = if (r_kk >= 0.0) -norm_x else norm_x;
            const v0 = r_kk - alpha;
            r[k * n + k] = alpha;

            v[0] = v0;
            for (i of (k + 1)..m) {
                v[i - k] = r[i * n + k];
            }
            const v_len = m - k;
            const v_dot_v = v0 * v0 + tail_sq;
            if (v_dot_v < PIVOT_EPS * PIVOT_EPS) continue;

            for (j of (k + 1)..n) {
                let dot_vj: f64 = 0.0;
                for (idx of 0..v_len) {
                    dot_vj += v[idx] * r[(k + idx) * n + j];
                }
                const scale_j = (2.0 * dot_vj) / v_dot_v;
                for (idx of 0..v_len) {
                    r[(k + idx) * n + j] -= scale_j * v[idx];
                }
            }

            let dot_vb: f64 = 0.0;
            for (idx of 0..v_len) {
                dot_vb += v[idx] * qty[k + idx];
            }
            const scale_b = (2.0 * dot_vb) / v_dot_v;
            for (idx of 0..v_len) {
                qty[k + idx] -= scale_b * v[idx];
            }
        }

        let x = alloc.alloc(f64, n) catch return LinalgError.OutOfMemory;
        errdefer alloc.free(x);

        let i_rev: usize = n;
        while (i_rev > 0) {
            i_rev -= 1;
            let sum = qty[i_rev];
            const row_i = i_rev * n;
            for (j of (i_rev + 1)..n) {
                sum -= r[row_i + j] * x[j];
            }
            const diag = r[row_i + i_rev];
            if (@abs(diag) < PIVOT_EPS) return LinalgError.SingularMatrix;
            x[i_rev] = sum / diag;
        }

        return x;
    }
}

/// Solves the linear least-squares problem `min ||Ax - b||_2` for `m >= n` via Householder QR factorization.
pub fn leastSquares(alloc: std.mem.Allocator, A: Matrix, b: []const f64): []f64 ! LinalgError {
    return Matrix.leastSquares(alloc, A, b);
}
