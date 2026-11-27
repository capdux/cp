// Origin: Pure-CP Stdlib (Zero-Alloc Bipartite Soft Matching Token Merging — ToMe) | Created: 2026-11-23
// Pure CP zero-heap-allocation Token Merging (ToMe: Bolya et al.) engine for multimodal & long-context
// Transformer acceleration:
//   - Classic Bipartite Soft Matching: splits mergeable tokens into even/odd sets `A` and `B` while
//     preserving `protect_prefix` CLS/system prefix tokens untouched; computes cosine similarity via
//     CP native SIMD `@dot` + `@sqrt`
//   - Greedy Top-`r` weighted token fusion (`mergeTokensF32`) with optional `min_similarity` floor,
//     many-to-one weighted averaging `x_new = sum(s_i * x_i) / sum(s_i)` and cumulative token sizes
//     `s_new = sum(s_i)`, producing compressed tokens, updated sizes, and `merge_map`
//   - Proportional Attention log-size bias helper (`computeProportionalLogBias`: `log(size)`) &
//     zero-alloc inverse unmerging (`unmergeTokensF32`) back to original sequence length
//   - Full numerical hygiene (NaN/Inf/zero-norm/size/dimension guards) & compression telemetry report

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum sequence length supported in a single zero-heap ToMe pass.
pub const TOME_MAX_TOKENS: usize = 256;

/// Maximum hidden dimension supported per token (`1..64`).
pub const TOME_MAX_DIM: usize = 64;

/// Permille scale (1000 = 100.0%) for sequence compression rate telemetry.
pub const TOME_PERMILLE_SCALE: u32 = 1000;

/// Status returned by `mergeTokensF32` and `unmergeTokensF32`.
pub enum TokenMergeStatus {
    ok,
    invalid_input,
    capacity_exceeded,
    buffer_too_small,
}

/// Configuration for a single Bipartite Soft Matching ToMe layer.
pub struct TokenMergeConfig {
    r: usize = 0;
    protect_prefix: usize = 1;
    min_similarity: f32 = -1.0;

    static fn make(r: usize, protect_prefix: usize, min_similarity: f32): TokenMergeConfig {
        return TokenMergeConfig{
            r: r,
            protect_prefix: protect_prefix,
            min_similarity: min_similarity,
        };
    }

    pub fn isValid(): bool {
        if (!isFiniteF32(min_similarity)) return false;
        if (min_similarity < -1.0 or min_similarity > 1.0) return false;
        return true;
    }
}

/// Telemetry & outcome report returned by `mergeTokensF32`.
pub struct TokenMergeReport {
    status: TokenMergeStatus = TokenMergeStatus.invalid_input;
    input_tokens: usize = 0;
    output_tokens: usize = 0;
    merged_pairs: usize = 0;
    protected_tokens: usize = 0;
    compression_permille: u32 = 0;
    avg_merge_sim: f32 = 0.0;
    min_merge_sim: f32 = 0.0;
}

// ============================================================================
// 2. Internal Numerical Hygiene Helpers
// ============================================================================

fn isFiniteF32(v: f32): bool {
    if (v != v) return false;
    if (v > 3.4e38 or v < -3.4e38) return false;
    return true;
}

fn clampCosineF32(v: f32): f32 {
    if (v > 1.0) return 1.0;
    if (v < -1.0) return -1.0;
    return v;
}

// ============================================================================
// 3. Core Bipartite Soft Matching Token Merging, Log Bias & Unmerging
// ============================================================================

