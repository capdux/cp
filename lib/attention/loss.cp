// Origin: Pure-CP Stdlib (AI Training & Alignment Losses: CrossEntropy, KL, DPO & InfoNCE) | Created: 2026-11-22
// Zero-heap-allocation numerically stable training, alignment, and evaluation loss functions:
//   - `logSumExp`, `crossEntropyLoss` (with label smoothing), `sequencePerplexity` (with ignore_index)
//   - `klDivergenceFromLogits` (distillation & RLHF/GRPO KL penalty without materializing prob buffers)
//   - `dpoLoss` (Direct Preference Optimization, Rafailov et al. 2023) with overflow-proof `-log(sigmoid(m))`
//   - `infoNceLoss` (contrastive embedding fine-tuning & reranking)

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn stableSoftplus(x: f64): f64 {
    if (x > 35.0) return x;
    if (x >= 0.0) return x + std.math.log1p(std.math.exp(0.0 - x));
    return std.math.log1p(std.math.exp(x));
}

/// Output metrics returned by `dpoLoss` (Direct Preference Optimization).
pub struct DpoResult {
    loss: f64 = 0.0;
    chosen_reward: f64 = 0.0;
    rejected_reward: f64 = 0.0;
    reward_margin: f64 = 0.0;
    accuracy: bool = false;
}

/// Numerically stable `log(sum_i exp(logits[i]))` using max-shift reduction.
/// Returns `null` on empty slice or NaN/Inf inputs.
pub fn logSumExp(logits: []const f64): f64? {
    if (logits.len == 0) return null;

    let max_v: f64 = 0.0 - std.math.inf(f64);
    let i: usize = 0;
    while (i < logits.len) : (i += 1) {
        const v = logits[i];
        if (!isFiniteF64(v)) return null;
        if (v > max_v) max_v = v;
    }

    let sum_e: f64 = 0.0;
    let j: usize = 0;
    while (j < logits.len) : (j += 1) {
        sum_e += std.math.exp(logits[j] - max_v);
    }
    if (!(sum_e > 0.0) || !isFiniteF64(sum_e)) return null;

    const res = max_v + @log(sum_e);
    if (!isFiniteF64(res)) return null;
    return res;
}

/// Numerically stable Cross-Entropy loss with optional label smoothing `eps in [0.0, 1.0)`:
/// `L = logSumExp(logits) - (1 - eps) * logits[target_idx] - eps * mean(logits)`.
/// Returns `null` on empty logits, out-of-range `target_idx`, invalid `label_smoothing`, or NaN/Inf.
pub fn crossEntropyLoss(
    logits: []const f64,
    target_idx: usize,
    label_smoothing: f64,
): f64? {
    if (logits.len == 0 || target_idx >= logits.len) return null;
    if (!isFiniteF64(label_smoothing) || label_smoothing < 0.0 || label_smoothing >= 1.0) return null;

    const lse = logSumExp(logits) ?? return null;
    if (label_smoothing == 0.0) {
        const loss = lse - logits[target_idx];
        return if (loss < 0.0) 0.0 else loss;
    }

    let sum_logits: f64 = 0.0;
    let i: usize = 0;
    while (i < logits.len) : (i += 1) {
        sum_logits += logits[i];
    }
    const mean_logit = sum_logits / (logits.len as f64);
    const loss = lse - (1.0 - label_smoothing) * logits[target_idx] - label_smoothing * mean_logit;
    if (!isFiniteF64(loss)) return null;
    return if (loss < 0.0) 0.0 else loss;
}

/// Computes sequence perplexity `exp(mean_CE)` over `[seq_len, vocab_size]` logits and `targets`.
/// Positions where `targets[t] == ignore_index` (e.g. `-100` for PAD/prompt tokens) are skipped.
/// Returns `null` on zero dims, all-ignored targets, out-of-range target IDs, undersized slices, or NaN/Inf.
pub fn sequencePerplexity(
    logits_flat: []const f64,
    targets: []const i32,
    seq_len: usize,
    vocab_size: usize,
    ignore_index: i32,
): f64? {
    if (seq_len == 0 || vocab_size == 0) return null;
    if (seq_len > std.math.maxInt(usize) / vocab_size) return null;
    const total_logits = seq_len * vocab_size;
    if (logits_flat.len < total_logits || targets.len < seq_len) return null;

    let total_ce: f64 = 0.0;
    let valid_count: usize = 0;

    let t: usize = 0;
    while (t < seq_len) : (t += 1) {
        const tgt = targets[t];
        if (tgt == ignore_index) continue;
        if (tgt < 0) return null;
        const tgt_u = tgt as usize;
        if (tgt_u >= vocab_size) return null;

        const row_start = t * vocab_size;
        const ce = crossEntropyLoss(logits_flat[row_start .. row_start + vocab_size], tgt_u, 0.0) ?? return null;
        total_ce += ce;
        valid_count += 1;
    }

    if (valid_count == 0) return null;
    const mean_ce = total_ce / (valid_count as f64);
    const ppl = std.math.exp(mean_ce);
    if (!isFiniteF64(ppl)) return null;
    return ppl;
}

