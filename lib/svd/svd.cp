// Origin: Pure-CP Stdlib (Zero-Alloc One-Sided Jacobi SVD, Truncated Least-Squares Solver & Moore-Penrose Pseudoinverse) | Created: 2026-11-22

import std from "std";

const DEFAULT_TOL: f64 = 1.0e-14;
const RANK_EPS: f64 = 1.0e-12;

/// Summary metrics returned by `svdJacobi`.
pub struct SvdResult {
    converged: bool,
    sweeps: usize,
    rank: usize,
    conditionNumber: f64,
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn checkMatrixLen(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn checkFinitePrefix(data: []const f64, count: usize): bool {
    let i: usize = 0;
    while (i < count) {
        if (!isFiniteF64(data[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn completeOrthonormalColumn(u_out: *[]f64, m: usize, n: usize, target_col: usize) {
    // Try standard basis vectors e_0 .. e_{m-1} and orthogonalize against all already-orthonormal columns
    let e_idx: usize = 0;
    while (e_idx < m) {
        let r: usize = 0;
        while (r < m) {
            u_out.*[r * n + target_col] = if (r == e_idx) 1.0 else 0.0;
            r += 1;
        }

        // Two passes of Modified Gram-Schmidt against columns 0..target_col-1 for ~1e-15 orthogonality
        let pass: usize = 0;
        while (pass < 2) {
            let c: usize = 0;
            while (c < target_col) {
                let dot: f64 = 0.0;
                let r_d: usize = 0;
                while (r_d < m) {
                    dot += u_out.*[r_d * n + c] * u_out.*[r_d * n + target_col];
                    r_d += 1;
                }
                let r_s: usize = 0;
                while (r_s < m) {
                    u_out.*[r_s * n + target_col] -= dot * u_out.*[r_s * n + c];
                    r_s += 1;
                }
                c += 1;
            }
            pass += 1;
        }

        let norm_sq: f64 = 0.0;
        let r_n: usize = 0;
        while (r_n < m) {
            const val: f64 = u_out.*[r_n * n + target_col];
            norm_sq += val * val;
            r_n += 1;
        }
        if (norm_sq > 1.0e-4) {
            const inv_norm: f64 = 1.0 / std.math.sqrt(norm_sq);
            let r_u: usize = 0;
            while (r_u < m) {
                u_out.*[r_u * n + target_col] *= inv_norm;
                r_u += 1;
            }
            return;
        }
        e_idx += 1;
    }
}

/// Computes the economy-size Singular Value Decomposition `A = U * diag(S) * V^T` via
/// One-Sided Jacobi orthogonalization for row-major `m x n` matrices with `m >= n >= 1`.
/// - `u_out`: caller buffer of length `>= m * n` (receives orthonormal `m x n` left singular vectors)
/// - `s_out`: caller buffer of length `>= n` (receives non-negative singular values in descending order)
/// - `v_out`: caller buffer of length `>= n * n` (receives orthogonal `n x n` right singular vectors)
/// Returns `null` if `m == 0`, `n == 0`, `m < n`, `tol < 0`, buffers are too small, or `a` has NaN/Inf.
pub fn svdJacobi(
    a: []const f64,
    m: usize,
    n: usize,
    u_out: *[]f64,
    s_out: *[]f64,
    v_out: *[]f64,
    max_sweeps: usize,
    tol: f64,
): SvdResult? {
    if (m == 0 || n == 0 || m < n) {
        return null;
    }
    if (!isFiniteF64(tol) || tol < 0.0) {
        return null;
    }
    if (!checkMatrixLen(a.len, m, n) || !checkMatrixLen(u_out.*.len, m, n)) {
        return null;
    }
    if (s_out.*.len < n || !checkMatrixLen(v_out.*.len, n, n)) {
        return null;
    }
    if (!checkFinitePrefix(a, m * n)) {
        return null;
    }

    // Copy A -> U
    const mn: usize = m * n;
    let cp_i: usize = 0;
    while (cp_i < mn) {
        u_out.*[cp_i] = a[cp_i];
        cp_i += 1;
    }

    // Initialize V = I_n
    const nn: usize = n * n;
    let zv: usize = 0;
    while (zv < nn) {
        v_out.*[zv] = 0.0;
        zv += 1;
    }
    let diag_i: usize = 0;
    while (diag_i < n) {
        v_out.*[diag_i * n + diag_i] = 1.0;
        diag_i += 1;
    }

    const eff_tol: f64 = if (tol > 0.0) tol else DEFAULT_TOL;
    let converged: bool = n == 1;
    let sweeps_done: usize = 0;

    if (n > 1) {
        let sweep: usize = 0;
        while (sweep < max_sweeps) {
            let rotated: bool = false;
            let p: usize = 0;
            while (p + 1 < n) {
                let q: usize = p + 1;
                while (q < n) {
                    let alpha: f64 = 0.0;
                    let beta: f64 = 0.0;
                    let gamma: f64 = 0.0;

                    let r: usize = 0;
                    while (r < m) {
                        const up: f64 = u_out.*[r * n + p];
                        const uq: f64 = u_out.*[r * n + q];
                        alpha += up * up;
                        beta += uq * uq;
                        gamma += up * uq;
                        r += 1;
                    }

                    if (alpha > 0.0 && beta > 0.0) {
                        const scale: f64 = std.math.sqrt(alpha * beta);
                        if (absF64(gamma) > eff_tol * scale) {
                            const zeta: f64 = (beta - alpha) / (2.0 * gamma);
                            const sign_z: f64 = if (zeta >= 0.0) 1.0 else -1.0;
                            const t: f64 = sign_z / (absF64(zeta) + std.math.sqrt(1.0 + zeta * zeta));
                            const c: f64 = 1.0 / std.math.sqrt(1.0 + t * t);
                            const s: f64 = c * t;

                            let ru: usize = 0;
                            while (ru < m) {
                                const idx_p: usize = ru * n + p;
                                const idx_q: usize = ru * n + q;
                                const up: f64 = u_out.*[idx_p];
                                const uq: f64 = u_out.*[idx_q];
                                u_out.*[idx_p] = c * up - s * uq;
                                u_out.*[idx_q] = s * up + c * uq;
                                ru += 1;
                            }

                            let rv: usize = 0;
                            while (rv < n) {
                                const idx_p: usize = rv * n + p;
                                const idx_q: usize = rv * n + q;
                                const vp: f64 = v_out.*[idx_p];
                                const vq: f64 = v_out.*[idx_q];
                                v_out.*[idx_p] = c * vp - s * vq;
                                v_out.*[idx_q] = s * vp + c * vq;
                                rv += 1;
                            }

                            rotated = true;
                        }
                    }
                    q += 1;
                }
                p += 1;
            }

            sweeps_done = sweep + 1;
            if (!rotated) {
                converged = true;
                break;
            }
            sweep += 1;
        }
    }

    // Compute singular values and normalize non-zero columns of U
    let col: usize = 0;
    while (col < n) {
        let norm_sq: f64 = 0.0;
        let r: usize = 0;
        while (r < m) {
            const val: f64 = u_out.*[r * n + col];
            norm_sq += val * val;
            r += 1;
        }
        const sigma: f64 = std.math.sqrt(norm_sq);
        s_out.*[col] = sigma;
        if (sigma > 0.0) {
            const inv_s: f64 = 1.0 / sigma;
            let rn: usize = 0;
            while (rn < m) {
                u_out.*[rn * n + col] *= inv_s;
                rn += 1;
            }
        }
        col += 1;
    }

    // Sort singular values (and corresponding columns of U and V) in descending order
    let si: usize = 0;
    while (si + 1 < n) {
        let best_j: usize = si;
        let best_s: f64 = s_out.*[si];
        let sj: usize = si + 1;
        while (sj < n) {
            if (s_out.*[sj] > best_s) {
                best_s = s_out.*[sj];
                best_j = sj;
            }
            sj += 1;
        }
        if (best_j != si) {
            const tmp_s: f64 = s_out.*[si];
            s_out.*[si] = s_out.*[best_j];
            s_out.*[best_j] = tmp_s;

            let ru: usize = 0;
            while (ru < m) {
                const idx1: usize = ru * n + si;
                const idx2: usize = ru * n + best_j;
                const tmp_u: f64 = u_out.*[idx1];
                u_out.*[idx1] = u_out.*[idx2];
                u_out.*[idx2] = tmp_u;
                ru += 1;
            }

            let rv: usize = 0;
            while (rv < n) {
                const idx1: usize = rv * n + si;
                const idx2: usize = rv * n + best_j;
                const tmp_v: f64 = v_out.*[idx1];
                v_out.*[idx1] = v_out.*[idx2];
                v_out.*[idx2] = tmp_v;
                rv += 1;
            }
        }
        si += 1;
    }

    // Complete orthonormal basis in U for any zero-singular-value trailing columns
    const sigma_max: f64 = s_out.*[0];
    const max_dim_f: f64 = (if (m > n) m else n) as f64;
    const rank_cutoff: f64 = max_dim_f * RANK_EPS * sigma_max;

    let rank: usize = 0;
    let rk_i: usize = 0;
    while (rk_i < n) {
        if (sigma_max > 0.0 && s_out.*[rk_i] > rank_cutoff) {
            rank += 1;
        } else if (s_out.*[rk_i] == 0.0) {
            completeOrthonormalColumn(u_out, m, n, rk_i);
        }
        rk_i += 1;
    }

    const sigma_min: f64 = s_out.*[n - 1];
    const cond: f64 = if (rank == n && sigma_min > 0.0)
        (sigma_max / sigma_min)
    else
        std.math.inf(f64);

    return SvdResult {
        converged: converged,
        sweeps: sweeps_done,
        rank: rank,
        conditionNumber: cond,
    };
}

/// Computes the minimum-norm least-squares solution `x` to `A * x ≈ b` using a precomputed SVD
/// `A = U * diag(S) * V^T`, zeroing singular values `<= rcond * max(S)`.
pub fn svdSolve(
    u: []const f64,
    s: []const f64,
    v: []const f64,
    m: usize,
    n: usize,
    b: []const f64,
    x_out: *[]f64,
    utb_scratch: *[]f64,
    rcond: f64,
): bool {
    if (m == 0 || n == 0 || m < n) {
        return false;
    }
    if (!isFiniteF64(rcond) || rcond < 0.0) {
        return false;
    }
    if (!checkMatrixLen(u.len, m, n) || s.len < n || !checkMatrixLen(v.len, n, n)) {
        return false;
    }
    if (b.len < m || x_out.*.len < n || utb_scratch.*.len < n) {
        return false;
    }
    if (!checkFinitePrefix(b, m) || !checkFinitePrefix(s, n)) {
        return false;
    }

    let sigma_max: f64 = 0.0;
    let si: usize = 0;
    while (si < n) {
        if (s[si] > sigma_max) {
            sigma_max = s[si];
        }
        si += 1;
    }
    const cutoff: f64 = rcond * sigma_max;

    // utb_scratch[j] = (U^T b)[j] / s[j] for s[j] > cutoff
    let j: usize = 0;
    while (j < n) {
        if (s[j] > cutoff && s[j] > 0.0) {
            let dot: f64 = 0.0;
            let r: usize = 0;
            while (r < m) {
                dot += u[r * n + j] * b[r];
                r += 1;
            }
            utb_scratch.*[j] = dot / s[j];
        } else {
            utb_scratch.*[j] = 0.0;
        }
        j += 1;
    }

    // x_out[i] = sum_j V[i, j] * utb_scratch[j]
    let i: usize = 0;
    while (i < n) {
        let sum: f64 = 0.0;
        let k: usize = 0;
        while (k < n) {
            sum += v[i * n + k] * utb_scratch.*[k];
            k += 1;
        }
        x_out.*[i] = sum;
        i += 1;
    }

    return true;
}

/// Computes the `n x m` row-major Moore-Penrose pseudoinverse `A^+ = V * S^+ * U^T`,
/// inverting only singular values strictly greater than `rcond * max(S)`.
pub fn svdPseudoinverse(
    u: []const f64,
    s: []const f64,
    v: []const f64,
    m: usize,
    n: usize,
    pinv_out: *[]f64,
    rcond: f64,
): bool {
    if (m == 0 || n == 0 || m < n) {
        return false;
    }
    if (!isFiniteF64(rcond) || rcond < 0.0) {
        return false;
    }
    if (!checkMatrixLen(u.len, m, n) || s.len < n || !checkMatrixLen(v.len, n, n)) {
        return false;
    }
    if (!checkMatrixLen(pinv_out.*.len, n, m)) {
        return false;
    }
    if (!checkFinitePrefix(s, n)) {
        return false;
    }

    let sigma_max: f64 = 0.0;
    let si: usize = 0;
    while (si < n) {
        if (s[si] > sigma_max) {
            sigma_max = s[si];
        }
        si += 1;
    }
    const cutoff: f64 = rcond * sigma_max;

    let i: usize = 0;
    while (i < n) {
        let r: usize = 0;
        while (r < m) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                const sk: f64 = s[k];
                if (sk > cutoff && sk > 0.0) {
                    sum += (v[i * n + k] / sk) * u[r * n + k];
                }
                k += 1;
            }
            pinv_out.*[i * m + r] = sum;
            r += 1;
        }
        i += 1;
    }

    return true;
}

/// Computes the element-wise infinity-norm reconstruction error `||A - U * diag(S) * V^T||_inf`.
/// Returns `-1.0` if dimensions or slice lengths are invalid.
pub fn svdReconstructionError(
    a: []const f64,
    u: []const f64,
    s: []const f64,
    v: []const f64,
    m: usize,
    n: usize,
): f64 {
    if (m == 0 || n == 0 || m < n) {
        return -1.0;
    }
    if (!checkMatrixLen(a.len, m, n) || !checkMatrixLen(u.len, m, n) || s.len < n || !checkMatrixLen(v.len, n, n)) {
        return -1.0;
    }

    let max_err: f64 = 0.0;
    let r: usize = 0;
    while (r < m) {
        let c: usize = 0;
        while (c < n) {
            let recon: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                recon += u[r * n + k] * s[k] * v[c * n + k];
                k += 1;
            }
            const err: f64 = absF64(a[r * n + c] - recon);
            if (err > max_err) {
                max_err = err;
            }
            c += 1;
        }
        r += 1;
    }
    return max_err;
}
