// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Late Chunking & Contextual Token-Span Pooling Engine using @dot/@fma) | Created: 2026-11-22
// Pure CP zero-heap-allocation Late Chunking engine for long-context RAG:
//   - Pools full-document contextual token embeddings `[num_tokens, dim]` over semantic spans `[start, end)`
//     so every chunk retains cross-chunk coreference and long-range discourse context.
//   - 3 pooling modes: `mean_pool`, `weighted_attention_pool`, `max_norm_pool` powered by native `@dot` / `@fma`.
//   - Automatic semantic boundary detection (`detectSemanticBoundaries`) via adjacent token cosine similarity drops.
//   - Zero-allocation in-document Top-K retrieval (`searchLateChunks`) with deterministic tie-breaking.
//   - Full defensive guards: invalid dimensions, empty inputs, out-of-range spans, NaN/Inf sanitization,
//     zero-vector safe L2 normalization, and undersized output buffer protection.

const std = @import("std");

// ============================================================================
// Internal Validation & SIMD-Accelerated Math Helpers
// ============================================================================

const NORM_SQ_EPS: f32 = 1.0e-12;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn sanitizeF32(x: f32): f32 {
    if (isFiniteF32(x)) {
        return x;
    }
    return 0.0;
}

fn clampCosineF32(x: f32): f32 {
    if (!isFiniteF32(x)) {
        return 0.0;
    }
    if (x > 1.0) {
        return 1.0;
    }
    if (x < -1.0) {
        return -1.0;
    }
    return x;
}

/// Fast-path 8-lane SIMD `@dot` with automatic NaN/Inf sanitization fallback.
fn safeDotF32(a: []const f32, b: []const f32): f32 {
    const fast: f32 = @dot(a, b);
    if (isFiniteF32(fast)) {
        return fast;
    }
    let acc: f32 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const va = sanitizeF32(a[i]);
        const vb = sanitizeF32(b[i]);
        acc = @fma(va, vb, acc);
    }
    if (!isFiniteF32(acc)) {
        return 0.0;
    }
    return acc;
}

/// Fast-path 8-lane SIMD squared L2 norm with NaN/Inf sanitization fallback.
fn safeNormSqF32(v: []const f32): f32 {
    const fast: f32 = @dot(v, v);
    if (isFiniteF32(fast) && fast >= 0.0) {
        return fast;
    }
    let acc: f32 = 0.0;
    let i: usize = 0;
    while (i < v.len) : (i += 1) {
        const x = sanitizeF32(v[i]);
        acc = @fma(x, x, acc);
    }
    if (!isFiniteF32(acc) || acc < 0.0) {
        return 0.0;
    }
    return acc;
}

/// Safe SIMD cosine similarity in `[-1.0, 1.0]`. Returns `0.0` for zero vectors.
fn safeCosineF32(a: []const f32, b: []const f32): f32 {
    const na = safeNormSqF32(a);
    const nb = safeNormSqF32(b);
    if (na <= NORM_SQ_EPS || nb <= NORM_SQ_EPS) {
        return 0.0;
    }
    const denom: f32 = @sqrt(na) * @sqrt(nb);
    if (!isFiniteF32(denom) || denom <= 0.0) {
        return 0.0;
    }
    const ab = safeDotF32(a, b);
    return clampCosineF32(ab / denom);
}

fn checkMatrixBounds(num_rows: usize, dim: usize, flat_len: usize): bool {
    if (dim == 0) {
        return false;
    }
    if (num_rows == 0) {
        return true;
    }
    if (num_rows > std.math.maxInt(usize) / dim) {
        return false;
    }
    return flat_len >= num_rows * dim;
}

// ============================================================================
// 1. Data Models & Pooling Modes
// ============================================================================

/// Token-span pooling strategy for Late Chunking.
pub enum LateChunkPoolMode {
    mean_pool,
    weighted_attention_pool,
    max_norm_pool,
}

/// Half-open token interval `[start_token, end_token)` representing one late chunk.
pub struct LateChunkSpan {
    start_token: usize = 0;
    end_token: usize = 0;
    boundary_sim: f32 = 1.0;

