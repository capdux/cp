// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Householder QR Factorization & Linear Least Squares)

/// Default rank-deficiency tolerance for `qrSolveLeastSquares`.
pub const QR_DEFAULT_TOL: f64 = 1.0e-12;

fn isFiniteF64(v: f64): bool {
    const diff = v - v;
    return diff == 0.0;
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

/// Computes the in-place Householder QR factorization of a row-major `m x n` matrix (`m >= n > 0`).
/// - Upper triangle of `a_qr` stores `R`; strictly lower triangle stores normalized Householder vectors `v_k` (`v_k[k] = 1` implicit).
/// - `tau_out[0..n]` stores the corresponding Householder reflector scale factors.
/// - Returns `false` on invalid dimensions (`m < n` or zero), undersized buffers, or non-finite (`NaN`/`Inf`) inputs.
pub fn qrFactorize(a_qr: *[]f64, m: usize, n: usize, tau_out: *[]f64): bool {
    if (m == 0 || n == 0 || m < n) {
        return false;
    }
    const total = m * n;
    if (a_qr.*.len < total || tau_out.*.len < n) {
        return false;
    }

    let idx: usize = 0;
    while (idx < total) {
        if (!isFiniteF64(a_qr.*[idx])) {
            return false;
        }
        idx += 1;
    }

    let k: usize = 0;
    while (k < n) {
        // Compute max magnitude in column k[k..m] to guard against overflow/underflow
        let scale: f64 = 0.0;
        let i: usize = k;
        while (i < m) {
            const av = absF64(a_qr.*[i * n + k]);
            if (av > scale) {
                scale = av;
            }
            i += 1;
        }

        if (scale == 0.0) {
            tau_out.*[k] = 0.0;
            k += 1;
            continue;
        }

        // Check sub-diagonal norm
        let sub_sq: f64 = 0.0;
        i = k + 1;
        while (i < m) {
            const s = a_qr.*[i * n + k] / scale;
            sub_sq += s * s;
            i += 1;
        }

        if (sub_sq == 0.0) {
            tau_out.*[k] = 0.0;
            k += 1;
            continue;
        }

        const diag_s = a_qr.*[k * n + k] / scale;
        const norm = scale * @sqrt(diag_s * diag_s + sub_sq);
        const alpha = a_qr.*[k * n + k];
        const beta = if (alpha >= 0.0) -norm else norm;
        const tau_k = (beta - alpha) / beta;
        const inv_denom = 1.0 / (alpha - beta);

        // Normalize Householder vector in-place below diagonal
        i = k + 1;
        while (i < m) {
            a_qr.*[i * n + k] *= inv_denom;
            i += 1;
        }
        a_qr.*[k * n + k] = beta;
        tau_out.*[k] = tau_k;

        // Apply reflector H_k = I - tau_k * v * v^T to trailing columns j = k+1..n-1
        let j: usize = k + 1;
        while (j < n) {
            let dot = a_qr.*[k * n + j];
            i = k + 1;
            while (i < m) {
                dot += a_qr.*[i * n + k] * a_qr.*[i * n + j];
                i += 1;
            }
            const factor = tau_k * dot;
            a_qr.*[k * n + j] -= factor;
            i = k + 1;
            while (i < m) {
                a_qr.*[i * n + j] -= factor * a_qr.*[i * n + k];
                i += 1;
            }
            j += 1;
        }

        k += 1;
    }

    return true;
}

/// Applies `Q^T` in-place to vector `y_inout[0..m]` using the compact Householder representation from `qrFactorize`.
/// Returns `false` on invalid dimensions or undersized buffers.
pub fn qrApplyQt(
    a_qr: []const f64,
    tau: []const f64,
    m: usize,
    n: usize,
    y_inout: *[]f64,
): bool {
    if (m == 0 || n == 0 || m < n) {
        return false;
    }
    if (a_qr.len < m * n || tau.len < n || y_inout.*.len < m) {
        return false;
    }

    let k: usize = 0;
    while (k < n) {
        const tau_k = tau[k];
        if (tau_k != 0.0) {
            let dot = y_inout.*[k];
            let i: usize = k + 1;
            while (i < m) {
                dot += a_qr[i * n + k] * y_inout.*[i];
                i += 1;
            }
            const factor = tau_k * dot;
            y_inout.*[k] -= factor;
            i = k + 1;
            while (i < m) {
                y_inout.*[i] -= factor * a_qr[i * n + k];
                i += 1;
            }
        }
        k += 1;
    }

    return true;
}

/// Forms the economy-sized `m x n` orthonormal matrix `Q` (row-major) satisfying `Q^T Q = I_n` and `Q R = A`.
/// Returns `false` on invalid dimensions or undersized buffers.
pub fn qrFormQ(
    a_qr: []const f64,
    tau: []const f64,
    m: usize,
    n: usize,
    q_out: *[]f64,
): bool {
    if (m == 0 || n == 0 || m < n) {
        return false;
    }
    if (a_qr.len < m * n || tau.len < n || q_out.*.len < m * n) {
        return false;
    }

    // Initialize Q to leading n columns of I_m
    let i: usize = 0;
    while (i < m) {
        let j: usize = 0;
        while (j < n) {
            q_out.*[i * n + j] = if (i == j) 1.0 else 0.0;
            j += 1;
        }
        i += 1;
    }

    // Accumulate H_0 * H_1 * ... * H_{n-1} in reverse order
    let k: usize = n;
    while (k > 0) {
        k -= 1;
        const tau_k = tau[k];
        if (tau_k == 0.0) {
            continue;
        }
        let j: usize = k;
        while (j < n) {
            let dot = q_out.*[k * n + j];
            i = k + 1;
            while (i < m) {
                dot += a_qr[i * n + k] * q_out.*[i * n + j];
                i += 1;
            }
            const factor = tau_k * dot;
            q_out.*[k * n + j] -= factor;
            i = k + 1;
            while (i < m) {
                q_out.*[i * n + j] -= factor * a_qr[i * n + k];
                i += 1;
            }
            j += 1;
        }
    }

    return true;
}

/// Extracts the `n x n` upper triangular factor `R` (row-major) from the compact `a_qr` buffer.
/// Returns `false` on invalid dimensions or undersized buffers.
pub fn qrExtractR(
    a_qr: []const f64,
    m: usize,
    n: usize,
    r_out: *[]f64,
): bool {
    if (m == 0 || n == 0 || m < n) {
        return false;
    }
    if (a_qr.len < m * n || r_out.*.len < n * n) {
        return false;
    }

    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            r_out.*[i * n + j] = if (j >= i) a_qr[i * n + j] else 0.0;
            j += 1;
        }
        i += 1;
    }

    return true;
}

