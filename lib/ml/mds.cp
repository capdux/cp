// Origin: Pure-CP Stdlib (Zero-Alloc Multidimensional Scaling: Classical Torgerson-Gower MDS & SMACOF Metric MDS) | Created: 2026-11-22

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

/// Summary returned by `classicalMds`.
pub struct MdsResult {
    positive_eigenvalues: usize,
    variance_explained: f64,
    stress1: f64,
    sweeps: usize,
}

/// Summary returned by `smacofRefine`.
pub struct SmacofResult {
    stress1: f64,
    iterations: usize,
    converged: bool,
}

fn validateDistMatrix(dist_mat: []const f64, n: usize): bool {
    if (n == 0 || dist_mat.len / n < n) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        const diag = dist_mat[i * n + i];
        if (!isFiniteF64(diag) || absF64(diag) > 1.0e-12) {
            return false;
        }
        let j: usize = i + 1;
        while (j < n) {
            const dij = dist_mat[i * n + j];
            const dji = dist_mat[j * n + i];
            if (!isFiniteF64(dij) || !isFiniteF64(dji) || dij < 0.0 || dji < 0.0) {
                return false;
            }
            if (absF64(dij - dji) > 1.0e-9 * (1.0 + dij + dji)) {
                return false;
            }
            j += 1;
        }
        i += 1;
    }
    return true;
}

/// Computes symmetric `n x n` pairwise Euclidean distance matrix from row-major `points` (`n x dims`).
pub fn pairwiseEuclidean(points: []const f64, n: usize, dims: usize, dist_out: *[]f64): bool {
    if (n == 0 || dims == 0 || points.len / dims < n || dist_out.*.len / n < n) {
        return false;
    }
    let idx: usize = 0;
    const total = n * dims;
    while (idx < total) {
        if (!isFiniteF64(points[idx])) {
            return false;
        }
        idx += 1;
    }

    let i: usize = 0;
    while (i < n) {
        dist_out.*[i * n + i] = 0.0;
        let j: usize = i + 1;
        while (j < n) {
            let sum_sq: f64 = 0.0;
            let d: usize = 0;
            while (d < dims) {
                const diff = points[i * dims + d] - points[j * dims + d];
                sum_sq += diff * diff;
                d += 1;
            }
            const dist = @sqrt(sum_sq);
            dist_out.*[i * n + j] = dist;
            dist_out.*[j * n + i] = dist;
            j += 1;
        }
        i += 1;
    }
    return true;
}

/// Computes normalized Kruskal Stress-1: `sqrt(sum_{i<j} (d_ij - d_hat_ij)^2 / sum_{i<j} d_ij^2)`.
pub fn kruskalStress1(
    target_dist: []const f64,
    coords: []const f64,
    n: usize,
    dims: usize,
): f64? {
    if (!validateDistMatrix(target_dist, n) || dims == 0 || coords.len / dims < n) {
        return null;
    }
    let idx: usize = 0;
    const total = n * dims;
    while (idx < total) {
        if (!isFiniteF64(coords[idx])) {
            return null;
        }
        idx += 1;
    }

    let num: f64 = 0.0;
    let den: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        let j: usize = i + 1;
        while (j < n) {
            let sum_sq: f64 = 0.0;
            let d: usize = 0;
            while (d < dims) {
                const diff = coords[i * dims + d] - coords[j * dims + d];
                sum_sq += diff * diff;
                d += 1;
            }
            const d_hat = @sqrt(sum_sq);
            const d_tgt = target_dist[i * n + j];
            const err = d_tgt - d_hat;
            num += err * err;
            den += d_tgt * d_tgt;
            j += 1;
        }
        i += 1;
    }

    if (den <= 1.0e-30) {
        return 0.0;
    }
    return @sqrt(num / den);
}

/// Computes the double-centered Gram matrix `B = -0.5 * J * D^(2) * J` from symmetric distance matrix `dist_mat`.
/// Returns `false` if `dist_mat` fails symmetry/non-negativity/zero-diagonal checks or buffers are too small.
pub fn doubleCenterGram(
    dist_mat: []const f64,
    n: usize,
    gram_out: *[]f64,
    row_mean_s: *[]f64,
): bool {
    if (!validateDistMatrix(dist_mat, n) || gram_out.*.len / n < n || row_mean_s.*.len < n) {
        return false;
    }

    const inv_n = 1.0 / (n as f64);
    let grand_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        let rsum: f64 = 0.0;
        let j: usize = 0;
        while (j < n) {
            const d = dist_mat[i * n + j];
            rsum += d * d;
            j += 1;
        }
        const rm = rsum * inv_n;
        row_mean_s.*[i] = rm;
        grand_sum += rm;
        i += 1;
    }
    const grand_mean = grand_sum * inv_n;

    let row: usize = 0;
    while (row < n) {
        let j: usize = 0;
        while (j < n) {
            const d = dist_mat[row * n + j];
            gram_out.*[row * n + j] = -0.5 * (d * d - row_mean_s.*[row] - row_mean_s.*[j] + grand_mean);
            j += 1;
        }
        row += 1;
    }
    return true;
}

