// Origin: Pure-CP Stdlib (Zero-Alloc SIMD @l2sq Residual IVF-PQ Approximate Nearest Neighbor Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation Residual Inverted File + Product Quantization (FAISS IndexIVFPQ-style):
//   - Two-stage deterministic K-Means training powered by CP native `@l2sq` SIMD distance:
//       1) Coarse quantizer clustering into `nlist` centroids
//       2) Per-subspace residual PQ codebook clustering (`r = x - c_l`) into `m_subspaces x ksub` centroids
//   - Compact `[IVFPQ_MAX_SUBSPACES]u8` residual code storage with O(1) intrusive inverted lists
//   - Vector reconstruction (`reconstruct`) summing coarse centroid + quantized subspace residuals
//   - Multi-probe Asymmetric Distance Computation (ADC) search precomputing `[m_subspaces][ksub]f32`
//     distance tables per probed list and scoring each candidate in O(M) table lookups

// ============================================================================
// 1. Constants, Errors & Data Models
// ============================================================================

/// Maximum number of coarse inverted lists (`nlist`).
pub const IVFPQ_MAX_LISTS: usize = 16;

/// Maximum number of PQ subspaces (`M`).
pub const IVFPQ_MAX_SUBSPACES: usize = 8;

/// Maximum number of sub-centroids per PQ subspace (`Ksub`).
pub const IVFPQ_MAX_KSUB: usize = 16;

/// Maximum vector dimension supported inline.
pub const IVFPQ_MAX_DIM: usize = 64;

/// Maximum number of indexed documents stored inline.
pub const IVFPQ_MAX_DOCS: usize = 256;

const IVFPQ_NO_NEXT: u16 = 0xFFFF;

/// Error set for `IvfPqIndex` configuration, training, and insertion.
pub err IvfPqError {
    InvalidDim,
    InvalidParams,
    NotTrained,
    CapacityFull,
    DimMismatch,
}

/// Configuration for `IvfPqIndex`.
pub struct IvfPqConfig {
    dim: usize = 16;
    nlist: usize = 4;
    m_subspaces: usize = 4;
    ksub: usize = 8;
    nprobe: usize = 2;
    train_iters: usize = 8;

    /// Validate that all dimensions, cluster counts, and subspace divisibility constraints hold.
    fn isValid(): bool {
        if (dim == 0 || dim > IVFPQ_MAX_DIM) return false;
        if (nlist == 0 || nlist > IVFPQ_MAX_LISTS) return false;
        if (m_subspaces == 0 || m_subspaces > IVFPQ_MAX_SUBSPACES) return false;
        if ((dim % m_subspaces) != 0) return false;
        if (ksub == 0 || ksub > IVFPQ_MAX_KSUB) return false;
        if (nprobe == 0 || nprobe > nlist) return false;
        if (train_iters == 0) return false;
        return true;
    }
}

/// Single approximate nearest-neighbor hit returned by `IvfPqIndex.search`.
pub struct IvfPqHit {
    doc_id: u64 = 0;
    list_id: u16 = 0;
    approx_l2_sq: f32 = 0.0;
}

/// Index telemetry and compression statistics returned by `IvfPqIndex.stats`.
pub struct IvfPqStats {
    doc_count: usize = 0;
    nlist: usize = 0;
    m_subspaces: usize = 0;
    ksub: usize = 0;
    bytes_per_code: usize = 0;
    compression_ratio_x100: u32 = 0;
    trained: bool = false;
}

// ============================================================================
// 2. IvfPqIndex — Zero-Heap Residual IVF-PQ Engine
// ============================================================================

