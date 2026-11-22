// Origin: Pure-CP Stdlib (AI-Native SIMD K-Means++ Clustering & IVF/PQ Codebook Trainer) | Created: 2026-11-22
// Pure CP zero-allocation SIMD-accelerated K-Means++ initialization and Lloyd clustering
// powered by CP's native 8-lane SIMD intrinsics (@l2sq, @fma) for IVF/PQ codebook training.

const std = @import("std");

/// Configuration for `kmeansFitF32`.
pub struct KMeansConfig {
    /// Number of clusters (`1 <= k <= n`).
    k: usize = 1,
    /// Maximum number of Lloyd iterations (`>= 1`).
    max_iters: u32 = 50,
    /// Convergence tolerance on relative inertia change (`>= 0.0`).
    tol: f32 = 0.0001,
    /// Deterministic SplitMix64 PRNG seed for K-Means++ initialization.
    seed: u64 = 42,
}

/// Convergence and telemetry summary returned by `kmeansFitF32`.
pub struct KMeansResult {
    /// Number of Lloyd iterations executed.
    iters: u32 = 0,
    /// Final sum of squared Euclidean distances from each sample to its assigned centroid.
    inertia: f64 = 0.0,
    /// True if assignments stabilized or relative inertia change fell within `cfg.tol`.
    converged: bool = false,
    /// Total number of empty-cluster reseed events triggered during fitting.
    empty_reseeded: u32 = 0,
}

// ============================================================================
// Internal SplitMix64 PRNG & Validation Helpers
// ============================================================================

struct SplitMixRng {
    state: u64 = 0,

    fn next(*_): u64 {
        const gamma: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
        const mul1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
        const mul2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
        _.state = _.state +% gamma;
        let z: u64 = _.state;
        z = (z ^ (z >> 30)) *% mul1;
        z = (z ^ (z >> 27)) *% mul2;
        return z ^ (z >> 31);
    }

    fn unitF64(*_): f64 {
        const r: u64 = _.next() >> 11;
        return (r as f64) / 9007199254740992.0;
    }
}

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn checkDataFinite(data: []const f32, total_elems: usize): bool {
    let i: usize = 0;
    while (i < total_elems) : (i += 1) {
        if (!isFiniteF32(data[i])) {
            return false;
        }
    }
    return true;
}

fn copyVecF32(src: []const f32, dst: *[]f32, dim: usize) {
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        dst[d] = src[d];
    }
}

// ============================================================================
// 1. Deterministic SIMD K-Means++ Initialization
// ============================================================================

/// Initializes `k` cluster centroids in `centroids` (`[k, dim]` row-major) using deterministic
/// SplitMix64 + `@l2sq` D^2-weighted K-Means++ seeding.
/// `min_dists_buf` must have length `>= n` and is used as zero-heap scratch storage.
pub fn kmeansInitPlusPlusF32(
    data: []const f32,
    n: usize,
    dim: usize,
    k: usize,
    seed: u64,
    centroids: *[]f32,
    min_dists_buf: *[]f32,
): bool {
    if (n == 0 || dim == 0 || k == 0 || k > n) {
        return false;
    }
    if (n > std.math.maxInt(usize) / dim || k > std.math.maxInt(usize) / dim) {
        return false;
    }
    const data_elems: usize = n * dim;
    const cent_elems: usize = k * dim;
    if (data.len < data_elems || centroids.len < cent_elems || min_dists_buf.len < n) {
        return false;
    }
    if (!checkDataFinite(data, data_elems)) {
        return false;
    }

    let rng = SplitMixRng{ state: seed };

    // 1. Pick first centroid uniformly at random via SplitMix64.
    const first_idx: usize = (rng.next() % (n as u64)) as usize;
    let c0_dst = centroids[0..dim];
    copyVecF32(data[first_idx * dim .. (first_idx + 1) * dim], c0_dst, dim);

    // 2. Compute initial squared L2 distances to centroid 0 using native @l2sq SIMD.
    let i: usize = 0;
    while (i < n) : (i += 1) {
        let d2: f32 = @l2sq(data[i * dim .. (i + 1) * dim], centroids[0..dim]);
        if (d2 < 0.0) {
            d2 = 0.0;
        }
        min_dists_buf[i] = d2;
    }

    // 3. Iteratively sample remaining k - 1 centroids proportional to D(x)^2.
    let c: usize = 1;
    while (c < k) : (c += 1) {
        let total_w: f64 = 0.0;
        let last_pos_idx: usize = 0;
        i = 0;
        while (i < n) : (i += 1) {
            const w: f64 = min_dists_buf[i] as f64;
            if (w > 0.0) {
                total_w += w;
                last_pos_idx = i;
            }
        }

        let chosen: usize = 0;
        if (total_w <= 0.0) {
            chosen = (rng.next() % (n as u64)) as usize;
        } else {
            const target: f64 = rng.unitF64() * total_w;
            let acc: f64 = 0.0;
            chosen = last_pos_idx;
            i = 0;
            while (i < n) : (i += 1) {
                acc += min_dists_buf[i] as f64;
                if (acc >= target) {
                    chosen = i;
                    break;
                }
            }
        }

        const c_base: usize = c * dim;
        let c_dst = centroids[c_base .. c_base + dim];
        copyVecF32(data[chosen * dim .. (chosen + 1) * dim], c_dst, dim);

        if (c + 1 < k) {
            i = 0;
            while (i < n) : (i += 1) {
                let d2: f32 = @l2sq(data[i * dim .. (i + 1) * dim], centroids[c_base .. c_base + dim]);
                if (d2 < 0.0) {
                    d2 = 0.0;
                }
                if (d2 < min_dists_buf[i]) {
                    min_dists_buf[i] = d2;
                }
            }
        }
    }

    return true;
}

