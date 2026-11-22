// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc ColBERT Multi-Vector Late-Interaction MaxSim Retrieval Index) | Created: 2026-11-22
// Pure CP zero-heap-allocation ColBERTv2 / ColPali token-level late-interaction (MaxSim) retrieval kernel:
//   - Caller-owned flat `f32` token pool with zero dynamic heap allocations (`ColbertLiteIndex`)
//   - Optional L2 token normalization on insert with duplicate-ID, capacity, and dimension guards
//   - 8-lane SIMD `@dot`-accelerated MaxSim scoring (`sum_i max_j dot(q_i, d_j)`)
//   - Deterministic exact Top-K retrieval with `doc_id` ascending tie-breaking (`search_topk`)
//   - 8-lane SIMD `@l2sq`-powered low-norm stopword/noise token pruning (`prune_low_norm_tokens`)

const std = @import("std");

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

/// Maximum number of document slots supported by `ColbertLiteIndex` (zero-heap fixed table).
pub const COLBERT_MAX_DOCS: usize = 128;

const NORM_SQ_EPS: f32 = 0.000000000001;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn allFiniteF32(slice: []const f32): bool {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (!isFiniteF32(slice[i])) {
            return false;
        }
    }
    return true;
}

/// Computes squared L2 norm `sum(v[i]^2)` of an arbitrary-dimension `f32` vector
/// using CP's native 8-lane SIMD `@l2sq` intrinsic against a stack zero block.
fn l2NormSqSimd(v: []const f32): f32 {
    const zeros: [64]f32 = [0.0; 64];
    let sum: f32 = 0.0;
    let offset: usize = 0;
    while (offset < v.len) {
        const rem = v.len - offset;
        const step = if (rem < 64) rem else 64;
        const seg = v[offset .. offset + step];
        const zseg = zeros[0 .. step];
        const part: f32 = @l2sq(seg, zseg);
        sum += part;
        offset += step;
    }
    return sum;
}

fn isBetterColbertHit(cand: ColbertSearchHit, existing: ColbertSearchHit): bool {
    if (cand.maxsim_score_f32 > existing.maxsim_score_f32) {
        return true;
    }
    if (cand.maxsim_score_f32 < existing.maxsim_score_f32) {
        return false;
    }
    return cand.doc_id < existing.doc_id;
}

// ============================================================================
// 1. Public Models: Config, Document Span, Search Hit & Index Stats
// ============================================================================

/// Configuration for `ColbertLiteIndex`.
pub struct ColbertConfig {
    dim: usize = 0;
    l2_normalize_on_insert: bool = true;

    /// Constructs and validates a `ColbertConfig`. Returns `null` when `dim == 0`.
    static fn init(dim: usize, l2_normalize_on_insert: bool = true): ColbertConfig? {
        const cfg = ColbertConfig{
            dim: dim,
            l2_normalize_on_insert: l2_normalize_on_insert,
        };
        if (!cfg.isValid()) {
            return null;
        }
        return cfg;
    }

    /// Validates configuration invariants (`dim > 0`).
    fn isValid(): bool {
        return _.dim > 0;
    }
}

/// Contiguous token span metadata for a single indexed document inside the flat token pool.
pub struct ColbertDocSpan {
    doc_id: u64 = 0;
    token_offset: usize = 0;
    token_count: usize = 0;
    active: bool = false;
}

/// Single Top-K MaxSim retrieval hit returned by `ColbertLiteIndex.search_topk`.
pub struct ColbertSearchHit {
    doc_id: u64 = 0;
    maxsim_score_f32: f32 = 0.0;
    avg_maxsim_f32: f32 = 0.0;
    matched_tokens: usize = 0;
}

/// Operational capacity and usage snapshot for `ColbertLiteIndex`.
pub struct ColbertIndexStats {
    active_docs: usize = 0;
    total_stored_tokens: usize = 0;
    token_capacity: usize = 0;
    dim: usize = 0;
}

// ============================================================================
// 2. Standalone SIMD Token Pruning Kernel
// ============================================================================

