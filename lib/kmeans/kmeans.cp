// Origin: Pure-CP Stdlib (Zero-Alloc D-Dimensional K-Means++ Clustering, Lloyd Iteration, Empty-Cluster Recovery & Silhouette Score) | Created: 2026-11-22

import std from "std";

const INV_2_POW_53: f64 = 1.0 / 9007199254740992.0;

/// Summary metrics returned by `kmeansFit`.
pub struct KmeansResult {
    iterations: u32,
    converged: bool,
    inertia: f64,
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
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

const SPLITMIX_INC: u64 = 0x9E3779B97F4A7C15;

fn splitmix64Mix(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn u64ToFloat01(r: u64): f64 {
    const bits: u64 = r >> 11;
    return (bits as f64) * INV_2_POW_53;
}

/// Computes the squared Euclidean distance between two vectors `a` and `b`.
/// Iterates up to `min(a.len, b.len)`.
pub fn sqDist(a: []const f64, b: []const f64): f64 {
    const n: usize = if (a.len < b.len) a.len else b.len;
    let sum: f64 = 0.0;
    let d: usize = 0;
    while (d < n) {
        const diff: f64 = a[d] - b[d];
        sum += diff * diff;
        d += 1;
    }
    return sum;
}

/// Assigns a single `dim`-dimensional `point` to its nearest centroid index in `0..k-1`.
/// Returns `null` on invalid dimensions, insufficient buffers, or non-finite inputs.
pub fn kmeansPredictOne(point: []const f64, centroids: []const f64, k: usize, dim: usize): usize? {
    if (k == 0 || dim == 0 || point.len < dim || !checkMatrixLen(centroids.len, k, dim)) {
        return null;
    }
    if (!checkFinitePrefix(point, dim) || !checkFinitePrefix(centroids, k * dim)) {
        return null;
    }

    let best_c: usize = 0;
    let best_d: f64 = sqDist(point[0..dim], centroids[0..dim]);
    let c: usize = 1;
    while (c < k) {
        const base: usize = c * dim;
        const d: f64 = sqDist(point[0..dim], centroids[base..(base + dim)]);
        if (d < best_d) {
            best_d = d;
            best_c = c;
        }
        c += 1;
    }
    return best_c;
}

/// Deterministic K-Means++ centroid initialization using SplitMix64 and D^2-weighted sampling.
/// - `points`: row-major `n x dim` input samples (`points.len >= n * dim`).
/// - `centroids_out`: caller-owned buffer of length `>= k * dim`.
/// - `min_dists_scratch`: caller-owned scratch buffer of length `>= n`.
/// Returns `false` on invalid arguments, insufficient buffers, or non-finite coordinates.
pub fn kmeansInitPlusPlus(
    points: []const f64,
    n: usize,
    dim: usize,
    k: usize,
    seed: u64,
    centroids_out: *[]f64,
    min_dists_scratch: *[]f64,
): bool {
    if (n == 0 || dim == 0 || k == 0 || k > n) {
        return false;
    }
    if (!checkMatrixLen(points.len, n, dim) || !checkMatrixLen(centroids_out.*.len, k, dim)) {
        return false;
    }
    if (min_dists_scratch.*.len < n) {
        return false;
    }
    if (!checkFinitePrefix(points, n * dim)) {
        return false;
    }

    let rng: u64 = seed;
    rng = rng +% SPLITMIX_INC;
    const first_idx: usize = (splitmix64Mix(rng) % (n as u64)) as usize;
    const first_base: usize = first_idx * dim;
    let d: usize = 0;
    while (d < dim) {
        centroids_out.*[d] = points[first_base + d];
        d += 1;
    }

    let i: usize = 0;
    while (i < n) {
        const p_base: usize = i * dim;
        min_dists_scratch.*[i] = sqDist(points[p_base..(p_base + dim)], centroids_out.*[0..dim]);
        i += 1;
    }

    let c: usize = 1;
    while (c < k) {
        let total_dist: f64 = 0.0;
        let last_pos: usize = n;
        i = 0;
        while (i < n) {
            const md: f64 = min_dists_scratch.*[i];
            total_dist += md;
            if (md > 0.0) {
                last_pos = i;
            }
            i += 1;
        }

        let chosen: usize = 0;
        if (total_dist > 0.0 && last_pos < n) {
            rng = rng +% SPLITMIX_INC;
            const target: f64 = u64ToFloat01(splitmix64Mix(rng)) * total_dist;
            let cum: f64 = 0.0;
            let found: bool = false;
            i = 0;
            while (i < n) {
                const md: f64 = min_dists_scratch.*[i];
                cum += md;
                if (md > 0.0 && cum > target) {
                    chosen = i;
                    found = true;
                    break;
                }
                i += 1;
            }
            if (!found) {
                chosen = last_pos;
            }
        } else {
            rng = rng +% SPLITMIX_INC;
            chosen = (splitmix64Mix(rng) % (n as u64)) as usize;
        }

        const c_base: usize = c * dim;
        const p_chosen_base: usize = chosen * dim;
        d = 0;
        while (d < dim) {
            centroids_out.*[c_base + d] = points[p_chosen_base + d];
            d += 1;
        }

        if (c + 1 < k) {
            i = 0;
            while (i < n) {
                const p_base: usize = i * dim;
                const dist_sq: f64 = sqDist(
                    points[p_base..(p_base + dim)],
                    centroids_out.*[c_base..(c_base + dim)],
                );
                if (dist_sq < min_dists_scratch.*[i]) {
                    min_dists_scratch.*[i] = dist_sq;
                }
                i += 1;
            }
        }
        c += 1;
    }

    return true;
}

/// Computes total within-cluster sum of squared errors (SSE / inertia).
/// Returns `null` on invalid dimensions, out-of-range labels, or non-finite values.
pub fn kmeansInertia(
    points: []const f64,
    n: usize,
    dim: usize,
    centroids: []const f64,
    k: usize,
    labels: []const usize,
): f64? {
    if (n == 0 || dim == 0 || k == 0) {
        return null;
    }
    if (!checkMatrixLen(points.len, n, dim) || !checkMatrixLen(centroids.len, k, dim)) {
        return null;
    }
    if (labels.len < n) {
        return null;
    }

    let sse: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const c: usize = labels[i];
        if (c >= k) {
            return null;
        }
        const p_base: usize = i * dim;
        const c_base: usize = c * dim;
        const dist_sq: f64 = sqDist(
            points[p_base..(p_base + dim)],
            centroids[c_base..(c_base + dim)],
        );
        if (!isFiniteF64(dist_sq)) {
            return null;
        }
        sse += dist_sq;
        i += 1;
    }
    if (!isFiniteF64(sse)) {
        return null;
    }
    return sse;
}

/// Fits K-Means++ clustering via deterministic initialization and Lloyd iterations.
/// Automatically recovers empty clusters by re-seeding with the farthest sample point.
/// - `centroids_out`: length `>= k * dim`
/// - `labels_out`: length `>= n`
/// - `counts_scratch`: length `>= k`
/// - `dists_scratch`: length `>= n`
/// Returns `null` if inputs or buffers are invalid.
pub fn kmeansFit(
    points: []const f64,
    n: usize,
    dim: usize,
    k: usize,
    max_iter: u32,
    tol: f64,
    seed: u64,
    centroids_out: *[]f64,
    labels_out: *[]usize,
    counts_scratch: *[]usize,
    dists_scratch: *[]f64,
): KmeansResult? {
    if (n == 0 || dim == 0 || k == 0 || k > n) {
        return null;
    }
    if (!isFiniteF64(tol) || tol < 0.0) {
        return null;
    }
    if (!checkMatrixLen(points.len, n, dim) || !checkMatrixLen(centroids_out.*.len, k, dim)) {
        return null;
    }
    if (labels_out.*.len < n || counts_scratch.*.len < k || dists_scratch.*.len < n) {
        return null;
    }
    if (!checkFinitePrefix(points, n * dim)) {
        return null;
    }

    if (!kmeansInitPlusPlus(points, n, dim, k, seed, centroids_out, dists_scratch)) {
        return null;
    }

    let i: usize = 0;
    while (i < n) {
        labels_out.*[i] = k;
        i += 1;
    }

    if (max_iter == 0) {
        i = 0;
        while (i < n) {
            const p_base: usize = i * dim;
            let best_c: usize = 0;
            let best_d: f64 = sqDist(points[p_base..(p_base + dim)], centroids_out.*[0..dim]);
            let c: usize = 1;
            while (c < k) {
                const c_base: usize = c * dim;
                const d_sq: f64 = sqDist(
                    points[p_base..(p_base + dim)],
                    centroids_out.*[c_base..(c_base + dim)],
                );
                if (d_sq < best_d) {
                    best_d = d_sq;
                    best_c = c;
                }
                c += 1;
            }
            labels_out.*[i] = best_c;
            dists_scratch.*[i] = best_d;
            i += 1;
        }
        const init_inertia = kmeansInertia(points, n, dim, centroids_out.*, k, labels_out.*) ?? return null;
        return KmeansResult {
            iterations: 0,
            converged: false,
            inertia: init_inertia,
        };
    }

    const tol_sq: f64 = tol * tol;
    let iterations_done: u32 = 0;
    let converged: bool = false;
    let sum_block: [64]f64 = std.mem.zeroes([64]f64);

    let iter: u32 = 0;
    while (iter < max_iter) {
        let c: usize = 0;
        while (c < k) {
            counts_scratch.*[c] = 0;
            c += 1;
        }

        let changed: bool = false;
        i = 0;
        while (i < n) {
            const p_base: usize = i * dim;
            let best_c: usize = 0;
            let best_d: f64 = sqDist(points[p_base..(p_base + dim)], centroids_out.*[0..dim]);
            c = 1;
            while (c < k) {
                const c_base: usize = c * dim;
                const d_sq: f64 = sqDist(
                    points[p_base..(p_base + dim)],
                    centroids_out.*[c_base..(c_base + dim)],
                );
                if (d_sq < best_d) {
                    best_d = d_sq;
                    best_c = c;
                }
                c += 1;
            }
            if (labels_out.*[i] != best_c) {
                labels_out.*[i] = best_c;
                changed = true;
            }
            counts_scratch.*[best_c] += 1;
            dists_scratch.*[i] = best_d;
            i += 1;
        }

        // Empty-cluster recovery: steal the farthest sample from a non-singleton cluster
        let c_rec: usize = 0;
        while (c_rec < k) {
            if (counts_scratch.*[c_rec] == 0) {
                let far_idx: usize = 0;
                let far_dist: f64 = -1.0;
                let found_donor: bool = false;
                let idx_d: usize = 0;
                while (idx_d < n) {
                    const donor_c: usize = labels_out.*[idx_d];
                    if (counts_scratch.*[donor_c] > 1) {
                        const d_sq: f64 = dists_scratch.*[idx_d];
                        if (!found_donor || d_sq > far_dist) {
                            far_dist = d_sq;
                            far_idx = idx_d;
                            found_donor = true;
                        }
                    }
                    idx_d += 1;
                }
                if (found_donor) {
                    const old_c: usize = labels_out.*[far_idx];
                    counts_scratch.*[old_c] -= 1;
                    counts_scratch.*[c_rec] = 1;
                    labels_out.*[far_idx] = c_rec;
                    dists_scratch.*[far_idx] = 0.0;
                    changed = true;
                }
            }
            c_rec += 1;
        }

        // Update centroids in-place in 64-dim stack blocks and measure max centroid shift
        let max_shift_sq: f64 = 0.0;
        let c_upd: usize = 0;
        while (c_upd < k) {
            const cnt: usize = counts_scratch.*[c_upd];
            if (cnt > 0) {
                const inv_cnt: f64 = 1.0 / (cnt as f64);
                const c_base: usize = c_upd * dim;
                let shift_sq: f64 = 0.0;
                let d_base: usize = 0;
                while (d_base < dim) {
                    const rem: usize = dim - d_base;
                    const blk: usize = if (rem < 64) rem else (64 as usize);
                    let b: usize = 0;
                    while (b < blk) {
                        sum_block[b] = 0.0;
                        b += 1;
                    }
                    let idx_p: usize = 0;
                    while (idx_p < n) {
                        if (labels_out.*[idx_p] == c_upd) {
                            const p_off: usize = idx_p * dim + d_base;
                            let b_acc: usize = 0;
                            while (b_acc < blk) {
                                sum_block[b_acc] += points[p_off + b_acc];
                                b_acc += 1;
                            }
                        }
                        idx_p += 1;
                    }
                    let b_w: usize = 0;
                    while (b_w < blk) {
                        const mean_v: f64 = sum_block[b_w] * inv_cnt;
                        const old_v: f64 = centroids_out.*[c_base + d_base + b_w];
                        const diff: f64 = mean_v - old_v;
                        shift_sq += diff * diff;
                        centroids_out.*[c_base + d_base + b_w] = mean_v;
                        b_w += 1;
                    }
                    d_base += blk;
                }
                if (shift_sq > max_shift_sq) {
                    max_shift_sq = shift_sq;
                }
            }
            c_upd += 1;
        }

        iterations_done = iter + 1;
        if (!changed || max_shift_sq <= tol_sq) {
            converged = true;
            break;
        }
        iter += 1;
    }

    const final_inertia = kmeansInertia(points, n, dim, centroids_out.*, k, labels_out.*) ?? return null;
    return KmeansResult {
        iterations: iterations_done,
        converged: converged,
        inertia: final_inertia,
    };
}

/// Computes the global mean Silhouette score in `[-1.0, 1.0]` for a clustering assignment.
/// Requires `n >= 2`, `dim >= 1`, `2 <= k <= n`, and at least two non-empty clusters.
/// Returns `null` on invalid inputs or degenerate single-cluster assignments.
pub fn silhouetteScore(
    points: []const f64,
    n: usize,
    dim: usize,
    labels: []const usize,
    k: usize,
): f64? {
    if (n < 2 || dim == 0 || k < 2 || k > n) {
        return null;
    }
    if (!checkMatrixLen(points.len, n, dim) || labels.len < n) {
        return null;
    }
    if (!checkFinitePrefix(points, n * dim)) {
        return null;
    }

    let i: usize = 0;
    while (i < n) {
        if (labels[i] >= k) {
            return null;
        }
        i += 1;
    }

    let total_s: f64 = 0.0;
    let i_sil: usize = 0;
    while (i_sil < n) {
        const ci: usize = labels[i_sil];
        const pi_base: usize = i_sil * dim;
        const pi: []const f64 = points[pi_base..(pi_base + dim)];

        let intra_sum: f64 = 0.0;
        let intra_cnt: usize = 0;
        let j_in: usize = 0;
        while (j_in < n) {
            if (j_in != i_sil && labels[j_in] == ci) {
                const pj_base: usize = j_in * dim;
                intra_sum += std.math.sqrt(sqDist(pi, points[pj_base..(pj_base + dim)]));
                intra_cnt += 1;
            }
            j_in += 1;
        }

        let has_other: bool = false;
        let min_inter: f64 = 0.0;
        let c: usize = 0;
        while (c < k) {
            if (c != ci) {
                let inter_sum: f64 = 0.0;
                let inter_cnt: usize = 0;
                let j_out: usize = 0;
                while (j_out < n) {
                    if (labels[j_out] == c) {
                        const pj_base: usize = j_out * dim;
                        inter_sum += std.math.sqrt(sqDist(pi, points[pj_base..(pj_base + dim)]));
                        inter_cnt += 1;
                    }
                    j_out += 1;
                }
                if (inter_cnt > 0) {
                    const mean_inter: f64 = inter_sum / (inter_cnt as f64);
                    if (!has_other || mean_inter < min_inter) {
                        min_inter = mean_inter;
                        has_other = true;
                    }
                }
            }
            c += 1;
        }

        if (!has_other) {
            return null;
        }

        let s_i: f64 = 0.0;
        if (intra_cnt > 0) {
            const a_i: f64 = intra_sum / (intra_cnt as f64);
            const b_i: f64 = min_inter;
            const denom: f64 = if (a_i > b_i) a_i else b_i;
            if (denom > 0.0) {
                s_i = (b_i - a_i) / denom;
            }
        }
        total_s += s_i;
        i_sil += 1;
    }

    return total_s / (n as f64);
}