// ============================================================================
// 2. SIMD Nearest-Centroid Assignment
// ============================================================================

/// Assigns each of `n` vectors in `data` (`[n, dim]`) to its nearest centroid in `centroids`
/// (`[k, dim]`) using `@l2sq` and returns the total squared-distance inertia.
pub fn kmeansAssignF32(
    data: []const f32,
    n: usize,
    dim: usize,
    centroids: []const f32,
    k: usize,
    assignments: *[]usize,
): f64? {
    if (n == 0 || dim == 0 || k == 0) {
        return null;
    }
    if (n > std.math.maxInt(usize) / dim || k > std.math.maxInt(usize) / dim) {
        return null;
    }
    if (data.len < n * dim || centroids.len < k * dim || assignments.len < n) {
        return null;
    }

    let total_inertia: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const x = data[i * dim .. (i + 1) * dim];
        let best_c: usize = 0;
        let best_d2: f32 = @l2sq(x, centroids[0..dim]);
        if (std.math.isNan(best_d2)) {
            return null;
        }
        if (best_d2 < 0.0) {
            best_d2 = 0.0;
        }

        let c: usize = 1;
        while (c < k) : (c += 1) {
            let d2: f32 = @l2sq(x, centroids[c * dim .. (c + 1) * dim]);
            if (std.math.isNan(d2)) {
                return null;
            }
            if (d2 < 0.0) {
                d2 = 0.0;
            }
            if (d2 < best_d2) {
                best_d2 = d2;
                best_c = c;
            }
        }

        assignments[i] = best_c;
        total_inertia += best_d2 as f64;
    }

    return total_inertia;
}

// ============================================================================
// 3. Full Zero-Allocation Lloyd K-Means Fit with Empty-Cluster Reseeding
// ============================================================================