/// Cyclic Jacobi eigendecomposition of symmetric `n x n` matrix `a` (modified in-place).
/// Eigenvectors are stored column-wise in `v[row * n + col]`, eigenvalues in `evals[0..n]`.
fn jacobiSymmetricEigen(
    a: *[]f64,
    v: *[]f64,
    evals: *[]f64,
    n: usize,
    max_sweeps: usize,
    tol: f64,
): usize {
    // Initialize V = I.
    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            v.*[i * n + j] = if (i == j) 1.0 else 0.0;
            j += 1;
        }
        i += 1;
    }

    let sweeps: usize = 0;
    let sw: usize = 0;
    while (sw < max_sweeps) {
        let max_off: f64 = 0.0;
        let p: usize = 0;
        while (p < n) {
            let q: usize = p + 1;
            while (q < n) {
                const val = absF64(a.*[p * n + q]);
                if (val > max_off) {
                    max_off = val;
                }
                q += 1;
            }
            p += 1;
        }
        if (max_off <= tol) {
            break;
        }
        sweeps = sw + 1;

        let p2: usize = 0;
        while (p2 < n) {
            let q: usize = p2 + 1;
            while (q < n) {
                const apq = a.*[p2 * n + q];
                if (absF64(apq) > tol) {
                    const app = a.*[p2 * n + p2];
                    const aqq = a.*[q * n + q];
                    const tau = (aqq - app) / (2.0 * apq);
                    const root = @sqrt(1.0 + tau * tau);
                    let t: f64 = 0.0;
                    if (tau >= 0.0) {
                        t = 1.0 / (tau + root);
                    } else {
                        t = (0.0 - 1.0) / ((0.0 - tau) + root);
                    }
                    const c = 1.0 / @sqrt(1.0 + t * t);
                    const s = t * c;

                    a.*[p2 * n + p2] = app - t * apq;
                    a.*[q * n + q] = aqq + t * apq;
                    a.*[p2 * n + q] = 0.0;
                    a.*[q * n + p2] = 0.0;

                    let r: usize = 0;
                    while (r < n) {
                        if (r != p2 && r != q) {
                            const arp = a.*[r * n + p2];
                            const arq = a.*[r * n + q];
                            const new_rp = c * arp - s * arq;
                            const new_rq = s * arp + c * arq;
                            a.*[r * n + p2] = new_rp;
                            a.*[p2 * n + r] = new_rp;
                            a.*[r * n + q] = new_rq;
                            a.*[q * n + r] = new_rq;
                        }
                        const vrp = v.*[r * n + p2];
                        const vrq = v.*[r * n + q];
                        v.*[r * n + p2] = c * vrp - s * vrq;
                        v.*[r * n + q] = s * vrp + c * vrq;
                        r += 1;
                    }
                }
                q += 1;
            }
            p2 += 1;
        }
        sw += 1;
    }

    let ei: usize = 0;
    while (ei < n) {
        evals.*[ei] = a.*[ei * n + ei];
        ei += 1;
    }

    // Sort eigenpairs in descending order of eigenvalue.
    let si: usize = 0;
    while (si < n) {
        let best = si;
        let j = si + 1;
        while (j < n) {
            if (evals.*[j] > evals.*[best]) {
                best = j;
            }
            j += 1;
        }
        if (best != si) {
            const tmp_e = evals.*[si];
            evals.*[si] = evals.*[best];
            evals.*[best] = tmp_e;

            let r: usize = 0;
            while (r < n) {
                const tmp_v = v.*[r * n + si];
                v.*[r * n + si] = v.*[r * n + best];
                v.*[r * n + best] = tmp_v;
                r += 1;
            }
        }
        si += 1;
    }

    return sweeps;
}

