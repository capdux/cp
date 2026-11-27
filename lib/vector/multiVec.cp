// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Vector Late-Interaction Chamfer/MaxSim Index & Token Alignment) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation multi-vector document retrieval & token-level attribution index:
//   - Flat contiguous `f32` token embedding arena (`MULTIVEC_ARENA_FLOATS = 16384`) supporting up to
//     `MULTIVEC_MAX_DOCS = 64` variable-length multi-vector documents with auto-maintained L2 centroids
//   - Two-stage SIMD `@dot` retrieval (`searchTwoStage`):
//       Stage 1: Fast centroid coarse pruning via `@dot(query_centroid, doc_centroid)` -> Top-`coarse_k`
//       Stage 2: Weighted MaxSim (`sum_i w_i * max_j @dot(q_i, d_j)`) or symmetric Chamfer rescoring
//   - Token-level alignment attribution (`explainAlignment`) reporting each query token's best-matching
//     document token index and weighted dot-product contribution for explainable RAG highlighting

const std = @import("std");

/// Maximum number of multi-vector documents stored inline in `MultiVecIndex`.
pub const MULTIVEC_MAX_DOCS: usize = 64;

/// Maximum token embedding dimensionality supported inline.
pub const MULTIVEC_MAX_DIM: usize = 32;

/// Contiguous `f32` token embedding arena capacity (16,384 floats = 64 KiB).
pub const MULTIVEC_ARENA_FLOATS: usize = 16384;

// ============================================================================
// 1. Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Second-stage fine scoring mode for multi-vector late interaction.
pub enum MultiVecScoreMode: u8 {
    /// Asymmetric weighted MaxSim: `sum_i w_i * max_j @dot(q_i, d_j)`.
    weighted_maxsim = 0,
    /// Symmetric Chamfer similarity: `0.5 * (weighted_q2d_avg + d2q_avg)`.
    chamfer_symmetric = 1,
}

/// Ranked document hit returned by `MultiVecIndex.searchTwoStage`.
pub struct MultiVecHit {
    doc_id: u64 = 0;
    score: f32 = 0.0;
    coarse_score: f32 = 0.0;
    q2d_maxsim: f32 = 0.0;
    d2q_maxsim: f32 = 0.0;
    token_count: usize = 0;
}

/// Token-level alignment attribution record returned by `MultiVecIndex.explainAlignment`.
pub struct MultiVecTokenAlign {
    query_token_idx: usize = 0;
    best_doc_token_idx: usize = 0;
    dot_score: f32 = 0.0;
    weighted_contribution: f32 = 0.0;
}

/// Zero-heap telemetry snapshot of `MultiVecIndex`.
pub struct MultiVecIndexStats {
    dim: usize = 0;
    doc_capacity: usize = MULTIVEC_MAX_DOCS;
    doc_count: usize = 0;
    total_tokens: usize = 0;
    arena_used_floats: usize = 0;
    arena_capacity_floats: usize = MULTIVEC_ARENA_FLOATS;
    total_searches: u64 = 0;
    total_coarse_pruned: u64 = 0;
}

// ============================================================================
// 2. Internal Storage & Finite-Float Helpers
// ============================================================================

struct MultiVecDocSlot {
    occupied: bool = false;
    doc_id: u64 = 0;
    token_offset: usize = 0;
    token_count: usize = 0;
    centroid: [MULTIVEC_MAX_DIM]f32 = [0.0; MULTIVEC_MAX_DIM];
}

struct CoarseCandidate {
    slot_idx: usize = 0;
    doc_id: u64 = 0;
    coarse_score: f32 = 0.0;
}

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn allFiniteF32(slice: []const f32): bool {
    let idx: usize = 0;
    while (idx < slice.len) {
        if (!isFiniteF32(slice[idx])) {
            return false;
        }
        idx += 1;
    }
    return true;
}

fn normalizeInPlace(vec: *[]f32) {
    const v_ro: []const f32 = vec[0..];
    const norm_sq: f32 = @dot(v_ro, v_ro);
    if (norm_sq > 1.0e-12 && isFiniteF32(norm_sq)) {
        const inv: f32 = 1.0 / @sqrt(norm_sq);
        let idx: usize = 0;
        while (idx < vec.len) {
            vec[idx] *= inv;
            idx += 1;
        }
    }
}

fn coarseBetter(a: CoarseCandidate, b: CoarseCandidate): bool {
    if (a.coarse_score != b.coarse_score) {
        return a.coarse_score > b.coarse_score;
    }
    return a.doc_id < b.doc_id;
}

fn hitBetter(a: MultiVecHit, b: MultiVecHit): bool {
    if (a.score != b.score) {
        return a.score > b.score;
    }
    return a.doc_id < b.doc_id;
}

