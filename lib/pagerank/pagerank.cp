// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc CSR Sparse Directed Graph PageRank, Personalized PageRank & HITS Hub/Authority Centrality)

import std from "std";

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

/// Compressed Sparse Row (CSR) representation of a directed graph with `num_nodes` vertices.
///
/// Outgoing edges of vertex `u` occupy `col_idx[row_ptr[u]..row_ptr[u + 1]]`.
/// Zero-allocation: borrows caller-owned index slices.
pub struct CsrGraph {
    num_nodes: usize;
    row_ptr: []const usize;
    col_idx: []const usize;

    /// Validates CSR structural invariants:
    /// - `row_ptr.len == num_nodes + 1`
    /// - `row_ptr[0] == 0`
    /// - `row_ptr` is monotonically non-decreasing
    /// - `row_ptr[num_nodes] <= col_idx.len`
    /// - Every target vertex in `col_idx[0..row_ptr[num_nodes]]` satisfies `< num_nodes`
    fn validate(): bool {
        if (_.row_ptr.len != _.num_nodes + 1) {
            return false;
        }
        if (_.row_ptr[0] != 0) {
            return false;
        }
        let u: usize = 0;
        while (u < _.num_nodes) {
            if (_.row_ptr[u] > _.row_ptr[u + 1]) {
                return false;
            }
            u += 1;
        }
        const total_edges = _.row_ptr[_.num_nodes];
        if (total_edges > _.col_idx.len) {
            return false;
        }
        let e: usize = 0;
        while (e < total_edges) {
            if (_.col_idx[e] >= _.num_nodes) {
                return false;
            }
            e += 1;
        }
        return true;
    }

    /// Returns the total number of directed edges stored in the graph (or 0 if `row_ptr` is malformed).
    fn numEdges(): usize {
        if (_.row_ptr.len <= _.num_nodes) {
            return 0;
        }
        return _.row_ptr[_.num_nodes];
    }

    /// Returns the out-degree of vertex `u` (or 0 if `u` is out of bounds or `row_ptr` is malformed).
    fn outDegree(u: usize): usize {
        if (u >= _.num_nodes || _.row_ptr.len <= u + 1) {
            return 0;
        }
        const start = _.row_ptr[u];
        const end = _.row_ptr[u + 1];
        if (end < start) {
            return 0;
        }
        return end - start;
    }

    /// Returns the outgoing neighbor slice of vertex `u` (empty slice if `u` is out of bounds or malformed).
    fn neighbors(u: usize): []const usize {
        if (u >= _.num_nodes || _.row_ptr.len <= u + 1) {
            return _.col_idx[0..0];
        }
        const start = _.row_ptr[u];
        const end = _.row_ptr[u + 1];
        if (start > end || end > _.col_idx.len) {
            return _.col_idx[0..0];
        }
        return _.col_idx[start..end];
    }
}

/// Convergence report for PageRank and Personalized PageRank power iteration.
pub struct PageRankResult {
    iterations: u32;
    converged: bool;
    l1_diff: f64;
}

/// Convergence report for Kleinberg HITS (Hubs & Authorities) bidirectional power iteration.
pub struct HitsResult {
    iterations: u32;
    converged: bool;
    l1_diff: f64;
}

/// Computes standard PageRank on a CSR directed graph with dangling-node probability mass redistribution.
///
/// Invariant: `sum(ranks_out[0..n]) == 1.0` at every iteration.
/// Returns `null` if the graph is invalid/empty, `damping` is not in `(0, 1)`, `tol <= 0`,
/// or `ranks_out` / `next_scratch` has length `< graph.num_nodes`.
pub fn pagerank(
    graph: CsrGraph,
    damping: f64,
    tol: f64,
    max_iter: u32,
    ranks_out: []f64,
    next_scratch: []f64,
): PageRankResult? {
    if (!graph.validate() || graph.num_nodes == 0) {
        return null;
    }
    if (!isFiniteF64(damping) || damping <= 0.0 || damping >= 1.0) {
        return null;
    }
    if (!isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }
    const n = graph.num_nodes;
    if (ranks_out.len < n || next_scratch.len < n) {
        return null;
    }

    const inv_n = 1.0 / (n as f64);
    let i: usize = 0;
    while (i < n) {
        ranks_out[i] = inv_n;
        i += 1;
    }

    if (max_iter == 0) {
        return PageRankResult {
            iterations: 0,
            converged: false,
            l1_diff: 0.0,
        };
    }

    let iter: u32 = 0;
    let last_diff: f64 = 0.0;
    while (iter < max_iter) {
        iter += 1;

        let z: usize = 0;
        while (z < n) {
            next_scratch[z] = 0.0;
            z += 1;
        }

        let dangling_sum: f64 = 0.0;
        let u: usize = 0;
        while (u < n) {
            const start = graph.row_ptr[u];
            const end = graph.row_ptr[u + 1];
            const deg = end - start;
            if (deg == 0) {
                dangling_sum = dangling_sum + ranks_out[u];
            } else {
                const contrib = damping * (ranks_out[u] / (deg as f64));
                let e = start;
                while (e < end) {
                    const v = graph.col_idx[e];
                    next_scratch[v] = next_scratch[v] + contrib;
                    e += 1;
                }
            }
            u += 1;
        }

        const base = ((1.0 - damping) + damping * dangling_sum) * inv_n;
        let diff: f64 = 0.0;
        let k: usize = 0;
        while (k < n) {
            const val = next_scratch[k] + base;
            diff = diff + absF64(val - ranks_out[k]);
            ranks_out[k] = val;
            k += 1;
        }

        last_diff = diff;
        if (diff <= tol) {
            return PageRankResult {
                iterations: iter,
                converged: true,
                l1_diff: diff,
            };
        }
    }

    return PageRankResult {
        iterations: max_iter,
        converged: false,
        l1_diff: last_diff,
    };
}

