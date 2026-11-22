// Origin: Pure-CP Stdlib (Generative Agents Three-Factor Memory Retrieval: Recency, Importance & SIMD @dot Relevance, Dedup Merge & Prompt Packing) | Created: 2026-11-22
// Pure CP zero-heap-allocation memory retrieval & reflection compaction engine:
//   1. `scoreAndRankMemoriesF32`: Recency exp(-decay*age_h) + Importance + 8-lane SIMD `@dot` cosine relevance Top-K ranking with tag filtering
//   2. `mergeRedundantMemoriesF32`: in-place `@dot` similarity-based redundant episodic memory merging
//   3. `packMemoriesForPrompt`: token-budget greedy packing into compact `[mem#<id>] ...` LLM prompt blocks

const std = @import("std");

// ============================================================================
// Internal Numeric, SIMD & Formatting Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clamp01F32(x: f32): f32 {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

fn clampCosineF32(x: f32): f32 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

fn simdCosinePairF32(a: []const f32, b: []const f32): f32 {
    const na: f32 = @dot(a, a);
    const nb: f32 = @dot(b, b);
    if (na <= 0.0 || nb <= 0.0) return 0.0;
    const ab: f32 = @dot(a, b);
    const denom: f32 = @sqrt(na) * @sqrt(nb);
    if (denom <= 0.0 || !isFiniteF32(denom)) return 0.0;
    return clampCosineF32(ab / denom);
}

fn validateFlatEmbeddingsF32(embeddings_flat: []const f32, n: usize, dim: usize): bool {
    let rem: []const f32 = embeddings_flat[0 .. n * dim];
    let r: usize = 0;
    while (r < n) : (r += 1) {
        const [row, next_rem] = rem.split(dim);
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            if (!isFiniteF32(row[d])) return false;
        }
        rem = next_rem;
    }
    return true;
}

fn writeByte(out_buf: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeRaw(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn writeU64Dec(out_buf: *[]u8, pos: *usize, value: u64): bool {
    let digits: [20]u8 = [0; 20];
    let len: usize = 0;
    let v: u64 = value;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) : (len += 1) {
            digits[len] = ('0' as u8) + ((v % 10) as u8);
            v = v / 10;
        }
    }
    while (len > 0) {
        len -= 1;
        if (!writeByte(out_buf, pos, digits[len])) return false;
    }
    return true;
}

// ============================================================================
// 1. Three-Factor Memory Retrieval (`scoreAndRankMemoriesF32`)
// ============================================================================

/// Configuration for three-factor memory retrieval (Recency + Importance + Relevance).
pub struct MemoryRetrievalConfig {
    w_recency: f32 = 1.0;
    w_importance: f32 = 1.0;
    w_relevance: f32 = 1.0;
    decay_rate_per_hour: f32 = 0.05;
    min_score: f32 = -1.0e30;
    required_tag: string = "";

    /// Constructs a validated `MemoryRetrievalConfig`.
    /// Rejects negative/non-finite weights or decay rate, or all-zero weights.
    static fn init(
        w_recency: f32,
        w_importance: f32,
        w_relevance: f32,
        decay_rate_per_hour: f32,
    ): MemoryRetrievalConfig? {
        if (!isFiniteF32(w_recency) || w_recency < 0.0) return null;
        if (!isFiniteF32(w_importance) || w_importance < 0.0) return null;
        if (!isFiniteF32(w_relevance) || w_relevance < 0.0) return null;
        if (!isFiniteF32(decay_rate_per_hour) || decay_rate_per_hour < 0.0) return null;
        if (w_recency + w_importance + w_relevance <= 0.0) return null;

        return MemoryRetrievalConfig{
            w_recency: w_recency,
            w_importance: w_importance,
            w_relevance: w_relevance,
            decay_rate_per_hour: decay_rate_per_hour,
            min_score: -1.0e30,
            required_tag: "",
        };
    }
}

/// Episodic or semantic memory candidate entry for zero-heap retrieval & compaction.
pub struct MemoryCandidate {
    mem_id: u64 = 0;
    importance: f32 = 0.5;
    last_access_ms: i64 = 0;
    access_count: u32 = 1;
    token_count: u32 = 0;
    tag: string = "";
    summary: string = "";
    alive: bool = true;
}

/// Scored retrieval hit decomposing the three factors (`recency`, `importance`, `relevance`).
pub struct MemoryScoredHit {
    candidate_index: usize = 0;
    mem_id: u64 = 0;
    total_score: f32 = 0.0;
    recency_score: f32 = 0.0;
    importance_score: f32 = 0.0;
    relevance_score: f32 = 0.0;
}

fn memoryHitBetterDesc(a: MemoryScoredHit, b: MemoryScoredHit): bool {
    if (a.total_score != b.total_score) return a.total_score > b.total_score;
    return a.mem_id < b.mem_id;
}