    /// Number of tokens covered by this span (`0` if empty or inverted).
    fn tokenCount(): usize {
        if (_.end_token <= _.start_token) {
            return 0;
        }
        return _.end_token - _.start_token;
    }

    /// Returns `true` iff `start_token < end_token <= num_tokens`.
    fn isValid(num_tokens: usize): bool {
        return _.start_token < _.end_token && _.end_token <= num_tokens;
    }
}

/// Single Top-K match produced by in-document `searchLateChunks`.
pub struct LateChunkHit {
    chunk_idx: usize = 0;
    score: f32 = 0.0;
}

/// Summary statistics for a batch of late-chunked spans and their pooled vectors.
pub struct LateChunkStats {
    num_tokens: usize = 0;
    num_chunks: usize = 0;
    dim: usize = 0;
    min_chunk_len: usize = 0;
    max_chunk_len: usize = 0;
    zero_norm_chunks: usize = 0;
}

// ============================================================================
// 2. Sanitization & Safe L2 Normalization Primitives
// ============================================================================

/// Copies `src` into `dst[0..src.len]`, replacing any `NaN`/`Inf` with `0.0`.
/// Returns the number of sanitized non-finite elements, or `null` if `dst.len < src.len`.
pub fn sanitizeVectorF32(src: []const f32, dst: *[]f32): usize? {
    if (dst.len < src.len) {
        return null;
    }
    let cleaned: usize = 0;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        if (isFiniteF32(src[i])) {
            dst[i] = src[i];
        } else {
            dst[i] = 0.0;
            cleaned += 1;
        }
    }
    return cleaned;
}

/// Sanitizes and L2-normalizes `vec` in-place using native `@dot` + `@sqrt`.
/// Returns `true` if normalized to unit length; returns `false` (leaving a safe zero vector)
/// if `vec` is empty or has near-zero norm.
pub fn normalizeVectorInPlaceF32(vec: *[]f32): bool {
    if (vec.len == 0) {
        return false;
    }
    let norm_sq: f32 = @dot(vec[0..vec.len], vec[0..vec.len]);
    if (!isFiniteF32(norm_sq)) {
        let i: usize = 0;
        while (i < vec.len) : (i += 1) {
            vec[i] = sanitizeF32(vec[i]);
        }
        norm_sq = @dot(vec[0..vec.len], vec[0..vec.len]);
    }
    if (!isFiniteF32(norm_sq) || norm_sq <= NORM_SQ_EPS) {
        let i: usize = 0;
        while (i < vec.len) : (i += 1) {
            vec[i] = 0.0;
        }
        return false;
    }
    const norm: f32 = @sqrt(norm_sq);
    if (!isFiniteF32(norm) || norm <= 0.0) {
        let i: usize = 0;
        while (i < vec.len) : (i += 1) {
            vec[i] = 0.0;
        }
        return false;
    }
    const inv_norm: f32 = 1.0 / norm;
    if (!isFiniteF32(inv_norm)) {
        let i: usize = 0;
        while (i < vec.len) : (i += 1) {
            vec[i] = 0.0;
        }
        return false;
    }
    let i: usize = 0;
    while (i < vec.len) : (i += 1) {
        vec[i] = vec[i] * inv_norm;
    }
    return true;
}

// ============================================================================
// 3. Semantic Boundary Detection via Adjacent Token Cosine Drops
// ============================================================================

