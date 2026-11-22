// Origin: Pure-CP Stdlib (Zero-Alloc SIMD @dot/@l2sq Incremental MMR Diversity Reranker) | Created: 2026-11-23
// Pure CP zero-heap-allocation, hardware-accelerated Maximal Marginal Relevance (MMR) Reranker:
//   - Native `@dot` and `@l2sq` SIMD similarity kernels supporting `cosine | dot | neg_l2`
//   - O(K*N*D) incremental greedy MMR maintaining per-candidate `max_sim_to_selected` cache
//     (avoiding naive O(K^2*N*D) recomputation)
//   - Multi-source diversity quotas (`max_per_source`), minimum relevance floor (`min_relevance`),
//     and hard near-duplicate suppression (`redundancy_threshold`)
//   - Supports both query-vector scoring (`selectMmr`) and precomputed BM25/Cross-Encoder scoring
//     (`selectMmrWithPrecomputedScores`), plus pairwise list diversity evaluation (`computeListDiversity`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of candidates evaluated in a single zero-heap MMR pass.
pub const MMR_MAX_CANDIDATES: usize = 128;

/// Vector similarity metric used by MMR relevance and pairwise redundancy calculations.
pub enum MmrMetric {
    cosine,
    dot,
    neg_l2,
}

/// Configuration for incremental MMR diversity reranking.
pub struct MmrConfig {
    /// Relevance vs. diversity trade-off in `[0.0, 1.0]` (`1.0` = pure relevance, `0.0` = pure diversity).
    lambda: f32 = 0.5;
    /// Maximum number of items to select.
    top_k: usize = 10;
    /// Minimum relevance threshold; candidates scoring strictly below this are skipped.
    min_relevance: f32 = -1.0e30;
    /// Hard redundancy threshold; candidates with `max_sim_to_selected >= redundancy_threshold` are skipped.
    redundancy_threshold: f32 = 1.0e30;
    /// Maximum selected items allowed per distinct `source_id` (`0` = unlimited).
    max_per_source: usize = 0;
    /// Vector similarity metric for query/document and pairwise document comparisons.
    metric: MmrMetric = MmrMetric.cosine;

    /// Return balanced default MMR configuration (`lambda = 0.5`, `top_k = 10`, `cosine`).
    static fn defaults(): MmrConfig {
        return MmrConfig{};
    }

    /// Validate that floating-point parameters are finite and `lambda` is in `[0.0, 1.0]`.
    fn isValid(): bool {
        if (lambda != lambda || lambda < 0.0 || lambda > 1.0) return false;
        if (min_relevance != min_relevance) return false;
        if (redundancy_threshold != redundancy_threshold) return false;
        return true;
    }
}

/// Input candidate metadata referencing a vector slice inside `flat_vecs`.
pub struct MmrCandidate {
    doc_id: u64 = 0;
    source_id: u32 = 0;
    base_score: f32 = 0.0;
    vec_offset: usize = 0;
}

/// Selected item emitted by MMR reranking with full score attribution.
pub struct MmrSelectedItem {
    doc_id: u64 = 0;
    source_id: u32 = 0;
    relevance: f32 = 0.0;
    max_redundancy: f32 = 0.0;
    mmr_score: f32 = 0.0;
    orig_index: usize = 0;
    vec_offset: usize = 0;
}

/// Zero-heap execution telemetry for an MMR selection pass.
pub struct MmrStats {
    candidates_evaluated: usize = 0;
    selected_count: usize = 0;
    skipped_low_relevance: usize = 0;
    skipped_redundant: usize = 0;
    skipped_source_cap: usize = 0;
}

// ============================================================================
// 2. Native Hardware-Accelerated Similarity Kernel (`@dot` / `@l2sq`)
// ============================================================================

fn isNaN(x: f32): bool {
    return x != x;
}

