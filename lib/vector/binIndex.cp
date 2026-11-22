// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc 1-Bit Binary Quantized Vector Index & Two-Stage SIMD Rescorer) | Created: 2026-11-22
// Pure CP zero-heap-allocation 1-Bit Binary Quantization Vector Index (32x memory compression):
//   - Fixed inline 256-document bitmap pool (`BIN_IDX_MAX_DOCS`) supporting up to 1024-D (`BIN_IDX_MAX_WORDS = 16`)
//   - Little-endian 64-bit word packing (`vec[i] >= threshold_f32`) with NaN/Inf guards
//   - Stage 1 coarse Top-K retrieval using hardware `@popCount(q_word ^ d_word)` Hamming distance
//   - Stage 2 exact Top-K rescoring using CP native 8-lane SIMD `@dot` over caller's `raw_doc_pool`
//   - Deterministic tie-breaking by `doc_id` ascending, O(1) slot reclamation on `remove_document`

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum documents stored inline in `BinaryVectorIndex` (zero dynamic heap).
pub const BIN_IDX_MAX_DOCS: usize = 256;

/// Maximum 64-bit bit-packed words per document vector (`16 * 64 = 1024` dimensions).
pub const BIN_IDX_MAX_WORDS: usize = 16;

/// Configuration for `BinaryVectorIndex`.
pub struct BinaryIndexConfig {
    dim: usize = 64;
    threshold_f32: f32 = 0.0;

    /// Returns the number of 64-bit words required to pack `dim` bits (`ceil(dim / 64)`).
    fn wordsPerVec(): usize {
        if (_.dim == 0) return 0;
        return (_.dim + 63) / 64;
    }

    /// Validates `1 <= dim <= 1024` and finite `threshold_f32`.
    fn isValid(): bool {
        return _.dim >= 1 && _.dim <= (BIN_IDX_MAX_WORDS * 64) && isFiniteF32(_.threshold_f32);
    }
}

/// Search hit returned by Stage 1 Hamming retrieval or Stage 2 `@dot` rescoring.
pub struct BinaryIndexHit {
    doc_id: u64 = 0;
    hamming_dist: u32 = 0;
    approx_cosine_f32: f32 = 0.0;
    exact_score_f32: f32 = 0.0;
}

/// Memory footprint & compression telemetry snapshot returned by `BinaryVectorIndex.stats()`.
pub struct BinaryIndexStats {
    active_docs: usize = 0;
    dim: usize = 0;
    words_per_vec: usize = 0;
    bytes_per_vec: usize = 0;
    compression_ratio_x: f32 = 0.0;
}

// ============================================================================
// 2. Internal Numeric & Ordering Helpers
// ============================================================================

fn isFiniteF32(v: f32): bool {
    return (v == v) && ((v - v) == 0.0);
}

/// Ordering for Stage 1 coarse hits: `(hamming_dist asc, doc_id asc)`.
fn hammingHitBetter(a: BinaryIndexHit, b: BinaryIndexHit): bool {
    if (a.hamming_dist != b.hamming_dist) return a.hamming_dist < b.hamming_dist;
    return a.doc_id < b.doc_id;
}

/// Ordering for Stage 2 exact rescored hits: `(exact_score_f32 desc, doc_id asc)`.
fn exactHitBetter(a: BinaryIndexHit, b: BinaryIndexHit): bool {
    if (a.exact_score_f32 > b.exact_score_f32) return true;
    if (a.exact_score_f32 < b.exact_score_f32) return false;
    return a.doc_id < b.doc_id;
}

// ============================================================================
// 3. Zero-Alloc 1-Bit Binary Vector Index
// ============================================================================

