// Origin: Pure-CP Stdlib (AI Hybrid RAG Pipeline: Weighted RRF Fusion, SIMD @dot/@l2sq MMR Rerank & Token Budget Pruning) | Created: 2026-11-22
// Pure CP zero-heap-allocation hybrid retrieval & RAG context construction pipeline:
//   1. `HybridRrfFusion`: configurable dense/sparse & M-way weighted Reciprocal Rank Fusion
//   2. `mmrRerankSimdF32`: 8-lane SIMD `@dot`/`@fma` Maximal Marginal Relevance diversity reranking
//   3. `dedupNearDuplicatesL2F32`: 8-lane SIMD `@l2sq` near-duplicate chunk pruning
//   4. `selectWithinTokenBudget`: score-filtered greedy/prefix token-budget context window packing

const std = @import("std");

// ============================================================================
// Internal Numeric, SIMD & Sorting Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clampCosineF32(x: f32): f32 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

/// Computes 8-lane SIMD cosine similarity between two equal-length `f32` slices via `@dot`.
/// Zero-norm vectors safely return `0.0`.
fn simdCosinePairF32(a: []const f32, b: []const f32): f32 {
    const na: f32 = @dot(a, a);
    const nb: f32 = @dot(b, b);
    if (na <= 0.0 || nb <= 0.0) return 0.0;
    const ab: f32 = @dot(a, b);
    const denom: f32 = @sqrt(na) * @sqrt(nb);
    if (denom <= 0.0 || !isFiniteF32(denom)) return 0.0;
    return clampCosineF32(ab / denom);
}

/// Validates that all `n_docs * dim` elements of a row-major flat embedding slice are finite
/// using CP's native `slice.split(dim)` zero-overhead row traversal.
fn validateFlatEmbeddingsF32(embeddings_flat: []const f32, n_docs: usize, dim: usize): bool {
    let rem: []const f32 = embeddings_flat[0 .. n_docs * dim];
    let row_idx: usize = 0;
    while (row_idx < n_docs) : (row_idx += 1) {
        const [row, next_rem] = rem.split(dim);
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            if (!isFiniteF32(row[d])) return false;
        }
        rem = next_rem;
    }
    return true;
}

// ============================================================================
// 1. Weighted Dual-Channel & M-Way RRF Fusion (`HybridRrfFusion`)
// ============================================================================

/// Fused hybrid retrieval hit with compact numeric document ID, fused RRF score,
/// and 1-based channel ranks (`0` indicates absence from that channel).
pub struct HybridRagHit {
    doc_id: u32 = 0;
    score: f32 = 0.0;
    dense_rank: u32 = 0;
    sparse_rank: u32 = 0;
}

fn hybridHitBetterDesc(a: HybridRagHit, b: HybridRagHit): bool {
    if (a.score != b.score) return a.score > b.score;
    return a.doc_id < b.doc_id;
}

fn siftDownWorstHit(hits: *[]HybridRagHit, start: usize, len: usize) {
    let idx: usize = start;
    while (true) {
        const left: usize = idx * 2 + 1;
        if (left >= len) break;
        const right: usize = left + 1;
        let worst: usize = left;
        if (right < len && hybridHitBetterDesc(hits[worst], hits[right])) {
            worst = right;
        }
        if (!hybridHitBetterDesc(hits[idx], hits[worst])) break;
        const tmp = hits[idx];
        hits[idx] = hits[worst];
        hits[worst] = tmp;
        idx = worst;
    }
}

fn sortHybridHitsDesc(hits: *[]HybridRagHit, count: usize) {
    if (count <= 1) return;
    let i: usize = count / 2;
    while (i > 0) {
        i -= 1;
        siftDownWorstHit(hits, i, count);
    }
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = hits[0];
        hits[0] = hits[end];
        hits[end] = tmp;
        siftDownWorstHit(hits, 0, end);
    }
}

/// Configurable zero-heap-allocation Dense + Sparse / M-way Reciprocal Rank Fusion (RRF) engine.
///
/// For dual-channel retrieval:
/// `score(doc) = dense_weight / (rrf_k + dense_rank) + sparse_weight / (rrf_k + sparse_rank)`
/// where ranks are 1-based (`1, 2, ...`) and intra-list duplicates retain their earliest/highest rank.
pub struct HybridRrfFusion {
    dense_weight: f32 = 1.0;
    sparse_weight: f32 = 1.0;
    rrf_k: f32 = 60.0;

