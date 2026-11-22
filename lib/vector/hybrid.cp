// Origin: Pure-CP Stdlib (RAG Hybrid Retrieval: RRF, MMR Rerank & Int8 Scalar Quantization) | Created: 2026-11-22
// Source: cp/lib/vector/hybrid.cp (Reciprocal Rank Fusion Hybrid RAG Ranker)
// Industrial-standard RAG hybrid retrieval & reranking primitives:
//   1. RrfFusion (Allocator-backed string-ID RRF ranker for SearchHit)
//   2. Zero-heap-allocation M-way RRF fusion (rrfFuse)
//   3. Min-Max / Z-Score normalization & linear weighted fusion (normalizeScoresMinMax, normalizeScoresZScore, weightedScoreFuse)
//   4. Maximal Marginal Relevance diversity reranking (mmrRerank)
//   5. Asymmetric U8/Int8 scalar quantization & fast integer-accumulated dot product (quantizeVectorU8, dequantizeVectorU8, quantizedDotU8)

const std = @import("std");
import { SearchHit } from "./flatIndex.cp";

struct FusedAcc {
    score: f32 = 0.0;
    meta: string = "";
}

fn rrfHitBetterDesc(a: SearchHit, b: SearchHit): bool {
    if (a.score != b.score) return a.score > b.score;
    return std.mem.order(u8, a.id, b.id) == .lt;
}

fn rrfSiftDownWorstRoot(hits: []SearchHit, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && rrfHitBetterDesc(hits[worst], hits[right])) {
            worst = right;
        }
        if (!rrfHitBetterDesc(hits[idx], hits[worst])) break;
        const tmp = hits[idx];
        hits[idx] = hits[worst];
        hits[worst] = tmp;
        idx = worst;
    }
}

fn rrfSortHitsDesc(hits: []SearchHit) {
    if (hits.len <= 1) return;
    let n = hits.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        rrfSiftDownWorstRoot(hits, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = hits[0];
        hits[0] = hits[end];
        hits[end] = tmp;
        rrfSiftDownWorstRoot(hits, 0, end);
    }
}

/// Reciprocal Rank Fusion (RRF) ranker for hybrid RAG retrieval.
pub static struct RrfFusion {
    /// Fuses ranked `vector_hits` and `text_hits` using Reciprocal Rank Fusion:
    /// `score(doc) = sum(1.0 / (rrf_k + rank))` with 1-based rank (`1, 2, ...`).
    /// Deduplicates results by `id`, preserves non-empty `meta` (preferring `vector_hits`),
    /// and returns up to `top_k` results sorted descending by fused RRF score.
    static fn fuse(
        alloc: std.mem.Allocator,
        vector_hits: []const SearchHit,
        text_hits: []const SearchHit,
        rrf_k: f32 = 60.0,
        top_k: usize = 10,
    ): ([]SearchHit)! {
        if (top_k == 0 || (vector_hits.len == 0 && text_hits.len == 0)) {
            return alloc.alloc(SearchHit, 0)?;
        }

        const k: f32 = if (rrf_k >= 0.0) rrf_k else 60.0;

        let fused: std.StringHashMapUnmanaged(FusedAcc) = .empty;
        defer fused.deinit(alloc);

        let seen_vec: std.StringHashMapUnmanaged(u8) = .empty;
        defer seen_vec.deinit(alloc);

        let seen_text: std.StringHashMapUnmanaged(u8) = .empty;
        defer seen_text.deinit(alloc);

        let i: usize = 0;
        while (i < vector_hits.len) : (i += 1) {
            const hit = vector_hits[i];
            if (seen_vec.contains(hit.id)) continue;
            seen_vec.put(alloc, hit.id, 1)?;

            const rank = (i + 1) as f32;
            const rrf_contrib = 1.0 / (k + rank);
            if (entry := fused.getEntry(hit.id)) {
                entry.value_ptr.score += rrf_contrib;
                if (entry.value_ptr.meta.len == 0 && hit.meta.len > 0) {
                    entry.value_ptr.meta = hit.meta;
                }
            } else {
                fused.put(alloc, hit.id, FusedAcc{
                    score: rrf_contrib,
                    meta: hit.meta,
                })?;
            }
        }

        let j: usize = 0;
        while (j < text_hits.len) : (j += 1) {
            const hit = text_hits[j];
            if (seen_text.contains(hit.id)) continue;
            seen_text.put(alloc, hit.id, 1)?;

            const rank = (j + 1) as f32;
            const rrf_contrib = 1.0 / (k + rank);
            if (entry := fused.getEntry(hit.id)) {
                entry.value_ptr.score += rrf_contrib;
                if (entry.value_ptr.meta.len == 0 && hit.meta.len > 0) {
                    entry.value_ptr.meta = hit.meta;
                }
            } else {
                fused.put(alloc, hit.id, FusedAcc{
                    score: rrf_contrib,
                    meta: hit.meta,
                })?;
            }
        }

        if (fused.count() == 0) {
            return alloc.alloc(SearchHit, 0)?;
        }

        let candidates: std.ArrayList(SearchHit) = .empty;
        defer candidates.deinit(alloc);

        let f_iter = fused.iterator();
        while (f_entry := f_iter.next()) {
            candidates.append(alloc, SearchHit{
                id: f_entry.key_ptr.*,
                score: f_entry.value_ptr.score,
                meta: f_entry.value_ptr.meta,
            })?;
        }

        rrfSortHitsDesc(candidates.items);
        const out_len = if (top_k < candidates.items.len) top_k else candidates.items.len;
        let out = alloc.alloc(SearchHit, out_len)?;
        @memcpy(out, candidates.items[0..out_len]);
        return out;
    }
}