/// Classical (Torgerson-Gower) Multidimensional Scaling (zero-alloc).
/// Computes double-centered Gram matrix, symmetric Jacobi eigendecomposition, and top `out_dims` embedding `X = V_k * sqrt(Lambda_k)`.
pub fn classicalMds(
    dist_mat: []const f64,
    n: usize,
    out_dims: usize,
    coords_out: *[]f64,
    gram_s: *[]f64,
    row_mean_s: *[]f64,
    evals_s: *[]f64,
    evecs_s: *[]f64,
    max_sweeps: usize,
    tol: f64,
): MdsResult? {
    if (
        n == 0 ||
        out_dims == 0 ||
        !isFiniteF64(tol) ||
        tol < 0.0 ||
        coords_out.*.len / out_dims < n ||
        gram_s.*.len / n < n ||
        row_mean_s.*.len < n ||
        evals_s.*.len < n ||
        evecs_s.*.len / n < n
    ) {
        return null;
    }

    if (!doubleCenterGram(dist_mat, n, gram_s, row_mean_s)) {
        return null;
    }

    const sweeps = jacobiSymmetricEigen(gram_s, evecs_s, evals_s, n, max_sweeps, tol);

    let pos_count: usize = 0;
    let total_pos_var: f64 = 0.0;
    let kept_pos_var: f64 = 0.0;
    let k: usize = 0;
    while (k < n) {
        const ev = evals_s.*[k];
        if (ev > 1.0e-12) {
            pos_count += 1;
            total_pos_var += ev;
            if (k < out_dims) {
                kept_pos_var += ev;
            }
        }
        k += 1;
    }

    let var_exp: f64 = 1.0;
    if (total_pos_var > 1.0e-30) {
        var_exp = kept_pos_var / total_pos_var;
        if (var_exp > 1.0) {
            var_exp = 1.0;
        }
        if (var_exp < 0.0) {
            var_exp = 0.0;
        }
    }

    let i: usize = 0;
    while (i < n) {
        let d: usize = 0;
        while (d < out_dims) {
            if (d < n && evals_s.*[d] > 0.0) {
                coords_out.*[i * out_dims + d] = evecs_s.*[i * n + d] * @sqrt(evals_s.*[d]);
            } else {
                coords_out.*[i * out_dims + d] = 0.0;
            }
            d += 1;
        }
        i += 1;
    }

    const stress = kruskalStress1(dist_mat, coords_out.*[0..n * out_dims], n, out_dims) ?? return null;
    return MdsResult {
        positive_eigenvalues: pos_count,
        variance_explained: var_exp,
        stress1: stress,
        sweeps: sweeps,
    };
}

/// SMACOF (Scaling by MAjorizing a COmplicated Function) iterative metric MDS refinement.
/// Refines initial coordinates `coords_inout` in-place via Guttman transforms, guaranteeing monotonically non-increasing Stress-1.
pub fn smacofRefine(
    dist_mat: []const f64,
    n: usize,
    out_dims: usize,
    coords_inout: *[]f64,
    max_iter: usize,
    tol: f64,
    b_mat_s: *[]f64,
    next_coords_s: *[]f64,
): SmacofResult? {
    if (
        !validateDistMatrix(dist_mat, n) ||
        out_dims == 0 ||
        !isFiniteF64(tol) ||
        tol < 0.0 ||
        coords_inout.*.len / out_dims < n ||
        b_mat_s.*.len / n < n ||
        next_coords_s.*.len / out_dims < n
    ) {
        return null;
    }

    let prev_stress = kruskalStress1(dist_mat, coords_inout.*[0..n * out_dims], n, out_dims) ?? return null;
    let converged: bool = false;
    let iterations: usize = 0;
    const inv_n = 1.0 / (n as f64);

    let iter: usize = 0;
    while (iter < max_iter) {
        // 1. Build Guttman matrix B(X).
        let i: usize = 0;
        while (i < n) {
            let row_sum: f64 = 0.0;
            let j: usize = 0;
            while (j < n) {
                if (i != j) {
                    let sum_sq: f64 = 0.0;
                    let d: usize = 0;
                    while (d < out_dims) {
                        const diff = coords_inout.*[i * out_dims + d] - coords_inout.*[j * out_dims + d];
                        sum_sq += diff * diff;
                        d += 1;
                    }
                    const d_hat = @sqrt(sum_sq);
                    let bij: f64 = 0.0;
                    if (d_hat > 1.0e-12) {
                        bij = (0.0 - dist_mat[i * n + j]) / d_hat;
                    }
                    b_mat_s.*[i * n + j] = bij;
                    row_sum += bij;
                }
                j += 1;
            }
            b_mat_s.*[i * n + i] = -row_sum;
            i += 1;
        }

        // 2. Guttman update: X_next = (1 / n) * B(X) * X.
        let row: usize = 0;
        while (row < n) {
            let d: usize = 0;
            while (d < out_dims) {
                let acc: f64 = 0.0;
                let j: usize = 0;
                while (j < n) {
                    acc += b_mat_s.*[row * n + j] * coords_inout.*[j * out_dims + d];
                    j += 1;
                }
                next_coords_s.*[row * out_dims + d] = acc * inv_n;
                d += 1;
            }
            row += 1;
        }

        const total = n * out_dims;
        let idx: usize = 0;
        while (idx < total) {
            coords_inout.*[idx] = next_coords_s.*[idx];
            idx += 1;
        }

        const new_stress = kruskalStress1(dist_mat, coords_inout.*[0..total], n, out_dims) ?? return null;
        iterations = iter + 1;
        if (absF64(prev_stress - new_stress) <= tol || new_stress <= tol) {
            prev_stress = new_stress;
            converged = true;
            break;
        }
        prev_stress = new_stress;
        iter += 1;
    }

    return SmacofResult {
        stress1: prev_stress,
        iterations: iterations,
        converged: converged,
    };
}