/// Zero-heap-allocation 1-Bit Binary Quantized Vector Index supporting up to 256 documents
/// and 1024 dimensions with hardware `@popCount` coarse search and `@dot` SIMD exact rescoring.
pub struct BinaryVectorIndex {
    cfg: BinaryIndexConfig = BinaryIndexConfig{};
    words_per_vec: usize = 1;
    doc_ids: [BIN_IDX_MAX_DOCS]u64 = [0; BIN_IDX_MAX_DOCS];
    occupied: [BIN_IDX_MAX_DOCS]bool = [false; BIN_IDX_MAX_DOCS];
    bit_codes: [BIN_IDX_MAX_DOCS * BIN_IDX_MAX_WORDS]u64 = [0; BIN_IDX_MAX_DOCS * BIN_IDX_MAX_WORDS];
    active_docs: usize = 0;

    /// Initializes a `BinaryVectorIndex` after validating `cfg`. Returns `null` if invalid.
    static fn init(cfg: BinaryIndexConfig): BinaryVectorIndex? {
        if (!cfg.isValid()) return null;
        return BinaryVectorIndex{
            cfg: cfg,
            words_per_vec: cfg.wordsPerVec(),
            doc_ids: [0; BIN_IDX_MAX_DOCS],
            occupied: [false; BIN_IDX_MAX_DOCS],
            bit_codes: [0; BIN_IDX_MAX_DOCS * BIN_IDX_MAX_WORDS],
            active_docs: 0,
        };
    }

    /// Quantizes `vec` into little-endian 64-bit words where bit `i` is `1` iff `vec[i] >= cfg.threshold_f32`.
    /// Validates exact `vec.len == cfg.dim`, `out_words.len >= words_per_vec`, and finiteness of all elements.
    fn quantize_vec(vec: []const f32, out_words: *[]u64): bool {
        if (vec.len != _.cfg.dim || out_words.len < _.words_per_vec) return false;

        // Pre-flight finiteness check so out_words is never partially mutated on NaN/Inf
        let chk: usize = 0;
        while (chk < _.cfg.dim) : (chk += 1) {
            if (!isFiniteF32(vec[chk])) return false;
        }

        let w: usize = 0;
        while (w < _.words_per_vec) : (w += 1) {
            out_words[w] = 0;
        }

        let i: usize = 0;
        while (i < _.cfg.dim) : (i += 1) {
            if (vec[i] >= _.cfg.threshold_f32) {
                const word_idx: usize = i / 64;
                const bit_idx: u6 = @intCast(i % 64);
                out_words[word_idx] |= ((1 as u64) << bit_idx);
            }
        }
        return true;
    }

    /// Quantizes and inserts `vec` under `doc_id`.
    /// Rejects duplicate active `doc_id`, dimension mismatch, non-finite values, or full 256-slot pool.
    fn add_document(*_, doc_id: u64, vec: []const f32): bool {
        let q_words: [BIN_IDX_MAX_WORDS]u64 = [0; BIN_IDX_MAX_WORDS];
        let q_s: []u64 = q_words[0..];
        if (!_.quantize_vec(vec, q_s)) return false;

        let free_idx: usize = BIN_IDX_MAX_DOCS;
        let i: usize = 0;
        while (i < BIN_IDX_MAX_DOCS) : (i += 1) {
            if (_.occupied[i]) {
                if (_.doc_ids[i] == doc_id) return false;
            } else if (free_idx == BIN_IDX_MAX_DOCS) {
                free_idx = i;
            }
        }
        if (free_idx == BIN_IDX_MAX_DOCS) return false;

        _.doc_ids[free_idx] = doc_id;
        _.occupied[free_idx] = true;
        const base: usize = free_idx * BIN_IDX_MAX_WORDS;
        let w: usize = 0;
        while (w < _.words_per_vec) : (w += 1) {
            _.bit_codes[base + w] = q_words[w];
        }
        _.active_docs += 1;
        return true;
    }

    /// Removes `doc_id` from the index, freeing its inline slot for immediate reuse.
    fn remove_document(*_, doc_id: u64): bool {
        let i: usize = 0;
        while (i < BIN_IDX_MAX_DOCS) : (i += 1) {
            if (_.occupied[i] && _.doc_ids[i] == doc_id) {
                _.occupied[i] = false;
                _.doc_ids[i] = 0;
                _.active_docs -= 1;
                return true;
            }
        }
        return false;
    }

