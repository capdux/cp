// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Matryoshka Multi-Resolution Two-Stage SIMD Vector Retrieval) | Created: 2026-11-22
// Pure CP zero-heap-allocation Matryoshka Representation Learning (MRL) multi-resolution
// coarse-to-fine vector retrieval pipeline for modern nested embeddings (OpenAI text-embedding-3,
// Nomic, Jina, Gemini):
//   - Truncation & L2 normalization via 8-lane SIMD `@dot` + `@sqrt` (`truncateAndNormalizeF32`)
//   - Batch compact coarse embedding bank builder using zero-overhead `slice.split` (`buildCoarseEmbeddingsF32`)
//   - Two-stage coarse-to-fine cosine retrieval (`searchMatryoshkaTwoStageF32`) powered by `@dot`
//   - Two-stage coarse-to-fine L2 retrieval (`searchMatryoshkaTwoStageL2F32`) powered by `@l2sq`

const std = @import("std");

// ============================================================================
// Internal Validation & Math Helpers
// ============================================================================

const NORM_SQ_EPS: f32 = 0.000000000001;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clampCosineF32(x: f32): f32 {
    if (x > 1.0) {
        return 1.0;
    }
    if (x < -1.0) {
        return -1.0;
    }
    return x;
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

// ============================================================================
// 1. Configuration, Hit & Statistics Models
// ============================================================================

/// Configuration for two-stage Matryoshka coarse-to-fine vector retrieval.
pub struct MatryoshkaConfig {
    full_dim: usize = 0;
    coarse_dim: usize = 0;
    coarse_top_k: usize = 0;
    final_top_k: usize = 0;

    /// Constructs and validates a `MatryoshkaConfig`.
    /// Requires `0 < coarse_dim <= full_dim` and `0 < final_top_k <= coarse_top_k`.
    static fn init(
        full_dim: usize,
        coarse_dim: usize,
        coarse_top_k: usize,
        final_top_k: usize,
    ): MatryoshkaConfig? {
        const cfg = MatryoshkaConfig{
            full_dim: full_dim,
            coarse_dim: coarse_dim,
            coarse_top_k: coarse_top_k,
            final_top_k: final_top_k,
        };
        if (!cfg.isValid()) {
            return null;
        }
        return cfg;
    }

    /// Validates dimension nesting and Top-K invariants.
    fn isValid(): bool {
        if (_.full_dim == 0 || _.coarse_dim == 0) {
            return false;
        }
        if (_.coarse_dim > _.full_dim) {
            return false;
        }
        if (_.final_top_k == 0 || _.coarse_top_k < _.final_top_k) {
            return false;
        }
        return true;
    }
}

/// Single candidate / final match produced by the Matryoshka two-stage search pipeline.
/// - In cosine mode (`searchMatryoshkaTwoStageF32`), higher `coarse_score` / `fine_score` is better.
/// - In L2 mode (`searchMatryoshkaTwoStageL2F32`), lower `coarse_score` (squared coarse L2) /
///   `fine_score` (exact full-dim L2 distance) is better.
pub struct MatryoshkaHit {
    doc_idx: usize = 0;
    coarse_score: f32 = 0.0;
    fine_score: f32 = 0.0;
}

/// Recall and compute compression statistics returned by two-stage Matryoshka retrieval.
pub struct MatryoshkaStats {
    scanned_docs: usize = 0;
    coarse_candidates: usize = 0;
    returned_hits: usize = 0;
    coarse_dim: usize = 0;
    full_dim: usize = 0;
    coarse_macs: u64 = 0;
    fine_macs: u64 = 0;
    brute_force_macs: u64 = 0;
    compute_speedup_ratio: f32 = 1.0;
}

// ============================================================================
// 2. Matryoshka Truncation & Compact Coarse Bank Construction
// ============================================================================

/// Truncates `full_vec` to its leading `target_dim` dimensions and writes the L2-normalized
/// unit vector into `out_vec[0..target_dim]` using 8-lane SIMD `@dot` + `@sqrt`.
/// Returns `false` on zero `target_dim`, out-of-bounds lengths, non-finite elements, or zero vector.
pub fn truncateAndNormalizeF32(
    full_vec: []const f32,
    target_dim: usize,
    out_vec: *[]f32,
): bool {
    if (target_dim == 0 || full_vec.len < target_dim || out_vec.len < target_dim) {
        return false;
    }
    const [prefix, tail] = full_vec.split(target_dim);
    * = tail;

    if (!allFiniteF32(prefix)) {
        return false;
    }

    const norm_sq: f32 = @dot(prefix, prefix);
    if (!isFiniteF32(norm_sq) || norm_sq <= NORM_SQ_EPS) {
        return false;
    }

    const norm: f32 = @sqrt(norm_sq);
    if (!isFiniteF32(norm) || norm <= 0.0) {
        return false;
    }
    const inv_norm: f32 = 1.0 / norm;
    if (!isFiniteF32(inv_norm)) {
        return false;
    }

    let i: usize = 0;
    while (i < target_dim) : (i += 1) {
        out_vec[i] = prefix[i] * inv_norm;
    }
    return true;
}

/// Batch-builds a compact row-major `[n_docs, coarse_dim]` Matryoshka coarse embedding bank
/// from a flat `[n_docs, full_dim]` corpus by truncating each vector to `coarse_dim` and
/// L2-normalizing it. Uses CP's native `slice.split` for zero-overhead row traversal.
/// Returns `false` on invalid dimensions, overflow, undersized buffers, or any zero/non-finite vector.
pub fn buildCoarseEmbeddingsF32(
    full_vecs_flat: []const f32,
    n_docs: usize,
    full_dim: usize,
    coarse_dim: usize,
    out_coarse_flat: *[]f32,
): bool {
    if (full_dim == 0 || coarse_dim == 0 || coarse_dim > full_dim) {
        return false;
    }
    if (n_docs == 0) {
        return true;
    }
    if (n_docs > std.math.maxInt(usize) / full_dim) {
        return false;
    }
    if (full_vecs_flat.len < n_docs * full_dim || out_coarse_flat.len < n_docs * coarse_dim) {
        return false;
    }

    let in_rem: []const f32 = full_vecs_flat;
    let out_rem: []f32 = out_coarse_flat;
    let doc: usize = 0;
    while (doc < n_docs) : (doc += 1) {
        const [doc_full, next_in] = in_rem.split(full_dim);
        let [doc_coarse, next_out] = out_rem.split(coarse_dim);
        if (!truncateAndNormalizeF32(doc_full, coarse_dim, doc_coarse)) {
            return false;
        }
        in_rem = next_in;
        out_rem = next_out;
    }
    return true;
}

// ============================================================================
// 3. Internal Top-K Insertion Helpers (Zero-Allocation)
// ============================================================================

fn insertCoarseDesc(
    buf: *[]MatryoshkaHit,
    count: *usize,
    limit: usize,
    doc_idx: usize,
    coarse_score: f32,
) {
    if (count.* < limit) {
        let pos: usize = count.*;
        while (pos > 0 && (coarse_score > buf[pos - 1].coarse_score ||
            (coarse_score == buf[pos - 1].coarse_score && doc_idx < buf[pos - 1].doc_idx))) : (pos -= 1)
        {
            buf[pos] = buf[pos - 1];
        }
        buf[pos] = MatryoshkaHit{
            doc_idx: doc_idx,
            coarse_score: coarse_score,
            fine_score: 0.0,
        };
        count.* += 1;
    } else if (coarse_score > buf[limit - 1].coarse_score ||
        (coarse_score == buf[limit - 1].coarse_score && doc_idx < buf[limit - 1].doc_idx))
    {
        let pos: usize = limit - 1;
        while (pos > 0 && (coarse_score > buf[pos - 1].coarse_score ||
            (coarse_score == buf[pos - 1].coarse_score && doc_idx < buf[pos - 1].doc_idx))) : (pos -= 1)
        {
            buf[pos] = buf[pos - 1];
        }
        buf[pos] = MatryoshkaHit{
            doc_idx: doc_idx,
            coarse_score: coarse_score,
            fine_score: 0.0,
        };
    }
}

fn insertFineDesc(
    buf: *[]MatryoshkaHit,
    count: *usize,
    limit: usize,
    doc_idx: usize,
    coarse_score: f32,
    fine_score: f32,
) {
    if (count.* < limit) {
        let pos: usize = count.*;
        while (pos > 0 && (fine_score > buf[pos - 1].fine_score ||
            (fine_score == buf[pos - 1].fine_score && doc_idx < buf[pos - 1].doc_idx))) : (pos -= 1)
        {
            buf[pos] = buf[pos - 1];
        }
        buf[pos] = MatryoshkaHit{
            doc_idx: doc_idx,
            coarse_score: coarse_score,
            fine_score: fine_score,
        };
        count.* += 1;
    } else if (fine_score > buf[limit - 1].fine_score ||
        (fine_score == buf[limit - 1].fine_score && doc_idx < buf[limit - 1].doc_idx))
    {
        let pos: usize = limit - 1;
        while (pos > 0 && (fine_score > buf[pos - 1].fine_score ||
            (fine_score == buf[pos - 1].fine_score && doc_idx < buf[pos - 1].doc_idx))) : (pos -= 1)
        {
            buf[pos] = buf[pos - 1];
        }
        buf[pos] = MatryoshkaHit{
            doc_idx: doc_idx,
            coarse_score: coarse_score,
            fine_score: fine_score,
        };
    }
}

fn insertCoarseAsc(
    buf: *[]MatryoshkaHit,
    count: *usize,
    limit: usize,
    doc_idx: usize,
    coarse_dist_sq: f32,
) {
    if (count.* < limit) {
        let pos: usize = count.*;
        while (pos > 0 && (coarse_dist_sq < buf[pos - 1].coarse_score ||
            (coarse_dist_sq == buf[pos - 1].coarse_score && doc_idx < buf[pos - 1].doc_idx))) : (pos -= 1)
        {
            buf[pos] = buf[pos - 1];
        }
        buf[pos] = MatryoshkaHit{
            doc_idx: doc_idx,
            coarse_score: coarse_dist_sq,
            fine_score: 0.0,
        };
        count.* += 1;
    } else if (coarse_dist_sq < buf[limit - 1].coarse_score ||
        (coarse_dist_sq == buf[limit - 1].coarse_score && doc_idx < buf[limit - 1].doc_idx))
    {
        let pos: usize = limit - 1;
        while (pos > 0 && (coarse_dist_sq < buf[pos - 1].coarse_score ||
            (coarse_dist_sq == buf[pos - 1].coarse_score && doc_idx < buf[pos - 1].doc_idx))) : (pos -= 1)
        {
            buf[pos] = buf[pos - 1];
        }
        buf[pos] = MatryoshkaHit{
            doc_idx: doc_idx,
            coarse_score: coarse_dist_sq,
            fine_score: 0.0,
        };
    }
}

fn insertFineAsc(
    buf: *[]MatryoshkaHit,
    count: *usize,
    limit: usize,
    doc_idx: usize,
    coarse_dist_sq: f32,
    fine_dist_sq: f32,
) {
    if (count.* < limit) {
        let pos: usize = count.*;
        while (pos > 0 && (fine_dist_sq < buf[pos - 1].fine_score ||
            (fine_dist_sq == buf[pos - 1].fine_score && doc_idx < buf[pos - 1].doc_idx))) : (pos -= 1)
        {
            buf[pos] = buf[pos - 1];
        }
        buf[pos] = MatryoshkaHit{
            doc_idx: doc_idx,
            coarse_score: coarse_dist_sq,
            fine_score: fine_dist_sq,
        };
        count.* += 1;
    } else if (fine_dist_sq < buf[limit - 1].fine_score ||
        (fine_dist_sq == buf[limit - 1].fine_score && doc_idx < buf[limit - 1].doc_idx))
    {
        let pos: usize = limit - 1;
        while (pos > 0 && (fine_dist_sq < buf[pos - 1].fine_score ||
            (fine_dist_sq == buf[pos - 1].fine_score && doc_idx < buf[pos - 1].doc_idx))) : (pos -= 1)
        {
            buf[pos] = buf[pos - 1];
        }
        buf[pos] = MatryoshkaHit{
            doc_idx: doc_idx,
            coarse_score: coarse_dist_sq,
            fine_score: fine_dist_sq,
        };
    }
}

fn buildStats(
    n_docs: usize,
    coarse_candidates: usize,
    returned_hits: usize,
    cfg: MatryoshkaConfig,
): MatryoshkaStats {
    const coarse_macs: u64 = (n_docs as u64) * (cfg.coarse_dim as u64);
    const fine_macs: u64 = (coarse_candidates as u64) * (cfg.full_dim as u64);
    const brute_force_macs: u64 = (n_docs as u64) * (cfg.full_dim as u64);
    const two_stage_macs: u64 = coarse_macs + fine_macs;
    const speedup: f32 = if (two_stage_macs > 0) {
        (brute_force_macs as f32) / (two_stage_macs as f32)
    } else {
        1.0
    };
    return MatryoshkaStats{
        scanned_docs: n_docs,
        coarse_candidates: coarse_candidates,
        returned_hits: returned_hits,
        coarse_dim: cfg.coarse_dim,
        full_dim: cfg.full_dim,
        coarse_macs: coarse_macs,
        fine_macs: fine_macs,
        brute_force_macs: brute_force_macs,
        compute_speedup_ratio: speedup,
    };
}

// ============================================================================
// 4. Two-Stage Matryoshka SIMD Cosine & L2 Retrieval Kernels
// ============================================================================

/// Zero-heap-allocation two-stage Matryoshka cosine retrieval:
/// - Stage 1 (Coarse Recall): scans all `n_docs` compact L2-normalized `coarse_dim` vectors
///   in `coarse_vecs_flat` via 8-lane SIMD `@dot` to select the top `min(cfg.coarse_top_k, n_docs)`
///   candidates into `scratch_coarse_hits`.
/// - Stage 2 (Fine Rescore): rescores only those candidates against `full_vecs_flat` at `full_dim`
///   via exact `@dot` cosine similarity and writes the top `min(cfg.final_top_k, coarse_candidates)`
///   sorted descending (tie-broken by ascending `doc_idx`) into `out_final_hits`.
///
/// Supports arbitrary embedding dimensions without stack dimension caps.
/// Returns `null` on invalid config, zero/non-finite query, overflow, or undersized buffers.
pub fn searchMatryoshkaTwoStageF32(
    query_full: []const f32,
    coarse_vecs_flat: []const f32,
    full_vecs_flat: []const f32,
    n_docs: usize,
    cfg: MatryoshkaConfig,
    scratch_coarse_hits: *[]MatryoshkaHit,
    out_final_hits: *[]MatryoshkaHit,
): MatryoshkaStats? {
    if (!cfg.isValid()) {
        return null;
    }
    if (query_full.len != cfg.full_dim) {
        return null;
    }
    if (scratch_coarse_hits.len < cfg.coarse_top_k || out_final_hits.len < cfg.final_top_k) {
        return null;
    }
    if (!allFiniteF32(query_full)) {
        return null;
    }

    const q_full_norm_sq: f32 = @dot(query_full, query_full);
    if (!isFiniteF32(q_full_norm_sq) || q_full_norm_sq <= NORM_SQ_EPS) {
        return null;
    }
    const q_full_inv_norm: f32 = 1.0 / @sqrt(q_full_norm_sq);

    const [q_coarse_raw, q_tail] = query_full.split(cfg.coarse_dim);
    * = q_tail;
    const q_coarse_norm_sq: f32 = @dot(q_coarse_raw, q_coarse_raw);
    if (!isFiniteF32(q_coarse_norm_sq) || q_coarse_norm_sq <= NORM_SQ_EPS) {
        return null;
    }
    const q_coarse_inv_norm: f32 = 1.0 / @sqrt(q_coarse_norm_sq);

    if (n_docs == 0) {
        return buildStats(0, 0, 0, cfg);
    }
    if (n_docs > std.math.maxInt(usize) / cfg.full_dim) {
        return null;
    }
    if (coarse_vecs_flat.len < n_docs * cfg.coarse_dim || full_vecs_flat.len < n_docs * cfg.full_dim) {
        return null;
    }

    // Stage 1: Coarse scan over compact [n_docs, coarse_dim] using slice.split + 8-lane SIMD @dot
    let coarse_limit: usize = cfg.coarse_top_k;
    if (n_docs < coarse_limit) {
        coarse_limit = n_docs;
    }

    let coarse_count: usize = 0;
    let coarse_rem: []const f32 = coarse_vecs_flat;
    let doc_idx: usize = 0;
    while (doc_idx < n_docs) : (doc_idx += 1) {
        const [doc_coarse, next_coarse] = coarse_rem.split(cfg.coarse_dim);
        coarse_rem = next_coarse;

        const raw_dot: f32 = @dot(q_coarse_raw, doc_coarse);
        if (!isFiniteF32(raw_dot)) {
            return null;
        }
        const c_score: f32 = clampCosineF32(raw_dot * q_coarse_inv_norm);
        insertCoarseDesc(scratch_coarse_hits, *coarse_count, coarse_limit, doc_idx, c_score);
    }

    // Stage 2: Exact full-dim cosine rescore over coarse_count candidates only
    let final_limit: usize = cfg.final_top_k;
    if (coarse_count < final_limit) {
        final_limit = coarse_count;
    }

    let final_count: usize = 0;
    let c: usize = 0;
    while (c < coarse_count) : (c += 1) {
        const cand = scratch_coarse_hits[c];
        const base: usize = cand.doc_idx * cfg.full_dim;
        const doc_full = full_vecs_flat[base .. base + cfg.full_dim];

        const d_norm_sq: f32 = @dot(doc_full, doc_full);
        const ab: f32 = @dot(query_full, doc_full);
        if (!isFiniteF32(d_norm_sq) || !isFiniteF32(ab)) {
            return null;
        }

        let f_score: f32 = 0.0;
        if (d_norm_sq > NORM_SQ_EPS) {
            const denom: f32 = @sqrt(d_norm_sq);
            if (denom > 0.0 && isFiniteF32(denom)) {
                f_score = clampCosineF32((ab * q_full_inv_norm) / denom);
            }
        }
        insertFineDesc(out_final_hits, *final_count, final_limit, cand.doc_idx, cand.coarse_score, f_score);
    }

    return buildStats(n_docs, coarse_count, final_count, cfg);
}

/// Zero-heap-allocation two-stage Matryoshka L2 nearest-neighbor retrieval powered by `@l2sq`:
/// - Stage 1: scans all `n_docs` compact `coarse_dim` vectors via `@l2sq` (comparing prefix
///   `query_full[0..coarse_dim]` against `coarse_vecs_flat`) to pick `coarse_top_k` nearest candidates.
/// - Stage 2: rescores candidates at `full_dim` via `@l2sq` and applies `@sqrt` only to the final
///   `final_top_k` winners, sorted ascending by exact L2 distance (tie-broken by ascending `doc_idx`).
pub fn searchMatryoshkaTwoStageL2F32(
    query_full: []const f32,
    coarse_vecs_flat: []const f32,
    full_vecs_flat: []const f32,
    n_docs: usize,
    cfg: MatryoshkaConfig,
    scratch_coarse_hits: *[]MatryoshkaHit,
    out_final_hits: *[]MatryoshkaHit,
): MatryoshkaStats? {
    if (!cfg.isValid()) {
        return null;
    }
    if (query_full.len != cfg.full_dim) {
        return null;
    }
    if (scratch_coarse_hits.len < cfg.coarse_top_k || out_final_hits.len < cfg.final_top_k) {
        return null;
    }
    if (!allFiniteF32(query_full)) {
        return null;
    }

    const [q_coarse, q_tail] = query_full.split(cfg.coarse_dim);
    * = q_tail;

    if (n_docs == 0) {
        return buildStats(0, 0, 0, cfg);
    }
    if (n_docs > std.math.maxInt(usize) / cfg.full_dim) {
        return null;
    }
    if (coarse_vecs_flat.len < n_docs * cfg.coarse_dim || full_vecs_flat.len < n_docs * cfg.full_dim) {
        return null;
    }

    // Stage 1: Coarse scan via 8-lane SIMD @l2sq
    let coarse_limit: usize = cfg.coarse_top_k;
    if (n_docs < coarse_limit) {
        coarse_limit = n_docs;
    }

    let coarse_count: usize = 0;
    let coarse_rem: []const f32 = coarse_vecs_flat;
    let doc_idx: usize = 0;
    while (doc_idx < n_docs) : (doc_idx += 1) {
        const [doc_coarse, next_coarse] = coarse_rem.split(cfg.coarse_dim);
        coarse_rem = next_coarse;

        let d2: f32 = @l2sq(q_coarse, doc_coarse);
        if (!isFiniteF32(d2)) {
            return null;
        }
        if (d2 < 0.0) {
            d2 = 0.0;
        }
        insertCoarseAsc(scratch_coarse_hits, *coarse_count, coarse_limit, doc_idx, d2);
    }

    // Stage 2: Exact full-dim @l2sq rescore on coarse candidates only
    let final_limit: usize = cfg.final_top_k;
    if (coarse_count < final_limit) {
        final_limit = coarse_count;
    }

    let final_count: usize = 0;
    let c: usize = 0;
    while (c < coarse_count) : (c += 1) {
        const cand = scratch_coarse_hits[c];
        const base: usize = cand.doc_idx * cfg.full_dim;
        const doc_full = full_vecs_flat[base .. base + cfg.full_dim];

        let d2: f32 = @l2sq(query_full, doc_full);
        if (!isFiniteF32(d2)) {
            return null;
        }
        if (d2 < 0.0) {
            d2 = 0.0;
        }
        insertFineAsc(out_final_hits, *final_count, final_limit, cand.doc_idx, cand.coarse_score, d2);
    }

    // Apply @sqrt only to the final Top-K winners
    let f: usize = 0;
    while (f < final_count) : (f += 1) {
        out_final_hits[f].fine_score = @sqrt(out_final_hits[f].fine_score);
    }

    return buildStats(n_docs, coarse_count, final_count, cfg);
}
