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

// ============================================================================
// 3. Unbounded External-Slice Residual IVF-PQ Engine (IvfPqFlatIndex)
//    + Deterministic K-Means++ + Soft Delete + IEEE CRC32 Binary Snapshot
// ============================================================================

/// Little-endian ASCII magic `"IVPQ"` (`0x51505649`) for binary snapshot persistence.
pub const IVFPQ_SNAPSHOT_MAGIC: u32 = 0x51505649;

/// Current binary snapshot format version (`1`).
pub const IVFPQ_SNAPSHOT_VERSION: u32 = 1;

/// Sentinel link value indicating end-of-list in `list_heads` / `list_next`.
pub const IVFPQ_INVALID_LINK: u32 = 0xFFFF_FFFF;

const IVFPQ_SNAPSHOT_HEADER_BYTES: usize = 56;
const IVFPQ_SNAPSHOT_CRC_BYTES: usize = 4;

/// Error set for unbounded `IvfPqFlatIndex` operations and binary snapshot persistence.
pub err IvfPqFlatError {
    InvalidConfig,
    DimMismatch,
    NotTrained,
    CapacityFull,
    BufferTooSmall,
    InvalidSnapshot,
    ChecksumMismatch,
}

/// Configuration for the unbounded caller-slice `IvfPqFlatIndex` engine.
/// Supports arbitrary `dim`, `max_docs`, `nlist`, `m_subspaces`, and `ksub <= 256`.
pub struct IvfPqFlatConfig {
    dim: usize = 32;
    max_docs: usize = 1024;
    nlist: usize = 16;
    m_subspaces: usize = 8;
    ksub: usize = 16;
    train_iters: usize = 8;
    seed: u64 = 0x9E3779B97F4A7C15;

    /// Validates configuration constraints (`dim % m_subspaces == 0`, `1 <= ksub <= 256`, positive sizes).
    fn isValid(): bool {
        if (dim == 0 || max_docs == 0 || nlist == 0 || m_subspaces == 0) return false;
        if ((dim % m_subspaces) != 0) return false;
        if (ksub == 0 || ksub > 256) return false;
        if (train_iters == 0) return false;
        return true;
    }
}

/// Exact `f32` buffer length required for `coarse_centroids` (`nlist * dim`).
pub fn ivfPqCoarseBufLen(cfg: IvfPqFlatConfig): usize {
    return cfg.nlist * cfg.dim;
}

/// Exact `f32` buffer length required for `pq_codebooks` (`ksub * dim`).
pub fn ivfPqCodebookBufLen(cfg: IvfPqFlatConfig): usize {
    return cfg.ksub * cfg.dim;
}

/// Exact `u8` buffer length required for compressed residual `codes` (`max_docs * m_subspaces`).
pub fn ivfPqCodesBufLen(cfg: IvfPqFlatConfig): usize {
    return cfg.max_docs * cfg.m_subspaces;
}

/// Exact `f32` scratch buffer length required by `IvfPqFlatIndex.train(..., work_f32, work_u32)`.
pub fn ivfPqTrainWorkF32Len(cfg: IvfPqFlatConfig, num_vecs: usize): usize {
    const dsub: usize = if (cfg.m_subspaces > 0) cfg.dim / cfg.m_subspaces else 0;
    const coarse_sums: usize = cfg.nlist * cfg.dim;
    const sub_scratch: usize = cfg.ksub * dsub + dsub;
    const stage_need: usize = if (coarse_sums > sub_scratch) coarse_sums else sub_scratch;
    return num_vecs + stage_need;
}

/// Exact `u32` scratch buffer length required by `IvfPqFlatIndex.train(..., work_f32, work_u32)`.
pub fn ivfPqTrainWorkU32Len(cfg: IvfPqFlatConfig, num_vecs: usize): usize {
    const max_k: usize = if (cfg.nlist > cfg.ksub) cfg.nlist else cfg.ksub;
    return num_vecs + max_k;
}

/// Exact `f32` scratch buffer length required by `IvfPqFlatIndex.search(..., adc_lut_work, out_hits)`.
pub fn ivfPqSearchWorkF32Len(cfg: IvfPqFlatConfig): usize {
    return cfg.m_subspaces * cfg.ksub + cfg.dim + cfg.nlist;
}