/// Compute pairwise vector similarity using native CP `@dot` / `@l2sq` intrinsics.
/// Returns `0.0` on empty, mismatched-length, zero-norm (for cosine), or NaN inputs.
pub fn mmrPairSimilarity(a: []const f32, b: []const f32, metric: MmrMetric): f32 {
    if (a.len == 0 || a.len != b.len) return 0.0;

    if (metric == MmrMetric.dot) {
        const d: f32 = @dot(a, b);
        return if (isNaN(d)) 0.0 else d;
    }

    if (metric == MmrMetric.neg_l2) {
        const d2: f32 = @l2sq(a, b);
        if (isNaN(d2)) return 0.0;
        return if (d2 < 0.0) 0.0 else -d2;
    }

    // MmrMetric.cosine
    const na: f32 = @dot(a, a);
    const nb: f32 = @dot(b, b);
    if (isNaN(na) || isNaN(nb) || na <= 0.0 || nb <= 0.0) return 0.0;
    const ab: f32 = @dot(a, b);
    if (isNaN(ab)) return 0.0;
    const denom: f32 = @sqrt(na) * @sqrt(nb);
    if (isNaN(denom) || denom <= 0.0) return 0.0;
    const cos: f32 = ab / denom;
    if (cos > 1.0) return 1.0;
    if (cos < -1.0) return -1.0;
    return cos;
}

// ============================================================================
// 3. O(K*N*D) Incremental Greedy MMR Reranker Core
// ============================================================================

fn runIncrementalMmr(
    use_query: bool,
    query_vec: []const f32,
    flat_vecs: []const f32,
    dim: usize,
    candidates: []const MmrCandidate,
    cfg: MmrConfig,
    out_selected: *[]MmrSelectedItem,
    out_stats: *MmrStats,
): usize {
    out_stats.candidates_evaluated = 0;
    out_stats.selected_count = 0;
    out_stats.skipped_low_relevance = 0;
    out_stats.skipped_redundant = 0;
    out_stats.skipped_source_cap = 0;

    if (!cfg.isValid() || dim == 0 || cfg.top_k == 0 || candidates.len == 0 || out_selected.len == 0) {
        return 0;
    }
    if (use_query && query_vec.len != dim) {
        return 0;
    }

    const n: usize = if (candidates.len > MMR_MAX_CANDIDATES) MMR_MAX_CANDIDATES else candidates.len;
    const limit: usize = if (cfg.top_k < out_selected.len) cfg.top_k else out_selected.len;

    let rel: [MMR_MAX_CANDIDATES]f32 = [0.0; MMR_MAX_CANDIDATES];
    let max_sim: [MMR_MAX_CANDIDATES]f32 = [-1.0e30; MMR_MAX_CANDIDATES];
    let alive: [MMR_MAX_CANDIDATES]bool = [false; MMR_MAX_CANDIDATES];
    let selected_sources: [MMR_MAX_CANDIDATES]u32 = [0; MMR_MAX_CANDIDATES];

    // Stage 1: Validate vector bounds and compute/load initial relevance scores.
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const off = candidates[i].vec_offset;
        if (off > flat_vecs.len || flat_vecs.len - off < dim) {
            alive[i] = false;
            continue;
        }
        out_stats.candidates_evaluated += 1;

        const r: f32 = if (use_query)
            mmrPairSimilarity(query_vec, flat_vecs[off .. off + dim], cfg.metric)
        else
            candidates[i].base_score;

        if (isNaN(r) || r < cfg.min_relevance) {
            alive[i] = false;
            out_stats.skipped_low_relevance += 1;
            continue;
        }

        rel[i] = r;
        alive[i] = true;
    }

    // Stage 2: O(K*N*D) incremental greedy MMR selection.
    let selected_count: usize = 0;
    const one_minus_lambda: f32 = 1.0 - cfg.lambda;

    while (selected_count < limit) {
        let found = false;
        let best_idx: usize = 0;
        let best_score: f32 = -1.0e30;

        let c_i: usize = 0;
        while (c_i < n) : (c_i += 1) {
            if (!alive[c_i]) continue;

            // Enforce per-source quota (monotonic: once capped, permanently ineligible).
            if (cfg.max_per_source > 0) {
                let src_cnt: usize = 0;
                let s_i: usize = 0;
                while (s_i < selected_count) : (s_i += 1) {
                    if (selected_sources[s_i] == candidates[c_i].source_id) {
                        src_cnt += 1;
                    }
                }
                if (src_cnt >= cfg.max_per_source) {
                    alive[c_i] = false;
                    out_stats.skipped_source_cap += 1;
                    continue;
                }
            }

            // Enforce hard redundancy suppression (monotonic: max_sim only increases).
            if (selected_count > 0 && max_sim[c_i] >= cfg.redundancy_threshold) {
                alive[c_i] = false;
                out_stats.skipped_redundant += 1;
                continue;
            }

            const mmr_val: f32 = if (selected_count == 0)
                rel[c_i]
            else
                (cfg.lambda * rel[c_i]) - (one_minus_lambda * max_sim[c_i]);

            if (!found ||
                mmr_val > best_score ||
                (mmr_val == best_score && rel[c_i] > rel[best_idx]) ||
                (mmr_val == best_score && rel[c_i] == rel[best_idx] && candidates[c_i].doc_id < candidates[best_idx].doc_id))
            {
                found = true;
                best_idx = c_i;
                best_score = mmr_val;
            }
        }

        if (!found) break;

        const chosen_redundancy: f32 = if (selected_count == 0) 0.0 else max_sim[best_idx];
        out_selected[selected_count] = MmrSelectedItem{
            doc_id: candidates[best_idx].doc_id,
            source_id: candidates[best_idx].source_id,
            relevance: rel[best_idx],
            max_redundancy: chosen_redundancy,
            mmr_score: best_score,
            orig_index: best_idx,
            vec_offset: candidates[best_idx].vec_offset,
        };
        selected_sources[selected_count] = candidates[best_idx].source_id;
        alive[best_idx] = false;
        selected_count += 1;

        // Incremental O(N*D) update of `max_sim` against ONLY the newly selected vector.
        if (selected_count < limit) {
            const b_off = candidates[best_idx].vec_offset;
            const best_vec = flat_vecs[b_off .. b_off + dim];
            let j: usize = 0;
            while (j < n) : (j += 1) {
                if (!alive[j]) continue;
                const j_off = candidates[j].vec_offset;
                const sim = mmrPairSimilarity(best_vec, flat_vecs[j_off .. j_off + dim], cfg.metric);
                if (selected_count == 1 || sim > max_sim[j]) {
                    max_sim[j] = sim;
                }
            }
        }
    }

    out_stats.selected_count = selected_count;
    return selected_count;
}