/// Computes Personalized PageRank (PPR) with an arbitrary non-negative teleportation vector `teleport`.
///
/// `teleport[0..n]` is automatically normalized to sum to `1.0`. Both teleportation mass `(1 - damping)`
/// and dangling-node mass `damping * dangling_sum` are redistributed according to the normalized
/// teleportation distribution, strictly preserving `sum(ranks_out[0..n]) == 1.0`.
///
/// Returns `null` if the graph is invalid/empty, `teleport` has length `< n`, contains negative/non-finite
/// values, or sums to `0`, or if `damping` / `tol` / output buffers are invalid.
pub fn personalizedPagerank(
    graph: CsrGraph,
    teleport: []const f64,
    damping: f64,
    tol: f64,
    max_iter: u32,
    ranks_out: []f64,
    next_scratch: []f64,
): PageRankResult? {
    if (!graph.validate() || graph.num_nodes == 0) {
        return null;
    }
    if (!isFiniteF64(damping) || damping <= 0.0 || damping >= 1.0) {
        return null;
    }
    if (!isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }
    const n = graph.num_nodes;
    if (teleport.len < n || ranks_out.len < n || next_scratch.len < n) {
        return null;
    }

    let t_sum: f64 = 0.0;
    let ti: usize = 0;
    while (ti < n) {
        const w = teleport[ti];
        if (!isFiniteF64(w) || w < 0.0) {
            return null;
        }
        t_sum = t_sum + w;
        ti += 1;
    }
    if (!isFiniteF64(t_sum) || t_sum <= 0.0) {
        return null;
    }

    const inv_t_sum = 1.0 / t_sum;
    let i: usize = 0;
    while (i < n) {
        ranks_out[i] = teleport[i] * inv_t_sum;
        i += 1;
    }

    if (max_iter == 0) {
        return PageRankResult {
            iterations: 0,
            converged: false,
            l1_diff: 0.0,
        };
    }

    let iter: u32 = 0;
    let last_diff: f64 = 0.0;
    while (iter < max_iter) {
        iter += 1;

        let z: usize = 0;
        while (z < n) {
            next_scratch[z] = 0.0;
            z += 1;
        }

        let dangling_sum: f64 = 0.0;
        let u: usize = 0;
        while (u < n) {
            const start = graph.row_ptr[u];
            const end = graph.row_ptr[u + 1];
            const deg = end - start;
            if (deg == 0) {
                dangling_sum = dangling_sum + ranks_out[u];
            } else {
                const contrib = damping * (ranks_out[u] / (deg as f64));
                let e = start;
                while (e < end) {
                    const v = graph.col_idx[e];
                    next_scratch[v] = next_scratch[v] + contrib;
                    e += 1;
                }
            }
            u += 1;
        }

        const teleport_weight = (1.0 - damping) + damping * dangling_sum;
        let diff: f64 = 0.0;
        let k: usize = 0;
        while (k < n) {
            const val = next_scratch[k] + teleport_weight * (teleport[k] * inv_t_sum);
            diff = diff + absF64(val - ranks_out[k]);
            ranks_out[k] = val;
            k += 1;
        }

        last_diff = diff;
        if (diff <= tol) {
            return PageRankResult {
                iterations: iter,
                converged: true,
                l1_diff: diff,
            };
        }
    }

    return PageRankResult {
        iterations: max_iter,
        converged: false,
        l1_diff: last_diff,
    };
}