// ============================================================================
// Pure-CP Zero-Heap-Allocation RAG Hybrid Retrieval Primitives
// ============================================================================

/// Ranked document entry with compact numeric document ID and relevance score.
pub struct RankedDoc {
    doc_id: u32 = 0;
    score: f64 = 0.0;
}

/// Asymmetric U8 scalar quantization parameters mapping [min_val, max_val] <-> [0, 255].
pub struct ScalarQuantParams {
    min_val: f64 = 0.0;
    scale: f64 = 0.0;
    inv_scale: f64 = 0.0;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn rankedDocBetterDesc(a: RankedDoc, b: RankedDoc): bool {
    if (a.score != b.score) return a.score > b.score;
    return a.doc_id < b.doc_id;
}

fn rankedSiftDownWorstRoot(docs: *[]RankedDoc, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && rankedDocBetterDesc(docs[worst], docs[right])) {
            worst = right;
        }
        if (!rankedDocBetterDesc(docs[idx], docs[worst])) break;
        const tmp = docs[idx];
        docs[idx] = docs[worst];
        docs[worst] = tmp;
        idx = worst;
    }
}

fn sortRankedDocsDesc(docs: *[]RankedDoc, count: usize) {
    if (count <= 1) return;
    let i: usize = count / 2;
    while (i > 0) {
        i -= 1;
        rankedSiftDownWorstRoot(docs, i, count);
    }
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = docs[0];
        docs[0] = docs[end];
        docs[end] = tmp;
        rankedSiftDownWorstRoot(docs, 0, end);
    }
}