fn pruneLowNormTokensImpl(
    tokens: []const f32,
    num_tokens: usize,
    dim: usize,
    min_l2_norm: f32,
    out_tokens: *[]f32,
): usize {
    if (num_tokens == 0 || dim == 0) {
        return 0;
    }
    if (!isFiniteF32(min_l2_norm) || min_l2_norm < 0.0) {
        return 0;
    }
    if (num_tokens > 0xFFFFFFFFFFFFFFFF / dim) {
        return 0;
    }
    const needed_floats = num_tokens * dim;
    if (tokens.len < needed_floats) {
        return 0;
    }

    const max_out_tokens = out_tokens.len / dim;
    if (max_out_tokens == 0) {
        return 0;
    }

    const min_sq: f32 = min_l2_norm * min_l2_norm;
    let kept: usize = 0;
    let t: usize = 0;
    while (t < num_tokens) : (t += 1) {
        const src_start = t * dim;
        const tok = tokens[src_start .. src_start + dim];
        if (!allFiniteF32(tok)) {
            continue;
        }
        const norm_sq: f32 = l2NormSqSimd(tok);
        if (!isFiniteF32(norm_sq)) {
            continue;
        }
        if (norm_sq >= min_sq) {
            if (kept >= max_out_tokens) {
                break;
            }
            const dst_start = kept * dim;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                out_tokens[dst_start + d] = tok[d];
            }
            kept += 1;
        }
    }
    return kept;
}

/// Prunes low-norm noise / stopword token embeddings whose L2 norm is below `min_l2_norm`
/// using CP's native 8-lane SIMD `@l2sq` intrinsic, writing retained token vectors contiguously
/// into `out_tokens`.
///
/// - Skips tokens containing `NaN` or `Inf`.
/// - Returns the number of retained tokens written into `out_tokens`.
/// - Returns `0` on invalid dimensions, negative/non-finite threshold, or insufficient input length.
pub fn prune_low_norm_tokens(
    tokens: []const f32,
    num_tokens: usize,
    dim: usize,
    min_l2_norm: f32,
    out_tokens: *[]f32,
): usize {
    return pruneLowNormTokensImpl(tokens, num_tokens, dim, min_l2_norm, out_tokens);
}

// ============================================================================
// 3. Zero-Alloc ColBERT Late-Interaction MaxSim Index
// ============================================================================

/// Zero-heap-allocation ColBERT multi-vector late-interaction index backed by a caller-owned
/// flat `f32` token pool and fixed-size document span table (`COLBERT_MAX_DOCS`).
pub struct ColbertLiteIndex {
    cfg: ColbertConfig = ColbertConfig{};
    token_pool: []f32;
    docs: [COLBERT_MAX_DOCS]ColbertDocSpan = [ColbertDocSpan{}; COLBERT_MAX_DOCS];
    doc_count: usize = 0;
    active_docs: usize = 0;
    used_tokens: usize = 0;
    token_capacity: usize = 0;

    /// Initializes a zero-alloc ColBERT index over caller-owned `token_pool`.
    /// Requires `cfg.isValid()` (`dim > 0`) and `token_pool.len >= cfg.dim`.
    static fn init(cfg: ColbertConfig, token_pool: *[]f32): ColbertLiteIndex? {
        if (!cfg.isValid()) {
            return null;
        }
        const cap_tokens = token_pool.len / cfg.dim;
        if (cap_tokens == 0) {
            return null;
        }
        return ColbertLiteIndex{
            cfg: cfg,
            token_pool: token_pool,
            docs: [ColbertDocSpan{}; COLBERT_MAX_DOCS],
            doc_count: 0,
            active_docs: 0,
            used_tokens: 0,
            token_capacity: cap_tokens,
        };
    }

    /// Convenience static forwarder to `prune_low_norm_tokens`.
    static fn prune_low_norm_tokens(
        tokens: []const f32,
        num_tokens: usize,
        dim: usize,
        min_l2_norm: f32,
        out_tokens: *[]f32,
    ): usize {
        return pruneLowNormTokensImpl(tokens, num_tokens, dim, min_l2_norm, out_tokens);
    }