/// Perform zero-heap Bipartite Soft Matching Token Merging (ToMe) on `tokens[seq_len * dim]`.
///   - `sizes`: per-token positive weights (`sizes.len == seq_len`), or empty slice to default all to `1.0`
///   - `out_tokens`: receives `report.output_tokens * dim` fused features
///   - `out_sizes`: receives `report.output_tokens` updated cumulative token sizes
///   - `out_merge_map`: receives `seq_len` indices mapping each original token -> output token index
pub fn mergeTokensF32(
    tokens: []const f32,
    sizes: []const f32,
    seq_len: usize,
    dim: usize,
    cfg: TokenMergeConfig,
    out_tokens: *[]f32,
    out_sizes: *[]f32,
    out_merge_map: *[]usize,
): TokenMergeReport {
    if (seq_len == 0 or dim == 0 or !cfg.isValid()) {
        return TokenMergeReport{ status: TokenMergeStatus.invalid_input };
    }
    if (seq_len > TOME_MAX_TOKENS or dim > TOME_MAX_DIM) {
        return TokenMergeReport{ status: TokenMergeStatus.capacity_exceeded };
    }
    if (tokens.len != seq_len * dim) {
        return TokenMergeReport{ status: TokenMergeStatus.invalid_input };
    }
    if (sizes.len != 0 and sizes.len != seq_len) {
        return TokenMergeReport{ status: TokenMergeStatus.invalid_input };
    }
    if (out_merge_map.len < seq_len) {
        return TokenMergeReport{ status: TokenMergeStatus.buffer_too_small };
    }

    // Validate token values and weights, and precompute inverse L2 norms via SIMD `@dot`.
    let inv_norms: [TOME_MAX_TOKENS]f32 = undefined;
    let eff_sizes: [TOME_MAX_TOKENS]f32 = [1.0; TOME_MAX_TOKENS];

    let t: usize = 0;
    while (t < seq_len) : (t += 1) {
        if (sizes.len > 0) {
            const w = sizes[t];
            if (!isFiniteF32(w) or w <= 0.0) {
                return TokenMergeReport{ status: TokenMergeStatus.invalid_input };
            }
            eff_sizes[t] = w;
        }
        const row = tokens[t * dim .. (t + 1) * dim];
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            if (!isFiniteF32(row[d])) {
                return TokenMergeReport{ status: TokenMergeStatus.invalid_input };
            }
        }
        const norm_sq: f32 = @dot(row, row);
        if (!isFiniteF32(norm_sq)) {
            return TokenMergeReport{ status: TokenMergeStatus.invalid_input };
        }
        if (norm_sq > 0.000000000001) {
            inv_norms[t] = 1.0 / @sqrt(norm_sq);
        } else {
            inv_norms[t] = 0.0;
        }
    }

    const prot = if (cfg.protect_prefix < seq_len) cfg.protect_prefix else seq_len;
    const mergeable = seq_len - prot;

    // Partition mergeable tokens into even (set A) and odd (set B).
    let set_a: [TOME_MAX_TOKENS]usize = undefined;
    let set_b: [TOME_MAX_TOKENS]usize = undefined;
    let num_a: usize = 0;
    let num_b: usize = 0;

    let m_i: usize = 0;
    while (m_i < mergeable) : (m_i += 1) {
        if ((m_i % 2) == 0) {
            set_a[num_a] = prot + m_i;
            num_a += 1;
        } else {
            set_b[num_b] = prot + m_i;
            num_b += 1;
        }
    }

    // Fast path: nothing to merge -> identity copy.
    if (cfg.r == 0 or num_a == 0 or num_b == 0) {
        if (out_tokens.len < seq_len * dim or out_sizes.len < seq_len) {
            return TokenMergeReport{ status: TokenMergeStatus.buffer_too_small };
        }
        let i: usize = 0;
        while (i < seq_len * dim) : (i += 1) {
            out_tokens[i] = tokens[i];
        }
        let idx: usize = 0;
        while (idx < seq_len) : (idx += 1) {
            out_sizes[idx] = eff_sizes[idx];
            out_merge_map[idx] = idx;
        }
        return TokenMergeReport{
            status: TokenMergeStatus.ok,
            input_tokens: seq_len,
            output_tokens: seq_len,
            merged_pairs: 0,
            protected_tokens: prot,
            compression_permille: 0,
            avg_merge_sim: 0.0,
            min_merge_sim: 0.0,
        };
    }

    // 1. Bipartite Soft Matching: for each a in A, find argmax_{b in B} cos(a, b) via SIMD `@dot`.
    let best_b_idx: [TOME_MAX_TOKENS]usize = undefined;
    let best_sim: [TOME_MAX_TOKENS]f32 = undefined;
    let order: [TOME_MAX_TOKENS]usize = undefined;

    let ai: usize = 0;
    while (ai < num_a) : (ai += 1) {
        const a_tok = set_a[ai];
        const a_row = tokens[a_tok * dim .. (a_tok + 1) * dim];
        const a_inv = inv_norms[a_tok];

        let max_s: f32 = -2.0;
        let max_bi: usize = 0;

        let bi: usize = 0;
        while (bi < num_b) : (bi += 1) {
            const b_tok = set_b[bi];
            const b_row = tokens[b_tok * dim .. (b_tok + 1) * dim];
            const b_inv = inv_norms[b_tok];
            const sim: f32 = if (a_inv == 0.0 or b_inv == 0.0)
                0.0
            else
                clampCosineF32(@dot(a_row, b_row) * a_inv * b_inv);
            if (sim > max_s) {
                max_s = sim;
                max_bi = bi;
            }
        }

        best_b_idx[ai] = max_bi;
        best_sim[ai] = max_s;
        order[ai] = ai;
    }

    // 2. Stable insertion sort of A indices in descending order of `best_sim`.
    let s_i: usize = 1;
    while (s_i < num_a) : (s_i += 1) {
        const key_idx = order[s_i];
        const key_sim = best_sim[key_idx];
        let j = s_i;
        while (j > 0 and best_sim[order[j - 1]] < key_sim) : (j -= 1) {
            order[j] = order[j - 1];
        }
        order[j] = key_idx;
    }

    // 3. Select up to `min(cfg.r, num_a)` pairs meeting `best_sim >= cfg.min_similarity`.
    const max_merges = if (cfg.r < num_a) cfg.r else num_a;
    let a_merged: [TOME_MAX_TOKENS]bool = [false; TOME_MAX_TOKENS];
    let merged_pairs: usize = 0;
    let sim_sum: f32 = 0.0;
    let sim_min: f32 = 2.0;

    let k: usize = 0;
    while (k < max_merges) : (k += 1) {
        const a_idx = order[k];
        const sim = best_sim[a_idx];
        if (sim < cfg.min_similarity) break;
        a_merged[a_idx] = true;
        merged_pairs += 1;
        sim_sum += sim;
        if (sim < sim_min) sim_min = sim;
    }

    const out_len = seq_len - merged_pairs;
    if (out_tokens.len < out_len * dim or out_sizes.len < out_len) {
        return TokenMergeReport{ status: TokenMergeStatus.buffer_too_small };
    }

    // 4. Emit fused output sequence: [protected prefix] + [unmerged A] + [merged/unmerged B].
    let out_pos: usize = 0;

    // 4a. Protected prefix tokens
    let p_i: usize = 0;
    while (p_i < prot) : (p_i += 1) {
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            out_tokens[out_pos * dim + d] = tokens[p_i * dim + d];
        }
        out_sizes[out_pos] = eff_sizes[p_i];
        out_merge_map[p_i] = out_pos;
        out_pos += 1;
    }

    // 4b. Unmerged tokens from set A
    ai = 0;
    while (ai < num_a) : (ai += 1) {
        if (a_merged[ai]) continue;
        const a_tok = set_a[ai];
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            out_tokens[out_pos * dim + d] = tokens[a_tok * dim + d];
        }
        out_sizes[out_pos] = eff_sizes[a_tok];
        out_merge_map[a_tok] = out_pos;
        out_pos += 1;
    }

    // 4c. Tokens from set B with weighted averaging of all A tokens merged into each B token
    let bi: usize = 0;
    while (bi < num_b) : (bi += 1) {
        const b_tok = set_b[bi];
        let total_w = eff_sizes[b_tok];
        let acc: [TOME_MAX_DIM]f32 = [0.0; TOME_MAX_DIM];
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            acc[d] = tokens[b_tok * dim + d] * total_w;
        }

        // Accumulate all A tokens matched & merged into this B token
        let a_scan: usize = 0;
        while (a_scan < num_a) : (a_scan += 1) {
            if (a_merged[a_scan] and best_b_idx[a_scan] == bi) {
                const a_tok = set_a[a_scan];
                const wa = eff_sizes[a_tok];
                total_w += wa;
                d = 0;
                while (d < dim) : (d += 1) {
                    acc[d] += tokens[a_tok * dim + d] * wa;
                }
                out_merge_map[a_tok] = out_pos;
            }
        }

        const inv_w = 1.0 / total_w;
        d = 0;
        while (d < dim) : (d += 1) {
            out_tokens[out_pos * dim + d] = acc[d] * inv_w;
        }
        out_sizes[out_pos] = total_w;
        out_merge_map[b_tok] = out_pos;
        out_pos += 1;
    }

    const comp_permille = @as(u32, @intCast(((seq_len - out_len) * @as(usize, TOME_PERMILLE_SCALE)) / seq_len));
    const avg_sim: f32 = if (merged_pairs > 0)
        sim_sum / @as(f32, @floatFromInt(merged_pairs))
    else
        0.0;
    const final_min_sim: f32 = if (merged_pairs > 0) sim_min else 0.0;

    return TokenMergeReport{
        status: TokenMergeStatus.ok,
        input_tokens: seq_len,
        output_tokens: out_len,
        merged_pairs: merged_pairs,
        protected_tokens: prot,
        compression_permille: comp_permille,
        avg_merge_sim: avg_sim,
        min_merge_sim: final_min_sim,
    };
}

