// Origin: Pure-CP Stdlib (AI RAG Reranker: MMR, ColBERT MaxSim & IR Metrics) | Created: 2026-11-22
// Zero-heap-allocation RAG reranking, diversity selection, late-interaction scoring,
// score normalization, and information-retrieval evaluation metrics:
//   - `mmrSelect`: Maximal Marginal Relevance without external mask buffers
//   - `colbertMaxSim`: ColBERT token-level late-interaction MaxSim scoring
//   - `minMaxNormalizeInPlace` & `zScoreNormalizeInPlace`: zero-variance-safe in-place score normalization
//   - `ndcgAtK`, `mrrScore`, `recallAtK`: zero-heap IR quality metrics

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn cosinePair(a: []const f64, b: []const f64, dim: usize): f64 {
    let dot: f64 = 0.0;
    let na_sq: f64 = 0.0;
    let nb_sq: f64 = 0.0;
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        const av = a[d];
        const bv = b[d];
        dot += av * bv;
        na_sq += av * av;
        nb_sq += bv * bv;
    }
    const denom = std.math.sqrt(na_sq) * std.math.sqrt(nb_sq);
    if (!(denom > 0.0) || !isFiniteF64(denom)) return 0.0;
    return dot / denom;
}

/// Zero-heap-allocation Maximal Marginal Relevance (MMR) diversity selection:
/// iteratively picks `argmax_{d not in S} [lambda * query_sims[d] - (1 - lambda) * max_{s in S} cos(d, s)]`.
///
/// - `lambda == 1.0`: pure relevance ranking (descending `query_sims`).
/// - `lambda == 0.0`: pure diversity selection after the first highest-relevance pick.
/// - Ties are deterministically broken by ascending document index.
/// - Requires no separate visited-mask scratch buffer (`O(1)` extra stack space).
///
/// Returns the number of selected indices written to `out_indices` (`min(k, n_docs)`),
/// or `null` on invalid dimensions, `lambda` outside `[0.0, 1.0]`, undersized buffers, or NaN/Inf.
pub fn mmrSelect(
    query_sims: []const f64,
    doc_embeddings: []const f64,
    n_docs: usize,
    dim: usize,
    k: usize,
    lambda: f64,
    out_indices: *[]usize,
): usize? {
    if (n_docs == 0 || dim == 0 || k == 0) return null;
    if (!isFiniteF64(lambda) || lambda < 0.0 || lambda > 1.0) return null;
    if (n_docs > std.math.maxInt(usize) / dim) return null;

    const emb_elems = n_docs * dim;
    const actual_k = if (k < n_docs) k else n_docs;
    if (query_sims.len < n_docs || doc_embeddings.len < emb_elems || out_indices.len < actual_k) {
        return null;
    }

    let iq: usize = 0;
    while (iq < n_docs) : (iq += 1) {
        if (!isFiniteF64(query_sims[iq])) return null;
    }
    let ie: usize = 0;
    while (ie < emb_elems) : (ie += 1) {
        if (!isFiniteF64(doc_embeddings[ie])) return null;
    }

    const one_m_lam = 1.0 - lambda;
    let selected: usize = 0;
    while (selected < actual_k) : (selected += 1) {
        let best_idx: usize = 0;
        let best_score: f64 = 0.0 - std.math.inf(f64);
        let found = false;

        let cand: usize = 0;
        while (cand < n_docs) : (cand += 1) {
            // Check if `cand` is already in `out_indices[0..selected]`
            let already = false;
            let s_chk: usize = 0;
            while (s_chk < selected) : (s_chk += 1) {
                if (out_indices[s_chk] == cand) {
                    already = true;
                    break;
                }
            }
            if (already) continue;

            let mmr_val = query_sims[cand];
            if (selected > 0) {
                let max_red: f64 = 0.0 - std.math.inf(f64);
                const cand_vec = doc_embeddings[cand * dim .. (cand + 1) * dim];
                let s: usize = 0;
                while (s < selected) : (s += 1) {
                    const sel_id = out_indices[s];
                    const sel_vec = doc_embeddings[sel_id * dim .. (sel_id + 1) * dim];
                    const sim = cosinePair(cand_vec, sel_vec, dim);
                    if (sim > max_red) max_red = sim;
                }
                mmr_val = lambda * query_sims[cand] - one_m_lam * max_red;
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

/// ColBERT Late-Interaction MaxSim score:
/// `MaxSim(Q, D) = sum_{i=0}^{q_len - 1} max_{j=0}^{d_len - 1} cos(Q[i], D[j])`.
/// For unit-normalized token embeddings, cosine similarity equals the exact dot product.
/// Zero-norm token vectors contribute `0.0` similarity.
/// Returns `null` on zero dimensions, undersized slices, or NaN/Inf inputs.
pub fn colbertMaxSim(
    query_tokens: []const f64,
    q_len: usize,
    doc_tokens: []const f64,
    d_len: usize,
    dim: usize,
): f64? {
    if (q_len == 0 || d_len == 0 || dim == 0) return null;
    if (q_len > std.math.maxInt(usize) / dim) return null;
    if (d_len > std.math.maxInt(usize) / dim) return null;

    const q_elems = q_len * dim;
    const d_elems = d_len * dim;
    if (query_tokens.len < q_elems || doc_tokens.len < d_elems) return null;

    let iq: usize = 0;
    while (iq < q_elems) : (iq += 1) {
        if (!isFiniteF64(query_tokens[iq])) return null;
    }
    let id: usize = 0;
    while (id < d_elems) : (id += 1) {
        if (!isFiniteF64(doc_tokens[id])) return null;
    }

    let total: f64 = 0.0;
    let qi: usize = 0;
    while (qi < q_len) : (qi += 1) {
        const q_vec = query_tokens[qi * dim .. (qi + 1) * dim];
        let best_sim: f64 = 0.0 - std.math.inf(f64);
        let dj: usize = 0;
        while (dj < d_len) : (dj += 1) {
            const d_vec = doc_tokens[dj * dim .. (dj + 1) * dim];
            const sim = cosinePair(q_vec, d_vec, dim);
            if (sim > best_sim) best_sim = sim;
        }
        total += best_sim;
    }

    if (!isFiniteF64(total)) return null;
    return total;
}

/// In-place Min-Max score normalization to `[0.0, 1.0]`.
/// Constant (zero-range) score slices safely normalize to `0.0`.
/// Returns `false` on empty slice or NaN/Inf inputs without mutating `scores`.
pub fn minMaxNormalizeInPlace(scores: *[]f64): bool {
    if (scores.len == 0) return false;

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

    const span = max_v - min_v;
    if (!(span > 0.0) || !isFiniteF64(span)) {
        let j: usize = 0;
        while (j < scores.len) : (j += 1) {
            scores[j] = 0.0;
        }
        return true;
    }

    const inv_span = 1.0 / span;
    let j: usize = 0;
    while (j < scores.len) : (j += 1) {
        scores[j] = (scores[j] - min_v) * inv_span;
    }
    return true;
}

/// In-place Z-Score standardization `(x - mean) / std`.
/// Constant (zero-variance) score slices safely normalize to `0.0`.
/// Returns `false` on empty slice or NaN/Inf inputs without mutating `scores`.
pub fn zScoreNormalizeInPlace(scores: *[]f64): bool {
    if (scores.len == 0) return false;

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < scores.len) : (i += 1) {
        const v = scores[i];
        if (!isFiniteF64(v)) return false;
        sum += v;
    }
    const n_f = scores.len as f64;
    const mean = sum / n_f;

    let var_sum: f64 = 0.0;
    let j: usize = 0;
    while (j < scores.len) : (j += 1) {
        const diff = scores[j] - mean;
        var_sum += diff * diff;
    }
    const variance = var_sum / n_f;
    const std_dev = std.math.sqrt(variance);

    if (!(std_dev > 0.0) || !isFiniteF64(std_dev)) {
        let k: usize = 0;
        while (k < scores.len) : (k += 1) {
            scores[k] = 0.0;
        }
        return true;
    }

    const inv_std = 1.0 / std_dev;
    let k: usize = 0;
    while (k < scores.len) : (k += 1) {
        scores[k] = (scores[k] - mean) * inv_std;
    }
    return true;
}

/// Computes Normalized Discounted Cumulative Gain at rank `k` (`NDCG@K`) in `O(1)` extra stack space:
/// `DCG@K = sum_{i=0}^{min(k, n) - 1} (2^rel[i] - 1) / log2(i + 2)`,
/// normalized by the ideal `IDCG@K` obtained by sorting all `relevances` descending.
/// Returns `0.0` when all relevance grades are zero, or `null` on empty input, `k == 0`,
/// negative relevance, or NaN/Inf.
pub fn ndcgAtK(relevances: []const f64, k: usize): f64? {
    if (relevances.len == 0 || k == 0) return null;

    let chk: usize = 0;
    while (chk < relevances.len) : (chk += 1) {
        const r = relevances[chk];
        if (!isFiniteF64(r) || r < 0.0) return null;
    }

    const actual_k = if (k < relevances.len) k else relevances.len;

    let dcg: f64 = 0.0;
    let i: usize = 0;
    while (i < actual_k) : (i += 1) {
        const gain = std.math.pow(f64, 2.0, relevances[i]) - 1.0;
        const disc = std.math.log2((i + 2) as f64);
        dcg += gain / disc;
    }

    // Zero-heap O(N * K) exact descending traversal for IDCG@K (handles duplicate ties cleanly)
    let idcg: f64 = 0.0;
    let prev_val: f64 = std.math.inf(f64);
    let remaining_at_prev: usize = 0;
    let rank: usize = 0;
    while (rank < actual_k) : (rank += 1) {
        let pick: f64 = 0.0;
        if (remaining_at_prev > 0) {
            pick = prev_val;
            remaining_at_prev -= 1;
        } else {
            let best: f64 = -1.0;
            let count_best: usize = 0;
            let idx: usize = 0;
            while (idx < relevances.len) : (idx += 1) {
                const v = relevances[idx];
                if (v < prev_val) {
                    if (v > best) {
                        best = v;
                        count_best = 1;
                    } else if (v == best) {
                        count_best += 1;
                    }
                }
            }
            if (count_best == 0) break;
            pick = best;
            prev_val = best;
            remaining_at_prev = count_best - 1;
        }
        const gain = std.math.pow(f64, 2.0, pick) - 1.0;
        const disc = std.math.log2((rank + 2) as f64);
        idcg += gain / disc;
    }

    if (!(idcg > 0.0)) return 0.0;
    const ndcg = dcg / idcg;
    if (!isFiniteF64(ndcg)) return null;
    return ndcg;
}

/// Computes Reciprocal Rank (`1 / (rank_of_first_relevant + 1)`, or `0.0` if no relevant doc appears).
/// Returns `null` if `ranked_ids` or `relevant_ids` is empty.
pub fn mrrScore(ranked_ids: []const u32, relevant_ids: []const u32): f64? {
    if (ranked_ids.len == 0 || relevant_ids.len == 0) return null;

    let i: usize = 0;
    while (i < ranked_ids.len) : (i += 1) {
        const doc_id = ranked_ids[i];
        let j: usize = 0;
        while (j < relevant_ids.len) : (j += 1) {
            if (relevant_ids[j] == doc_id) {
                return 1.0 / ((i + 1) as f64);
            }
        }
    }
    return 0.0;
}

/// Computes `Recall@K`: fraction of unique `relevant_ids` found within `ranked_ids[0..min(k, ranked_ids.len)]`.
/// Deduplicates `relevant_ids` in `O(1)` extra space so duplicate ground-truth IDs never inflate Recall > 1.0.
/// Returns `null` if `k == 0` or `relevant_ids` is empty.
pub fn recallAtK(ranked_ids: []const u32, relevant_ids: []const u32, k: usize): f64? {
    if (k == 0 || relevant_ids.len == 0) return null;

    const actual_k = if (k < ranked_ids.len) k else ranked_ids.len;
    let unique_rel: usize = 0;
    let hits: usize = 0;

    let r: usize = 0;
    while (r < relevant_ids.len) : (r += 1) {
        const target = relevant_ids[r];
        // Skip duplicates earlier in relevant_ids
        let dup = false;
        let prev: usize = 0;
        while (prev < r) : (prev += 1) {
            if (relevant_ids[prev] == target) {
                dup = true;
                break;
            }
        }
        if (dup) continue;
        unique_rel += 1;

        let i: usize = 0;
        while (i < actual_k) : (i += 1) {
            if (ranked_ids[i] == target) {
                hits += 1;
                break;
            }
        }
    }

    if (unique_rel == 0) return 0.0;
    return (hits as f64) / (unique_rel as f64);
}