/// Exact byte buffer capacity required to serialize an `IvfPqFlatIndex` snapshot with `doc_count` documents.
pub fn ivfPqSnapshotMaxBytes(cfg: IvfPqFlatConfig, doc_count: usize): usize {
    const coarse_bytes: usize = ivfPqCoarseBufLen(cfg) * 4;
    const codebook_bytes: usize = ivfPqCodebookBufLen(cfg) * 4;
    const codes_bytes: usize = doc_count * cfg.m_subspaces;
    const doc_id_bytes: usize = doc_count * 8;
    const deleted_bytes: usize = doc_count;
    const heads_bytes: usize = cfg.nlist * 4;
    const next_bytes: usize = doc_count * 4;
    const counts_bytes: usize = cfg.nlist * 4;
    return IVFPQ_SNAPSHOT_HEADER_BYTES
        + coarse_bytes
        + codebook_bytes
        + codes_bytes
        + doc_id_bytes
        + deleted_bytes
        + heads_bytes
        + next_bytes
        + counts_bytes
        + IVFPQ_SNAPSHOT_CRC_BYTES;
}

// ---- Internal Deterministic SplitMix64 PRNG, Little-Endian & IEEE CRC32 ----

struct IvfPqRng {
    state: u64 = 0x9E3779B97F4A7C15;

    static fn init(seed: u64): IvfPqRng {
        const s: u64 = if (seed == 0) 0x9E3779B97F4A7C15 else seed;
        return IvfPqRng{ state: s };
    }

    fn nextU64(*_): u64 {
        state = state +% 0x9E3779B97F4A7C15;
        let z: u64 = state;
        z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
        z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
        return z ^ (z >> 31);
    }

    fn nextUnitF64(*_): f64 {
        // Upper 53 bits mapped to [0, 1)
        const v: u64 = nextU64() >> 11;
        return (v as f64) * (1.0 / 9007199254740992.0);
    }
}

fn ivfPqCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= data[i] as u32;
        let bit: u32 = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