/// Automatically detects semantic chunk boundaries over a contextual token embedding matrix
/// `[num_tokens, dim]` by computing adjacent token cosine similarity with `@dot` and splitting
/// at semantic valleys where `cos(tok[t-1], tok[t]) < drop_threshold` (subject to
/// `[min_chunk_tokens, max_chunk_tokens]` length bounds).
///
/// Returns the number of emitted `LateChunkSpan` entries written to `out_spans`, or `null` on
/// invalid parameters (`dim == 0`, invalid length bounds, non-finite `drop_threshold`,
/// undersized `token_embeddings` or `out_spans`).
pub fn detectSemanticBoundaries(
    token_embeddings: []const f32,
    num_tokens: usize,
    dim: usize,
    min_chunk_tokens: usize,
    max_chunk_tokens: usize,
    drop_threshold: f32,
    out_spans: *[]LateChunkSpan,
): usize? {
    if (dim == 0 || min_chunk_tokens == 0 || max_chunk_tokens == 0) {
        return null;
    }
    if (min_chunk_tokens > max_chunk_tokens) {
        return null;
    }
    if (!isFiniteF32(drop_threshold)) {
        return null;
    }
    if (!checkMatrixBounds(num_tokens, dim, token_embeddings.len)) {
        return null;
    }
    if (num_tokens == 0) {
        return 0;
    }
    if (out_spans.len == 0) {
        return null;
    }

    let count: usize = 0;
    let chunk_start: usize = 0;
    let t: usize = 1;
    while (t < num_tokens) : (t += 1) {
        const cur_len = t - chunk_start;
        const prev_tok = token_embeddings[(t - 1) * dim .. t * dim];
        const curr_tok = token_embeddings[t * dim .. (t + 1) * dim];
        const sim = safeCosineF32(prev_tok, curr_tok);

        const hit_max = cur_len >= max_chunk_tokens;
        const hit_valley = cur_len >= min_chunk_tokens && sim < drop_threshold;
        if (hit_max || hit_valley) {
            if (count >= out_spans.len) {
                return null;
            }
            out_spans[count] = LateChunkSpan{
                start_token: chunk_start,
                end_token: t,
                boundary_sim: sim,
            };
            count += 1;
            chunk_start = t;
        }
    }

    if (chunk_start < num_tokens) {
        if (count >= out_spans.len) {
            return null;
        }
        out_spans[count] = LateChunkSpan{
            start_token: chunk_start,
            end_token: num_tokens,
            boundary_sim: 1.0,
        };
        count += 1;
    }

    return count;
}

// ============================================================================
// 4. Contextual Token-Span Late Pooling (Single Span & Batch)
// ============================================================================