/// Solves the linear least-squares problem `min_x ||A x - b||_2` using the compact Householder QR factors.
/// Returns `false` if any input is non-finite, buffers are undersized, or `|R[i, i]| <= tol` (rank-deficient).
pub fn qrSolveLeastSquares(
    a_qr: []const f64,
    tau: []const f64,
    m: usize,
    n: usize,
    b: []const f64,
    x_out: *[]f64,
    qty_scratch: *[]f64,
    tol: f64,
): bool {
    if (m == 0 || n == 0 || m < n || tol < 0.0 || !isFiniteF64(tol)) {
        return false;
    }
    if (
        a_qr.len < m * n ||
        tau.len < n ||
        b.len < m ||
        x_out.*.len < n ||
        qty_scratch.*.len < m
    ) {
        return false;
    }

    let i: usize = 0;
    while (i < m) {
        if (!isFiniteF64(b[i])) {
            return false;
        }
        qty_scratch.*[i] = b[i];
        i += 1;
    }

    if (!qrApplyQt(a_qr, tau, m, n, qty_scratch)) {
        return false;
    }

    // Back-substitution on R * x = (Q^T b)[0..n]
    let row: usize = n;
    while (row > 0) {
        row -= 1;
        const diag = a_qr[row * n + row];
        if (!isFiniteF64(diag) || absF64(diag) <= tol) {
            return false;
        }
        let sum = qty_scratch.*[row];
        let col: usize = row + 1;
        while (col < n) {
            sum -= a_qr[row * n + col] * x_out.*[col];
            col += 1;
        }
        x_out.*[row] = sum / diag;
    }

    return true;
}

/// Computes the max-entry orthogonality error `max_{i,j} |(Q^T Q - I_n)_{i,j}|`.
/// Returns `-1.0` on invalid dimensions or undersized slice.
pub fn qrOrthogonalityError(q: []const f64, m: usize, n: usize): f64 {
    if (m == 0 || n == 0 || m < n || q.len < m * n) {
        return -1.0;
    }

    let max_err: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let dot: f64 = 0.0;
            let k: usize = 0;
            while (k < m) {
                dot += q[k * n + i] * q[k * n + j];
                k += 1;
            }
            const target = if (i == j) 1.0 else 0.0;
            const err = absF64(dot - target);
            if (err > max_err) {
                max_err = err;
            }
            j += 1;
        }
        i += 1;
    }

    return max_err;
}

/// Computes the max-entry reconstruction error `max_{i,j} |(A - Q R)_{i,j}|`.
/// Returns `-1.0` on invalid dimensions or undersized slices.
pub fn qrReconstructionError(
    a: []const f64,
    q: []const f64,
    r: []const f64,
    m: usize,
    n: usize,
): f64 {
    if (m == 0 || n == 0 || m < n || a.len < m * n || q.len < m * n || r.len < n * n) {
        return -1.0;
    }

    let max_err: f64 = 0.0;
    let i: usize = 0;
    while (i < m) {
        let j: usize = 0;
        while (j < n) {
            let prod: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                prod += q[i * n + k] * r[k * n + j];
                k += 1;
            }
            const err = absF64(a[i * n + j] - prod);
            if (err > max_err) {
                max_err = err;
            }
            j += 1;
        }
        i += 1;
    }

    return max_err;
}