// ============================================================================
// 3. MultiVecIndex Core Engine
// ============================================================================

/// Zero-heap two-stage Multi-Vector Chamfer/MaxSim retrieval & token alignment index.
pub struct MultiVecIndex {
    dim: usize = 0;
    docs: [MULTIVEC_MAX_DOCS]MultiVecDocSlot = [MultiVecDocSlot{}; MULTIVEC_MAX_DOCS];
    arena: [MULTIVEC_ARENA_FLOATS]f32 = undefined;
    doc_count: usize = 0;
    total_tokens: usize = 0;
    arena_used: usize = 0;
    total_searches: u64 = 0;
    total_coarse_pruned: u64 = 0;

    /// Initializes an empty `MultiVecIndex` with embedding dimension `1..=MULTIVEC_MAX_DIM`.
    static fn init(dim: usize): MultiVecIndex? {
        if (dim == 0 || dim > MULTIVEC_MAX_DIM) {
            return null;
        }
        return MultiVecIndex{
            dim: dim,
        };
    }

    /// Clears all indexed documents, arena floats, and search statistics.
    fn clear(*_) {
        let idx: usize = 0;
        while (idx < _.doc_count && idx < MULTIVEC_MAX_DOCS) {
            _.docs[idx] = MultiVecDocSlot{};
            idx += 1;
        }
        _.doc_count = 0;
        _.total_tokens = 0;
        _.arena_used = 0;
        _.total_searches = 0;
        _.total_coarse_pruned = 0;
    }

    fn findDocSlot(doc_id: u64): usize? {
        if (doc_id == 0) {
            return null;
        }
        let idx: usize = 0;
        while (idx < _.doc_count && idx < MULTIVEC_MAX_DOCS) {
            if (_.docs[idx].occupied && _.docs[idx].doc_id == doc_id) {
                return idx;
            }
            idx += 1;
        }
        return null;
    }

    /// Indexes a variable-length multi-vector document (`doc_tokens_flat.len == n_tokens * dim`),
    /// optionally L2-normalizing each token vector and computing the normalized document centroid.
    fn addDocument(*_, doc_id: u64, doc_tokens_flat: []const f32, normalize_tokens: bool): bool {
        if (doc_id == 0 || _.dim == 0) {
            return false;
        }
        if (doc_tokens_flat.len == 0 || (doc_tokens_flat.len % _.dim) != 0) {
            return false;
        }
        if (_.doc_count >= MULTIVEC_MAX_DOCS) {
            return false;
        }
        if (_.arena_used + doc_tokens_flat.len > MULTIVEC_ARENA_FLOATS) {
            return false;
        }
        if (_ := _.findDocSlot(doc_id)) {
            return false;
        }
        if (!allFiniteF32(doc_tokens_flat)) {
            return false;
        }

        const n_tokens: usize = doc_tokens_flat.len / _.dim;
        const base_off: usize = _.arena_used;

        let centroid: [MULTIVEC_MAX_DIM]f32 = [0.0; MULTIVEC_MAX_DIM];
        let t: usize = 0;
        while (t < n_tokens) {
            const src_start: usize = t * _.dim;
            const dst_start: usize = base_off + src_start;
            if (_.dim > 0) {
                @memcpy(_.arena[dst_start..(dst_start + _.dim)], doc_tokens_flat[src_start..(src_start + _.dim)]);
            }
            if (normalize_tokens) {
                let tok_s: []f32 = _.arena[dst_start..(dst_start + _.dim)];
                normalizeInPlace(*tok_s);
            }
            let d2: usize = 0;
            while (d2 < _.dim) {
                centroid[d2] += _.arena[dst_start + d2];
                d2 += 1;
            }
            t += 1;
        }

        let cent_s: []f32 = centroid[0.._.dim];
        normalizeInPlace(*cent_s);

        _.docs[_.doc_count] = MultiVecDocSlot{
            occupied: true,
            doc_id: doc_id,
            token_offset: base_off,
            token_count: n_tokens,
            centroid: centroid,
        };
        _.doc_count += 1;
        _.total_tokens += n_tokens;
        _.arena_used = base_off + doc_tokens_flat.len;
        return true;
    }