/// Select diverse Top-K candidates via incremental O(K*N*D) MMR using `query_vec` similarity.
pub fn selectMmr(
    query_vec: []const f32,
    flat_vecs: []const f32,
    dim: usize,
    candidates: []const MmrCandidate,
    cfg: MmrConfig,
    out_selected: *[]MmrSelectedItem,
    out_stats: *MmrStats,
): usize {
    return runIncrementalMmr(true, query_vec, flat_vecs, dim, candidates, cfg, out_selected, out_stats);
}

/// Select diverse Top-K candidates via incremental O(K*N*D) MMR using each candidate's
/// precomputed `base_score` (e.g. BM25 or Cross-Encoder score) as relevance while using
/// `flat_vecs` for pairwise redundancy calculation.
pub fn selectMmrWithPrecomputedScores(
    flat_vecs: []const f32,
    dim: usize,
    candidates: []const MmrCandidate,
    cfg: MmrConfig,
    out_selected: *[]MmrSelectedItem,
    out_stats: *MmrStats,
): usize {
    const empty_q: [0]f32 = [];
    return runIncrementalMmr(false, empty_q[0..], flat_vecs, dim, candidates, cfg, out_selected, out_stats);
}

/// Compute average pairwise diversity `(1.0 - sim(v_i, v_j))` across a selected result list.
/// Returns `0.0` when fewer than 2 valid items are present.
pub fn computeListDiversity(
    flat_vecs: []const f32,
    dim: usize,
    selected: []const MmrSelectedItem,
    metric: MmrMetric,
): f32 {
    if (dim == 0 || selected.len < 2) return 0.0;

    let sum_div: f64 = 0.0;
    let pairs: u64 = 0;
    let i: usize = 0;
    while (i < selected.len) : (i += 1) {
        const off_i = selected[i].vec_offset;
        if (off_i > flat_vecs.len || flat_vecs.len - off_i < dim) continue;
        const vi = flat_vecs[off_i .. off_i + dim];

        let j: usize = i + 1;
        while (j < selected.len) : (j += 1) {
            const off_j = selected[j].vec_offset;
            if (off_j > flat_vecs.len || flat_vecs.len - off_j < dim) continue;
            const vj = flat_vecs[off_j .. off_j + dim];
            const sim = mmrPairSimilarity(vi, vj, metric);
            sum_div += @as(f64, 1.0 - sim);
            pairs += 1;
        }
    }

    if (pairs == 0) return 0.0;
    const avg_f64: f64 = sum_div / @as(f64, @floatFromInt(pairs));
    const avg_f32: f32 = @floatCast(avg_f64);
    return avg_f32;
}