/// Zero-heap-allocation Residual IVF-PQ approximate nearest-neighbor index.
pub struct IvfPqIndex {
    cfg: IvfPqConfig = IvfPqConfig{},
    dsub: usize = 0;
    trained: bool = false;
    coarse_centroids: [IVFPQ_MAX_LISTS * IVFPQ_MAX_DIM]f32 = [0.0; IVFPQ_MAX_LISTS * IVFPQ_MAX_DIM];
    pq_codebooks: [IVFPQ_MAX_KSUB * IVFPQ_MAX_DIM]f32 = [0.0; IVFPQ_MAX_KSUB * IVFPQ_MAX_DIM];
    doc_ids: [IVFPQ_MAX_DOCS]u64 = [0; IVFPQ_MAX_DOCS];
    doc_list_ids: [IVFPQ_MAX_DOCS]u16 = [0; IVFPQ_MAX_DOCS];
    doc_codes: [IVFPQ_MAX_DOCS * IVFPQ_MAX_SUBSPACES]u8 = [0; IVFPQ_MAX_DOCS * IVFPQ_MAX_SUBSPACES];
    doc_next: [IVFPQ_MAX_DOCS]u16 = [IVFPQ_NO_NEXT; IVFPQ_MAX_DOCS];
    list_head: [IVFPQ_MAX_LISTS]u16 = [IVFPQ_NO_NEXT; IVFPQ_MAX_LISTS];
    doc_count: usize = 0;

    /// Initialize an untrained `IvfPqIndex` after validating `cfg`.
    static fn init(cfg: IvfPqConfig): IvfPqIndex ! IvfPqError {
        if (cfg.dim == 0 || cfg.dim > IVFPQ_MAX_DIM) {
            return IvfPqError.InvalidDim;
        }
        if (!cfg.isValid()) {
            return IvfPqError.InvalidParams;
        }
        let idx = IvfPqIndex{};
        idx.cfg = cfg;
        idx.dsub = cfg.dim / cfg.m_subspaces;
        return idx;
    }

    fn coarseSlice(list_id: usize): []const f32 {
        const off = list_id * IVFPQ_MAX_DIM;
        return coarse_centroids[off .. off + cfg.dim];
    }

    fn pqCentroidOffset(m: usize, k: usize): usize {
        return (m * cfg.ksub + k) * dsub;
    }

    fn nearestCoarse(vec: []const f32, active_lists: usize): usize {
        let best_c: usize = 0;
        let best_d: f32 = @l2sq(vec, coarseSlice(0));
        let c: usize = 1;
        while (c < active_lists) : (c += 1) {
            const d: f32 = @l2sq(vec, coarseSlice(c));
            if (d < best_d) {
                best_d = d;
                best_c = c;
            }
        }
        return best_c;
    }

    fn nearestSubCentroid(m: usize, sub_vec: []const f32, active_k: usize): usize {
        const off0 = pqCentroidOffset(m, 0);
        let best_k: usize = 0;
        let best_d: f32 = @l2sq(sub_vec, pq_codebooks[off0 .. off0 + dsub]);
        let k: usize = 1;
        while (k < active_k) : (k += 1) {
            const off = pqCentroidOffset(m, k);
            const d: f32 = @l2sq(sub_vec, pq_codebooks[off .. off + dsub]);
            if (d < best_d) {
                best_d = d;
                best_k = k;
            }
        }
        return best_k;
    }

    /// Two-stage zero-heap training:
    ///   1) Deterministic farthest-first + Lloyd K-Means over raw vectors (`@l2sq`) -> `nlist` coarse centroids
    ///   2) Independent per-subspace K-Means over residual vectors `r = x - c_l` -> `m_subspaces x ksub` PQ codebooks
    fn train(*_, flat_vecs: []const f32, num_vecs: usize): void ! IvfPqError {
        if (num_vecs < cfg.nlist || num_vecs < cfg.ksub) {
            return IvfPqError.InvalidParams;
        }
        if (flat_vecs.len < num_vecs * cfg.dim) {
            return IvfPqError.DimMismatch;
        }

        const dim = cfg.dim;

        // ---- Stage 1: Coarse quantizer K-Means ----
        // 1a. Deterministic farthest-first seeding
        let d0: usize = 0;
        while (d0 < dim) : (d0 += 1) {
            coarse_centroids[d0] = flat_vecs[d0];
        }

        let c_init: usize = 1;
        while (c_init < cfg.nlist) : (c_init += 1) {
            let far_idx: usize = 0;
            let far_dist: f32 = -1.0;
            let v: usize = 0;
            while (v < num_vecs) : (v += 1) {
                const v_slice: []const f32 = flat_vecs[v * dim .. v * dim + dim];
                const nearest_c = nearestCoarse(v_slice, c_init);
                const md: f32 = @l2sq(v_slice, coarseSlice(nearest_c));
                if (md > far_dist) {
                    far_dist = md;
                    far_idx = v;
                }
            }
            const dst_off = c_init * IVFPQ_MAX_DIM;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                coarse_centroids[dst_off + d] = flat_vecs[far_idx * dim + d];
            }
        }