    /// Inserts a multi-token document into the flat token pool.
    ///
    /// Guards:
    /// - Rejects `num_tokens == 0`, dimension mismatch (`doc_tokens.len != num_tokens * dim`),
    ///   non-finite elements (`NaN`/`Inf`), duplicate active `doc_id`, full document table,
    ///   or insufficient remaining token pool capacity.
    /// - When `cfg.l2_normalize_on_insert` is enabled, each token vector is L2-normalized via
    ///   8-lane SIMD `@dot` + `@sqrt` before writing into `token_pool` (zero-norm tokens are
    ///   stored as zeros without producing `NaN`).
    fn add_document(
        *_,
        doc_id: u64,
        doc_tokens: []const f32,
        num_tokens: usize,
    ): bool {
        if (num_tokens == 0 || _.cfg.dim == 0) {
            return false;
        }
        if (_.doc_count >= COLBERT_MAX_DOCS) {
            return false;
        }
        if (num_tokens > 0xFFFFFFFFFFFFFFFF / _.cfg.dim) {
            return false;
        }
        const needed_floats = num_tokens * _.cfg.dim;
        if (doc_tokens.len != needed_floats) {
            return false;
        }
        if (num_tokens > _.token_capacity - _.used_tokens) {
            return false;
        }

        // Duplicate active doc_id guard
        let i: usize = 0;
        while (i < _.doc_count) : (i += 1) {
            if (_.docs[i].active && _.docs[i].doc_id == doc_id) {
                return false;
            }
        }

        // Validate all floats before mutating the token pool
        if (!allFiniteF32(doc_tokens)) {
            return false;
        }

        const dim = _.cfg.dim;
        const pool_base_token = _.used_tokens;
        let t: usize = 0;
        while (t < num_tokens) : (t += 1) {
            const src_start = t * dim;
            const dst_start = (pool_base_token + t) * dim;
            const tok = doc_tokens[src_start .. src_start + dim];

            if (_.cfg.l2_normalize_on_insert) {
                const norm_sq: f32 = @dot(tok, tok);
                if (!isFiniteF32(norm_sq)) {
                    return false;
                }
                if (norm_sq > NORM_SQ_EPS) {
                    const inv_norm: f32 = 1.0 / @sqrt(norm_sq);
                    let d: usize = 0;
                    while (d < dim) : (d += 1) {
                        _.token_pool[dst_start + d] = tok[d] * inv_norm;
                    }
                } else {
                    let d: usize = 0;
                    while (d < dim) : (d += 1) {
                        _.token_pool[dst_start + d] = 0.0;
                    }
                }
            } else {
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    _.token_pool[dst_start + d] = tok[d];
                }
            }
        }