/// Computes Kleinberg HITS (Hubs & Authorities) centrality via bidirectional power iteration
/// with L2 normalization (`||hubs||_2 == 1` and `||auths||_2 == 1` whenever the graph has edges).
///
/// Each iteration updates authorities `a <- A^T * h`, then updates hubs `h <- A * a`,
/// and L2-normalizes both vectors.
/// Returns `null` if the graph is invalid/empty, `tol <= 0`, or any output/scratch buffer has length `< n`.
pub fn hits(
    graph: CsrGraph,
    tol: f64,
    max_iter: u32,
    hubs_out: []f64,
    auths_out: []f64,
    next_hubs: []f64,
    next_auths: []f64,
): HitsResult? {
    if (!graph.validate() || graph.num_nodes == 0) {
        return null;
    }
    if (!isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }
    const n = graph.num_nodes;
    if (hubs_out.len < n || auths_out.len < n || next_hubs.len < n || next_auths.len < n) {
        return null;
    }

    const init_val = 1.0 / std.math.sqrt(n as f64);
    let i: usize = 0;
    while (i < n) {
        hubs_out[i] = init_val;
        auths_out[i] = init_val;
        i += 1;
    }

    if (max_iter == 0) {
        return HitsResult {
            iterations: 0,
            converged: false,
            l1_diff: 0.0,
        };
    }

    let iter: u32 = 0;
    let last_diff: f64 = 0.0;
    while (iter < max_iter) {
        iter += 1;

        let z: usize = 0;
        while (z < n) {
            next_auths[z] = 0.0;
            next_hubs[z] = 0.0;
            z += 1;
        }

        // Authority update: a[v] = sum_{u -> v} h[u]
        let u: usize = 0;
        while (u < n) {
            const h_u = hubs_out[u];
            const start = graph.row_ptr[u];
            const end = graph.row_ptr[u + 1];
            let e = start;
            while (e < end) {
                const v = graph.col_idx[e];
                next_auths[v] = next_auths[v] + h_u;
                e += 1;
            }
            u += 1;
        }

        // Hub update: h[u] = sum_{u -> v} a[v]
        u = 0;
        while (u < n) {
            const start = graph.row_ptr[u];
            const end = graph.row_ptr[u + 1];
            let sum_a: f64 = 0.0;
            let e = start;
            while (e < end) {
                sum_a = sum_a + next_auths[graph.col_idx[e]];
                e += 1;
            }
            next_hubs[u] = sum_a;
            u += 1;
        }

        // L2 normalization for authorities and hubs
        let auth_sq: f64 = 0.0;
        let hub_sq: f64 = 0.0;
        let k: usize = 0;
        while (k < n) {
            auth_sq = auth_sq + next_auths[k] * next_auths[k];
            hub_sq = hub_sq + next_hubs[k] * next_hubs[k];
            k += 1;
        }

        if (auth_sq > 0.0) {
            const inv_auth = 1.0 / std.math.sqrt(auth_sq);
            k = 0;
            while (k < n) {
                next_auths[k] = next_auths[k] * inv_auth;
                k += 1;
            }
        }
        if (hub_sq > 0.0) {
            const inv_hub = 1.0 / std.math.sqrt(hub_sq);
            k = 0;
            while (k < n) {
                next_hubs[k] = next_hubs[k] * inv_hub;
                k += 1;
            }
        }

        let diff: f64 = 0.0;
        k = 0;
        while (k < n) {
            diff = diff + absF64(next_hubs[k] - hubs_out[k]) + absF64(next_auths[k] - auths_out[k]);
            hubs_out[k] = next_hubs[k];
            auths_out[k] = next_auths[k];
            k += 1;
        }

        last_diff = diff;
        if (diff <= tol) {
            return HitsResult {
                iterations: iter,
                converged: true,
                l1_diff: diff,
            };
        }
    }

    return HitsResult {
        iterations: max_iter,
        converged: false,
        l1_diff: last_diff,
    };
}

/// Returns the index of the highest-scoring finite element in `scores` (smallest index on tie),
/// or `null` if `scores` is empty or contains no finite values.
pub fn topNode(scores: []const f64): usize? {
    if (scores.len == 0) {
        return null;
    }
    let found = false;
    let best_idx: usize = 0;
    let best_val: f64 = 0.0;
    let i: usize = 0;
    while (i < scores.len) {
        const v = scores[i];
        if (isFiniteF64(v)) {
            if (!found || v > best_val) {
                found = true;
                best_idx = i;
                best_val = v;
            }
        }
        i += 1;
    }
    if (!found) {
        return null;
    }
    return best_idx;
}

/// Computes the Kahan-compensated sum of all elements in `scores`.
pub fn sumScores(scores: []const f64): f64 {
    let sum: f64 = 0.0;
    let comp: f64 = 0.0;
    let i: usize = 0;
    while (i < scores.len) {
        const y = scores[i] - comp;
        const t = sum + y;
        comp = (t - sum) - y;
        sum = t;
        i += 1;
    }
    return sum;
}