        // 1b. Lloyd iterations for coarse centroids
        let sums: [IVFPQ_MAX_LISTS * IVFPQ_MAX_DIM]f32 = [0.0; IVFPQ_MAX_LISTS * IVFPQ_MAX_DIM];
        let counts: [IVFPQ_MAX_LISTS]u32 = [0; IVFPQ_MAX_LISTS];

        let iter: usize = 0;
        while (iter < cfg.train_iters) : (iter += 1) {
            let z: usize = 0;
            while (z < cfg.nlist) : (z += 1) {
                counts[z] = 0;
                const zoff = z * IVFPQ_MAX_DIM;
                let zd: usize = 0;
                while (zd < dim) : (zd += 1) {
                    sums[zoff + zd] = 0.0;
                }
            }

            let v: usize = 0;
            while (v < num_vecs) : (v += 1) {
                const v_slice: []const f32 = flat_vecs[v * dim .. v * dim + dim];
                const best_c = nearestCoarse(v_slice, cfg.nlist);
                counts[best_c] += 1;
                const soff = best_c * IVFPQ_MAX_DIM;
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    sums[soff + d] += v_slice[d];
                }
            }

            let c: usize = 0;
            while (c < cfg.nlist) : (c += 1) {
                if (counts[c] == 0) continue;
                const inv: f32 = 1.0 / @as(f32, @floatFromInt(counts[c]));
                const off = c * IVFPQ_MAX_DIM;
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    coarse_centroids[off + d] = sums[off + d] * inv;
                }
            }
        }

        // ---- Stage 2: Per-subspace residual PQ K-Means ----
        let res_sub: [IVFPQ_MAX_DIM]f32 = [0.0; IVFPQ_MAX_DIM];
        let sub_sums: [IVFPQ_MAX_KSUB * IVFPQ_MAX_DIM]f32 = [0.0; IVFPQ_MAX_KSUB * IVFPQ_MAX_DIM];
        let sub_counts: [IVFPQ_MAX_KSUB]u32 = [0; IVFPQ_MAX_KSUB];

        let m: usize = 0;
        while (m < cfg.m_subspaces) : (m += 1) {
            const sub_start = m * dsub;

            // 2a. Seed first sub-centroid from vector 0's residual
            const v0_slice: []const f32 = flat_vecs[0..dim];
            const c0 = nearestCoarse(v0_slice, cfg.nlist);
            const c0_off = c0 * IVFPQ_MAX_DIM;
            const cb0_off = pqCentroidOffset(m, 0);
            let sd0: usize = 0;
            while (sd0 < dsub) : (sd0 += 1) {
                pq_codebooks[cb0_off + sd0] = v0_slice[sub_start + sd0] - coarse_centroids[c0_off + sub_start + sd0];
            }

            // Farthest-first seeding for remaining sub-centroids
            let k_init: usize = 1;
            while (k_init < cfg.ksub) : (k_init += 1) {
                let best_v: usize = 0;
                let best_d: f32 = -1.0;
                let v: usize = 0;
                while (v < num_vecs) : (v += 1) {
                    const vs: []const f32 = flat_vecs[v * dim .. v * dim + dim];
                    const cv = nearestCoarse(vs, cfg.nlist);
                    const cv_off = cv * IVFPQ_MAX_DIM;
                    let sd: usize = 0;
                    while (sd < dsub) : (sd += 1) {
                        res_sub[sd] = vs[sub_start + sd] - coarse_centroids[cv_off + sub_start + sd];
                    }
                    const nk = nearestSubCentroid(m, res_sub[0..dsub], k_init);
                    const nk_off = pqCentroidOffset(m, nk);
                    const dist: f32 = @l2sq(res_sub[0..dsub], pq_codebooks[nk_off .. nk_off + dsub]);
                    if (dist > best_d) {
                        best_d = dist;
                        best_v = v;
                    }
                }
                const bvs: []const f32 = flat_vecs[best_v * dim .. best_v * dim + dim];
                const bcv = nearestCoarse(bvs, cfg.nlist);
                const bcv_off = bcv * IVFPQ_MAX_DIM;
                const dst_cb = pqCentroidOffset(m, k_init);
                let sd: usize = 0;
                while (sd < dsub) : (sd += 1) {
                    pq_codebooks[dst_cb + sd] = bvs[sub_start + sd] - coarse_centroids[bcv_off + sub_start + sd];
                }
            }

            // 2b. Lloyd iterations for subspace m
            let siter: usize = 0;
            while (siter < cfg.train_iters) : (siter += 1) {
                let k: usize = 0;
                while (k < cfg.ksub) : (k += 1) {
                    sub_counts[k] = 0;
                    const koff = k * IVFPQ_MAX_DIM;
                    let sd: usize = 0;
                    while (sd < dsub) : (sd += 1) {
                        sub_sums[koff + sd] = 0.0;
                    }
                }

                let v: usize = 0;
                while (v < num_vecs) : (v += 1) {
                    const vs: []const f32 = flat_vecs[v * dim .. v * dim + dim];
                    const cv = nearestCoarse(vs, cfg.nlist);
                    const cv_off = cv * IVFPQ_MAX_DIM;
                    let sd: usize = 0;
                    while (sd < dsub) : (sd += 1) {
                        res_sub[sd] = vs[sub_start + sd] - coarse_centroids[cv_off + sub_start + sd];
                    }
                    const bk = nearestSubCentroid(m, res_sub[0..dsub], cfg.ksub);
                    sub_counts[bk] += 1;
                    const koff = bk * IVFPQ_MAX_DIM;
                    let sd2: usize = 0;
                    while (sd2 < dsub) : (sd2 += 1) {
                        sub_sums[koff + sd2] += res_sub[sd2];
                    }
                }

                let ku: usize = 0;
                while (ku < cfg.ksub) : (ku += 1) {
                    if (sub_counts[ku] == 0) continue;
                    const inv: f32 = 1.0 / @as(f32, @floatFromInt(sub_counts[ku]));
                    const dst_cb = pqCentroidOffset(m, ku);
                    const koff = ku * IVFPQ_MAX_DIM;
                    let sd: usize = 0;
                    while (sd < dsub) : (sd += 1) {
                        pq_codebooks[dst_cb + sd] = sub_sums[koff + sd] * inv;
                    }
                }
            }
        }

        trained = true;
    }

    /// Encode `vec` as a coarse list ID plus compact residual PQ codes and insert into the inverted list.
    /// Returns the assigned coarse `list_id`.
    fn add(*_, doc_id: u64, vec: []const f32): u16 ! IvfPqError {
        if (!trained) return IvfPqError.NotTrained;
        if (vec.len != cfg.dim) return IvfPqError.DimMismatch;
        if (doc_count >= IVFPQ_MAX_DOCS) return IvfPqError.CapacityFull;

        const list_id = nearestCoarse(vec, cfg.nlist);
        const c_off = list_id * IVFPQ_MAX_DIM;

        let res: [IVFPQ_MAX_DIM]f32 = [0.0; IVFPQ_MAX_DIM];
        let d: usize = 0;
        while (d < cfg.dim) : (d += 1) {
            res[d] = vec[d] - coarse_centroids[c_off + d];
        }

        const slot = doc_count;
        const code_off = slot * IVFPQ_MAX_SUBSPACES;
        let m: usize = 0;
        while (m < cfg.m_subspaces) : (m += 1) {
            const sub_start = m * dsub;
            const bk = nearestSubCentroid(m, res[sub_start .. sub_start + dsub], cfg.ksub);
            doc_codes[code_off + m] = @as(u8, @intCast(bk));
        }

        doc_ids[slot] = doc_id;
        doc_list_ids[slot] = @as(u16, @intCast(list_id));
        doc_next[slot] = list_head[list_id];
        list_head[list_id] = @as(u16, @intCast(slot));
        doc_count += 1;

        return @as(u16, @intCast(list_id));
    }

    /// Reconstruct the approximate vector for stored `doc_idx`: `coarse_centroid + sum(pq_codebook[m][code[m]])`.
    /// Returns `true` on success, `false` if untrained / out of bounds / `out_vec.len < cfg.dim`.
    fn reconstruct(doc_idx: usize, out_vec: *[]f32): bool {
        if (!trained || doc_idx >= doc_count || out_vec.len < cfg.dim) return false;
        const list_id: usize = @as(usize, doc_list_ids[doc_idx]);
        const c_off = list_id * IVFPQ_MAX_DIM;
        let d: usize = 0;
        while (d < cfg.dim) : (d += 1) {
            out_vec[d] = coarse_centroids[c_off + d];
        }

        const code_off = doc_idx * IVFPQ_MAX_SUBSPACES;
        let m: usize = 0;
        while (m < cfg.m_subspaces) : (m += 1) {
            const code: usize = @as(usize, doc_codes[code_off + m]);
            const cb_off = pqCentroidOffset(m, code);
            const sub_start = m * dsub;
            let sd: usize = 0;
            while (sd < dsub) : (sd += 1) {
                out_vec[sub_start + sd] += pq_codebooks[cb_off + sd];
            }
        }
        return true;
    }

    /// Multi-probe Asymmetric Distance Computation (ADC) Top-K search.
    /// Probes the nearest `nprobe` coarse lists (defaults to `cfg.nprobe` if `nprobe == 0`),
    /// precomputes `[m_subspaces][ksub]f32` residual distance tables per list, and returns
    /// hits sorted ascending by `(approx_l2_sq, doc_id)`.
    fn search(*_, query_vec: []const f32, nprobe: usize, out_hits: *[]IvfPqHit): usize {
        if (!trained || query_vec.len != cfg.dim || out_hits.len == 0 || doc_count == 0) return 0;

        let eff_nprobe: usize = if (nprobe == 0) cfg.nprobe else nprobe;
        if (eff_nprobe > cfg.nlist) eff_nprobe = cfg.nlist;

        // 1. Rank coarse lists by `@l2sq(query_vec, coarse_centroid[l])` ascending.
        let list_order: [IVFPQ_MAX_LISTS]usize = [0; IVFPQ_MAX_LISTS];
        let list_dists: [IVFPQ_MAX_LISTS]f32 = [0.0; IVFPQ_MAX_LISTS];
        let l: usize = 0;
        while (l < cfg.nlist) : (l += 1) {
            list_order[l] = l;
            list_dists[l] = @l2sq(query_vec, coarseSlice(l));
        }

        let a: usize = 0;
        while (a < eff_nprobe) : (a += 1) {
            let best_idx = a;
            let b: usize = a + 1;
            while (b < cfg.nlist) : (b += 1) {
                if (list_dists[b] < list_dists[best_idx]) {
                    best_idx = b;
                }
            }
            if (best_idx != a) {
                const td = list_dists[a];
                list_dists[a] = list_dists[best_idx];
                list_dists[best_idx] = td;
                const tl = list_order[a];
                list_order[a] = list_order[best_idx];
                list_order[best_idx] = tl;
            }
        }

        // 2. For each probed list, build ADC lookup table and scan inverted list in O(M)/doc.
        let q_res: [IVFPQ_MAX_DIM]f32 = [0.0; IVFPQ_MAX_DIM];
        let adc: [IVFPQ_MAX_SUBSPACES * IVFPQ_MAX_KSUB]f32 = [0.0; IVFPQ_MAX_SUBSPACES * IVFPQ_MAX_KSUB];
        let hit_count: usize = 0;

        let p: usize = 0;
        while (p < eff_nprobe) : (p += 1) {
            const lid = list_order[p];
            if (list_head[lid] == IVFPQ_NO_NEXT) continue;

            const c_off = lid * IVFPQ_MAX_DIM;
            let d: usize = 0;
            while (d < cfg.dim) : (d += 1) {
                q_res[d] = query_vec[d] - coarse_centroids[c_off + d];
            }

            // Build ADC table: adc[m * IVFPQ_MAX_KSUB + k] = ||q_res_m - cb_{m,k}||^2
            let m: usize = 0;
            while (m < cfg.m_subspaces) : (m += 1) {
                const sub_start = m * dsub;
                const q_sub: []const f32 = q_res[sub_start .. sub_start + dsub];
                let k: usize = 0;
                while (k < cfg.ksub) : (k += 1) {
                    const cb_off = pqCentroidOffset(m, k);
                    adc[m * IVFPQ_MAX_KSUB + k] = @l2sq(q_sub, pq_codebooks[cb_off .. cb_off + dsub]);
                }
            }

            // Scan inverted list
            let cur: u16 = list_head[lid];
            while (cur != IVFPQ_NO_NEXT) {
                const slot: usize = @as(usize, cur);
                const code_off = slot * IVFPQ_MAX_SUBSPACES;
                let approx_d: f32 = 0.0;
                let sm: usize = 0;
                while (sm < cfg.m_subspaces) : (sm += 1) {
                    const code: usize = @as(usize, doc_codes[code_off + sm]);
                    approx_d += adc[sm * IVFPQ_MAX_KSUB + code];
                }

                const cand = IvfPqHit{
                    doc_id: doc_ids[slot],
                    list_id: @as(u16, @intCast(lid)),
                    approx_l2_sq: approx_d,
                };

                // Insert into sorted top-K `out_hits`
                if (hit_count < out_hits.len) {
                    out_hits[hit_count] = cand;
                    hit_count += 1;
                } else {
                    const last = out_hits[hit_count - 1];
                    const better = (cand.approx_l2_sq < last.approx_l2_sq) ||
                        (cand.approx_l2_sq == last.approx_l2_sq && cand.doc_id < last.doc_id);
                    if (better) {
                        out_hits[hit_count - 1] = cand;
                    } else {
                        cur = doc_next[slot];
                        continue;
                    }
                }

                // Bubble newly inserted hit up to maintain ascending order
                let pos: usize = hit_count - 1;
                while (pos > 0) {
                    const prev = out_hits[pos - 1];
                    const curr = out_hits[pos];
                    const swap = (curr.approx_l2_sq < prev.approx_l2_sq) ||
                        (curr.approx_l2_sq == prev.approx_l2_sq && curr.doc_id < prev.doc_id);
                    if (!swap) break;
                    out_hits[pos - 1] = curr;
                    out_hits[pos] = prev;
                    pos -= 1;
                }

                cur = doc_next[slot];
            }
        }

        return hit_count;
    }

    /// Return index telemetry and compression statistics.
    fn stats(): IvfPqStats {
        const raw_bytes: u64 = @as(u64, cfg.dim) * 4;
        const code_bytes: u64 = @as(u64, cfg.m_subspaces);
        const ratio_x100: u32 = if (code_bytes > 0) @as(u32, @intCast((raw_bytes * 100) / code_bytes)) else 0;
        return IvfPqStats{
            doc_count: doc_count,
            nlist: cfg.nlist,
            m_subspaces: cfg.m_subspaces,
            ksub: cfg.ksub,
            bytes_per_code: cfg.m_subspaces,
            compression_ratio_x100: ratio_x100,
            trained: trained,
        };
    }
}