/// Pools a single contextual token span `[span.start_token, span.end_token)` from
/// `token_embeddings` (`[num_tokens, dim]`) into `out_vec[0..dim]` using the requested
/// `LateChunkPoolMode` (`mean_pool`, `weighted_attention_pool`, or `max_norm_pool`).
///
/// - In `weighted_attention_pool` mode:
///   - If `token_weights.len >= num_tokens`, uses `max(0.0, sanitize(token_weights[t]))`.
///   - If `token_weights.len == 0`, automatically derives L2-norm salience weights via `@dot` + `@sqrt`.
///   - Any other non-empty `token_weights.len < num_tokens` returns `false`.
/// - When `l2_normalize` is `true`, L2-normalizes the pooled chunk via `@dot` + `@sqrt`
///   with zero-vector safety (zero vectors remain zero without producing `NaN`).
pub fn poolLateChunkSpan(
    token_embeddings: []const f32,
    num_tokens: usize,
    dim: usize,
    span: LateChunkSpan,
    mode: LateChunkPoolMode,
    token_weights: []const f32,
    l2_normalize: bool,
    out_vec: *[]f32,
): bool {
    if (dim == 0 || num_tokens == 0 || out_vec.len < dim) {
        return false;
    }
    if (!checkMatrixBounds(num_tokens, dim, token_embeddings.len)) {
        return false;
    }
    if (!span.isValid(num_tokens)) {
        return false;
    }

    let d: usize = 0;
    while (d < dim) : (d += 1) {
        out_vec[d] = 0.0;
    }

    const span_len = span.tokenCount();

    if (mode == LateChunkPoolMode.mean_pool) {
        const inv_len: f32 = 1.0 / (span_len as f32);
        let t: usize = span.start_token;
        while (t < span.end_token) : (t += 1) {
            const tok = token_embeddings[t * dim .. (t + 1) * dim];
            d = 0;
            while (d < dim) : (d += 1) {
                out_vec[d] = @fma(inv_len, sanitizeF32(tok[d]), out_vec[d]);
            }
        }
    } else if (mode == LateChunkPoolMode.weighted_attention_pool) {
        const use_external = token_weights.len > 0;
        if (use_external && token_weights.len < num_tokens) {
            return false;
        }

        let weight_sum: f32 = 0.0;
        let t: usize = span.start_token;
        while (t < span.end_token) : (t += 1) {
            let w: f32 = 0.0;
            if (use_external) {
                const raw_w = sanitizeF32(token_weights[t]);
                w = if (raw_w > 0.0) raw_w else 0.0;
            } else {
                const tok = token_embeddings[t * dim .. (t + 1) * dim];
                const nsq = safeNormSqF32(tok);
                w = if (nsq > NORM_SQ_EPS) @sqrt(nsq) else 0.0;
            }
            weight_sum += w;
        }

        if (isFiniteF32(weight_sum) && weight_sum > NORM_SQ_EPS) {
            const inv_w_sum: f32 = 1.0 / weight_sum;
            t = span.start_token;
            while (t < span.end_token) : (t += 1) {
                const tok = token_embeddings[t * dim .. (t + 1) * dim];
                let w: f32 = 0.0;
                if (use_external) {
                    const raw_w = sanitizeF32(token_weights[t]);
                    w = if (raw_w > 0.0) raw_w else 0.0;
                } else {
                    const nsq = safeNormSqF32(tok);
                    w = if (nsq > NORM_SQ_EPS) @sqrt(nsq) else 0.0;
                }
                const norm_w = w * inv_w_sum;
                if (norm_w > 0.0) {
                    d = 0;
                    while (d < dim) : (d += 1) {
                        out_vec[d] = @fma(norm_w, sanitizeF32(tok[d]), out_vec[d]);
                    }
                }
            }
        }
    } else {
        // LateChunkPoolMode.max_norm_pool
        let best_t: usize = span.start_token;
        let best_norm_sq: f32 = -1.0;
        let t: usize = span.start_token;
        while (t < span.end_token) : (t += 1) {
            const tok = token_embeddings[t * dim .. (t + 1) * dim];
            const nsq = safeNormSqF32(tok);
            if (nsq > best_norm_sq) {
                best_norm_sq = nsq;
                best_t = t;
            }
        }
        const best_tok = token_embeddings[best_t * dim .. (best_t + 1) * dim];
        d = 0;
        while (d < dim) : (d += 1) {
            out_vec[d] = sanitizeF32(best_tok[d]);
        }
    }

    if (l2_normalize) {
        let out_head: []f32 = out_vec[0..dim];
        * = normalizeVectorInPlaceF32(*out_head);
    }

    return true;
}

/// Batch-pools all `spans` from `token_embeddings` (`[num_tokens, dim]`) into flat row-major
/// `out_chunk_vecs` (`[spans.len, dim]`).
/// Returns the number of pooled chunks (`spans.len`), or `null` if any validation or span guard fails.
pub fn poolLateChunks(
    token_embeddings: []const f32,
    num_tokens: usize,
    dim: usize,
    spans: []const LateChunkSpan,
    mode: LateChunkPoolMode,
    token_weights: []const f32,
    l2_normalize: bool,
    out_chunk_vecs: *[]f32,
): usize? {
    if (dim == 0) {
        return null;
    }
    if (!checkMatrixBounds(num_tokens, dim, token_embeddings.len)) {
        return null;
    }
    if (spans.len == 0) {
        return 0;
    }
    if (num_tokens == 0) {
        return null;
    }
    if (!checkMatrixBounds(spans.len, dim, out_chunk_vecs.len)) {
        return null;
    }
    if (mode == LateChunkPoolMode.weighted_attention_pool && token_weights.len > 0 && token_weights.len < num_tokens) {
        return null;
    }

    let c: usize = 0;
    while (c < spans.len) : (c += 1) {
        let dst_row: []f32 = out_chunk_vecs[c * dim .. (c + 1) * dim];
        const ok = poolLateChunkSpan(
            token_embeddings,
            num_tokens,
            dim,
            spans[c],
            mode,
            token_weights,
            l2_normalize,
            *dst_row,
        );
        if (!ok) {
            return null;
        }
    }

    return spans.len;
}

// ============================================================================
// 5. In-Document Top-K Query Matching Over Late-Chunked Vectors
// ============================================================================