    /// Two-stage retrieval:
    ///   Stage 1: Centroid `@dot` coarse pruning to `coarse_k` candidates (`0` = scan all docs).
    ///   Stage 2: Weighted MaxSim or symmetric Chamfer late-interaction rescoring -> Top-`top_k`.
    fn searchTwoStage(
        *_,
        query_tokens_flat: []const f32,
        query_weights: []const f32,
        mode: MultiVecScoreMode,
        coarse_k: usize,
        top_k: usize,
        out_hits: *[]MultiVecHit,
    ): usize? {
        if (_.dim == 0 || query_tokens_flat.len == 0 || (query_tokens_flat.len % _.dim) != 0) {
            return null;
        }
        if (!allFiniteF32(query_tokens_flat)) {
            return null;
        }
        const n_q: usize = query_tokens_flat.len / _.dim;
        const use_weights: bool = query_weights.len > 0;
        if (use_weights) {
            if (query_weights.len != n_q || !allFiniteF32(query_weights)) {
                return null;
            }
            let wi: usize = 0;
            while (wi < n_q) {
                if (query_weights[wi] < 0.0) {
                    return null;
                }
                wi += 1;
            }
        }

        _.total_searches += 1;
        if (_.doc_count == 0 || top_k == 0 || out_hits.len == 0) {
            return 0;
        }

        // Compute weighted & normalized query centroid for Stage 1 coarse pruning.
        let q_centroid: [MULTIVEC_MAX_DIM]f32 = [0.0; MULTIVEC_MAX_DIM];
        let weight_sum: f32 = 0.0;
        let qi: usize = 0;
        while (qi < n_q) {
            const w: f32 = if (use_weights) query_weights[qi] else 1.0;
            weight_sum += w;
            const q_off: usize = qi * _.dim;
            let d: usize = 0;
            while (d < _.dim) {
                q_centroid[d] += w * query_tokens_flat[q_off + d];
                d += 1;
            }
            qi += 1;
        }
        let qc_s: []f32 = q_centroid[0.._.dim];
        normalizeInPlace(*qc_s);

        // Stage 1: Score all docs by centroid dot product.
        const q_cent_ro: []const f32 = q_centroid[0.._.dim];
        let coarse_cands: [MULTIVEC_MAX_DOCS]CoarseCandidate = undefined;
        let c_n: usize = 0;
        let idx: usize = 0;
        while (idx < _.doc_count) {
            const d_cent_ro: []const f32 = _.docs[idx].centroid[0.._.dim];
            const c_score: f32 = @dot(q_cent_ro, d_cent_ro);
            coarse_cands[c_n] = CoarseCandidate{
                slot_idx: idx,
                doc_id: _.docs[idx].doc_id,
                coarse_score: c_score,
            };
            c_n += 1;
            idx += 1;
        }

        // Sort coarse candidates descending by coarse_score.
        let s1: usize = 1;
        while (s1 < c_n) {
            const key = coarse_cands[s1];
            let j: usize = s1;
            while (j > 0 && coarseBetter(key, coarse_cands[j - 1])) {
                coarse_cands[j] = coarse_cands[j - 1];
                j -= 1;
            }
            coarse_cands[j] = key;
            s1 += 1;
        }

        const eff_coarse_k: usize = if (coarse_k == 0 || coarse_k > c_n) c_n else coarse_k;
        _.total_coarse_pruned += @as(u64, c_n - eff_coarse_k);

        // Stage 2: Exact late-interaction weighted MaxSim & Chamfer rescoring on Top-`eff_coarse_k`.
        let hits: [MULTIVEC_MAX_DOCS]MultiVecHit = undefined;
        let ci: usize = 0;
        while (ci < eff_coarse_k) {
            const cand = coarse_cands[ci];
            const doc = _.docs[cand.slot_idx];

            // Q -> D weighted MaxSim
            let q2d_sum: f32 = 0.0;
            let q_idx: usize = 0;
            while (q_idx < n_q) {
                const w: f32 = if (use_weights) query_weights[q_idx] else 1.0;
                const q_off: usize = q_idx * _.dim;
                const q_vec: []const f32 = query_tokens_flat[q_off..(q_off + _.dim)];

                let best_dot: f32 = -1.0e30;
                let d_idx: usize = 0;
                while (d_idx < doc.token_count) {
                    const d_off: usize = doc.token_offset + d_idx * _.dim;
                    const d_vec: []const f32 = _.arena[d_off..(d_off + _.dim)];
                    const dot_val: f32 = @dot(q_vec, d_vec);
                    if (dot_val > best_dot) {
                        best_dot = dot_val;
                    }
                    d_idx += 1;
                }
                q2d_sum += w * best_dot;
                q_idx += 1;
            }

            // D -> Q average MaxSim (for Chamfer symmetric similarity & diagnostics)
            let d2q_sum: f32 = 0.0;
            let dj: usize = 0;
            while (dj < doc.token_count) {
                const d_off: usize = doc.token_offset + dj * _.dim;
                const d_vec: []const f32 = _.arena[d_off..(d_off + _.dim)];

                let best_rev: f32 = -1.0e30;
                let qj: usize = 0;
                while (qj < n_q) {
                    const q_off: usize = qj * _.dim;
                    const q_vec: []const f32 = query_tokens_flat[q_off..(q_off + _.dim)];
                    const dot_val: f32 = @dot(d_vec, q_vec);
                    if (dot_val > best_rev) {
                        best_rev = dot_val;
                    }
                    qj += 1;
                }
                d2q_sum += best_rev;
                dj += 1;
            }
            const d2q_avg: f32 = if (doc.token_count > 0) d2q_sum / @as(f32, @floatFromInt(doc.token_count)) else 0.0;

            let final_score: f32 = q2d_sum;
            if (mode == MultiVecScoreMode.chamfer_symmetric) {
                const q2d_avg: f32 = if (weight_sum > 1.0e-12) q2d_sum / weight_sum else 0.0;
                final_score = 0.5 * (q2d_avg + d2q_avg);
            }

            hits[ci] = MultiVecHit{
                doc_id: doc.doc_id,
                score: final_score,
                coarse_score: cand.coarse_score,
                q2d_maxsim: q2d_sum,
                d2q_maxsim: d2q_avg,
                token_count: doc.token_count,
            };
            ci += 1;
        }

        // Deterministic insertion sort by (score desc, doc_id asc).
        let s2: usize = 1;
        while (s2 < eff_coarse_k) {
            const key = hits[s2];
            let j: usize = s2;
            while (j > 0 && hitBetter(key, hits[j - 1])) {
                hits[j] = hits[j - 1];
                j -= 1;
            }
            hits[j] = key;
            s2 += 1;
        }

        let out_n: usize = eff_coarse_k;
        if (top_k < out_n) {
            out_n = top_k;
        }
        if (out_hits.len < out_n) {
            out_n = out_hits.len;
        }

        let k: usize = 0;
        while (k < out_n) {
            out_hits[k] = hits[k];
            k += 1;
        }
        return out_n;
    }