    /// Stage 1 coarse Top-K search using hardware `@popCount(q_word ^ d_word)` Hamming distance.
    /// Computes `approx_cosine_f32 = 1.0 - 2.0 * hamming / dim` and orders hits by
    /// `(hamming_dist asc, doc_id asc)`.
    fn search_hamming_topk(
        query: []const f32,
        k: usize,
        out_hits: *[]BinaryIndexHit,
    ): usize {
        if (k == 0 || out_hits.len == 0 || _.active_docs == 0) return 0;

        let q_words: [BIN_IDX_MAX_WORDS]u64 = [0; BIN_IDX_MAX_WORDS];
        let q_s: []u64 = q_words[0..];
        if (!_.quantize_vec(query, q_s)) return 0;

        const eff_k: usize = if (k < out_hits.len) k else out_hits.len;
        const dim_f32: f32 = _.cfg.dim as f32;
        let found: usize = 0;

        let slot: usize = 0;
        while (slot < BIN_IDX_MAX_DOCS) : (slot += 1) {
            if (!_.occupied[slot]) continue;

            const base: usize = slot * BIN_IDX_MAX_WORDS;
            let dist: u32 = 0;
            let w: usize = 0;
            while (w < _.words_per_vec) : (w += 1) {
                const diff: u64 = q_words[w] ^ _.bit_codes[base + w];
                const bits: u32 = @popCount(diff);
                dist += bits;
            }

            const approx_cos: f32 = 1.0 - (2.0 * (dist as f32)) / dim_f32;
            const cand = BinaryIndexHit{
                doc_id: _.doc_ids[slot],
                hamming_dist: dist,
                approx_cosine_f32: approx_cos,
                exact_score_f32: approx_cos,
            };

            if (found < eff_k) {
                let ins: usize = found;
                while (ins > 0 && hammingHitBetter(cand, out_hits[ins - 1])) : (ins -= 1) {
                    out_hits[ins] = out_hits[ins - 1];
                }
                out_hits[ins] = cand;
                found += 1;
            } else if (hammingHitBetter(cand, out_hits[eff_k - 1])) {
                let ins: usize = eff_k - 1;
                while (ins > 0 && hammingHitBetter(cand, out_hits[ins - 1])) : (ins -= 1) {
                    out_hits[ins] = out_hits[ins - 1];
                }
                out_hits[ins] = cand;
            }
        }

        return found;
    }