/// Runs full SIMD-accelerated K-Means++ initialization + Lloyd iterations with empty-cluster
/// farthest-point reseeding and relative-tolerance / zero-reassignment early stopping.
///
/// Caller-provided scratch buffers:
/// - `centroids`: length `>= cfg.k * dim`
/// - `assignments`: length `>= n`
/// - `counts_buf`: length `>= cfg.k`
/// - `scratch_dists`: length `>= n`
pub fn kmeansFitF32(
    data: []const f32,
    n: usize,
    dim: usize,
    cfg: KMeansConfig,
    centroids: *[]f32,
    assignments: *[]usize,
    counts_buf: *[]usize,
    scratch_dists: *[]f32,
): KMeansResult? {
    if (n == 0 || dim == 0 || cfg.k == 0 || cfg.k > n || cfg.max_iters == 0) {
        return null;
    }
    if (!isFiniteF32(cfg.tol) || cfg.tol < 0.0) {
        return null;
    }
    if (n > std.math.maxInt(usize) / dim || cfg.k > std.math.maxInt(usize) / dim) {
        return null;
    }
    if (data.len < n * dim || centroids.len < cfg.k * dim) {
        return null;
    }
    if (assignments.len < n || counts_buf.len < cfg.k || scratch_dists.len < n) {
        return null;
    }

    if (!kmeansInitPlusPlusF32(data, n, dim, cfg.k, cfg.seed, centroids, scratch_dists)) {
        return null;
    }

    // Initial assignment pass (also records per-sample squared distance in scratch_dists).
    let prev_inertia: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const x = data[i * dim .. (i + 1) * dim];
        let best_c: usize = 0;
        let best_d2: f32 = @l2sq(x, centroids[0..dim]);
        if (best_d2 < 0.0) {
            best_d2 = 0.0;
        }
        let c: usize = 1;
        while (c < cfg.k) : (c += 1) {
            let d2: f32 = @l2sq(x, centroids[c * dim .. (c + 1) * dim]);
            if (d2 < 0.0) {
                d2 = 0.0;
            }
            if (d2 < best_d2) {
                best_d2 = d2;
                best_c = c;
            }
        }
        assignments[i] = best_c;
        scratch_dists[i] = best_d2;
        prev_inertia += best_d2 as f64;
    }

    let rng = SplitMixRng{ state: cfg.seed ^ 0x517cc1b727220a95 };
    let iters_done: u32 = 0;
    let converged: bool = false;
    let empty_reseeded: u32 = 0;

    let iter: u32 = 0;
    while (iter < cfg.max_iters) : (iter += 1) {
        iters_done = iter + 1;
        let reseeded_this_iter: u32 = 0;

        // 1. Count cluster memberships.
        let c: usize = 0;
        while (c < cfg.k) : (c += 1) {
            counts_buf[c] = 0;
        }
        i = 0;
        while (i < n) : (i += 1) {
            counts_buf[assignments[i]] += 1;
        }

        // 2. Reseed any empty cluster using the farthest point from a multi-member cluster.
        let c_rs: usize = 0;
        while (c_rs < cfg.k) : (c_rs += 1) {
            if (counts_buf[c_rs] == 0) {
                let far_idx: usize = (rng.next() % (n as u64)) as usize;
                let max_d2: f32 = -1.0;
                let si: usize = 0;
                while (si < n) : (si += 1) {
                    if (counts_buf[assignments[si]] > 1 && scratch_dists[si] > max_d2) {
                        max_d2 = scratch_dists[si];
                        far_idx = si;
                    }
                }
                const old_c: usize = assignments[far_idx];
                if (counts_buf[old_c] > 0) {
                    counts_buf[old_c] -= 1;
                }
                assignments[far_idx] = c_rs;
                counts_buf[c_rs] = 1;
                scratch_dists[far_idx] = 0.0;
                reseeded_this_iter += 1;
                empty_reseeded += 1;
            }
        }

        // 3. Recompute centroids in-place using hardware @fma scaled accumulation.
        let ce: usize = 0;
        while (ce < cfg.k * dim) : (ce += 1) {
            centroids[ce] = 0.0;
        }
        i = 0;
        while (i < n) : (i += 1) {
            const cid: usize = assignments[i];
            const cnt: usize = counts_buf[cid];
            if (cnt > 0) {
                const inv_cnt: f32 = 1.0 / (cnt as f32);
                const x_base: usize = i * dim;
                const c_base: usize = cid * dim;
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    centroids[c_base + d] = @fma(inv_cnt, data[x_base + d], centroids[c_base + d]);
                }
            }
        }

        // 4. Reassign samples via @l2sq and evaluate convergence.
        let new_inertia: f64 = 0.0;
        let changed: usize = 0;
        i = 0;
        while (i < n) : (i += 1) {
            const x = data[i * dim .. (i + 1) * dim];
            let best_c: usize = 0;
            let best_d2: f32 = @l2sq(x, centroids[0..dim]);
            if (best_d2 < 0.0) {
                best_d2 = 0.0;
            }
            let c_cmp: usize = 1;
            while (c_cmp < cfg.k) : (c_cmp += 1) {
                let d2: f32 = @l2sq(x, centroids[c_cmp * dim .. (c_cmp + 1) * dim]);
                if (d2 < 0.0) {
                    d2 = 0.0;
                }
                if (d2 < best_d2) {
                    best_d2 = d2;
                    best_c = c_cmp;
                }
            }
            if (best_c != assignments[i]) {
                changed += 1;
                assignments[i] = best_c;
            }
            scratch_dists[i] = best_d2;
            new_inertia += best_d2 as f64;
        }

        let diff: f64 = prev_inertia - new_inertia;
        if (diff < 0.0) {
            diff = -diff;
        }
        prev_inertia = new_inertia;

        if (reseeded_this_iter == 0 && (changed == 0 || diff <= (cfg.tol as f64) * (1.0 + new_inertia))) {
            converged = true;
            break;
        }
    }

    return KMeansResult{
        iters: iters_done,
        inertia: prev_inertia,
        converged: converged,
        empty_reseeded: empty_reseeded,
    };
}
