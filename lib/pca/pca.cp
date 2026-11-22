// Origin: Pure-CP Stdlib (Zero-Alloc Classic Jacobi Symmetric Eigensolver & Principal Component Analysis PCA) | Created: 2026-11-22

import std from "std";

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn checkRectLen(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

struct JacobiState {
    sweeps: usize;
    converged: bool;
}

fn jacobiEigenInternal(
    sym_mat: []f64,
    n: usize,
    evals_out: []f64,
    evecs_out: []f64,
    max_sweeps: usize,
    tol: f64,
): JacobiState? {
    if (!checkRectLen(sym_mat.len, n, n) || !checkRectLen(evecs_out.len, n, n) || evals_out.len < n) {
        return null;
    }
    if (!isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }

    // Validate finite entries and enforce exact symmetry
    let r: usize = 0;
    while (r < n) {
        let c = r;
        while (c < n) {
            const a_rc = sym_mat[r * n + c];
            const a_cr = sym_mat[c * n + r];
            if (!isFiniteF64(a_rc) || !isFiniteF64(a_cr)) {
                return null;
            }
            if (r != c) {
                const avg = 0.5 * (a_rc + a_cr);
                sym_mat[r * n + c] = avg;
                sym_mat[c * n + r] = avg;
            }
            c += 1;
        }
        r += 1;
    }

    // Initialize eigenvectors to identity matrix
    r = 0;
    while (r < n) {
        let c: usize = 0;
        while (c < n) {
            evecs_out[r * n + c] = if (r == c) 1.0 else 0.0;
            c += 1;
        }
        r += 1;
    }

    if (n == 1) {
        evals_out[0] = sym_mat[0];
        return JacobiState { sweeps: 0, converged: true };
    }

    let sweeps_used: usize = 0;
    let converged = false;
    let sweep: usize = 0;
    while (sweep < max_sweeps) {
        let max_off: f64 = 0.0;
        let p: usize = 0;
        while (p < n) {
            let q = p + 1;
            while (q < n) {
                const v = absF64(sym_mat[p * n + q]);
                if (v > max_off) {
                    max_off = v;
                }
                q += 1;
            }
            p += 1;
        }

        if (max_off <= tol) {
            converged = true;
            break;
        }

        const skip_thresh = tol * 0.1;
        p = 0;
        while (p < n) {
            let q = p + 1;
            while (q < n) {
                const apq = sym_mat[p * n + q];
                if (absF64(apq) > skip_thresh) {
                    const app = sym_mat[p * n + p];
                    const aqq = sym_mat[q * n + q];
                    const tau = (aqq - app) / (2.0 * apq);
                    const t = if (tau >= 0.0)
                        1.0 / (tau + std.math.sqrt(1.0 + tau * tau))
                    else
                        -1.0 / ((0.0 - tau) + std.math.sqrt(1.0 + tau * tau));
                    const c_rot = 1.0 / std.math.sqrt(1.0 + t * t);
                    const s_rot = t * c_rot;

                    sym_mat[p * n + p] = app - t * apq;
                    sym_mat[q * n + q] = aqq + t * apq;
                    sym_mat[p * n + q] = 0.0;
                    sym_mat[q * n + p] = 0.0;

                    let k: usize = 0;
                    while (k < n) {
                        if (k != p && k != q) {
                            const akp = sym_mat[k * n + p];
                            const akq = sym_mat[k * n + q];
                            const new_kp = c_rot * akp - s_rot * akq;
                            const new_kq = s_rot * akp + c_rot * akq;
                            sym_mat[k * n + p] = new_kp;
                            sym_mat[p * n + k] = new_kp;
                            sym_mat[k * n + q] = new_kq;
                            sym_mat[q * n + k] = new_kq;
                        }
                        k += 1;
                    }

                    k = 0;
                    while (k < n) {
                        const vkp = evecs_out[k * n + p];
                        const vkq = evecs_out[k * n + q];
                        evecs_out[k * n + p] = c_rot * vkp - s_rot * vkq;
                        evecs_out[k * n + q] = s_rot * vkp + c_rot * vkq;
                        k += 1;
                    }
                }
                q += 1;
            }
            p += 1;
        }

        sweep += 1;
        sweeps_used = sweep;
    }

    if (!converged) {
        let max_off_end: f64 = 0.0;
        let p: usize = 0;
        while (p < n) {
            let q = p + 1;
            while (q < n) {
                const v = absF64(sym_mat[p * n + q]);
                if (v > max_off_end) {
                    max_off_end = v;
                }
                q += 1;
            }
            p += 1;
        }
        if (max_off_end <= tol) {
            converged = true;
        }
    }

    let i: usize = 0;
    while (i < n) {
        evals_out[i] = sym_mat[i * n + i];
        i += 1;
    }

    // Sort eigenvalues descending and permute corresponding eigenvector columns
    i = 0;
    while (i < n) {
        let best = i;
        let j = i + 1;
        while (j < n) {
            if (evals_out[j] > evals_out[best]) {
                best = j;
            }
            j += 1;
        }
        if (best != i) {
            const tmp_ev = evals_out[i];
            evals_out[i] = evals_out[best];
            evals_out[best] = tmp_ev;

            let row: usize = 0;
            while (row < n) {
                const tmp_v = evecs_out[row * n + i];
                evecs_out[row * n + i] = evecs_out[row * n + best];
                evecs_out[row * n + best] = tmp_v;
                row += 1;
            }
        }
        i += 1;
    }

    // Re-normalize eigenvector columns to unit L2 norm
    let col: usize = 0;
    while (col < n) {
        let sq: f64 = 0.0;
        let row: usize = 0;
        while (row < n) {
            const v = evecs_out[row * n + col];
            sq = sq + v * v;
            row += 1;
        }
        if (sq > 0.0) {
            const inv_norm = 1.0 / std.math.sqrt(sq);
            row = 0;
            while (row < n) {
                evecs_out[row * n + col] = evecs_out[row * n + col] * inv_norm;
                row += 1;
            }
        }
        col += 1;
    }

    return JacobiState {
        sweeps: sweeps_used,
        converged: converged,
    };
}

/// Computes all eigenvalues (sorted descending) and orthonormal column eigenvectors
/// of an `n x n` real symmetric matrix `sym_mat` via the cyclic Jacobi method.
///
/// Mutates `sym_mat` in-place during diagonalization.
/// Returns the number of Jacobi sweeps performed, or `null` if dimensions/buffers are invalid,
/// `tol <= 0`, or `sym_mat` contains non-finite values.
pub fn jacobiEigenSymmetric(
    sym_mat: []f64,
    n: usize,
    evals_out: []f64,
    evecs_out: []f64,
    max_sweeps: usize,
    tol: f64,
): usize? {
    if (st := jacobiEigenInternal(sym_mat, n, evals_out, evecs_out, max_sweeps, tol)) {
        return st.sweeps;
    }
    return null;
}

/// Summary metrics returned by `pcaFit`.
pub struct PcaFitResult {
    converged: bool;
    totalVariance: f64;
    explainedVarianceSum: f64;
    explainedRatioSum: f64;
    sweeps: usize;
}

/// Fits Principal Component Analysis (PCA) on an `n_samples x n_features` row-major dataset `data`.
///
/// Zero-allocation: writes feature means into `mean_out[0..n_features]`, principal component rows
/// `W` into `components_out[0..n_components * n_features]`, explained variances into
/// `explained_var_out[0..n_components]`, and explained variance ratios into
/// `explained_ratio_out[0..n_components]` using caller-provided scratch buffers.
///
/// Returns `null` if `n_samples < 2`, `n_features == 0`, `n_components == 0` or `> n_features`,
/// `tol <= 0`, any buffer is undersized, or `data` contains non-finite values.
pub fn pcaFit(
    data: []const f64,
    n_samples: usize,
    n_features: usize,
    n_components: usize,
    mean_out: []f64,
    components_out: []f64,
    explained_var_out: []f64,
    explained_ratio_out: []f64,
    cov_scratch: []f64,
    evals_scratch: []f64,
    evecs_scratch: []f64,
    max_sweeps: usize,
    tol: f64,
): PcaFitResult? {
    if (n_samples < 2 || n_features == 0 || n_components == 0 || n_components > n_features) {
        return null;
    }
    if (!isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }
    if (!checkRectLen(data.len, n_samples, n_features)) {
        return null;
    }
    if (mean_out.len < n_features || !checkRectLen(components_out.len, n_components, n_features)) {
        return null;
    }
    if (explained_var_out.len < n_components || explained_ratio_out.len < n_components) {
        return null;
    }
    if (!checkRectLen(cov_scratch.len, n_features, n_features) || evals_scratch.len < n_features || !checkRectLen(evecs_scratch.len, n_features, n_features)) {
        return null;
    }

    const total_elems = n_samples * n_features;
    let chk: usize = 0;
    while (chk < total_elems) {
        if (!isFiniteF64(data[chk])) {
            return null;
        }
        chk += 1;
    }

    // 1. Compute feature means
    const inv_n = 1.0 / (n_samples as f64);
    let f: usize = 0;
    while (f < n_features) {
        let sum: f64 = 0.0;
        let s: usize = 0;
        while (s < n_samples) {
            sum = sum + data[s * n_features + f];
            s += 1;
        }
        mean_out[f] = sum * inv_n;
        f += 1;
    }

    // 2. Compute unbiased sample covariance matrix: (X - mean)^T (X - mean) / (n_samples - 1)
    const inv_dof = 1.0 / ((n_samples - 1) as f64);
    let a: usize = 0;
    while (a < n_features) {
        let b = a;
        while (b < n_features) {
            let dot: f64 = 0.0;
            let s: usize = 0;
            while (s < n_samples) {
                const da = data[s * n_features + a] - mean_out[a];
                const db = data[s * n_features + b] - mean_out[b];
                dot = dot + da * db;
                s += 1;
            }
            const cov_ab = dot * inv_dof;
            cov_scratch[a * n_features + b] = cov_ab;
            cov_scratch[b * n_features + a] = cov_ab;
            b += 1;
        }
        a += 1;
    }

    // 3. Eigendecomposition of covariance matrix
    const st = jacobiEigenInternal(cov_scratch, n_features, evals_scratch, evecs_scratch, max_sweeps, tol) ?? return null;

    // 4. Clamp tiny negative floating-point eigenvalues to 0 and sum total variance
    let total_var: f64 = 0.0;
    let k: usize = 0;
    while (k < n_features) {
        const ev = if (evals_scratch[k] < 0.0) 0.0 else evals_scratch[k];
        evals_scratch[k] = ev;
        total_var = total_var + ev;
        k += 1;
    }

    // 5. Extract top `n_components` principal axes (stored row-major in components_out)
    let explained_var_sum: f64 = 0.0;
    let explained_ratio_sum: f64 = 0.0;
    k = 0;
    while (k < n_components) {
        const ev = evals_scratch[k];
        const ratio = if (total_var > 0.0) ev / total_var else 0.0;
        explained_var_out[k] = ev;
        explained_ratio_out[k] = ratio;
        explained_var_sum = explained_var_sum + ev;
        explained_ratio_sum = explained_ratio_sum + ratio;

        let j: usize = 0;
        while (j < n_features) {
            components_out[k * n_features + j] = evecs_scratch[j * n_features + k];
            j += 1;
        }
        k += 1;
    }

    return PcaFitResult {
        converged: st.converged,
        totalVariance: total_var,
        explainedVarianceSum: explained_var_sum,
        explainedRatioSum: explained_ratio_sum,
        sweeps: st.sweeps,
    };
}

/// Projects centered data `Z = (X - mean) * W^T` into `projected_out[0..n_samples * n_components]`.
pub fn pcaTransform(
    data: []const f64,
    n_samples: usize,
    n_features: usize,
    mean: []const f64,
    components: []const f64,
    n_components: usize,
    projected_out: []f64,
): bool {
    if (n_samples == 0 || n_features == 0 || n_components == 0 || n_components > n_features) {
        return false;
    }
    if (!checkRectLen(data.len, n_samples, n_features) || mean.len < n_features) {
        return false;
    }
    if (!checkRectLen(components.len, n_components, n_features) || !checkRectLen(projected_out.len, n_samples, n_components)) {
        return false;
    }

    let s: usize = 0;
    while (s < n_samples) {
        let k: usize = 0;
        while (k < n_components) {
            let dot: f64 = 0.0;
            let j: usize = 0;
            while (j < n_features) {
                dot = dot + (data[s * n_features + j] - mean[j]) * components[k * n_features + j];
                j += 1;
            }
            if (!isFiniteF64(dot)) {
                return false;
            }
            projected_out[s * n_components + k] = dot;
            k += 1;
        }
        s += 1;
    }
    return true;
}

/// Reconstructs approximate original data `X_hat = Z * W + mean` into `reconstructed_out[0..n_samples * n_features]`.
pub fn pcaInverseTransform(
    projected: []const f64,
    n_samples: usize,
    n_components: usize,
    mean: []const f64,
    components: []const f64,
    n_features: usize,
    reconstructed_out: []f64,
): bool {
    if (n_samples == 0 || n_features == 0 || n_components == 0 || n_components > n_features) {
        return false;
    }
    if (!checkRectLen(projected.len, n_samples, n_components) || mean.len < n_features) {
        return false;
    }
    if (!checkRectLen(components.len, n_components, n_features) || !checkRectLen(reconstructed_out.len, n_samples, n_features)) {
        return false;
    }

    let s: usize = 0;
    while (s < n_samples) {
        let j: usize = 0;
        while (j < n_features) {
            let val = mean[j];
            let k: usize = 0;
            while (k < n_components) {
                val = val + projected[s * n_components + k] * components[k * n_features + j];
                k += 1;
            }
            if (!isFiniteF64(val)) {
                return false;
            }
            reconstructed_out[s * n_features + j] = val;
            j += 1;
        }
        s += 1;
    }
    return true;
}

/// Computes the element-wise Mean Squared Error `(1 / len) * sum_i (original[i] - reconstructed[i])^2`.
/// Returns `+inf` if `len == 0` or either slice has length `< len`.
pub fn pcaReconstructionMse(
    original: []const f64,
    reconstructed: []const f64,
    len: usize,
): f64 {
    if (len == 0 || original.len < len || reconstructed.len < len) {
        return std.math.inf(f64);
    }
    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < len) {
        const diff = original[i] - reconstructed[i];
        sum_sq = sum_sq + diff * diff;
        i += 1;
    }
    return sum_sq / (len as f64);
}