fn writeLeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeLeU64(buf: []u8, off: usize, v: u64) {
    writeLeU32(buf, off, (v & 0xFFFFFFFF) as u32);
    writeLeU32(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn readLeU32(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32)
        | ((buf[off + 1] as u32) << 8)
        | ((buf[off + 2] as u32) << 16)
        | ((buf[off + 3] as u32) << 24);
}

fn readLeU64(buf: []const u8, off: usize): u64 {
    const lo: u64 = readLeU32(buf, off) as u64;
    const hi: u64 = readLeU32(buf, off + 4) as u64;
    return lo | (hi << 32);
}

/// Unbounded external-slice Residual IVF-PQ engine with deterministic K-Means++ training,
/// native `@l2sq` SIMD distance scoring, soft deletion, and CRC32 binary snapshot persistence.
pub struct IvfPqFlatIndex {
    cfg: IvfPqFlatConfig = IvfPqFlatConfig{},
    dsub: usize = 0;
    trained: bool = false;
    doc_count: usize = 0;
    deleted_count: usize = 0;
    coarse_centroids: []f32 = [];
    pq_codebooks: []f32 = [];
    codes: []u8 = [];
    doc_ids: []u64 = [];
    deleted_flags: []u8 = [];
    list_heads: []u32 = [];
    list_next: []u32 = [];
    list_counts: []u32 = [];

    /// Initializes an empty untrained `IvfPqFlatIndex` backed entirely by caller-provided slices.
    static fn init(
        cfg: IvfPqFlatConfig,
        coarse_centroids: []f32,
        pq_codebooks: []f32,
        codes: []u8,
        doc_ids: []u64,
        deleted_flags: []u8,
        list_heads: []u32,
        list_next: []u32,
        list_counts: []u32,
    ): IvfPqFlatIndex ! IvfPqFlatError {
        if (!cfg.isValid()) {
            return IvfPqFlatError.InvalidConfig;
        }
        if (coarse_centroids.len < ivfPqCoarseBufLen(cfg)
            || pq_codebooks.len < ivfPqCodebookBufLen(cfg)
            || codes.len < ivfPqCodesBufLen(cfg)
            || doc_ids.len < cfg.max_docs
            || deleted_flags.len < cfg.max_docs
            || list_heads.len < cfg.nlist
            || list_next.len < cfg.max_docs
            || list_counts.len < cfg.nlist)
        {
            return IvfPqFlatError.BufferTooSmall;
        }

        let i: usize = 0;
        while (i < cfg.nlist) : (i += 1) {
            list_heads[i] = IVFPQ_INVALID_LINK;
            list_counts[i] = 0;
        }
        let j: usize = 0;
        while (j < cfg.max_docs) : (j += 1) {
            deleted_flags[j] = 0;
            list_next[j] = IVFPQ_INVALID_LINK;
        }

        return IvfPqFlatIndex{
            cfg: cfg,
            dsub: cfg.dim / cfg.m_subspaces,
            trained: false,
            doc_count: 0,
            deleted_count: 0,
            coarse_centroids: coarse_centroids,
            pq_codebooks: pq_codebooks,
            codes: codes,
            doc_ids: doc_ids,
            deleted_flags: deleted_flags,
            list_heads: list_heads,
            list_next: list_next,
            list_counts: list_counts,
        };
    }

    fn coarseSlice(list_id: usize): []const f32 {
        const off = list_id * cfg.dim;
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

    /// Two-stage deterministic K-Means++ + Lloyd training over caller-provided scratch buffers:
    ///   - `work_f32.len >= ivfPqTrainWorkF32Len(cfg, num_vecs)`
    ///   - `work_u32.len >= ivfPqTrainWorkU32Len(cfg, num_vecs)`
    fn train(
        *_,
        flat_vecs: []const f32,
        num_vecs: usize,
        work_f32: []f32,
        work_u32: []u32,
    ): void ! IvfPqFlatError {
        if (num_vecs < cfg.nlist || num_vecs < cfg.ksub) {
            return IvfPqFlatError.InvalidConfig;
        }
        if (flat_vecs.len < num_vecs * cfg.dim) {
            return IvfPqFlatError.DimMismatch;
        }
        if (work_f32.len < ivfPqTrainWorkF32Len(cfg, num_vecs)
            || work_u32.len < ivfPqTrainWorkU32Len(cfg, num_vecs))
        {
            return IvfPqFlatError.BufferTooSmall;
        }

        const dim = cfg.dim;
        let rng = IvfPqRng.init(cfg.seed);

        let min_dists: []f32 = work_f32[0..num_vecs];
        let coarse_sums: []f32 = work_f32[num_vecs .. num_vecs + cfg.nlist * dim];
        let coarse_assign: []u32 = work_u32[0..num_vecs];
        let counts: []u32 = work_u32[num_vecs..];

        // ---- Stage 1: Coarse Quantizer K-Means++ Seeding + Lloyd ----
        const first_idx: usize = @as(usize, @intCast(rng.nextU64() % @as(u64, num_vecs)));
        let d0: usize = 0;
        while (d0 < dim) : (d0 += 1) {
            coarse_centroids[d0] = flat_vecs[first_idx * dim + d0];
        }

        let v: usize = 0;
        while (v < num_vecs) : (v += 1) {
            min_dists[v] = @l2sq(flat_vecs[v * dim .. v * dim + dim], coarseSlice(0));
        }

        let c_init: usize = 1;
        while (c_init < cfg.nlist) : (c_init += 1) {
            let total_d2: f64 = 0.0;
            v = 0;
            while (v < num_vecs) : (v += 1) {
                total_d2 += min_dists[v] as f64;
            }

            let chosen: usize = c_init % num_vecs;
            if (total_d2 > 0.0) {
                const target: f64 = rng.nextUnitF64() * total_d2;
                let acc: f64 = 0.0;
                chosen = num_vecs - 1;
                v = 0;
                while (v < num_vecs) : (v += 1) {
                    acc += min_dists[v] as f64;
                    if (acc >= target) {
                        chosen = v;
                        break;
                    }
                }
            }

            const dst_off = c_init * dim;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                coarse_centroids[dst_off + d] = flat_vecs[chosen * dim + d];
            }

            const new_c: []const f32 = coarseSlice(c_init);
            v = 0;
            while (v < num_vecs) : (v += 1) {
                const dist: f32 = @l2sq(flat_vecs[v * dim .. v * dim + dim], new_c);
                if (dist < min_dists[v]) {
                    min_dists[v] = dist;
                }
            }
        }

        // Lloyd iterations for coarse centroids
        let iter: usize = 0;
        while (iter < cfg.train_iters) : (iter += 1) {
            let z: usize = 0;
            while (z < cfg.nlist) : (z += 1) {
                counts[z] = 0;
                const zoff = z * dim;
                let zd: usize = 0;
                while (zd < dim) : (zd += 1) {
                    coarse_sums[zoff + zd] = 0.0;
                }
            }

            v = 0;
            while (v < num_vecs) : (v += 1) {
                const vs: []const f32 = flat_vecs[v * dim .. v * dim + dim];
                const best_c = nearestCoarse(vs, cfg.nlist);
                coarse_assign[v] = @as(u32, @intCast(best_c));
                counts[best_c] += 1;
                const soff = best_c * dim;
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    coarse_sums[soff + d] += vs[d];
                }
            }

            let c: usize = 0;
            while (c < cfg.nlist) : (c += 1) {
                if (counts[c] == 0) continue;
                const inv: f32 = 1.0 / @as(f32, @floatFromInt(counts[c]));
                const off = c * dim;
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    coarse_centroids[off + d] = coarse_sums[off + d] * inv;
                }
            }
        }

        // Cache final coarse assignments for Stage 2 residual computation
        v = 0;
        while (v < num_vecs) : (v += 1) {
            const vs: []const f32 = flat_vecs[v * dim .. v * dim + dim];
            coarse_assign[v] = @as(u32, @intCast(nearestCoarse(vs, cfg.nlist)));
        }

        // ---- Stage 2: Per-Subspace Residual PQ K-Means++ + Lloyd ----
        let sub_sums: []f32 = work_f32[num_vecs .. num_vecs + cfg.ksub * dsub];
        let res_sub: []f32 = work_f32[num_vecs + cfg.ksub * dsub .. num_vecs + cfg.ksub * dsub + dsub];

        let m: usize = 0;
        while (m < cfg.m_subspaces) : (m += 1) {
            const sub_start = m * dsub;

            // 2a. K-Means++ seeding over residuals in subspace m
            const first_v: usize = @as(usize, @intCast(rng.nextU64() % @as(u64, num_vecs)));
            const fc_off: usize = @as(usize, coarse_assign[first_v]) * dim + sub_start;
            const cb0_off = pqCentroidOffset(m, 0);
            let sd: usize = 0;
            while (sd < dsub) : (sd += 1) {
                pq_codebooks[cb0_off + sd] = flat_vecs[first_v * dim + sub_start + sd] - coarse_centroids[fc_off + sd];
            }

            v = 0;
            while (v < num_vecs) : (v += 1) {
                const cv_off: usize = @as(usize, coarse_assign[v]) * dim + sub_start;
                sd = 0;
                while (sd < dsub) : (sd += 1) {
                    res_sub[sd] = flat_vecs[v * dim + sub_start + sd] - coarse_centroids[cv_off + sd];
                }
                min_dists[v] = @l2sq(res_sub[0..dsub], pq_codebooks[cb0_off .. cb0_off + dsub]);
            }

            let k_init: usize = 1;
            while (k_init < cfg.ksub) : (k_init += 1) {
                let total_d2: f64 = 0.0;
                v = 0;
                while (v < num_vecs) : (v += 1) {
                    total_d2 += min_dists[v] as f64;
                }

                let chosen: usize = k_init % num_vecs;
                if (total_d2 > 0.0) {
                    const target: f64 = rng.nextUnitF64() * total_d2;
                    let acc: f64 = 0.0;
                    chosen = num_vecs - 1;
                    let vk: usize = 0;
                    while (vk < num_vecs) : (vk += 1) {
                        acc += min_dists[vk] as f64;
                        if (acc >= target) {
                            chosen = vk;
                            break;
                        }
                    }
                }

                const ch_off: usize = @as(usize, coarse_assign[chosen]) * dim + sub_start;
                const dst_cb = pqCentroidOffset(m, k_init);
                sd = 0;
                while (sd < dsub) : (sd += 1) {
                    pq_codebooks[dst_cb + sd] = flat_vecs[chosen * dim + sub_start + sd] - coarse_centroids[ch_off + sd];
                }

                v = 0;
                while (v < num_vecs) : (v += 1) {
                    const cv_off: usize = @as(usize, coarse_assign[v]) * dim + sub_start;
                    sd = 0;
                    while (sd < dsub) : (sd += 1) {
                        res_sub[sd] = flat_vecs[v * dim + sub_start + sd] - coarse_centroids[cv_off + sd];
                    }
                    const dist: f32 = @l2sq(res_sub[0..dsub], pq_codebooks[dst_cb .. dst_cb + dsub]);
                    if (dist < min_dists[v]) {
                        min_dists[v] = dist;
                    }
                }
            }

            // 2b. Lloyd iterations for subspace m
            let siter: usize = 0;
            while (siter < cfg.train_iters) : (siter += 1) {
                let k: usize = 0;
                while (k < cfg.ksub) : (k += 1) {
                    counts[k] = 0;
                    const koff = k * dsub;
                    sd = 0;
                    while (sd < dsub) : (sd += 1) {
                        sub_sums[koff + sd] = 0.0;
                    }
                }

                v = 0;
                while (v < num_vecs) : (v += 1) {
                    const cv_off: usize = @as(usize, coarse_assign[v]) * dim + sub_start;
                    sd = 0;
                    while (sd < dsub) : (sd += 1) {
                        res_sub[sd] = flat_vecs[v * dim + sub_start + sd] - coarse_centroids[cv_off + sd];
                    }
                    const bk = nearestSubCentroid(m, res_sub[0..dsub], cfg.ksub);
                    counts[bk] += 1;
                    const koff = bk * dsub;
                    sd = 0;
                    while (sd < dsub) : (sd += 1) {
                        sub_sums[koff + sd] += res_sub[sd];
                    }
                }

                let ku: usize = 0;
                while (ku < cfg.ksub) : (ku += 1) {
                    if (counts[ku] == 0) continue;
                    const inv: f32 = 1.0 / @as(f32, @floatFromInt(counts[ku]));
                    const dst_cb = pqCentroidOffset(m, ku);
                    const koff = ku * dsub;
                    sd = 0;
                    while (sd < dsub) : (sd += 1) {
                        pq_codebooks[dst_cb + sd] = sub_sums[koff + sd] * inv;
                    }
                }
            }
        }

        trained = true;
    }

    /// Encodes `vec` as residual PQ codes and prepends it to its nearest coarse inverted list.
    /// Returns the assigned coarse `list_id` (`u32`).
    fn add(*_, doc_id: u64, vec: []const f32): u32 ! IvfPqFlatError {
        if (!trained) return IvfPqFlatError.NotTrained;
        if (vec.len != cfg.dim) return IvfPqFlatError.DimMismatch;
        if (doc_count >= cfg.max_docs) return IvfPqFlatError.CapacityFull;

        const list_id = nearestCoarse(vec, cfg.nlist);
        const c_off = list_id * cfg.dim;
        const slot = doc_count;
        const code_off = slot * cfg.m_subspaces;

        // Quantize each subspace residual without fixed-dim stack buffers
        let m: usize = 0;
        while (m < cfg.m_subspaces) : (m += 1) {
            const sub_start = m * dsub;
            let best_k: usize = 0;
            let best_d: f32 = 0.0;
            let k: usize = 0;
            while (k < cfg.ksub) : (k += 1) {
                const cb_off = pqCentroidOffset(m, k);
                let dist: f32 = 0.0;
                let sd: usize = 0;
                while (sd < dsub) : (sd += 1) {
                    const r: f32 = vec[sub_start + sd] - coarse_centroids[c_off + sub_start + sd];
                    const diff: f32 = r - pq_codebooks[cb_off + sd];
                    dist += diff * diff;
                }
                if (k == 0 || dist < best_d) {
                    best_d = dist;
                    best_k = k;
                }
            }
            codes[code_off + m] = @as(u8, @intCast(best_k));
        }

        doc_ids[slot] = doc_id;
        deleted_flags[slot] = 0;
        list_next[slot] = list_heads[list_id];
        list_heads[list_id] = @as(u32, @intCast(slot));
        list_counts[list_id] += 1;
        doc_count += 1;

        return @as(u32, @intCast(list_id));
    }

    /// Soft-deletes the first active document matching `doc_id`. Returns `true` if deleted.
    fn markDeleted(*_, doc_id: u64): bool {
        let i: usize = 0;
        while (i < doc_count) : (i += 1) {
            if (deleted_flags[i] == 0 && doc_ids[i] == doc_id) {
                deleted_flags[i] = 1;
                deleted_count += 1;
                return true;
            }
        }
        return false;
    }

    /// Reconstructs the approximate vector for stored slot `doc_idx`: `coarse_centroid + sum(pq_codebook[m][code[m]])`.
    fn reconstruct(doc_idx: usize, out_vec: []f32): bool {
        if (!trained || doc_idx >= doc_count || out_vec.len < cfg.dim) return false;

        // Locate owning coarse list for `doc_idx`
        let found_list: usize = cfg.nlist;
        let l: usize = 0;
        while (l < cfg.nlist) : (l += 1) {
            let cur: u32 = list_heads[l];
            while (cur != IVFPQ_INVALID_LINK) {
                if (@as(usize, cur) == doc_idx) {
                    found_list = l;
                    break;
                }
                cur = list_next[@as(usize, cur)];
            }
            if (found_list < cfg.nlist) break;
        }
        if (found_list >= cfg.nlist) return false;

        const c_off = found_list * cfg.dim;
        let d: usize = 0;
        while (d < cfg.dim) : (d += 1) {
            out_vec[d] = coarse_centroids[c_off + d];
        }

        const code_off = doc_idx * cfg.m_subspaces;
        let m: usize = 0;
        while (m < cfg.m_subspaces) : (m += 1) {
            const code: usize = @as(usize, codes[code_off + m]);
            const cb_off = pqCentroidOffset(m, code);
            const sub_start = m * dsub;
            let sd: usize = 0;
            while (sd < dsub) : (sd += 1) {
                out_vec[sub_start + sd] += pq_codebooks[cb_off + sd];
            }
        }
        return true;
    }

    /// Multi-probe Asymmetric Distance Computation (ADC) Top-K search over caller-provided
    /// scratch buffer `adc_lut_work` (`len >= ivfPqSearchWorkF32Len(cfg)`), skipping soft-deleted
    /// entries and writing hits sorted strictly ascending by `(approx_l2_sq, doc_id)`.
    fn search(
        *_,
        query_vec: []const f32,
        nprobe: usize,
        adc_lut_work: []f32,
        out_hits: []IvfPqHit,
    ): usize ! IvfPqFlatError {
        if (!trained) return IvfPqFlatError.NotTrained;
        if (query_vec.len != cfg.dim) return IvfPqFlatError.DimMismatch;
        if (adc_lut_work.len < ivfPqSearchWorkF32Len(cfg)) {
            return IvfPqFlatError.BufferTooSmall;
        }
        if (out_hits.len == 0 || doc_count == 0) return 0;

        let eff_nprobe: usize = if (nprobe == 0) 1 else nprobe;
        if (eff_nprobe > cfg.nlist) eff_nprobe = cfg.nlist;

        const adc_len: usize = cfg.m_subspaces * cfg.ksub;
        let adc: []f32 = adc_lut_work[0..adc_len];
        let q_res: []f32 = adc_lut_work[adc_len .. adc_len + cfg.dim];
        let list_dists: []f32 = adc_lut_work[adc_len + cfg.dim .. adc_len + cfg.dim + cfg.nlist];

        // 1. Compute distance from query to every coarse centroid
        let l: usize = 0;
        while (l < cfg.nlist) : (l += 1) {
            list_dists[l] = @l2sq(query_vec, coarseSlice(l));
        }

        // 2. Iteratively select top `eff_nprobe` closest coarse lists & score active docs in O(M)
        let hit_count: usize = 0;
        let p: usize = 0;
        while (p < eff_nprobe) : (p += 1) {
            let best_l: usize = cfg.nlist;
            let best_ld: f32 = 0.0;
            l = 0;
            while (l < cfg.nlist) : (l += 1) {
                if (list_dists[l] < 0.0) continue; // Already probed
                if (best_l == cfg.nlist || list_dists[l] < best_ld) {
                    best_l = l;
                    best_ld = list_dists[l];
                }
            }
            if (best_l == cfg.nlist) break;
            list_dists[best_l] = -1.0; // Mark probed

            if (list_heads[best_l] == IVFPQ_INVALID_LINK) continue;

            const c_off = best_l * cfg.dim;
            let d: usize = 0;
            while (d < cfg.dim) : (d += 1) {
                q_res[d] = query_vec[d] - coarse_centroids[c_off + d];
            }

            // Precompute ADC distance table `[m_subspaces][ksub]`
            let m: usize = 0;
            while (m < cfg.m_subspaces) : (m += 1) {
                const sub_start = m * dsub;
                const q_sub: []const f32 = q_res[sub_start .. sub_start + dsub];
                let k: usize = 0;
                while (k < cfg.ksub) : (k += 1) {
                    const cb_off = pqCentroidOffset(m, k);
                    adc[m * cfg.ksub + k] = @l2sq(q_sub, pq_codebooks[cb_off .. cb_off + dsub]);
                }
            }

            // Walk inverted list and accumulate O(M) ADC distance
            let cur: u32 = list_heads[best_l];
            while (cur != IVFPQ_INVALID_LINK) {
                const slot: usize = @as(usize, cur);
                if (deleted_flags[slot] != 0) {
                    cur = list_next[slot];
                    continue;
                }

                const code_off = slot * cfg.m_subspaces;
                let approx_d: f32 = 0.0;
                let sm: usize = 0;
                while (sm < cfg.m_subspaces) : (sm += 1) {
                    const code: usize = @as(usize, codes[code_off + sm]);
                    approx_d += adc[sm * cfg.ksub + code];
                }

                const cand = IvfPqHit{
                    doc_id: doc_ids[slot],
                    list_id: @as(u16, @intCast(best_l & 0xFFFF)),
                    approx_l2_sq: approx_d,
                };

                if (hit_count < out_hits.len) {
                    out_hits[hit_count] = cand;
                    hit_count += 1;
                } else {
                    const last = out_hits[hit_count - 1];
                    const better = (cand.approx_l2_sq < last.approx_l2_sq)
                        || (cand.approx_l2_sq == last.approx_l2_sq && cand.doc_id < last.doc_id);
                    if (better) {
                        out_hits[hit_count - 1] = cand;
                    } else {
                        cur = list_next[slot];
                        continue;
                    }
                }

                let pos: usize = hit_count - 1;
                while (pos > 0) {
                    const prev = out_hits[pos - 1];
                    const curr = out_hits[pos];
                    const swap = (curr.approx_l2_sq < prev.approx_l2_sq)
                        || (curr.approx_l2_sq == prev.approx_l2_sq && curr.doc_id < prev.doc_id);
                    if (!swap) break;
                    out_hits[pos - 1] = curr;
                    out_hits[pos] = prev;
                    pos -= 1;
                }

                cur = list_next[slot];
            }
        }

        return hit_count;
    }

    /// Returns index telemetry and compression statistics (`doc_count` reflects active non-deleted docs).
    fn stats(): IvfPqStats {
        const raw_bytes: u64 = @as(u64, cfg.dim) * 4;
        const code_bytes: u64 = @as(u64, cfg.m_subspaces);
        const ratio_x100: u32 = if (code_bytes > 0) @as(u32, @intCast((raw_bytes * 100) / code_bytes)) else 0;
        const live_docs: usize = if (doc_count >= deleted_count) doc_count - deleted_count else 0;
        return IvfPqStats{
            doc_count: live_docs,
            nlist: cfg.nlist,
            m_subspaces: cfg.m_subspaces,
            ksub: cfg.ksub,
            bytes_per_code: cfg.m_subspaces,
            compression_ratio_x100: ratio_x100,
            trained: trained,
        };
    }
}