    /// Computes token-level alignment attribution between `query_tokens_flat` and indexed `doc_id`.
    /// Writes up to `min(n_query_tokens, out_pairs.len)` alignment entries and returns count, or `null` on invalid input.
    fn explainAlignment(
        doc_id: u64,
        query_tokens_flat: []const f32,
        query_weights: []const f32,
        out_pairs: *[]MultiVecTokenAlign,
    ): usize? {
        if (_.dim == 0 || query_tokens_flat.len == 0 || (query_tokens_flat.len % _.dim) != 0) {
            return null;
        }
        if (!allFiniteF32(query_tokens_flat)) {
            return null;
        }
        const slot_idx = _.findDocSlot(doc_id) ?? return null;
        const n_q: usize = query_tokens_flat.len / _.dim;
        const use_weights: bool = query_weights.len > 0;
        if (use_weights && (query_weights.len != n_q || !allFiniteF32(query_weights))) {
            return null;
        }

        const doc = _.docs[slot_idx];
        const out_n: usize = if (n_q < out_pairs.len) n_q else out_pairs.len;

        let q_idx: usize = 0;
        while (q_idx < out_n) {
            const w: f32 = if (use_weights) query_weights[q_idx] else 1.0;
            const q_off: usize = q_idx * _.dim;
            const q_vec: []const f32 = query_tokens_flat[q_off..(q_off + _.dim)];

            let best_j: usize = 0;
            let best_dot: f32 = -1.0e30;
            let d_idx: usize = 0;
            while (d_idx < doc.token_count) {
                const d_off: usize = doc.token_offset + d_idx * _.dim;
                const d_vec: []const f32 = _.arena[d_off..(d_off + _.dim)];
                const dot_val: f32 = @dot(q_vec, d_vec);
                if (dot_val > best_dot) {
                    best_dot = dot_val;
                    best_j = d_idx;
                }
                d_idx += 1;
            }

            out_pairs[q_idx] = MultiVecTokenAlign{
                query_token_idx: q_idx,
                best_doc_token_idx: best_j,
                dot_score: best_dot,
                weighted_contribution: w * best_dot,
            };
            q_idx += 1;
        }

        return out_n;
    }

    /// Returns a zero-heap telemetry snapshot of the index.
    fn stats(): MultiVecIndexStats {
        return MultiVecIndexStats{
            dim: _.dim,
            doc_capacity: MULTIVEC_MAX_DOCS,
            doc_count: _.doc_count,
            total_tokens: _.total_tokens,
            arena_used_floats: _.arena_used,
            arena_capacity_floats: MULTIVEC_ARENA_FLOATS,
            total_searches: _.total_searches,
            total_coarse_pruned: _.total_coarse_pruned,
        };
    }
}
