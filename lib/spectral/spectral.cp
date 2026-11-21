// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Spectral Graph Clustering, Laplacian & NJW Embedding)

const SYM_TOL: f64 = 1.0e-9;
const ZERO_EIG_TOL: f64 = 1.0e-7;
const DEG_EPS: f64 = 1.0e-15;

/// Graph Laplacian formulation.
pub enum LaplacianKind {
    Unnormalized,
    SymmetricNormalized,
}

/// Result summary returned by `fiedlerBisection`.
pub struct FiedlerResult {
    algebraic_connectivity: f64,
    connected_components: usize,
    sweeps: usize,
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn validateSymmetricAdjacency(adj: []const f64, n: usize): bool {
    if (n == 0 || adj.len / n < n) {
        return false;
    }
    let row: usize = 0;
    while (row < n) {
        let col: usize = row;
        while (col < n) {
            const a = adj[row * n + col];
            const b = adj[col * n + row];
            if (!isFinite(a) || !isFinite(b) || a < 0.0 || b < 0.0) {
                return false;
            }
            const tol = SYM_TOL * (1.0 + absF64(a));
            if (absF64(a - b) > tol) {
                return false;
            }
            col += 1;
        }
        row += 1;
    }
    return true;
}

/// Builds the graph Laplacian (`L = D - W` or `L_sym = I - D^-1/2 W D^-1/2`) from a symmetric non-negative adjacency matrix `adj` (`n x n`).
/// Isolated vertices (`deg == 0`) are safely assigned zero rows/columns in `L_sym`.
pub fn buildLaplacian(
    adj: []const f64,
    n: usize,
    kind: LaplacianKind,
    lap_out: *[]f64,
    deg_out: *[]f64,
): bool {
    if (!validateSymmetricAdjacency(adj, n)) {
        return false;
    }
    if (lap_out.*.len / n < n || deg_out.*.len < n) {
        return false;
    }

    // Compute vertex degrees
    let i: usize = 0;
    while (i < n) {
        let d: f64 = 0.0;
        const row_off = i * n;
        let j: usize = 0;
        while (j < n) {
            d += adj[row_off + j];
            j += 1;
        }
        deg_out.*[i] = d;
        i += 1;
    }

    match (kind) {
        LaplacianKind.Unnormalized => {
            let r: usize = 0;
            while (r < n) {
                const row_off = r * n;
                let c: usize = 0;
                while (c < n) {
                    if (r == c) {
                        lap_out.*[row_off + c] = deg_out.*[r] - adj[row_off + c];
                    } else {
                        lap_out.*[row_off + c] = 0.0 - adj[row_off + c];
                    }
                    c += 1;
                }
                r += 1;
            }
        },
        LaplacianKind.SymmetricNormalized => {
            let r: usize = 0;
            while (r < n) {
                const row_off = r * n;
                const dr = deg_out.*[r];
                const inv_r = if (dr > DEG_EPS) 1.0 / @sqrt(dr) else 0.0;
                let c: usize = 0;
                while (c < n) {
                    if (r == c) {
                        if (dr > DEG_EPS) {
                            lap_out.*[row_off + c] = 1.0 - adj[row_off + c] / dr;
                        } else {
                            lap_out.*[row_off + c] = 0.0;
                        }
                    } else {
                        const dc = deg_out.*[c];
                        if (dr > DEG_EPS && dc > DEG_EPS) {
                            const inv_c = 1.0 / @sqrt(dc);
                            lap_out.*[row_off + c] = (0.0 - adj[row_off + c]) * inv_r * inv_c;
                        } else {
                            lap_out.*[row_off + c] = 0.0;
                        }
                    }
                    c += 1;
                }
                r += 1;
            }
        },
    }
    return true;
}

// Cyclic Jacobi symmetric eigensolver (in-place on `a`, eigenvectors stored in columns of `v`),
// followed by ascending eigenvalue sort. Returns number of sweeps performed.
fn jacobiSymmetricEigen(
    a: []f64,
    evals: []f64,
    v: []f64,
    n: usize,
    max_sweeps: usize,
    tol: f64,
): usize {
    // Initialize V = I
    let r0: usize = 0;
    while (r0 < n) {
        const row_off = r0 * n;
        let c0: usize = 0;
        while (c0 < n) {
            v[row_off + c0] = if (r0 == c0) 1.0 else 0.0;
            c0 += 1;
        }
        r0 += 1;
    }

    let sweeps: usize = 0;
    while (sweeps < max_sweeps) {
        let max_off: f64 = 0.0;
        let p: usize = 0;
        while (p < n) {
            let q = p + 1;
            while (q < n) {
                const apq = a[p * n + q];
                const abs_apq = absF64(apq);
                if (abs_apq > max_off) {
                    max_off = abs_apq;
                }
                if (abs_apq > tol) {
                    const app = a[p * n + p];
                    const aqq = a[q * n + q];
                    const tau = (aqq - app) / (2.0 * apq);
                    const t = if (tau >= 0.0)
                        1.0 / (tau + @sqrt(1.0 + tau * tau))
                    else
                        -1.0 / ((0.0 - tau) + @sqrt(1.0 + tau * tau));
                    const cos_t = 1.0 / @sqrt(1.0 + t * t);
                    const sin_t = t * cos_t;

                    a[p * n + p] = app - t * apq;
                    a[q * n + q] = aqq + t * apq;
                    a[p * n + q] = 0.0;
                    a[q * n + p] = 0.0;

                    let k: usize = 0;
                    while (k < n) {
                        if (k != p && k != q) {
                            const akp = a[k * n + p];
                            const akq = a[k * n + q];
                            const new_p = cos_t * akp - sin_t * akq;
                            const new_q = sin_t * akp + cos_t * akq;
                            a[k * n + p] = new_p;
                            a[p * n + k] = new_p;
                            a[k * n + q] = new_q;
                            a[q * n + k] = new_q;
                        }
                        const vkp = v[k * n + p];
                        const vkq = v[k * n + q];
                        v[k * n + p] = cos_t * vkp - sin_t * vkq;
                        v[k * n + q] = sin_t * vkp + cos_t * vkq;
                        k += 1;
                    }
                }
                q += 1;
            }
            p += 1;
        }
        sweeps += 1;
        if (max_off <= tol) {
            break;
        }
    }

    let d_i: usize = 0;
    while (d_i < n) {
        evals[d_i] = a[d_i * n + d_i];
        d_i += 1;
    }

    // Selection sort eigenvalues ascending and permute eigenvector columns accordingly
    let s_i: usize = 0;
    while (s_i < n) {
        let min_idx = s_i;
        let scan = s_i + 1;
        while (scan < n) {
            if (evals[scan] < evals[min_idx]) {
                min_idx = scan;
            }
            scan += 1;
        }
        if (min_idx != s_i) {
            const tmp_e = evals[s_i];
            evals[s_i] = evals[min_idx];
            evals[min_idx] = tmp_e;

            let row: usize = 0;
            while (row < n) {
                const tmp_v = v[row * n + s_i];
                v[row * n + s_i] = v[row * n + min_idx];
                v[row * n + min_idx] = tmp_v;
                row += 1;
            }
        }
        s_i += 1;
    }
    return sweeps;
}

/// Computes algebraic connectivity (`lambda_2`), connected component count, Fiedler vector,
/// and 2-way spectral bisection (`0`/`1` labels in `partition_out`).
pub fn fiedlerBisection(
    adj: []const f64,
    n: usize,
    kind: LaplacianKind,
    fiedler_vec_out: *[]f64,
    partition_out: *[]u8,
    lap_s: *[]f64,
    deg_s: *[]f64,
    evals_s: *[]f64,
    evecs_s: *[]f64,
    max_sweeps: usize,
    tol: f64,
): FiedlerResult? {
    if (n < 2 || max_sweeps == 0 || !isFinite(tol) || tol <= 0.0) {
        return null;
    }
    if (fiedler_vec_out.*.len < n || partition_out.*.len < n) {
        return null;
    }
    if (lap_s.*.len / n < n || deg_s.*.len < n || evals_s.*.len < n || evecs_s.*.len / n < n) {
        return null;
    }
    if (!buildLaplacian(adj, n, kind, lap_s, deg_s)) {
        return null;
    }

    const sweeps = jacobiSymmetricEigen(lap_s.*, evals_s.*, evecs_s.*, n, max_sweeps, tol);

    // Count connected components (multiplicity of zero eigenvalue)
    let components: usize = 0;
    let i_c: usize = 0;
    while (i_c < n) {
        if (evals_s.*[i_c] <= ZERO_EIG_TOL) {
            components += 1;
        }
        i_c += 1;
    }
    if (components == 0) {
        components = 1;
    }

    const lam2 = if (evals_s.*[1] > 0.0) evals_s.*[1] else 0.0;

    // Extract Fiedler vector (column 1) and canonicalize sign on first non-trivial entry
    let sign_flip: f64 = 1.0;
    let s_find: usize = 0;
    while (s_find < n) {
        const val = evecs_s.*[s_find * n + 1];
        if (absF64(val) > 1.0e-12) {
            if (val < 0.0) {
                sign_flip = -1.0;
            }
            break;
        }
        s_find += 1;
    }

    let pos_cnt: usize = 0;
    let sum_f: f64 = 0.0;
    let idx: usize = 0;
    while (idx < n) {
        const fv = evecs_s.*[idx * n + 1] * sign_flip;
        fiedler_vec_out.*[idx] = fv;
        sum_f += fv;
        if (fv >= 0.0) {
            partition_out.*[idx] = 1;
            pos_cnt += 1;
        } else {
            partition_out.*[idx] = 0;
        }
        idx += 1;
    }

    // Fallback to mean threshold if sign split was degenerate (e.g. disconnected orthogonal basis)
    if (pos_cnt == 0 || pos_cnt == n) {
        const mean_f = sum_f / (n as f64);
        let idx2: usize = 0;
        while (idx2 < n) {
            partition_out.*[idx2] = if (fiedler_vec_out.*[idx2] >= mean_f) 1 else 0;
            idx2 += 1;
        }
    }

    return {
        algebraic_connectivity: lam2,
        connected_components: components,
        sweeps: sweeps,
    };
}

/// Ng-Jordan-Weiss (NJW) k-way spectral embedding using the `k` smallest eigenvectors of `L_sym`
/// with per-row L2 unit normalization written to `embedding_out` (`n x k`, row-major).
pub fn spectralEmbedding(
    adj: []const f64,
    n: usize,
    k: usize,
    embedding_out: *[]f64,
    lap_s: *[]f64,
    deg_s: *[]f64,
    evals_s: *[]f64,
    evecs_s: *[]f64,
    max_sweeps: usize,
    tol: f64,
): bool {
    if (n == 0 || k == 0 || k > n || max_sweeps == 0 || !isFinite(tol) || tol <= 0.0) {
        return false;
    }
    if (embedding_out.*.len / n < k || lap_s.*.len / n < n || deg_s.*.len < n || evals_s.*.len < n || evecs_s.*.len / n < n) {
        return false;
    }
    if (!buildLaplacian(adj, n, LaplacianKind.SymmetricNormalized, lap_s, deg_s)) {
        return false;
    }

    * = jacobiSymmetricEigen(lap_s.*, evals_s.*, evecs_s.*, n, max_sweeps, tol);

    let row: usize = 0;
    while (row < n) {
        let sq_sum: f64 = 0.0;
        let col: usize = 0;
        while (col < k) {
            const v = evecs_s.*[row * n + col];
            embedding_out.*[row * k + col] = v;
            sq_sum += v * v;
            col += 1;
        }
        const norm = @sqrt(sq_sum);
        if (norm > DEG_EPS) {
            let col2: usize = 0;
            while (col2 < k) {
                embedding_out.*[row * k + col2] = embedding_out.*[row * k + col2] / norm;
                col2 += 1;
            }
        }
        row += 1;
    }
    return true;
}

/// Computes Normalized Cut `NCut(A, B) = cut(A, B) / vol(A) + cut(A, B) / vol(B)` for a binary partition (`0`/`1`).
/// Returns `null` on invalid adjacency/partition or if either side has zero volume.
pub fn normalizedCut(adj: []const f64, n: usize, partition: []const u8): f64? {
    if (n < 2 || partition.len < n || !validateSymmetricAdjacency(adj, n)) {
        return null;
    }
    let i_chk: usize = 0;
    while (i_chk < n) {
        if (partition[i_chk] > 1) {
            return null;
        }
        i_chk += 1;
    }

    let cut_ab: f64 = 0.0;
    let vol_a: f64 = 0.0;
    let vol_b: f64 = 0.0;

    let i: usize = 0;
    while (i < n) {
        const pi = partition[i];
        const row_off = i * n;
        let deg_i: f64 = 0.0;
        let j: usize = 0;
        while (j < n) {
            const w = adj[row_off + j];
            deg_i += w;
            if (pi == 0 && partition[j] == 1) {
                cut_ab += w;
            }
            j += 1;
        }
        if (pi == 0) {
            vol_a += deg_i;
        } else {
            vol_b += deg_i;
        }
        i += 1;
    }

    if (vol_a <= DEG_EPS || vol_b <= DEG_EPS) {
        return null;
    }
    return (cut_ab / vol_a) + (cut_ab / vol_b);
}

/// Builds a symmetric Gaussian RBF affinity matrix `W[i,j] = exp(-||x_i - x_j||^2 / (2 * sigma^2))` with zero diagonal.
pub fn rbfAffinityMatrix(
    points: []const f64,
    n_samples: usize,
    n_features: usize,
    sigma: f64,
    adj_out: *[]f64,
): bool {
    if (n_samples == 0 || n_features == 0 || !isFinite(sigma) || sigma <= 0.0) {
        return false;
    }
    if (points.len / n_samples < n_features || adj_out.*.len / n_samples < n_samples) {
        return false;
    }
    const total = n_samples * n_features;
    let chk: usize = 0;
    while (chk < total) {
        if (!isFinite(points[chk])) {
            return false;
        }
        chk += 1;
    }

    const inv_2sig2 = 0.5 / (sigma * sigma);
    let i: usize = 0;
    while (i < n_samples) {
        adj_out.*[i * n_samples + i] = 0.0;
        const off_i = i * n_features;
        let j = i + 1;
        while (j < n_samples) {
            const off_j = j * n_features;
            let d2: f64 = 0.0;
            let f: usize = 0;
            while (f < n_features) {
                const diff = points[off_i + f] - points[off_j + f];
                d2 += diff * diff;
                f += 1;
            }
            const w = @exp((0.0 - d2) * inv_2sig2);
            adj_out.*[i * n_samples + j] = w;
            adj_out.*[j * n_samples + i] = w;
            j += 1;
        }
        i += 1;
    }
    return true;
}