    /// Constructs a validated `HybridRrfFusion` configuration.
    /// Rejects negative or non-finite weights, negative or non-finite `rrf_k`,
    /// or configurations where both `dense_weight` and `sparse_weight` are `0.0`.
    static fn init(dense_weight: f32, sparse_weight: f32, rrf_k: f32): HybridRrfFusion? {
        if (!isFiniteF32(dense_weight) || dense_weight < 0.0) return null;
        if (!isFiniteF32(sparse_weight) || sparse_weight < 0.0) return null;
        if (!isFiniteF32(rrf_k) || rrf_k < 0.0) return null;
        if (dense_weight == 0.0 && sparse_weight == 0.0) return null;

        return HybridRrfFusion{
            dense_weight: dense_weight,
            sparse_weight: sparse_weight,
            rrf_k: rrf_k,
        };
    }

    /// Zero-heap-allocation dual-channel (Dense + Sparse) weighted RRF fusion.
    ///
    /// - Deduplicates `doc_id` within each channel (keeping earliest/best 1-based rank).
    /// - Combines scores across channels and records 1-based `dense_rank` / `sparse_rank`.
    /// - Writes up to `min(top_k, unique_count)` hits to `out_hits` sorted descending by `score`
    ///   (tie-broken deterministically by ascending `doc_id`).
    /// - `work_hits` is caller-provided scratch space sized for all unique candidates.
    /// - Returns `0` when `top_k == 0` or both input lists are empty; `null` on invalid config or insufficient capacity.
    fn fuseDual(
        dense_ids: []const u32,
        sparse_ids: []const u32,
        top_k: usize,
        out_hits: *[]HybridRagHit,
        work_hits: *[]HybridRagHit,
    ): usize? {
        if (!isFiniteF32(_.dense_weight) || _.dense_weight < 0.0) return null;
        if (!isFiniteF32(_.sparse_weight) || _.sparse_weight < 0.0) return null;
        if (!isFiniteF32(_.rrf_k) || _.rrf_k < 0.0) return null;

        if (top_k == 0 || (dense_ids.len == 0 && sparse_ids.len == 0)) {
            return 0;
        }

        let unique_count: usize = 0;

        // 1. Accumulate Dense channel (deduplicating within dense_ids).
        let di: usize = 0;
        while (di < dense_ids.len) : (di += 1) {
            const doc_id: u32 = dense_ids[di];
            let dup = false;
            let prev: usize = 0;
            while (prev < di) : (prev += 1) {
                if (dense_ids[prev] == doc_id) {
                    dup = true;
                    break;
                }
            }
            if (dup) continue;

            if (unique_count >= work_hits.len) return null;
            const rank_u32: u32 = (di + 1) as u32;
            const contrib: f32 = _.dense_weight / (_.rrf_k + (rank_u32 as f32));
            work_hits[unique_count] = HybridRagHit{
                doc_id: doc_id,
                score: contrib,
                dense_rank: rank_u32,
                sparse_rank: 0,
            };
            unique_count += 1;
        }

        // 2. Accumulate Sparse channel (deduplicating within sparse_ids and merging into dense hits).
        let si: usize = 0;
        while (si < sparse_ids.len) : (si += 1) {
            const doc_id: u32 = sparse_ids[si];
            let dup = false;
            let prev: usize = 0;
            while (prev < si) : (prev += 1) {
                if (sparse_ids[prev] == doc_id) {
                    dup = true;
                    break;
                }
            }
            if (dup) continue;

            const rank_u32: u32 = (si + 1) as u32;
            const contrib: f32 = _.sparse_weight / (_.rrf_k + (rank_u32 as f32));

            let found = false;
            let k: usize = 0;
            while (k < unique_count) : (k += 1) {
                if (work_hits[k].doc_id == doc_id) {
                    work_hits[k].score += contrib;
                    work_hits[k].sparse_rank = rank_u32;
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (unique_count >= work_hits.len) return null;
                work_hits[unique_count] = HybridRagHit{
                    doc_id: doc_id,
                    score: contrib,
                    dense_rank: 0,
                    sparse_rank: rank_u32,
                };
                unique_count += 1;
            }
        }

        sortHybridHitsDesc(work_hits, unique_count);

        const emit_count: usize = if (top_k < unique_count) top_k else unique_count;
        if (out_hits.len < emit_count) return null;

        let out_i: usize = 0;
        while (out_i < emit_count) : (out_i += 1) {
            out_hits[out_i] = work_hits[out_i];
        }
        return emit_count;
    }

    /// Zero-heap-allocation M-way weighted RRF fusion across arbitrary CSR-packed retrieval lists.
    ///
    /// - `lists_doc_ids`: concatenated ranked `doc_id`s for all `M` channels.
    /// - `list_offsets`: CSR offsets of length `M + 1` (non-decreasing, within `lists_doc_ids.len`).
    /// - `list_weights`: non-negative finite per-channel weights of length `M`.
    /// - Writes up to `min(top_k, unique_count)` hits sorted descending by fused score into `out_hits`.
    /// - Channel 0 rank is recorded in `dense_rank` and Channel 1 rank in `sparse_rank` when `M >= 1`/`2`.
    fn fuseMultiWeighted(
        lists_doc_ids: []const u32,
        list_offsets: []const usize,
        list_weights: []const f32,
        top_k: usize,
        out_hits: *[]HybridRagHit,
        work_hits: *[]HybridRagHit,
    ): usize? {
        if (!isFiniteF32(_.rrf_k) || _.rrf_k < 0.0) return null;
        if (list_offsets.len == 0) {
            if (list_weights.len != 0) return null;
            return 0;
        }
        if (list_offsets[0] > lists_doc_ids.len) return null;
        if (list_offsets.len == 1) {
            if (list_weights.len != 0) return null;
            return 0;
        }

        const num_lists: usize = list_offsets.len - 1;
        if (list_weights.len != num_lists) return null;

        let chk: usize = 0;
        while (chk < num_lists) : (chk += 1) {
            const s: usize = list_offsets[chk];
            const e: usize = list_offsets[chk + 1];
            if (e < s || e > lists_doc_ids.len) return null;
            if (!isFiniteF32(list_weights[chk]) || list_weights[chk] < 0.0) return null;
        }

        if (top_k == 0) return 0;

        let unique_count: usize = 0;
        let m: usize = 0;
        while (m < num_lists) : (m += 1) {
            const start: usize = list_offsets[m];
            const end: usize = list_offsets[m + 1];
            const w: f32 = list_weights[m];

            let pos: usize = start;
            while (pos < end) : (pos += 1) {
                const doc_id: u32 = lists_doc_ids[pos];
                let dup = false;
                let prev: usize = start;
                while (prev < pos) : (prev += 1) {
                    if (lists_doc_ids[prev] == doc_id) {
                        dup = true;
                        break;
                    }
                }
                if (dup) continue;

                const rank_u32: u32 = (pos - start + 1) as u32;
                const contrib: f32 = w / (_.rrf_k + (rank_u32 as f32));

                let found = false;
                let idx: usize = 0;
                while (idx < unique_count) : (idx += 1) {
                    if (work_hits[idx].doc_id == doc_id) {
                        work_hits[idx].score += contrib;
                        if (m == 0 && work_hits[idx].dense_rank == 0) {
                            work_hits[idx].dense_rank = rank_u32;
                        } else if (m == 1 && work_hits[idx].sparse_rank == 0) {
                            work_hits[idx].sparse_rank = rank_u32;
                        }
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    if (unique_count >= work_hits.len) return null;
                    work_hits[unique_count] = HybridRagHit{
                        doc_id: doc_id,
                        score: contrib,
                        dense_rank: if (m == 0) rank_u32 else 0,
                        sparse_rank: if (m == 1) rank_u32 else 0,
                    };
                    unique_count += 1;
                }
            }
        }

        if (unique_count == 0) return 0;

        sortHybridHitsDesc(work_hits, unique_count);

        const emit_count: usize = if (top_k < unique_count) top_k else unique_count;
        if (out_hits.len < emit_count) return null;

        let out_i: usize = 0;
        while (out_i < emit_count) : (out_i += 1) {
            out_hits[out_i] = work_hits[out_i];
        }
        return emit_count;
    }
}

// ============================================================================
// 2. SIMD @dot/@fma MMR Diversity Reranking & @l2sq Near-Duplicate Pruning
// ============================================================================

/// Zero-heap-allocation 8-lane SIMD Maximal Marginal Relevance (MMR) diversity reranker.
///
/// Iteratively selects up to `min(top_k, n_docs)` candidate indices maximizing:
///   `lambda * relevance_scores[d] - (1.0 - lambda) * max_{s in S} simdCosine(emb[d], emb[s])`
/// using CP's native `@dot` and `@fma` SIMD intrinsics.
///
/// - `lambda == 1.0`: pure relevance ranking (descending `relevance_scores`).
/// - `lambda == 0.0`: pure diversity selection after the highest-relevance initial pick.
/// - Requires no external visited-mask buffer (`O(1)` stack space, zero heap allocation).
/// - Ties are deterministically broken by ascending candidate index.
/// - Returns `0` when `n_docs == 0` or `top_k == 0`; `null` on invalid dimensions,
///   `lambda` outside `[0.0, 1.0]`, undersized buffers, or `NaN`/`Inf` inputs.
pub fn mmrRerankSimdF32(
    relevance_scores: []const f32,
    embeddings_flat: []const f32,
    n_docs: usize,
    dim: usize,
    top_k: usize,
    lambda: f32,
    out_indices: *[]usize,
): usize? {
    if (!isFiniteF32(lambda) || lambda < 0.0 || lambda > 1.0) return null;
    if (n_docs == 0 || top_k == 0) return 0;
    if (dim == 0) return null;
    if (n_docs > std.math.maxInt(usize) / dim) return null;

    const total_elems: usize = n_docs * dim;
    const actual_k: usize = if (top_k < n_docs) top_k else n_docs;
    if (relevance_scores.len < n_docs || embeddings_flat.len < total_elems || out_indices.len < actual_k) {
        return null;
    }

    let iq: usize = 0;
    while (iq < n_docs) : (iq += 1) {
        if (!isFiniteF32(relevance_scores[iq])) return null;
    }
    if (!validateFlatEmbeddingsF32(embeddings_flat, n_docs, dim)) return null;

    const neg_one_m_lam: f32 = lambda - 1.0;
    const neg_inf: f32 = 0.0 - std.math.inf(f32);

    let selected: usize = 0;
    while (selected < actual_k) : (selected += 1) {
        let best_idx: usize = 0;
        let best_score: f32 = neg_inf;
        let found = false;

        let cand: usize = 0;
        while (cand < n_docs) : (cand += 1) {
            let already = false;
            let s_chk: usize = 0;
            while (s_chk < selected) : (s_chk += 1) {
                if (out_indices[s_chk] == cand) {
                    already = true;
                    break;
                }
            }
            if (already) continue;

            let mmr_val: f32 = relevance_scores[cand];
            if (selected > 0 && neg_one_m_lam < 0.0) {
                const cand_off: usize = cand * dim;
                const cand_vec: []const f32 = embeddings_flat[cand_off .. cand_off + dim];
                let max_red: f32 = neg_inf;
                let s: usize = 0;
                while (s < selected) : (s += 1) {
                    const sel_id: usize = out_indices[s];
                    const sel_off: usize = sel_id * dim;
                    const sel_vec: []const f32 = embeddings_flat[sel_off .. sel_off + dim];
                    const sim: f32 = simdCosinePairF32(cand_vec, sel_vec);
                    if (sim > max_red) max_red = sim;
                }
                mmr_val = @fma(lambda, relevance_scores[cand], neg_one_m_lam * max_red);
            }

            if (!found || mmr_val > best_score) {
                best_score = mmr_val;
                best_idx = cand;
                found = true;
            }
        }

        if (!found) return null;
        out_indices[selected] = best_idx;
    }

    return actual_k;
}

/// Zero-heap-allocation SIMD `@l2sq` near-duplicate chunk pruner.
///
/// Scans `n_docs` ranked candidate embeddings in order and retains candidate `i`
/// only if `@l2sq(emb[i], emb[j]) >= min_l2_sq` for every previously kept candidate `j`,
/// stopping once `max_keep` candidates have been retained.
///
/// Useful as a fast pre-filter or post-MMR deduplication pass for RAG chunks.
/// Returns `0` on empty input or `max_keep == 0`; `null` on invalid inputs or undersized `out_indices`.
pub fn dedupNearDuplicatesL2F32(
    embeddings_flat: []const f32,
    n_docs: usize,
    dim: usize,
    min_l2_sq: f32,
    max_keep: usize,
    out_indices: *[]usize,
): usize? {
    if (!isFiniteF32(min_l2_sq) || min_l2_sq < 0.0) return null;
    if (n_docs == 0 || max_keep == 0) return 0;
    if (dim == 0) return null;
    if (n_docs > std.math.maxInt(usize) / dim) return null;

    const total_elems: usize = n_docs * dim;
    const cap_keep: usize = if (max_keep < n_docs) max_keep else n_docs;
    if (embeddings_flat.len < total_elems || out_indices.len < cap_keep) return null;
    if (!validateFlatEmbeddingsF32(embeddings_flat, n_docs, dim)) return null;

    let kept: usize = 0;
    let cand: usize = 0;
    while (cand < n_docs && kept < cap_keep) : (cand += 1) {
        const cand_off: usize = cand * dim;
        const cand_vec: []const f32 = embeddings_flat[cand_off .. cand_off + dim];
        let is_dup = false;
        let k: usize = 0;
        while (k < kept) : (k += 1) {
            const kept_id: usize = out_indices[k];
            const kept_off: usize = kept_id * dim;
            const kept_vec: []const f32 = embeddings_flat[kept_off .. kept_off + dim];
            const d2: f32 = @l2sq(cand_vec, kept_vec);
            if (d2 < min_l2_sq) {
                is_dup = true;
                break;
            }
        }
        if (!is_dup) {
            out_indices[kept] = cand;
            kept += 1;
        }
    }

    return kept;
}

// ============================================================================
// 3. Context Token Budget Pruning (`selectWithinTokenBudget`)
// ============================================================================

/// Candidate RAG context chunk ready for LLM prompt budget packing.
pub struct RagContextChunk {
    doc_id: u32 = 0;
    score: f32 = 0.0;
    token_count: u32 = 0;
}

/// Summary statistics returned by `selectWithinTokenBudget`.
pub struct TokenBudgetResult {
    selected_count: usize = 0;
    used_tokens: usize = 0;
    skipped_oversized: usize = 0;
    filtered_low_score: usize = 0;
    budget_limit: usize = 0;
}

/// Selects priority-ordered RAG context chunks within a hard `max_tokens` LLM prompt budget.
///
/// - Filters out any candidate with `score < min_score` (tracked in `filtered_low_score`).
/// - When `greedy_skip_oversized == true`, skips chunks exceeding the remaining budget and
///   continues scanning subsequent smaller chunks to maximize context utilization.
/// - When `greedy_skip_oversized == false`, halts at the first eligible chunk that exceeds
///   the remaining budget (strict ranked prefix truncation).
/// - Stops cleanly if `out_selected` reaches capacity; never overflows token arithmetic.
/// - Returns `null` if `min_score` or any candidate `score` is `NaN`/`Inf`.
pub fn selectWithinTokenBudget(
    candidates: []const RagContextChunk,
    max_tokens: usize,
    min_score: f32,
    greedy_skip_oversized: bool,
    out_selected: *[]RagContextChunk,
): TokenBudgetResult? {
    if (!isFiniteF32(min_score)) return null;

    let chk: usize = 0;
    while (chk < candidates.len) : (chk += 1) {
        if (!isFiniteF32(candidates[chk].score)) return null;
    }

    let res = TokenBudgetResult{
        selected_count: 0,
        used_tokens: 0,
        skipped_oversized: 0,
        filtered_low_score: 0,
        budget_limit: max_tokens,
    };

    if (candidates.len == 0 || out_selected.len == 0) {
        return res;
    }

    let i: usize = 0;
    while (i < candidates.len) : (i += 1) {
        const chunk = candidates[i];
        if (chunk.score < min_score) {
            res.filtered_low_score += 1;
            continue;
        }

        const tok: usize = chunk.token_count as usize;
        const remaining: usize = max_tokens - res.used_tokens;
        if (tok <= remaining) {
            if (res.selected_count >= out_selected.len) {
                break;
            }
            out_selected[res.selected_count] = chunk;
            res.selected_count += 1;
            res.used_tokens += tok;
        } else {
            res.skipped_oversized += 1;
            if (!greedy_skip_oversized) {
                break;
            }
        }
    }

    return res;
}