/// Serializes an `IvfPqFlatIndex` (header, config, coarse centroids, PQ codebooks, codes,
/// doc IDs, soft-delete flags, inverted list links & counts, plus trailing IEEE CRC32) into `out_bytes`.
pub fn serializeIvfPqSnapshot(idx: IvfPqFlatIndex, out_bytes: []u8): usize ! IvfPqFlatError {
    const req: usize = ivfPqSnapshotMaxBytes(idx.cfg, idx.doc_count);
    if (out_bytes.len < req) {
        return IvfPqFlatError.BufferTooSmall;
    }

    // 56-byte header
    writeLeU32(out_bytes, 0, IVFPQ_SNAPSHOT_MAGIC);
    writeLeU32(out_bytes, 4, IVFPQ_SNAPSHOT_VERSION);
    writeLeU32(out_bytes, 8, @as(u32, @intCast(idx.cfg.dim)));
    writeLeU32(out_bytes, 12, @as(u32, @intCast(idx.cfg.max_docs)));
    writeLeU32(out_bytes, 16, @as(u32, @intCast(idx.cfg.nlist)));
    writeLeU32(out_bytes, 20, @as(u32, @intCast(idx.cfg.m_subspaces)));
    writeLeU32(out_bytes, 24, @as(u32, @intCast(idx.cfg.ksub)));
    writeLeU32(out_bytes, 28, @as(u32, @intCast(idx.cfg.train_iters)));
    writeLeU32(out_bytes, 32, @as(u32, @intCast(idx.doc_count)));
    writeLeU32(out_bytes, 36, @as(u32, @intCast(idx.deleted_count)));
    writeLeU32(out_bytes, 40, if (idx.trained) 1 else 0);
    writeLeU32(out_bytes, 44, 0);
    writeLeU64(out_bytes, 48, idx.cfg.seed);

    let pos: usize = IVFPQ_SNAPSHOT_HEADER_BYTES;

    // 1. Coarse centroids
    const coarse_len = ivfPqCoarseBufLen(idx.cfg);
    let i: usize = 0;
    while (i < coarse_len) : (i += 1) {
        const bits: u32 = @bitCast(idx.coarse_centroids[i]);
        writeLeU32(out_bytes, pos, bits);
        pos += 4;
    }

    // 2. PQ codebooks
    const cb_len = ivfPqCodebookBufLen(idx.cfg);
    i = 0;
    while (i < cb_len) : (i += 1) {
        const bits: u32 = @bitCast(idx.pq_codebooks[i]);
        writeLeU32(out_bytes, pos, bits);
        pos += 4;
    }

    // 3. Compressed residual codes
    const codes_len = idx.doc_count * idx.cfg.m_subspaces;
    i = 0;
    while (i < codes_len) : (i += 1) {
        out_bytes[pos] = idx.codes[i];
        pos += 1;
    }

    // 4. Document IDs
    i = 0;
    while (i < idx.doc_count) : (i += 1) {
        writeLeU64(out_bytes, pos, idx.doc_ids[i]);
        pos += 8;
    }

    // 5. Soft-delete flags
    i = 0;
    while (i < idx.doc_count) : (i += 1) {
        out_bytes[pos] = idx.deleted_flags[i];
        pos += 1;
    }

    // 6. Inverted list heads
    i = 0;
    while (i < idx.cfg.nlist) : (i += 1) {
        writeLeU32(out_bytes, pos, idx.list_heads[i]);
        pos += 4;
    }

    // 7. Inverted list next pointers
    i = 0;
    while (i < idx.doc_count) : (i += 1) {
        writeLeU32(out_bytes, pos, idx.list_next[i]);
        pos += 4;
    }

    // 8. Inverted list counts
    i = 0;
    while (i < idx.cfg.nlist) : (i += 1) {
        writeLeU32(out_bytes, pos, idx.list_counts[i]);
        pos += 4;
    }

    // 9. Trailing IEEE CRC32
    const crc = ivfPqCrc32(out_bytes[0..pos]);
    writeLeU32(out_bytes, pos, crc);
    pos += IVFPQ_SNAPSHOT_CRC_BYTES;
    return pos;
}