/// Zero-heap-allocation M-way Reciprocal Rank Fusion (RRF).
///
/// `lists_doc_ids` stores concatenated ranked document IDs from M retrieval lists,
/// delineated by CSR offsets `list_offsets` (length `M + 1`, non-decreasing, within `lists_doc_ids.len`).
/// Each unique document accumulates `sum_m 1.0 / (k_const + rank_m(doc))` (1-based rank;
/// duplicate doc IDs within the same list only count their first/highest rank).
///
/// Outputs deduplicated results into `out_docs`, sorted descending by `score` with
/// deterministic tie-breaking by ascending `doc_id`. Returns the number of unique docs written,
/// or `null` on invalid parameters / offsets / insufficient workspace or output capacity.
pub fn rrfFuse(
    lists_doc_ids: []const u32,
    list_offsets: []const usize,
    k_const: f64,
    out_docs: *[]RankedDoc,
    work_ids: *[]u32,
    work_scores: *[]f64,
): usize? {
    if (!isFiniteF64(k_const) || k_const < 0.0) return null;
    if (list_offsets.len == 0) return 0;
    if (list_offsets[0] > lists_doc_ids.len) return null;
    if (list_offsets.len == 1) return 0;

    const num_lists = list_offsets.len - 1;
    let m_check: usize = 0;
    while (m_check < num_lists) : (m_check += 1) {
        const s = list_offsets[m_check];
        const e = list_offsets[m_check + 1];
        if (e < s || e > lists_doc_ids.len) return null;
    }

    let unique_count: usize = 0;
    let m: usize = 0;
    while (m < num_lists) : (m += 1) {
        const start = list_offsets[m];
        const end = list_offsets[m + 1];
        let pos: usize = start;
        while (pos < end) : (pos += 1) {
            const doc_id = lists_doc_ids[pos];

            // Skip duplicate doc_id within the same ranked list (keep earliest/best rank).
            let dup_in_list = false;
            let prev: usize = start;
            while (prev < pos) : (prev += 1) {
                if (lists_doc_ids[prev] == doc_id) {
                    dup_in_list = true;
                    break;
                }
            }
            if (dup_in_list) continue;

            const rank = (pos - start + 1) as f64;
            const contrib = 1.0 / (k_const + rank);

            let found = false;
            let idx: usize = 0;
            while (idx < unique_count) : (idx += 1) {
                if (work_ids[idx] == doc_id) {
                    work_scores[idx] += contrib;
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (unique_count >= work_ids.len || unique_count >= work_scores.len) {
                    return null;
                }
                work_ids[unique_count] = doc_id;
                work_scores[unique_count] = contrib;
                unique_count += 1;
            }
        }
    }

    if (unique_count > out_docs.len) return null;

    let i: usize = 0;
    while (i < unique_count) : (i += 1) {
        out_docs[i] = RankedDoc{
            doc_id: work_ids[i],
            score: work_scores[i],
        };
    }

    sortRankedDocsDesc(out_docs, unique_count);
    return unique_count;
}

/// Min-Max normalizes `scores` into `[0.0, 1.0]` written to `out`.
/// Constant inputs map to `0.0`. Returns `false` if `out` is too small or any score is NaN/Inf.
pub fn normalizeScoresMinMax(scores: []const f64, out: *[]f64): bool {
    if (out.len < scores.len) return false;
    if (scores.len == 0) return true;

    let min_v = scores[0];
    let max_v = scores[0];
    if (!isFiniteF64(min_v)) return false;

    let i: usize = 1;
    while (i < scores.len) : (i += 1) {
        const v = scores[i];
        if (!isFiniteF64(v)) return false;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }

    const range = max_v - min_v;
    if (!isFiniteF64(range)) return false;

    if (range == 0.0) {
        let j: usize = 0;
        while (j < scores.len) : (j += 1) {
            out[j] = 0.0;
        }
        return true;
    }

    const inv_range = 1.0 / range;
    let j: usize = 0;
    while (j < scores.len) : (j += 1) {
        out[j] = (scores[j] - min_v) * inv_range;
    }
    return true;
}

/// Z-Score standardizes `scores` to zero mean and unit variance into `out`.
/// Constant inputs map to `0.0`. Returns `false` if `out` is too small or any score is NaN/Inf.
pub fn normalizeScoresZScore(scores: []const f64, out: *[]f64): bool {
    if (out.len < scores.len) return false;
    if (scores.len == 0) return true;

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < scores.len) : (i += 1) {
        const v = scores[i];
        if (!isFiniteF64(v)) return false;
        sum += v;
    }
    const n_f = scores.len as f64;
    const mean = sum / n_f;
    if (!isFiniteF64(mean)) return false;

    let sq_sum: f64 = 0.0;
    let j: usize = 0;
    while (j < scores.len) : (j += 1) {
        const diff = scores[j] - mean;
        sq_sum += diff * diff;
    }
    const variance = sq_sum / n_f;
    if (!isFiniteF64(variance)) return false;

    if (variance == 0.0) {
        let k: usize = 0;
        while (k < scores.len) : (k += 1) {
            out[k] = 0.0;
        }
        return true;
    }

    const inv_std = 1.0 / std.math.sqrt(variance);
    let k: usize = 0;
    while (k < scores.len) : (k += 1) {
        out[k] = (scores[k] - mean) * inv_std;
    }
    return true;
}

