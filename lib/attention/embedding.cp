// Origin: Pure-CP Stdlib (AI Embedding Lookup, Attention-Masked Pooling & Matryoshka Truncation) | Created: 2026-11-22
// Zero-heap-allocation embedding inference primitives covering modern text/multimodal embedding pipelines
// (BGE / E5 / Nomic / OpenAI / Qwen3-Embedding):
//   - Token ID -> dense vector table lookup (`embeddingLookup`)
//   - Attention-masked sequence pooling (`Mean`, `Cls`, `LastToken`, `Max`)
//   - In-place L2 normalization & Matryoshka Representation Learning (MRL) truncation + re-normalization
//   - Batch query-document cosine similarity matrix (`cosineSimilarityMatrix`)

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Sequence pooling strategy for `poolHiddenStates`.
/// - `Mean`: average over active (`mask != 0`) tokens (E5 / Nomic / Sentence-BERT).
/// - `Cls`: first active token representation (BERT / BGE).
/// - `LastToken`: last active (`mask != 0`) token representation (Decoder-only models e.g. Qwen3-Embedding).
/// - `Max`: element-wise maximum over active tokens.
pub enum PoolKind {
    Mean,
    Cls,
    LastToken,
    Max,
}

/// Zero-heap-allocation token embedding table lookup (`token_ids` -> `[token_ids.len, dim]` row-major).
/// Validates every `token_id < vocab_size` and finiteness atomically before writing `out_seq`.
/// Returns `false` on zero `vocab_size`/`dim`, out-of-range `token_id`, undersized slices, or NaN/Inf.
pub fn embeddingLookup(
    table: []const f64,
    vocab_size: usize,
    dim: usize,
    token_ids: []const u32,
    out_seq: *[]f64,
): bool {
    if (vocab_size == 0 || dim == 0) return false;
    if (vocab_size > std.math.maxInt(usize) / dim) return false;
    if (token_ids.len > std.math.maxInt(usize) / dim) return false;

    const table_elems = vocab_size * dim;
    const out_elems = token_ids.len * dim;
    if (table.len < table_elems || out_seq.len < out_elems) return false;

    // Pass 1: validate all token IDs and referenced rows atomically
    let t0: usize = 0;
    while (t0 < token_ids.len) : (t0 += 1) {
        const tid = token_ids[t0] as usize;
        if (tid >= vocab_size) return false;
        const row_base = tid * dim;
        let d0: usize = 0;
        while (d0 < dim) : (d0 += 1) {
            if (!isFiniteF64(table[row_base + d0])) return false;
        }
    }

    // Pass 2: copy embedding rows
    let t: usize = 0;
    while (t < token_ids.len) : (t += 1) {
        const src_base = (token_ids[t] as usize) * dim;
        const dst_base = t * dim;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            out_seq[dst_base + d] = table[src_base + d];
        }
    }

    return true;
}

/// Attention-masked sequence pooling over `[seq_len, dim]` hidden states into `out_vec` (`[dim]`).
/// When `attention_mask` is empty, all tokens are treated as active (`1`).
/// Non-zero mask entries are active; zero entries (`[PAD]`) are excluded.
/// Returns `false` if all mask entries are zero, dimensions are zero, slices are undersized, or NaN/Inf is found.
pub fn poolHiddenStates(
    hidden_seq: []const f64,
    attention_mask: []const u8,
    seq_len: usize,
    dim: usize,
    kind: PoolKind,
    out_vec: *[]f64,
): bool {
    if (seq_len == 0 || dim == 0) return false;
    if (seq_len > std.math.maxInt(usize) / dim) return false;
    const total_elems = seq_len * dim;
    if (hidden_seq.len < total_elems || out_vec.len < dim) return false;

    const use_mask = attention_mask.len > 0;
    if (use_mask && attention_mask.len < seq_len) return false;

    let valid_count: usize = 0;
    let first_valid: usize = 0;
    let last_valid: usize = 0;

    let t: usize = 0;
    while (t < seq_len) : (t += 1) {
        const active = if (use_mask) attention_mask[t] != 0 else true;
        const base = t * dim;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            if (!isFiniteF64(hidden_seq[base + d])) return false;
        }
        if (active) {
            if (valid_count == 0) {
                first_valid = t;
            }
            last_valid = t;
            valid_count += 1;
        }
    }

    if (valid_count == 0) return false;

    switch (kind) {
        PoolKind.Cls => {
            const base = first_valid * dim;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                out_vec[d] = hidden_seq[base + d];
            }
        },
        PoolKind.LastToken => {
            const base = last_valid * dim;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                out_vec[d] = hidden_seq[base + d];
            }
        },
        PoolKind.Mean => {
            let d0: usize = 0;
            while (d0 < dim) : (d0 += 1) {
                out_vec[d0] = 0.0;
            }
            let t1: usize = 0;
            while (t1 < seq_len) : (t1 += 1) {
                if (use_mask && attention_mask[t1] == 0) continue;
                const base = t1 * dim;
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    out_vec[d] += hidden_seq[base + d];
                }
            }
            const inv_cnt = 1.0 / (valid_count as f64);
            let d1: usize = 0;
            while (d1 < dim) : (d1 += 1) {
                out_vec[d1] *= inv_cnt;
            }
        },
        PoolKind.Max => {
            const init_base = first_valid * dim;
            let d0: usize = 0;
            while (d0 < dim) : (d0 += 1) {
                out_vec[d0] = hidden_seq[init_base + d0];
            }
            let t1: usize = first_valid + 1;
            while (t1 < seq_len) : (t1 += 1) {
                if (use_mask && attention_mask[t1] == 0) continue;
                const base = t1 * dim;
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    const v = hidden_seq[base + d];
                    if (v > out_vec[d]) out_vec[d] = v;
                }
            }
        },
    }

    return true;
}