/// Deserializes an `IvfPqFlatIndex` snapshot with full magic, version, length, bounds, and
/// IEEE CRC32 verification into caller-provided backing slices (zero re-training).
pub fn deserializeIvfPqSnapshot(
    bytes: []const u8,
    coarse_centroids: []f32,
    pq_codebooks: []f32,
    codes: []u8,
    doc_ids: []u64,
    deleted_flags: []u8,
    list_heads: []u32,
    list_next: []u32,
    list_counts: []u32,
): IvfPqFlatIndex ! IvfPqFlatError {
    if (bytes.len < IVFPQ_SNAPSHOT_HEADER_BYTES + IVFPQ_SNAPSHOT_CRC_BYTES) {
        return IvfPqFlatError.InvalidSnapshot;
    }

    const magic = readLeU32(bytes, 0);
    const version = readLeU32(bytes, 4);
    if (magic != IVFPQ_SNAPSHOT_MAGIC || version != IVFPQ_SNAPSHOT_VERSION) {
        return IvfPqFlatError.InvalidSnapshot;
    }

    const cfg = IvfPqFlatConfig{
        dim: @as(usize, readLeU32(bytes, 8)),
        max_docs: @as(usize, readLeU32(bytes, 12)),
        nlist: @as(usize, readLeU32(bytes, 16)),
        m_subspaces: @as(usize, readLeU32(bytes, 20)),
        ksub: @as(usize, readLeU32(bytes, 24)),
        train_iters: @as(usize, readLeU32(bytes, 28)),
        seed: readLeU64(bytes, 48),
    };
    const doc_count: usize = @as(usize, readLeU32(bytes, 32));
    const deleted_count: usize = @as(usize, readLeU32(bytes, 36));
    const trained_flag: u32 = readLeU32(bytes, 40);

    if (!cfg.isValid() || doc_count > cfg.max_docs || deleted_count > doc_count || trained_flag > 1) {
        return IvfPqFlatError.InvalidSnapshot;
    }

    const expected_len: usize = ivfPqSnapshotMaxBytes(cfg, doc_count);
    if (bytes.len < expected_len) {
        return IvfPqFlatError.InvalidSnapshot;
    }

    // Verify IEEE CRC32 before mutating caller buffers
    const payload_end: usize = expected_len - IVFPQ_SNAPSHOT_CRC_BYTES;
    const wire_crc: u32 = readLeU32(bytes, payload_end);
    const calc_crc: u32 = ivfPqCrc32(bytes[0..payload_end]);
    if (wire_crc != calc_crc) {
        return IvfPqFlatError.ChecksumMismatch;
    }

    let idx = IvfPqFlatIndex.init(
        cfg,
        coarse_centroids,
        pq_codebooks,
        codes,
        doc_ids,
        deleted_flags,
        list_heads,
        list_next,
        list_counts,
    )?;

    let pos: usize = IVFPQ_SNAPSHOT_HEADER_BYTES;

    const coarse_len = ivfPqCoarseBufLen(cfg);
    let i: usize = 0;
    while (i < coarse_len) : (i += 1) {
        const fval: f32 = @bitCast(readLeU32(bytes, pos));
        idx.coarse_centroids[i] = fval;
        pos += 4;
    }

    const cb_len = ivfPqCodebookBufLen(cfg);
    i = 0;
    while (i < cb_len) : (i += 1) {
        const fval: f32 = @bitCast(readLeU32(bytes, pos));
        idx.pq_codebooks[i] = fval;
        pos += 4;
    }

    const codes_len = doc_count * cfg.m_subspaces;
    i = 0;
    while (i < codes_len) : (i += 1) {
        idx.codes[i] = bytes[pos];
        pos += 1;
    }

    i = 0;
    while (i < doc_count) : (i += 1) {
        idx.doc_ids[i] = readLeU64(bytes, pos);
        pos += 8;
    }

    i = 0;
    while (i < doc_count) : (i += 1) {
        idx.deleted_flags[i] = bytes[pos];
        pos += 1;
    }

    i = 0;
    while (i < cfg.nlist) : (i += 1) {
        idx.list_heads[i] = readLeU32(bytes, pos);
        pos += 4;
    }

    i = 0;
    while (i < doc_count) : (i += 1) {
        idx.list_next[i] = readLeU32(bytes, pos);
        pos += 4;
    }

    i = 0;
    while (i < cfg.nlist) : (i += 1) {
        idx.list_counts[i] = readLeU32(bytes, pos);
        pos += 4;
    }

    idx.doc_count = doc_count;
    idx.deleted_count = deleted_count;
    idx.trained = (trained_flag == 1);
    return idx;
}