        _.docs[_.doc_count] = ColbertDocSpan{
            doc_id: doc_id,
            token_offset: pool_base_token,
            token_count: num_tokens,
            active: true,
        };
        _.doc_count += 1;
        _.active_docs += 1;
        _.used_tokens += num_tokens;
        return true;
    }

    /// Logically removes the active document matching `doc_id` so it no longer participates
    /// in MaxSim scoring or Top-K search. Returns `false` if not found.
    fn remove_document(*_, doc_id: u64): bool {
        let i: usize = 0;
        while (i < _.doc_count) : (i += 1) {
            if (_.docs[i].active && _.docs[i].doc_id == doc_id) {
                _.docs[i].active = false;
                _.active_docs -= 1;
                return true;
            }
        }
        return false;
    }

    /// Internal helper: computes exact MaxSim score and count of positively matched query tokens
    /// (`best_sim > 0.0`) for an active document slot.
    fn scoreDocumentInternal(
        query_tokens: []const f32,
        q_tokens: usize,
        doc_idx: usize,
        out_matched: *usize,
    ): f32 {
        out_matched.* = 0;
        if (doc_idx >= _.doc_count || !_.docs[doc_idx].active) {
            return 0.0;
        }
        const dim = _.cfg.dim;
        if (q_tokens == 0 || dim == 0) {
            return 0.0;
        }
        if (q_tokens > 0xFFFFFFFFFFFFFFFF / dim) {
            return 0.0;
        }
        if (query_tokens.len != q_tokens * dim) {
            return 0.0;
        }
        const span = _.docs[doc_idx];
        if (span.token_count == 0) {
            return 0.0;
        }

        let total_score: f32 = 0.0;
        let matched: usize = 0;
        let qi: usize = 0;
        while (qi < q_tokens) : (qi += 1) {
            const q_start = qi * dim;
            const q_vec = query_tokens[q_start .. q_start + dim];
            let best_sim: f32 = 0.0;

            let dj: usize = 0;
            while (dj < span.token_count) : (dj += 1) {
                const d_start = (span.token_offset + dj) * dim;
                const d_vec = _.token_pool[d_start .. d_start + dim];
                const sim: f32 = @dot(q_vec, d_vec);
                if (dj == 0 || sim > best_sim) {
                    best_sim = sim;
                }
            }

            if (best_sim > 0.0) {
                matched += 1;
            }
            total_score += best_sim;
        }

        out_matched.* = matched;
        return total_score;
    }

    /// Computes exact ColBERT late-interaction MaxSim score `sum_i max_j dot(q_i, d_j)`
    /// for document slot `doc_idx` using CP's native 8-lane SIMD `@dot`.
    /// Returns `0.0` for inactive/out-of-range slots, empty queries, dimension mismatch,
    /// or non-finite query inputs.
    fn score_document_maxsim(
        query_tokens: []const f32,
        q_tokens: usize,
        doc_idx: usize,
    ): f32 {
        if (q_tokens == 0 || _.cfg.dim == 0) {
            return 0.0;
        }
        if (q_tokens > 0xFFFFFFFFFFFFFFFF / _.cfg.dim) {
            return 0.0;
        }
        if (query_tokens.len != q_tokens * _.cfg.dim) {
            return 0.0;
        }
        if (!allFiniteF32(query_tokens)) {
            return 0.0;
        }
        let matched: usize = 0;
        return _.scoreDocumentInternal(query_tokens, q_tokens, doc_idx, &matched);
    }

    /// Runs exact MaxSim Top-K search across all active documents, writing results into
    /// `out_hits` sorted by `maxsim_score_f32` descending with deterministic `doc_id`
    /// ascending tie-breaking. Returns the number of hits written (`<= min(k, out_hits.len)`).
    fn search_topk(
        query_tokens: []const f32,
        q_tokens: usize,
        k: usize,
        out_hits: *[]ColbertSearchHit,
    ): usize {
        if (k == 0 || out_hits.len == 0 || q_tokens == 0 || _.active_docs == 0 || _.cfg.dim == 0) {
            return 0;
        }
        if (q_tokens > 0xFFFFFFFFFFFFFFFF / _.cfg.dim) {
            return 0;
        }
        if (query_tokens.len != q_tokens * _.cfg.dim) {
            return 0;
        }
        if (!allFiniteF32(query_tokens)) {
            return 0;
        }

        const limit = if (k < out_hits.len) k else out_hits.len;
        const inv_q: f32 = 1.0 / (q_tokens as f32);
        let count: usize = 0;

        let doc_idx: usize = 0;
        while (doc_idx < _.doc_count) : (doc_idx += 1) {
            if (!_.docs[doc_idx].active) {
                continue;
            }
            let matched: usize = 0;
            const score = _.scoreDocumentInternal(query_tokens, q_tokens, doc_idx, &matched);
            if (!isFiniteF32(score)) {
                continue;
            }

            const hit = ColbertSearchHit{
                doc_id: _.docs[doc_idx].doc_id,
                maxsim_score_f32: score,
                avg_maxsim_f32: score * inv_q,
                matched_tokens: matched,
            };

            if (count < limit) {
                out_hits[count] = hit;
                let pos = count;
                count += 1;
                while (pos > 0 && isBetterColbertHit(out_hits[pos], out_hits[pos - 1])) {
                    const tmp = out_hits[pos - 1];
                    out_hits[pos - 1] = out_hits[pos];
                    out_hits[pos] = tmp;
                    pos -= 1;
                }
            } else if (isBetterColbertHit(hit, out_hits[count - 1])) {
                out_hits[count - 1] = hit;
                let pos = count - 1;
                while (pos > 0 && isBetterColbertHit(out_hits[pos], out_hits[pos - 1])) {
                    const tmp = out_hits[pos - 1];
                    out_hits[pos - 1] = out_hits[pos];
                    out_hits[pos] = tmp;
                    pos -= 1;
                }
            }
        }

        return count;
    }

    /// Returns a snapshot of current document count, token pool utilization, and embedding dimension.
    fn stats(): ColbertIndexStats {
        return ColbertIndexStats{
            active_docs: _.active_docs,
            total_stored_tokens: _.used_tokens,
            token_capacity: _.token_capacity,
            dim: _.cfg.dim,
        };
    }
}
