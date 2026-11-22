// Origin: Pure-CP Stdlib (Zero-Alloc Density-Based Spatial Clustering DBSCAN & Silhouette Score) | Created: 2026-11-22

import std from "std";

/// Cluster label assigned to noise (outlier) points.
pub const DBSCAN_NOISE: i32 = -1;

/// Internal sentinel for unvisited points prior to clustering.
pub const DBSCAN_UNVISITED: i32 = -2;

/// Distance metric used for neighborhood queries and silhouette evaluation.
pub enum DbscanMetric {
    Euclidean,
    Manhattan,
    Chebyshev,
    Cosine,
}

/// Summary statistics returned by `dbscanCluster` and `dbscanFromDistanceMatrix`.
pub struct DbscanResult {
    num_clusters: usize;
    noise_count: usize;
    core_count: usize;
}

/// Caller-provided zero-allocation workspace for BFS cluster expansion.
/// Each slice must have length `>= n_samples`.
pub struct DbscanWorkspace {
    queue: []usize;
    in_queue: []u8;
    neighbors_tmp: []usize;
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn checkRectLen(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn pointDistance(
    points: []const f64,
    n_features: usize,
    a: usize,
    b: usize,
    metric: DbscanMetric,
): f64 {
    const off_a = a * n_features;
    const off_b = b * n_features;

    if (metric == DbscanMetric.Euclidean) {
        let sum_sq: f64 = 0.0;
        let f: usize = 0;
        while (f < n_features) {
            const d = points[off_a + f] - points[off_b + f];
            sum_sq = sum_sq + d * d;
            f += 1;
        }
        return std.math.sqrt(sum_sq);
    }

    if (metric == DbscanMetric.Manhattan) {
        let sum_abs: f64 = 0.0;
        let f: usize = 0;
        while (f < n_features) {
            sum_abs = sum_abs + absF64(points[off_a + f] - points[off_b + f]);
            f += 1;
        }
        return sum_abs;
    }

    if (metric == DbscanMetric.Chebyshev) {
        let max_abs: f64 = 0.0;
        let f: usize = 0;
        while (f < n_features) {
            const d = absF64(points[off_a + f] - points[off_b + f]);
            if (d > max_abs) {
                max_abs = d;
            }
            f += 1;
        }
        return max_abs;
    }

    // Cosine distance: 1 - dot(a, b) / (||a|| * ||b||)
    let dot: f64 = 0.0;
    let na: f64 = 0.0;
    let nb: f64 = 0.0;
    let f: usize = 0;
    while (f < n_features) {
        const va = points[off_a + f];
        const vb = points[off_b + f];
        dot = dot + va * vb;
        na = na + va * va;
        nb = nb + vb * vb;
        f += 1;
    }
    if (na == 0.0 && nb == 0.0) {
        return 0.0;
    }
    if (na == 0.0 || nb == 0.0) {
        return 1.0;
    }
    let sim = dot / (std.math.sqrt(na) * std.math.sqrt(nb));
    if (sim > 1.0) {
        sim = 1.0;
    }
    if (sim < -1.0) {
        sim = -1.0;
    }
    return 1.0 - sim;
}

fn regionQueryPoints(
    points: []const f64,
    n_samples: usize,
    n_features: usize,
    p: usize,
    eps: f64,
    metric: DbscanMetric,
    out_neighbors: []usize,
): usize {
    let cnt: usize = 0;
    let j: usize = 0;
    while (j < n_samples) {
        if (pointDistance(points, n_features, p, j, metric) <= eps) {
            out_neighbors[cnt] = j;
            cnt += 1;
        }
        j += 1;
    }
    return cnt;
}

fn regionQueryMatrix(
    dist_mat: []const f64,
    n_samples: usize,
    p: usize,
    eps: f64,
    out_neighbors: []usize,
): usize {
    let cnt: usize = 0;
    const row_off = p * n_samples;
    let j: usize = 0;
    while (j < n_samples) {
        if (dist_mat[row_off + j] <= eps) {
            out_neighbors[cnt] = j;
            cnt += 1;
        }
        j += 1;
    }
    return cnt;
}

/// Performs Ester (1996) DBSCAN density-based clustering on an `n_samples x n_features`
/// row-major point set using caller-provided workspace buffers (zero heap allocation).
///
/// Writes cluster IDs `0..num_clusters-1` or `DBSCAN_NOISE` (`-1`) into `labels_out[0..n_samples]`,
/// and core-point flags (`1` for core, `0` for border/noise) into `is_core_out[0..n_samples]`.
///
/// Returns `null` if `n_samples == 0`, `n_features == 0`, `eps <= 0` or non-finite, `min_pts == 0`,
/// any buffer is undersized, or `points` contains non-finite values.
pub fn dbscanCluster(
    points: []const f64,
    n_samples: usize,
    n_features: usize,
    eps: f64,
    min_pts: usize,
    metric: DbscanMetric,
    labels_out: []i32,
    is_core_out: []u8,
    ws: *DbscanWorkspace,
): DbscanResult? {
    if (!checkRectLen(points.len, n_samples, n_features) || !isFiniteF64(eps) || eps <= 0.0 || min_pts == 0) {
        return null;
    }
    if (labels_out.len < n_samples || is_core_out.len < n_samples) {
        return null;
    }
    if (ws.queue.len < n_samples || ws.in_queue.len < n_samples || ws.neighbors_tmp.len < n_samples) {
        return null;
    }

    const total = n_samples * n_features;
    let chk: usize = 0;
    while (chk < total) {
        if (!isFiniteF64(points[chk])) {
            return null;
        }
        chk += 1;
    }

    let i: usize = 0;
    while (i < n_samples) {
        labels_out[i] = DBSCAN_UNVISITED;
        is_core_out[i] = 0;
        ws.in_queue[i] = 0;
        i += 1;
    }

    let num_clusters: usize = 0;
    let p: usize = 0;
    while (p < n_samples) {
        if (labels_out[p] == DBSCAN_UNVISITED) {
            const cnt = regionQueryPoints(points, n_samples, n_features, p, eps, metric, ws.neighbors_tmp);
            if (cnt < min_pts) {
                labels_out[p] = DBSCAN_NOISE;
            } else {
                const cid = num_clusters as i32;
                num_clusters += 1;
                labels_out[p] = cid;
                is_core_out[p] = 1;

                let head: usize = 0;
                let tail: usize = 0;
                let ni: usize = 0;
                while (ni < cnt) {
                    const nb = ws.neighbors_tmp[ni];
                    if (nb != p && ws.in_queue[nb] == 0) {
                        ws.in_queue[nb] = 1;
                        ws.queue[tail] = nb;
                        tail += 1;
                    }
                    ni += 1;
                }

                while (head < tail) {
                    const q = ws.queue[head];
                    head += 1;
                    if (labels_out[q] == DBSCAN_NOISE) {
                        labels_out[q] = cid;
                    } else if (labels_out[q] == DBSCAN_UNVISITED) {
                        labels_out[q] = cid;
                        const q_cnt = regionQueryPoints(points, n_samples, n_features, q, eps, metric, ws.neighbors_tmp);
                        if (q_cnt >= min_pts) {
                            is_core_out[q] = 1;
                            let qi: usize = 0;
                            while (qi < q_cnt) {
                                const nb = ws.neighbors_tmp[qi];
                                if (ws.in_queue[nb] == 0 && (labels_out[nb] == DBSCAN_UNVISITED || labels_out[nb] == DBSCAN_NOISE)) {
                                    ws.in_queue[nb] = 1;
                                    ws.queue[tail] = nb;
                                    tail += 1;
                                }
                                qi += 1;
                            }
                        }
                    }
                }

                // Clear in_queue markers touched during this cluster expansion
                let clr: usize = 0;
                while (clr < tail) {
                    ws.in_queue[ws.queue[clr]] = 0;
                    clr += 1;
                }
            }
        }
        p += 1;
    }

    let noise_count: usize = 0;
    let core_count: usize = 0;
    i = 0;
    while (i < n_samples) {
        if (labels_out[i] == DBSCAN_NOISE) {
            noise_count += 1;
        }
        if (is_core_out[i] != 0) {
            core_count += 1;
        }
        i += 1;
    }

    return DbscanResult {
        num_clusters: num_clusters,
        noise_count: noise_count,
        core_count: core_count,
    };
}

/// Performs Ester (1996) DBSCAN clustering directly on a precomputed `n_samples x n_samples`
/// row-major distance matrix `dist_mat`.
///
/// Returns `null` if `n_samples == 0`, `eps <= 0` or non-finite, `min_pts == 0`, buffers are undersized,
/// or `dist_mat` contains negative or non-finite values.
pub fn dbscanFromDistanceMatrix(
    dist_mat: []const f64,
    n_samples: usize,
    eps: f64,
    min_pts: usize,
    labels_out: []i32,
    is_core_out: []u8,
    ws: *DbscanWorkspace,
): DbscanResult? {
    if (!checkRectLen(dist_mat.len, n_samples, n_samples) || !isFiniteF64(eps) || eps <= 0.0 || min_pts == 0) {
        return null;
    }
    if (labels_out.len < n_samples || is_core_out.len < n_samples) {
        return null;
    }
    if (ws.queue.len < n_samples || ws.in_queue.len < n_samples || ws.neighbors_tmp.len < n_samples) {
        return null;
    }

    const total = n_samples * n_samples;
    let chk: usize = 0;
    while (chk < total) {
        const dv = dist_mat[chk];
        if (!isFiniteF64(dv) || dv < 0.0) {
            return null;
        }
        chk += 1;
    }

    let i: usize = 0;
    while (i < n_samples) {
        labels_out[i] = DBSCAN_UNVISITED;
        is_core_out[i] = 0;
        ws.in_queue[i] = 0;
        i += 1;
    }

    let num_clusters: usize = 0;
    let p: usize = 0;
    while (p < n_samples) {
        if (labels_out[p] == DBSCAN_UNVISITED) {
            const cnt = regionQueryMatrix(dist_mat, n_samples, p, eps, ws.neighbors_tmp);
            if (cnt < min_pts) {
                labels_out[p] = DBSCAN_NOISE;
            } else {
                const cid = num_clusters as i32;
                num_clusters += 1;
                labels_out[p] = cid;
                is_core_out[p] = 1;

                let head: usize = 0;
                let tail: usize = 0;
                let ni: usize = 0;
                while (ni < cnt) {
                    const nb = ws.neighbors_tmp[ni];
                    if (nb != p && ws.in_queue[nb] == 0) {
                        ws.in_queue[nb] = 1;
                        ws.queue[tail] = nb;
                        tail += 1;
                    }
                    ni += 1;
                }

                while (head < tail) {
                    const q = ws.queue[head];
                    head += 1;
                    if (labels_out[q] == DBSCAN_NOISE) {
                        labels_out[q] = cid;
                    } else if (labels_out[q] == DBSCAN_UNVISITED) {
                        labels_out[q] = cid;
                        const q_cnt = regionQueryMatrix(dist_mat, n_samples, q, eps, ws.neighbors_tmp);
                        if (q_cnt >= min_pts) {
                            is_core_out[q] = 1;
                            let qi: usize = 0;
                            while (qi < q_cnt) {
                                const nb = ws.neighbors_tmp[qi];
                                if (ws.in_queue[nb] == 0 && (labels_out[nb] == DBSCAN_UNVISITED || labels_out[nb] == DBSCAN_NOISE)) {
                                    ws.in_queue[nb] = 1;
                                    ws.queue[tail] = nb;
                                    tail += 1;
                                }
                                qi += 1;
                            }
                        }
                    }
                }

                let clr: usize = 0;
                while (clr < tail) {
                    ws.in_queue[ws.queue[clr]] = 0;
                    clr += 1;
                }
            }
        }
        p += 1;
    }

    let noise_count: usize = 0;
    let core_count: usize = 0;
    i = 0;
    while (i < n_samples) {
        if (labels_out[i] == DBSCAN_NOISE) {
            noise_count += 1;
        }
        if (is_core_out[i] != 0) {
            core_count += 1;
        }
        i += 1;
    }

    return DbscanResult {
        num_clusters: num_clusters,
        noise_count: noise_count,
        core_count: core_count,
    };
}

/// Computes the mean Silhouette Coefficient in `[-1, 1]` over all non-noise points (`labels[i] >= 0`).
///
/// Requires at least 2 distinct non-negative cluster IDs; returns `null` otherwise or on invalid inputs.
pub fn silhouetteScore(
    points: []const f64,
    n_samples: usize,
    n_features: usize,
    labels: []const i32,
    metric: DbscanMetric,
): f64? {
    if (!checkRectLen(points.len, n_samples, n_features) || labels.len < n_samples) {
        return null;
    }
    const total = n_samples * n_features;
    let chk: usize = 0;
    while (chk < total) {
        if (!isFiniteF64(points[chk])) {
            return null;
        }
        chk += 1;
    }

    // Count distinct non-negative cluster labels without heap allocation
    let distinct_clusters: usize = 0;
    let i: usize = 0;
    while (i < n_samples) {
        const li = labels[i];
        if (li >= 0) {
            let seen_before = false;
            let p: usize = 0;
            while (p < i) {
                if (labels[p] == li) {
                    seen_before = true;
                    break;
                }
                p += 1;
            }
            if (!seen_before) {
                distinct_clusters += 1;
            }
        }
        i += 1;
    }
    if (distinct_clusters < 2) {
        return null;
    }

    let score_sum: f64 = 0.0;
    let valid_points: usize = 0;
    i = 0;
    while (i < n_samples) {
        const ci = labels[i];
        if (ci >= 0) {
            valid_points += 1;

            // Intra-cluster mean distance a(i)
            let a_sum: f64 = 0.0;
            let a_cnt: usize = 0;
            let j: usize = 0;
            while (j < n_samples) {
                if (j != i && labels[j] == ci) {
                    a_sum = a_sum + pointDistance(points, n_features, i, j, metric);
                    a_cnt += 1;
                }
                j += 1;
            }

            if (a_cnt > 0) {
                const a_i = a_sum / (a_cnt as f64);

                // Nearest-cluster mean distance b(i)
                let has_b = false;
                let b_i: f64 = 0.0;
                let k: usize = 0;
                while (k < n_samples) {
                    const ck = labels[k];
                    if (ck >= 0 && ck != ci) {
                        let first_occ = true;
                        let prev: usize = 0;
                        while (prev < k) {
                            if (labels[prev] == ck) {
                                first_occ = false;
                                break;
                            }
                            prev += 1;
                        }
                        if (first_occ) {
                            let b_sum: f64 = 0.0;
                            let b_cnt: usize = 0;
                            let m: usize = 0;
                            while (m < n_samples) {
                                if (labels[m] == ck) {
                                    b_sum = b_sum + pointDistance(points, n_features, i, m, metric);
                                    b_cnt += 1;
                                }
                                m += 1;
                            }
                            const mean_d = b_sum / (b_cnt as f64);
                            if (!has_b || mean_d < b_i) {
                                has_b = true;
                                b_i = mean_d;
                            }
                        }
                    }
                    k += 1;
                }

                const denom = if (a_i > b_i) a_i else b_i;
                if (denom > 0.0) {
                    score_sum = score_sum + (b_i - a_i) / denom;
                }
            }
        }
        i += 1;
    }

    if (valid_points == 0) {
        return null;
    }
    return score_sum / (valid_points as f64);
}