/// Compute ToMe Proportional Attention additive log-bias: `out_bias[i] = log(sizes[i])`.
/// Add this bias to key logits before Softmax so merged tokens retain proportional attention mass.
pub fn computeProportionalLogBias(sizes: []const f32, out_bias: *[]f32): bool {
    if (sizes.len == 0 or out_bias.len < sizes.len) return false;
    let i: usize = 0;
    while (i < sizes.len) : (i += 1) {
        const s = sizes[i];
        if (!isFiniteF32(s) or s <= 0.0) return false;
        out_bias[i] = @log(s);
    }
    return true;
}

/// Zero-heap inverse projection (`unmerge`): reconstructs an `orig_seq_len * dim` sequence
/// from `merged_tokens[merged_seq_len * dim]` using the `merge_map` produced by `mergeTokensF32`.
pub fn unmergeTokensF32(
    merged_tokens: []const f32,
    merge_map: []const usize,
    orig_seq_len: usize,
    merged_seq_len: usize,
    dim: usize,
    out_tokens: *[]f32,
): TokenMergeStatus {
    if (orig_seq_len == 0 or merged_seq_len == 0 or dim == 0) {
        return TokenMergeStatus.invalid_input;
    }
    if (merged_tokens.len != merged_seq_len * dim or merge_map.len < orig_seq_len) {
        return TokenMergeStatus.invalid_input;
    }
    if (out_tokens.len < orig_seq_len * dim) {
        return TokenMergeStatus.buffer_too_small;
    }

    let i: usize = 0;
    while (i < orig_seq_len) : (i += 1) {
        const src = merge_map[i];
        if (src >= merged_seq_len) return TokenMergeStatus.invalid_input;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            out_tokens[i * dim + d] = merged_tokens[src * dim + d];
        }
    }
    return TokenMergeStatus.ok;
}