/// Zero-heap-allocation three-factor memory retrieval combining:
///   - Exponential Recency decay: `exp(-decay_rate_per_hour * age_hours)`
///   - Clamped Importance: `clamp(candidate.importance, 0.0, 1.0)`
///   - 8-lane SIMD `@dot` cosine Relevance: `simdCosine(query_emb, candidate_emb)`
///
/// Weighted score: `(w_recency * recency + w_importance * importance + w_relevance * relevance) / (w_recency + w_importance + w_relevance)`.
/// Filters dead candidates, optional `cfg.required_tag`, and `total_score < cfg.min_score`,
/// returning up to `min(top_k, matched)` hits sorted descending by `total_score` (tie-break: ascending `mem_id`).
pub fn scoreAndRankMemoriesF32(
    query_emb: []const f32,
    candidates: []const MemoryCandidate,
    embeddings_flat: []const f32,
    dim: usize,
    now_ms: i64,
    cfg: MemoryRetrievalConfig,
    top_k: usize,
    out_hits: *[]MemoryScoredHit,
): usize? {
    if (dim == 0 || query_emb.len != dim) return null;
    if (!isFiniteF32(cfg.w_recency) || cfg.w_recency < 0.0) return null;
    if (!isFiniteF32(cfg.w_importance) || cfg.w_importance < 0.0) return null;
    if (!isFiniteF32(cfg.w_relevance) || cfg.w_relevance < 0.0) return null;
    if (!isFiniteF32(cfg.decay_rate_per_hour) || cfg.decay_rate_per_hour < 0.0) return null;
    if (!isFiniteF32(cfg.min_score)) return null;

    const w_sum: f32 = cfg.w_recency + cfg.w_importance + cfg.w_relevance;
    if (!isFiniteF32(w_sum) || w_sum <= 0.0) return null;

    let qd: usize = 0;
    while (qd < dim) : (qd += 1) {
        if (!isFiniteF32(query_emb[qd])) return null;
    }

    if (candidates.len == 0 || top_k == 0) return 0;
    if (candidates.len > std.math.maxInt(usize) / dim) return null;

    const limit: usize = if (top_k < candidates.len) top_k else candidates.len;
    if (embeddings_flat.len < candidates.len * dim || out_hits.len < limit) return null;
    if (!validateFlatEmbeddingsF32(embeddings_flat, candidates.len, dim)) return null;

    const q_norm_sq: f32 = @dot(query_emb, query_emb);
    const q_norm: f32 = if (q_norm_sq > 0.0) @sqrt(q_norm_sq) else 0.0;
    const inv_w_sum: f32 = 1.0 / w_sum;

    let count: usize = 0;
    let idx: usize = 0;
    while (idx < candidates.len) : (idx += 1) {
        const cand = candidates[idx];
        if (!cand.alive) continue;
        if (!isFiniteF32(cand.importance)) return null;
        if (cfg.required_tag.len > 0 && !std.mem.eql(u8, cand.tag, cfg.required_tag)) {
            continue;
        }

        // 1. Exponential recency decay
        const age_ms_i64: i64 = if (now_ms > cand.last_access_ms) (now_ms - cand.last_access_ms) else 0;
        const age_hours: f32 = (age_ms_i64 as f32) / 3600000.0;
        const recency: f32 = std.math.exp((0.0 as f32) - cfg.decay_rate_per_hour * age_hours);

        // 2. Normalized importance
        const importance: f32 = clamp01F32(cand.importance);

        // 3. 8-lane SIMD @dot cosine relevance
        let relevance: f32 = 0.0;
        if (q_norm > 0.0) {
            const c_vec: []const f32 = embeddings_flat[idx * dim .. (idx + 1) * dim];
            const c_norm_sq: f32 = @dot(c_vec, c_vec);
            if (c_norm_sq > 0.0) {
                const denom: f32 = q_norm * @sqrt(c_norm_sq);
                if (denom > 0.0 && isFiniteF32(denom)) {
                    relevance = clampCosineF32(@dot(query_emb, c_vec) / denom);
                }
            }
        }

        const weighted_num: f32 = @fma(
            cfg.w_recency,
            recency,
            @fma(cfg.w_importance, importance, cfg.w_relevance * relevance),
        );
        const total_score: f32 = weighted_num * inv_w_sum;
        if (total_score < cfg.min_score) continue;

        const hit = MemoryScoredHit{
            candidate_index: idx,
            mem_id: cand.mem_id,
            total_score: total_score,
            recency_score: recency,
            importance_score: importance,
            relevance_score: relevance,
        };

        if (count < limit) {
            let pos: usize = count;
            while (pos > 0 && memoryHitBetterDesc(hit, out_hits[pos - 1])) : (pos -= 1) {
                out_hits[pos] = out_hits[pos - 1];
            }
            out_hits[pos] = hit;
            count += 1;
        } else if (memoryHitBetterDesc(hit, out_hits[limit - 1])) {
            let pos: usize = limit - 1;
            while (pos > 0 && memoryHitBetterDesc(hit, out_hits[pos - 1])) : (pos -= 1) {
                out_hits[pos] = out_hits[pos - 1];
            }
            out_hits[pos] = hit;
        }
    }

    return count;
}

// ============================================================================
// 2. SIMD @dot Redundant Episodic Memory Merging (`mergeRedundantMemoriesF32`)
// ============================================================================