/// Zero-heap-allocation linear weighted score fusion:
/// `fused_score(doc) = alpha * dense_score(doc) + (1.0 - alpha) * sparse_score(doc)`.
///
/// Deduplicates documents across `dense_docs` and `sparse_docs` (within each input list,
/// only the first occurrence of a `doc_id` is counted), writes merged results into `out_docs`
/// sorted descending by fused score with ascending `doc_id` tie-breaking.
/// Returns the number of unique docs written, or `null` if `alpha` is outside `[0, 1]`,
/// any score is NaN/Inf, or `out_docs` lacks capacity.
pub fn weightedScoreFuse(
    dense_docs: []const RankedDoc,
    sparse_docs: []const RankedDoc,
    alpha: f64,
    out_docs: *[]RankedDoc,
): usize? {
    if (!isFiniteF64(alpha) || alpha < 0.0 || alpha > 1.0) return null;

    let di: usize = 0;
    while (di < dense_docs.len) : (di += 1) {
        if (!isFiniteF64(dense_docs[di].score)) return null;
    }
    let si: usize = 0;
    while (si < sparse_docs.len) : (si += 1) {
        if (!isFiniteF64(sparse_docs[si].score)) return null;
    }

    const beta = 1.0 - alpha;
    let out_count: usize = 0;

    // 1. Insert unique dense docs weighted by alpha.
    di = 0;
    while (di < dense_docs.len) : (di += 1) {
        const doc = dense_docs[di];
        let dup = false;
        let k: usize = 0;
        while (k < out_count) : (k += 1) {
            if (out_docs[k].doc_id == doc.doc_id) {
                dup = true;
                break;
            }
        }
        if (dup) continue;
        if (out_count >= out_docs.len) return null;
        out_docs[out_count] = RankedDoc{
            doc_id: doc.doc_id,
            score: alpha * doc.score,
        };
        out_count += 1;
    }

    // 2. Merge unique sparse docs weighted by (1 - alpha).
    si = 0;
    while (si < sparse_docs.len) : (si += 1) {
        const doc = sparse_docs[si];

        // Skip duplicate doc_id within sparse_docs itself.
        let dup_sparse = false;
        let prev: usize = 0;
        while (prev < si) : (prev += 1) {
            if (sparse_docs[prev].doc_id == doc.doc_id) {
                dup_sparse = true;
                break;
            }
        }
        if (dup_sparse) continue;

        let found = false;
        let k: usize = 0;
        while (k < out_count) : (k += 1) {
            if (out_docs[k].doc_id == doc.doc_id) {
                out_docs[k].score += beta * doc.score;
                found = true;
                break;
            }
        }
        if (!found) {
            if (out_count >= out_docs.len) return null;
            out_docs[out_count] = RankedDoc{
                doc_id: doc.doc_id,
                score: beta * doc.score,
            };
            out_count += 1;
        }
    }

    sortRankedDocsDesc(out_docs, out_count);
    return out_count;
}