/// Performs zero-allocation Top-K cosine similarity retrieval for `query_vec` over
/// `num_chunks` pooled late-chunk embeddings (`chunk_vecs`, row-major `[num_chunks, dim]`).
///
/// Writes up to `min(num_chunks, out_hits.len)` hits into `out_hits`, sorted descending by
/// `score` (tie-broken by ascending `chunk_idx`).
/// Returns `null` on invalid dimension, buffer overflow, undersized `chunk_vecs`, or empty
/// `out_hits` when `num_chunks > 0`.
pub fn searchLateChunks(
    query_vec: []const f32,
    chunk_vecs: []const f32,
    num_chunks: usize,
    dim: usize,
    out_hits: *[]LateChunkHit,
): usize? {
    if (dim == 0 || query_vec.len != dim) {
        return null;
    }
    if (!checkMatrixBounds(num_chunks, dim, chunk_vecs.len)) {
        return null;
    }
    if (num_chunks == 0) {
        return 0;
    }
    if (out_hits.len == 0) {
        return null;
    }

    const q_norm_sq = safeNormSqF32(query_vec);
    const q_norm: f32 = if (q_norm_sq > NORM_SQ_EPS) @sqrt(q_norm_sq) else 0.0;

    let limit: usize = out_hits.len;
    if (num_chunks < limit) {
        limit = num_chunks;
    }

    let count: usize = 0;
    let c: usize = 0;
    while (c < num_chunks) : (c += 1) {
        const chunk_row = chunk_vecs[c * dim .. (c + 1) * dim];
        let score: f32 = 0.0;
        if (q_norm > 0.0) {
            const c_norm_sq = safeNormSqF32(chunk_row);
            if (c_norm_sq > NORM_SQ_EPS) {
                const denom = q_norm * @sqrt(c_norm_sq);
                if (isFiniteF32(denom) && denom > 0.0) {
                    score = clampCosineF32(safeDotF32(query_vec, chunk_row) / denom);
                }
            }
        }

        // Maintain sorted Top-K descending by score, tie-breaking by ascending chunk_idx.
        let insert_pos: usize = count;
        while (insert_pos > 0) {
            const prev = out_hits[insert_pos - 1];
            const better = (score > prev.score) || (score == prev.score && c < prev.chunk_idx);
            if (!better) {
                break;
            }
            if (insert_pos < limit) {
                out_hits[insert_pos] = prev;
            }
            insert_pos -= 1;
        }

        if (insert_pos < limit) {
            out_hits[insert_pos] = LateChunkHit{
                chunk_idx: c,
                score: score,
            };
            if (count < limit) {
                count += 1;
            }
        }
    }

    return count;
}

// ============================================================================
// 6. Diagnostics & Summary Statistics
// ============================================================================

/// Computes summary statistics (min/max chunk length, zero-norm chunk count) over a pooled
/// late-chunking output. Returns `null` on invalid bounds.
pub fn computeLateChunkStats(
    spans: []const LateChunkSpan,
    num_tokens: usize,
    dim: usize,
    chunk_vecs: []const f32,
): LateChunkStats? {
    if (dim == 0 || !checkMatrixBounds(spans.len, dim, chunk_vecs.len)) {
        return null;
    }
    let stats = LateChunkStats{
        num_tokens: num_tokens,
        num_chunks: spans.len,
        dim: dim,
        min_chunk_len: 0,
        max_chunk_len: 0,
        zero_norm_chunks: 0,
    };
    if (spans.len == 0) {
        return stats;
    }

    stats.min_chunk_len = std.math.maxInt(usize);
    let c: usize = 0;
    while (c < spans.len) : (c += 1) {
        if (!spans[c].isValid(num_tokens)) {
            return null;
        }
        const len = spans[c].tokenCount();
        if (len < stats.min_chunk_len) {
            stats.min_chunk_len = len;
        }
        if (len > stats.max_chunk_len) {
            stats.max_chunk_len = len;
        }
        const row = chunk_vecs[c * dim .. (c + 1) * dim];
        if (safeNormSqF32(row) <= NORM_SQ_EPS) {
            stats.zero_norm_chunks += 1;
        }
    }
    return stats;
}