/// Summary returned by `mergeRedundantMemoriesF32`.
pub struct MemoryMergeStats {
    surviving_count: usize = 0;
    merged_count: usize = 0;
}

/// In-place zero-heap merging of semantically redundant episodic memories using 8-lane SIMD `@dot`.
///
/// For any two alive memories `(i, j)` with `i < j` and `simdCosine(emb[i], emb[j]) >= similarity_threshold`:
/// - Retains slot `i` as the survivor and marks slot `j` dead (`alive = false`).
/// - Keeps the higher `importance` (and promotes `j`'s `summary` if `j.importance > i.importance`).
/// - Keeps the latest `last_access_ms` and saturating-adds `access_count`.
///
/// Returns `null` on invalid dimensions, `similarity_threshold` outside `(-1.0, 1.0]`,
/// undersized embeddings, or non-finite inputs.
pub fn mergeRedundantMemoriesF32(
    candidates: *[]MemoryCandidate,
    embeddings_flat: []const f32,
    dim: usize,
    similarity_threshold: f32,
): MemoryMergeStats? {
    if (dim == 0) return null;
    if (!isFiniteF32(similarity_threshold) || similarity_threshold <= -1.0 || similarity_threshold > 1.0) {
        return null;
    }
    if (candidates.len == 0) {
        return MemoryMergeStats{ surviving_count: 0, merged_count: 0 };
    }
    if (candidates.len > std.math.maxInt(usize) / dim) return null;
    if (embeddings_flat.len < candidates.len * dim) return null;
    if (!validateFlatEmbeddingsF32(embeddings_flat, candidates.len, dim)) return null;

    let merged: usize = 0;
    let surviving: usize = 0;
    const max_u32: u32 = std.math.maxInt(u32);

    let i: usize = 0;
    while (i < candidates.len) : (i += 1) {
        if (!candidates[i].alive) continue;
        surviving += 1;
        const vec_i: []const f32 = embeddings_flat[i * dim .. (i + 1) * dim];

        let j: usize = i + 1;
        while (j < candidates.len) : (j += 1) {
            if (!candidates[j].alive) continue;
            const vec_j: []const f32 = embeddings_flat[j * dim .. (j + 1) * dim];
            const sim: f32 = simdCosinePairF32(vec_i, vec_j);
            if (sim >= similarity_threshold) {
                if (candidates[j].importance > candidates[i].importance) {
                    candidates[i].importance = candidates[j].importance;
                    if (candidates[j].summary.len > 0) {
                        candidates[i].summary = candidates[j].summary;
                    }
                }
                if (candidates[j].last_access_ms > candidates[i].last_access_ms) {
                    candidates[i].last_access_ms = candidates[j].last_access_ms;
                }
                const add_acc: u32 = candidates[j].access_count;
                if (candidates[i].access_count > max_u32 - add_acc) {
                    candidates[i].access_count = max_u32;
                } else {
                    candidates[i].access_count += add_acc;
                }
                candidates[j].alive = false;
                merged += 1;
            }
        }
    }

    return MemoryMergeStats{
        surviving_count: surviving,
        merged_count: merged,
    };
}

// ============================================================================
// 3. Token-Budget Memory Prompt Packing (`packMemoriesForPrompt`)
// ============================================================================

/// Summary returned by `packMemoriesForPrompt`.
pub struct MemoryPackResult {
    packed_count: usize = 0;
    used_tokens: usize = 0;
    bytes_written: usize = 0;
    skipped_oversized: usize = 0;
}

/// Greedily packs priority-ranked memories within `max_tokens` and formats each selected memory
/// as `[mem#<id>] <summary>\n` into caller-provided `out_buf` (zero heap allocation).
///
/// - Skips dead candidates and entries whose `token_count` exceeds remaining token budget.
/// - Returns `null` if any `ranked_hits[i].candidate_index >= candidates.len` or `out_buf` overflows.
pub fn packMemoriesForPrompt(
    ranked_hits: []const MemoryScoredHit,
    candidates: []const MemoryCandidate,
    max_tokens: usize,
    out_buf: *[]u8,
): MemoryPackResult? {
    let res = MemoryPackResult{};
    let pos: usize = 0;

    let i: usize = 0;
    while (i < ranked_hits.len) : (i += 1) {
        const hit = ranked_hits[i];
        if (hit.candidate_index >= candidates.len) return null;
        const cand = candidates[hit.candidate_index];
        if (!cand.alive) continue;

        const tok: usize = cand.token_count as usize;
        const rem_tokens: usize = max_tokens - res.used_tokens;
        if (tok > rem_tokens) {
            res.skipped_oversized += 1;
            continue;
        }

        if (!writeRaw(out_buf, &pos, "[mem#")) return null;
        if (!writeU64Dec(out_buf, &pos, cand.mem_id)) return null;
        if (!writeRaw(out_buf, &pos, "] ")) return null;
        if (!writeRaw(out_buf, &pos, cand.summary)) return null;
        if (!writeByte(out_buf, &pos, '\n')) return null;

        res.packed_count += 1;
        res.used_tokens += tok;
    }

    res.bytes_written = pos;
    return res;
}