fn cosineSimF64(a: []const f64, b: []const f64): f64 {
    let dot: f64 = 0.0;
    let na: f64 = 0.0;
    let nb: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const va = a[i];
        const vb = b[i];
        dot += va * vb;
        na += va * va;
        nb += vb * vb;
    }
    if (na == 0.0 || nb == 0.0) return 0.0;
    const denom = std.math.sqrt(na) * std.math.sqrt(nb);
    if (denom == 0.0 || !isFiniteF64(denom)) return 0.0;
    const cos = dot / denom;
    if (cos > 1.0) return 1.0;
    if (cos < -1.0) return -1.0;
    return cos;
}

/// Zero-heap-allocation Maximal Marginal Relevance (MMR) diversity reranker.
///
/// Greedily selects up to `min(top_k, n_docs)` candidate indices maximizing:
///   `lambda * query_sims[d] - (1.0 - lambda) * max_{s in S} cos(emb[d], emb[s])`
/// When `lambda == 1.0`, reduces to pure relevance sorting by `query_sims`.
///
/// `embeddings` is row-major with length >= `n_docs * dim`.
/// `selected_mask` is caller-provided workspace of length >= `n_docs` (cleared internally).
/// Returns the number of selected indices written to `out_indices`, or `null` on invalid inputs.
pub fn mmrRerank(
    query_sims: []const f64,
    embeddings: []const f64,
    n_docs: usize,
    dim: usize,
    top_k: usize,
    lambda: f64,
    out_indices: *[]usize,
    selected_mask: *[]u8,
): usize? {
    if (!isFiniteF64(lambda) || lambda < 0.0 || lambda > 1.0) return null;
    if (n_docs == 0 || top_k == 0) return 0;
    if (dim == 0) return null;
    if (query_sims.len < n_docs) return null;
    if (n_docs > std.math.maxInt(usize) / dim) return null;
    const total_elems = n_docs * dim;
    if (embeddings.len < total_elems) return null;

    const k = if (top_k < n_docs) top_k else n_docs;
    if (out_indices.len < k) return null;
    if (selected_mask.len < n_docs) return null;

    // Validate finite inputs and clear selection mask.
    let i: usize = 0;
    while (i < n_docs) : (i += 1) {
        if (!isFiniteF64(query_sims[i])) return null;
        selected_mask[i] = 0;
    }
    let e: usize = 0;
    while (e < total_elems) : (e += 1) {
        if (!isFiniteF64(embeddings[e])) return null;
    }

    const neg_inf: f64 = 0.0 - std.math.inf(f64);
    const one_minus_lambda = 1.0 - lambda;
    let selected_count: usize = 0;

    while (selected_count < k) : (selected_count += 1) {
        let best_idx: usize = 0;
        let best_score: f64 = neg_inf;
        let has_best = false;

        let cand: usize = 0;
        while (cand < n_docs) : (cand += 1) {
            if (selected_mask[cand] != 0) continue;

            let mmr_score: f64 = 0.0;
            if (selected_count == 0) {
                mmr_score = query_sims[cand];
            } else if (one_minus_lambda == 0.0) {
                mmr_score = query_sims[cand];
            } else {
                const cand_off = cand * dim;
                const cand_vec = embeddings[cand_off .. cand_off + dim];
                let max_sim: f64 = neg_inf;
                let s_idx: usize = 0;
                while (s_idx < selected_count) : (s_idx += 1) {
                    const sel = out_indices[s_idx];
                    const sel_off = sel * dim;
                    const sel_vec = embeddings[sel_off .. sel_off + dim];
                    const sim = cosineSimF64(cand_vec, sel_vec);
                    if (sim > max_sim) max_sim = sim;
                }
                mmr_score = lambda * query_sims[cand] - one_minus_lambda * max_sim;
            }

            if (!has_best || mmr_score > best_score) {
                has_best = true;
                best_score = mmr_score;
                best_idx = cand;
            }
        }

        if (!has_best) break;
        selected_mask[best_idx] = 1;
        out_indices[selected_count] = best_idx;
    }

    return selected_count;
}