/// Computes KL divergence `D_KL(P || Q) >= 0` directly from unnormalized logit vectors
/// without allocating intermediate probability buffers.
/// Returns `null` on empty/mismatched slices or NaN/Inf inputs.
pub fn klDivergenceFromLogits(
    p_logits: []const f64,
    q_logits: []const f64,
): f64? {
    if (p_logits.len == 0 || p_logits.len != q_logits.len) return null;

    const lse_p = logSumExp(p_logits) ?? return null;
    const lse_q = logSumExp(q_logits) ?? return null;

    let kl: f64 = 0.0;
    let i: usize = 0;
    while (i < p_logits.len) : (i += 1) {
        const log_p = p_logits[i] - lse_p;
        const log_q = q_logits[i] - lse_q;
        const p_i = std.math.exp(log_p);
        kl += p_i * (log_p - log_q);
    }

    if (!isFiniteF64(kl)) return null;
    return if (kl < 0.0) 0.0 else kl;
}

/// Computes Direct Preference Optimization (DPO) loss and implicit reward diagnostics
/// (Rafailov et al. 2023):
/// - `chosen_reward = beta * (pi_chosen_logp - ref_chosen_logp)`
/// - `rejected_reward = beta * (pi_rejected_logp - ref_rejected_logp)`
/// - `reward_margin = chosen_reward - rejected_reward`
/// - `loss = -log(sigmoid(reward_margin)) = softplus(-reward_margin)` (overflow-proof for extreme margins)
/// - `accuracy = reward_margin > 0.0`
///
/// Returns `null` if `beta <= 0.0` or any input is NaN/Inf.
pub fn dpoLoss(
    pi_chosen_logp: f64,
    pi_rejected_logp: f64,
    ref_chosen_logp: f64,
    ref_rejected_logp: f64,
    beta: f64,
): DpoResult? {
    if (!isFiniteF64(pi_chosen_logp)
        || !isFiniteF64(pi_rejected_logp)
        || !isFiniteF64(ref_chosen_logp)
        || !isFiniteF64(ref_rejected_logp)
        || !isFiniteF64(beta)
        || !(beta > 0.0))
    {
        return null;
    }

    const chosen_reward = beta * (pi_chosen_logp - ref_chosen_logp);
    const rejected_reward = beta * (pi_rejected_logp - ref_rejected_logp);
    const reward_margin = chosen_reward - rejected_reward;
    if (!isFiniteF64(reward_margin)) return null;

    const loss = stableSoftplus(0.0 - reward_margin);
    if (!isFiniteF64(loss)) return null;

    return DpoResult{
        loss: loss,
        chosen_reward: chosen_reward,
        rejected_reward: rejected_reward,
        reward_margin: reward_margin,
        accuracy: reward_margin > 0.0,
    };
}

/// Zero-heap-allocation InfoNCE contrastive loss over similarity scores:
/// `L = -log(exp(sim[positive_idx] / temperature) / sum_j exp(sim[j] / temperature))`.
/// Returns `null` on empty similarities, out-of-range `positive_idx`, `temperature <= 0.0`, or NaN/Inf.
pub fn infoNceLoss(
    similarities: []const f64,
    positive_idx: usize,
    temperature: f64,
): f64? {
    if (similarities.len == 0 || positive_idx >= similarities.len) return null;
    if (!isFiniteF64(temperature) || !(temperature > 0.0)) return null;

    const inv_tau = 1.0 / temperature;
    if (!isFiniteF64(inv_tau)) return null;

    let max_s: f64 = 0.0 - std.math.inf(f64);
    let i: usize = 0;
    while (i < similarities.len) : (i += 1) {
        const s = similarities[i];
        if (!isFiniteF64(s)) return null;
        const scaled = s * inv_tau;
        if (!isFiniteF64(scaled)) return null;
        if (scaled > max_s) max_s = scaled;
    }

    let sum_e: f64 = 0.0;
    let j: usize = 0;
    while (j < similarities.len) : (j += 1) {
        sum_e += std.math.exp(similarities[j] * inv_tau - max_s);
    }
    if (!(sum_e > 0.0) || !isFiniteF64(sum_e)) return null;

    const pos_scaled = similarities[positive_idx] * inv_tau;
    const loss = (max_s + @log(sum_e)) - pos_scaled;
    if (!isFiniteF64(loss)) return null;
    return if (loss < 0.0) 0.0 else loss;
}