/// Normalizes `vec` in-place to unit L2 norm and returns its original pre-normalization L2 norm.
/// Returns `null` (without mutating `vec`) on empty vector, negative/non-finite `eps`,
/// NaN/Inf elements, or zero/degenerate norm (`norm == 0.0 || norm <= eps`).
pub fn l2NormalizeInPlace(vec: *[]f64, eps: f64): f64? {
    if (vec.len == 0) return null;
    if (!isFiniteF64(eps) || eps < 0.0) return null;

    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < vec.len) : (i += 1) {
        const v = vec[i];
        if (!isFiniteF64(v)) return null;
        sum_sq += v * v;
    }

    const norm = std.math.sqrt(sum_sq);
    if (!(norm > 0.0) || norm <= eps || !isFiniteF64(norm)) return null;

    const inv_norm = 1.0 / norm;
    let j: usize = 0;
    while (j < vec.len) : (j += 1) {
        vec[j] *= inv_norm;
    }

    return norm;
}

/// Matryoshka Representation Learning (MRL) dimension truncation & L2 re-normalization.
/// Truncates `full_vec` to its leading `target_dim <= full_vec.len` dimensions and writes
/// the re-normalized unit vector into `out_vec[0..target_dim]`.
/// Returns `false` if `target_dim == 0`, `target_dim > full_vec.len`, `out_vec.len < target_dim`,
/// the truncated prefix is a zero vector, or any element is NaN/Inf.
pub fn matryoshkaProject(
    full_vec: []const f64,
    target_dim: usize,
    out_vec: *[]f64,
): bool {
    if (target_dim == 0 || target_dim > full_vec.len || out_vec.len < target_dim) return false;

    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < target_dim) : (i += 1) {
        const v = full_vec[i];
        if (!isFiniteF64(v)) return false;
        sum_sq += v * v;
    }

    const norm = std.math.sqrt(sum_sq);
    if (!(norm > 0.0) || !isFiniteF64(norm)) return false;

    const inv_norm = 1.0 / norm;
    let j: usize = 0;
    while (j < target_dim) : (j += 1) {
        out_vec[j] = full_vec[j] * inv_norm;
    }

    return true;
}

/// Computes the `[n_q, n_d]` row-major cosine similarity matrix between `queries` (`[n_q, dim]`)
/// and `docs` (`[n_d, dim]`). Zero-norm vectors yield `0.0` similarity rather than NaN.
/// Returns `false` on zero dimensions, undersized slices, or NaN/Inf inputs.
pub fn cosineSimilarityMatrix(
    queries: []const f64,
    n_q: usize,
    docs: []const f64,
    n_d: usize,
    dim: usize,
    out_mat: *[]f64,
): bool {
    if (n_q == 0 || n_d == 0 || dim == 0) return false;
    if (n_q > std.math.maxInt(usize) / dim) return false;
    if (n_d > std.math.maxInt(usize) / dim) return false;
    if (n_q > std.math.maxInt(usize) / n_d) return false;

    const q_elems = n_q * dim;
    const d_elems = n_d * dim;
    const out_elems = n_q * n_d;
    if (queries.len < q_elems || docs.len < d_elems || out_mat.len < out_elems) return false;

    let iq: usize = 0;
    while (iq < q_elems) : (iq += 1) {
        if (!isFiniteF64(queries[iq])) return false;
    }
    let id: usize = 0;
    while (id < d_elems) : (id += 1) {
        if (!isFiniteF64(docs[id])) return false;
    }

    let qi: usize = 0;
    while (qi < n_q) : (qi += 1) {
        const q_base = qi * dim;
        let q_sq: f64 = 0.0;
        let k0: usize = 0;
        while (k0 < dim) : (k0 += 1) {
            const qv = queries[q_base + k0];
            q_sq += qv * qv;
        }
        const q_norm = std.math.sqrt(q_sq);

        let di: usize = 0;
        while (di < n_d) : (di += 1) {
            if (!(q_norm > 0.0)) {
                out_mat[qi * n_d + di] = 0.0;
                continue;
            }
            const d_base = di * dim;
            let dot: f64 = 0.0;
            let d_sq: f64 = 0.0;
            let k: usize = 0;
            while (k < dim) : (k += 1) {
                const dv = docs[d_base + k];
                dot += queries[q_base + k] * dv;
                d_sq += dv * dv;
            }
            const d_norm = std.math.sqrt(d_sq);
            if (!(d_norm > 0.0)) {
                out_mat[qi * n_d + di] = 0.0;
            } else {
                out_mat[qi * n_d + di] = dot / (q_norm * d_norm);
            }
        }
    }

    return true;
}