/// Asymmetric U8 scalar quantization (4x/8x compression for f32/f64 vectors).
/// Maps `[min_val, max_val]` uniformly onto `[0, 255]` with nearest-integer rounding.
/// Returns `null` if `vec` is empty, `q_out` is too small, or any component is NaN/Inf.
pub fn quantizeVectorU8(vec: []const f64, q_out: *[]u8): ScalarQuantParams? {
    if (vec.len == 0 || q_out.len < vec.len) return null;

    let min_v = vec[0];
    let max_v = vec[0];
    if (!isFiniteF64(min_v)) return null;

    let i: usize = 1;
    while (i < vec.len) : (i += 1) {
        const v = vec[i];
        if (!isFiniteF64(v)) return null;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }

    const range = max_v - min_v;
    if (!isFiniteF64(range)) return null;

    if (range == 0.0) {
        let j: usize = 0;
        while (j < vec.len) : (j += 1) {
            q_out[j] = 0;
        }
        return ScalarQuantParams{
            min_val: min_v,
            scale: 0.0,
            inv_scale: 0.0,
        };
    }

    const scale = range / 255.0;
    const inv_scale = 255.0 / range;
    let j: usize = 0;
    while (j < vec.len) : (j += 1) {
        let scaled = (vec[j] - min_v) * inv_scale;
        if (scaled < 0.0) scaled = 0.0;
        if (scaled > 255.0) scaled = 255.0;
        const rounded: f64 = @round(scaled);
        q_out[j] = rounded as u8;
    }

    return ScalarQuantParams{
        min_val: min_v,
        scale: scale,
        inv_scale: inv_scale,
    };
}

/// Reconstructs an approximate `f64` vector from U8 quantized codes and `ScalarQuantParams`.
/// Returns `false` if `out` is too small or `params` contains NaN/Inf/negative scale.
pub fn dequantizeVectorU8(q: []const u8, params: ScalarQuantParams, out: *[]f64): bool {
    if (out.len < q.len) return false;
    if (!isFiniteF64(params.min_val) || !isFiniteF64(params.scale) || params.scale < 0.0) {
        return false;
    }

    let i: usize = 0;
    while (i < q.len) : (i += 1) {
        out[i] = params.min_val + (q[i] as f64) * params.scale;
    }
    return true;
}

/// Fast approximate dot product directly over U8-quantized vectors using integer accumulation:
/// `dot(a_hat, b_hat) = d * min_a * min_b + min_a * scale_b * sum(qb) + min_b * scale_a * sum(qa) + scale_a * scale_b * sum(qa * qb)`.
/// Returns `null` if vector lengths mismatch or parameters are invalid.
pub fn quantizedDotU8(
    qa: []const u8,
    pa: ScalarQuantParams,
    qb: []const u8,
    pb: ScalarQuantParams,
): f64? {
    if (qa.len != qb.len) return null;
    if (!isFiniteF64(pa.min_val) || !isFiniteF64(pa.scale) || pa.scale < 0.0) return null;
    if (!isFiniteF64(pb.min_val) || !isFiniteF64(pb.scale) || pb.scale < 0.0) return null;
    if (qa.len == 0) return 0.0;

    let sum_a: u64 = 0;
    let sum_b: u64 = 0;
    let sum_ab: u64 = 0;

    let i: usize = 0;
    while (i < qa.len) : (i += 1) {
        const a_val = qa[i] as u64;
        const b_val = qb[i] as u64;
        sum_a += a_val;
        sum_b += b_val;
        sum_ab += a_val * b_val;
    }

    const d_f = qa.len as f64;
    const term_const = d_f * pa.min_val * pb.min_val;
    const term_b = pa.min_val * pb.scale * (sum_b as f64);
    const term_a = pb.min_val * pa.scale * (sum_a as f64);
    const term_ab = pa.scale * pb.scale * (sum_ab as f64);
    return term_const + term_b + term_a + term_ab;
}