    /// Two-stage retrieval:
    ///   - Stage 1: hardware `@popCount` Hamming coarse search collects top `oversample_k` candidates.
    ///   - Stage 2: CP native `@dot` SIMD exact Float32 inner-product rescoring over caller's
    ///     `raw_doc_pool` (indexed by document slot: `slot_idx * dim .. (slot_idx + 1) * dim`),
    ///     returning exact Top-K ordered by `(exact_score_f32 desc, doc_id asc)`.
    fn search_two_stage_rescore(
        query: []const f32,
        raw_doc_pool: []const f32,
        k: usize,
        oversample_k: usize,
        out_hits: *[]BinaryIndexHit,
    ): usize {
        if (k == 0 || out_hits.len == 0 || _.active_docs == 0) return 0;

        let q_words: [BIN_IDX_MAX_WORDS]u64 = [0; BIN_IDX_MAX_WORDS];
        let q_s: []u64 = q_words[0..];
        if (!_.quantize_vec(query, q_s)) return 0;

        const eff_k: usize = if (k < out_hits.len) k else out_hits.len;
        let stage1_k: usize = if (oversample_k > eff_k) oversample_k else eff_k;
        if (stage1_k > BIN_IDX_MAX_DOCS) stage1_k = BIN_IDX_MAX_DOCS;

        let cand_hits: [BIN_IDX_MAX_DOCS]BinaryIndexHit = [BinaryIndexHit{}; BIN_IDX_MAX_DOCS];
        let cand_slots: [BIN_IDX_MAX_DOCS]usize = [0; BIN_IDX_MAX_DOCS];
        let cand_count: usize = 0;

        const dim_f32: f32 = _.cfg.dim as f32;

        // Stage 1: coarse Hamming candidate collection
        let slot: usize = 0;
        while (slot < BIN_IDX_MAX_DOCS) : (slot += 1) {
            if (!_.occupied[slot]) continue;

            const base: usize = slot * BIN_IDX_MAX_WORDS;
            let dist: u32 = 0;
            let w: usize = 0;
            while (w < _.words_per_vec) : (w += 1) {
                const diff: u64 = q_words[w] ^ _.bit_codes[base + w];
                const bits: u32 = @popCount(diff);
                dist += bits;
            }

            const approx_cos: f32 = 1.0 - (2.0 * (dist as f32)) / dim_f32;
            const cand = BinaryIndexHit{
                doc_id: _.doc_ids[slot],
                hamming_dist: dist,
                approx_cosine_f32: approx_cos,
                exact_score_f32: 0.0,
            };

            if (cand_count < stage1_k) {
                let ins: usize = cand_count;
                while (ins > 0 && hammingHitBetter(cand, cand_hits[ins - 1])) : (ins -= 1) {
                    cand_hits[ins] = cand_hits[ins - 1];
                    cand_slots[ins] = cand_slots[ins - 1];
                }
                cand_hits[ins] = cand;
                cand_slots[ins] = slot;
                cand_count += 1;
            } else if (hammingHitBetter(cand, cand_hits[stage1_k - 1])) {
                let ins: usize = stage1_k - 1;
                while (ins > 0 && hammingHitBetter(cand, cand_hits[ins - 1])) : (ins -= 1) {
                    cand_hits[ins] = cand_hits[ins - 1];
                    cand_slots[ins] = cand_slots[ins - 1];
                }
                cand_hits[ins] = cand;
                cand_slots[ins] = slot;
            }
        }

        // Stage 2: @dot SIMD exact Float32 rescoring
        let final_count: usize = 0;
        let c: usize = 0;
        while (c < cand_count) : (c += 1) {
            const raw_off: usize = cand_slots[c] * _.cfg.dim;
            if (raw_off + _.cfg.dim > raw_doc_pool.len) continue;

            const doc_vec: []const f32 = raw_doc_pool[raw_off .. (raw_off + _.cfg.dim)];
            const exact_score: f32 = @dot(query, doc_vec);
            if (!isFiniteF32(exact_score)) continue;

            let hit = cand_hits[c];
            hit.exact_score_f32 = exact_score;

            if (final_count < eff_k) {
                let ins: usize = final_count;
                while (ins > 0 && exactHitBetter(hit, out_hits[ins - 1])) : (ins -= 1) {
                    out_hits[ins] = out_hits[ins - 1];
                }
                out_hits[ins] = hit;
                final_count += 1;
            } else if (exactHitBetter(hit, out_hits[eff_k - 1])) {
                let ins: usize = eff_k - 1;
                while (ins > 0 && exactHitBetter(hit, out_hits[ins - 1])) : (ins -= 1) {
                    out_hits[ins] = out_hits[ins - 1];
                }
                out_hits[ins] = hit;
                final_count = eff_k;
            }
        }

        return final_count;
    }

    /// Returns active document count, geometry, per-vector byte size & compression ratio vs f32.
    fn stats(): BinaryIndexStats {
        const bytes_per_vec: usize = _.words_per_vec * 8;
        const raw_bytes: usize = _.cfg.dim * 4;
        const ratio: f32 = if (bytes_per_vec > 0) ((raw_bytes as f32) / (bytes_per_vec as f32)) else 0.0;
        return BinaryIndexStats{
            active_docs: _.active_docs,
            dim: _.cfg.dim,
            words_per_vec: _.words_per_vec,
            bytes_per_vec: bytes_per_vec,
            compression_ratio_x: ratio,
        };
    }
}
