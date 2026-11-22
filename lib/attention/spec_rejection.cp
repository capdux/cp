// Origin: Pure-CP Stdlib (Zero-Alloc Leviathan Lossless Speculative Decoding Rejection Sampling & Residual Resampler) | Created: 2026-11-22
// Pure CP zero-heap-allocation Lossless Speculative Decoding engine (Leviathan et al. 2023 / DeepMind / vLLM):
//   - Exact rejection sampling: accept draft token x_i with probability min(1, p_i(x_i) / q_i(x_i))
//   - Two-pass O(V) zero-allocation residual resampler `sampleResidualDistributionF32` over norm(max(0, p - q))
//   - Full-acceptance Bonus Token sampling from the (k + 1)-th target distribution (up to k + 1 tokens/step)
//   - Hardware `@fma` EWMA adaptive draft step scheduler (`SpecStepScheduler`) with bounded auto grow/shrink
//   - Strict bounds, dimension, probability finiteness/non-negativity, and token-ID validation

const std = @import("std");

/// Maximum supported draft lookahead length K per speculative verification step (emits up to K + 1 tokens).
pub const SPEC_REJECT_MAX_DRAFT_K: usize = 16;

const PROB_SUM_EPS: f64 = 1.0e-12;
const UNIFORM_UPPER_CLAMP: f32 = 0.99999994;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clampUnitOpenF32(u: f32): f32 {
    if (u < 0.0) return 0.0;
    if (u > UNIFORM_UPPER_CLAMP) return UNIFORM_UPPER_CLAMP;
    return u;
}

/// Deterministic SplitMix64 PRNG for reproducible speculative verification.
struct SplitMix64Rng {
    state: u64 = 0;

    static fn init(seed: u64): SplitMix64Rng {
        return SplitMix64Rng{ state: seed };
    }

    fn nextU64(*_): u64 {
        _.state = _.state +% 0x9E3779B97F4A7C15;
        let z: u64 = _.state;
        z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
        z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
        return z ^ (z >> 31);
    }

    /// Returns a uniform f32 in [0.0, 1.0) with exact 2^-24 resolution.
    fn nextUniformF32(*_): f32 {
        const bits: u32 = (_.nextU64() >> 40) as u32;
        return (bits as f32) * (1.0 / 16777216.0);
    }
}

fn validateProbDistF32(probs: []const f32): f64? {
    if (probs.len == 0) return null;
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < probs.len) : (i += 1) {
        const v: f32 = probs[i];
        if (!isFiniteF32(v) || v < 0.0) return null;
        sum += v as f64;
    }
    if (sum <= PROB_SUM_EPS) return null;
    return sum;
}

fn sampleFromValidProbsF32(probs: []const f32, total_sum: f64, rng_u01: f32): u32 {
    const u: f64 = clampUnitOpenF32(rng_u01) as f64;
    const target: f64 = u * total_sum;
    let cdf: f64 = 0.0;
    let last_positive: u32 = 0;
    let i: usize = 0;
    while (i < probs.len) : (i += 1) {
        const v: f64 = probs[i] as f64;
        if (v > 0.0) {
            last_positive = i as u32;
            cdf += v;
            if (cdf >= target) {
                return i as u32;
            }
        }
    }
    return last_positive;
}

// ============================================================================
// 1. Public Models & Adaptive Draft Step Scheduler
// ============================================================================

/// Outcome of a single Leviathan lossless speculative decoding verification step.
/// Emits `accepted_draft_count + 1` tokens in `emitted_tokens[0..emitted_count]`.
pub struct SpecRejectionOutcome {
    ok: bool = false;
    accepted_draft_count: usize = 0;
    emitted_tokens: [17]u32 = [0; 17];
    emitted_count: usize = 0;
    rejected_at_idx: usize = 0;
    used_bonus_token: bool = false;
    acceptance_rate_f32: f32 = 0.0;
}

/// Configuration for the EWMA-based adaptive draft step length controller (`SpecStepScheduler`).
pub struct SpecStepSchedulerConfig {
    min_k: usize = 1;
    max_k: usize = 8;
    initial_k: usize = 4;
    ewma_alpha: f32 = 0.25;
    grow_threshold: f32 = 0.75;
    shrink_threshold: f32 = 0.45;

    /// Returns production defaults (`min_k=1`, `max_k=8`, `initial_k=4`,
    /// `ewma_alpha=0.25`, `grow_threshold=0.75`, `shrink_threshold=0.45`).
    static fn defaults(): SpecStepSchedulerConfig {
        return SpecStepSchedulerConfig{};
    }

    /// Validates all step bounds and EWMA threshold invariants.
    fn isValid(): bool {
        if (_.min_k < 1 || _.max_k > SPEC_REJECT_MAX_DRAFT_K || _.min_k > _.max_k) return false;
        if (_.initial_k < _.min_k || _.initial_k > _.max_k) return false;
        if (!isFiniteF32(_.ewma_alpha) || _.ewma_alpha <= 0.0 || _.ewma_alpha > 1.0) return false;
        if (!isFiniteF32(_.grow_threshold) || _.grow_threshold <= 0.0 || _.grow_threshold > 1.0) return false;
        if (!isFiniteF32(_.shrink_threshold) || _.shrink_threshold < 0.0 || _.shrink_threshold >= _.grow_threshold) return false;
        return true;
    }
}

/// Adaptive speculative draft step scheduler driven by `@fma` EWMA acceptance rate.
pub struct SpecStepScheduler {
    cfg: SpecStepSchedulerConfig = SpecStepSchedulerConfig{};
    k: usize = 4;
    ewma_rate: f32 = 0.60;
    initialized_ewma: bool = false;
    rounds: u64 = 0;

    /// Initializes a validated `SpecStepScheduler`, returning `null` if `cfg` is invalid.
    static fn init(cfg: SpecStepSchedulerConfig): SpecStepScheduler? {
        if (!cfg.isValid()) return null;
        return SpecStepScheduler{
            cfg: cfg,
            k: cfg.initial_k,
            ewma_rate: (cfg.grow_threshold + cfg.shrink_threshold) * 0.5,
            initialized_ewma: false,
            rounds: 0,
        };
    }

    /// Observes a verification round's accepted/proposed token counts, updates the
    /// `@fma` EWMA acceptance rate, adjusts draft step length `K` within `[min_k, max_k]`,
    /// and returns the updated `K`.
    fn observe_round(*_, accepted_count: usize, proposed_k: usize): usize {
        if (proposed_k == 0) {
            return _.k;
        }
        const clamped_acc: usize = if (accepted_count > proposed_k) proposed_k else accepted_count;
        const rate: f32 = (clamped_acc as f32) / (proposed_k as f32);

        if (!_.initialized_ewma) {
            _.ewma_rate = rate;
            _.initialized_ewma = true;
        } else {
            const one_minus_alpha: f32 = 1.0 - _.cfg.ewma_alpha;
            _.ewma_rate = @fma(_.cfg.ewma_alpha, rate, one_minus_alpha * _.ewma_rate);
        }
        _.rounds += 1;

        if (_.ewma_rate >= _.cfg.grow_threshold && _.k < _.cfg.max_k) {
            _.k += 1;
        } else if (_.ewma_rate <= _.cfg.shrink_threshold && _.k > _.cfg.min_k) {
            _.k -= 1;
        }
        return _.k;
    }

    /// Returns the current recommended draft step length `K`.
    fn current_k(): usize {
        return _.k;
    }

    /// Returns the current EWMA token acceptance rate in `[0.0, 1.0]`.
    fn ewma_acceptance_rate(): f32 {
        return _.ewma_rate;
    }
}

// ============================================================================
// 2. Pure Zero-Alloc Lossless Rejection Sampling & Residual Resampling
// ============================================================================

/// Samples a corrective token from the normalized residual distribution
/// `norm(max(0, target_probs[x] - draft_probs[x]))` in two O(V) zero-allocation passes.
/// Safely falls back to sampling directly from `target_probs` if residual mass is ~0.
/// Returns `null` on empty/mismatched slices or non-finite/negative/zero-sum distributions.
pub fn sampleResidualDistributionF32(
    target_probs: []const f32,
    draft_probs: []const f32,
    rng_u01: f32,
): u32? {
    const v: usize = target_probs.len;
    if (v == 0 || draft_probs.len != v || !isFiniteF32(rng_u01)) {
        return null;
    }

    let target_sum: f64 = 0.0;
    let res_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < v) : (i += 1) {
        const p_i: f32 = target_probs[i];
        const q_i: f32 = draft_probs[i];
        if (!isFiniteF32(p_i) || p_i < 0.0 || !isFiniteF32(q_i) || q_i < 0.0) {
            return null;
        }
        const p_f: f64 = p_i as f64;
        const q_f: f64 = q_i as f64;
        target_sum += p_f;
        if (p_f > q_f) {
            res_sum += p_f - q_f;
        }
    }

    if (target_sum <= PROB_SUM_EPS) {
        return null;
    }

    // Degenerate case (e.g., p == q everywhere): safely fall back to target_probs
    if (res_sum <= PROB_SUM_EPS) {
        return sampleFromValidProbsF32(target_probs, target_sum, rng_u01);
    }

    const u: f64 = clampUnitOpenF32(rng_u01) as f64;
    const threshold: f64 = u * res_sum;
    let cdf: f64 = 0.0;
    let last_pos: u32 = 0;

    i = 0;
    while (i < v) : (i += 1) {
        const diff: f64 = (target_probs[i] as f64) - (draft_probs[i] as f64);
        if (diff > 0.0) {
            last_pos = i as u32;
            cdf += diff;
            if (cdf >= threshold) {
                return i as u32;
            }
        }
    }
    return last_pos;
}

/// Performs Leviathan lossless speculative decoding rejection sampling across `k` draft tokens:
///   - `draft_tokens`: length `k` (`1 <= k <= SPEC_REJECT_MAX_DRAFT_K`)
///   - `draft_probs_flat`: length `k * vocab_size`
///   - `target_probs_flat`: length `(k + 1) * vocab_size` (includes the `(k + 1)`-th bonus distribution)
///   - On first rejection at position `i`, emits a corrective token from `norm(max(0, p_i - q_i))`.
///   - If all `k` draft tokens are accepted, emits an extra Bonus Token from `p_k` (`k + 1` tokens total).
/// Returns `SpecRejectionOutcome{ ok: false, ... }` on any invalid dimension, token ID, or NaN/Inf/negative prob.
pub fn verifySpeculativeRejectionF32(
    draft_tokens: []const u32,
    draft_probs_flat: []const f32,
    target_probs_flat: []const f32,
    vocab_size: usize,
    rng_seed: u64,
): SpecRejectionOutcome {
    const k: usize = draft_tokens.len;
    if (k == 0 || k > SPEC_REJECT_MAX_DRAFT_K || vocab_size == 0) {
        return SpecRejectionOutcome{};
    }
    if (draft_probs_flat.len != k * vocab_size || target_probs_flat.len != (k + 1) * vocab_size) {
        return SpecRejectionOutcome{};
    }

    // Upfront validation: bounds-check draft tokens and validate all probability slices
    let i: usize = 0;
    while (i < k) : (i += 1) {
        if ((draft_tokens[i] as usize) >= vocab_size) {
            return SpecRejectionOutcome{};
        }
        const start: usize = i * vocab_size;
        const end: usize = start + vocab_size;
        if (validateProbDistF32(draft_probs_flat[start..end]) == null) {
            return SpecRejectionOutcome{};
        }
        if (validateProbDistF32(target_probs_flat[start..end]) == null) {
            return SpecRejectionOutcome{};
        }
    }
    const bonus_start: usize = k * vocab_size;
    const bonus_end: usize = bonus_start + vocab_size;
    const bonus_sum: f64 = validateProbDistF32(target_probs_flat[bonus_start..bonus_end]) ?? return SpecRejectionOutcome{};

    let rng = SplitMix64Rng.init(rng_seed);
    let out = SpecRejectionOutcome{ ok: true };

    let step: usize = 0;
    while (step < k) : (step += 1) {
        const start: usize = step * vocab_size;
        const end: usize = start + vocab_size;
        const tok: u32 = draft_tokens[step];
        const q_slice: []const f32 = draft_probs_flat[start..end];
        const p_slice: []const f32 = target_probs_flat[start..end];

        const q_x: f64 = q_slice[tok as usize] as f64;
        const p_x: f64 = p_slice[tok as usize] as f64;

        let accept_prob: f64 = 0.0;
        if (p_x <= 0.0) {
            accept_prob = 0.0;
        } else if (q_x <= 0.0) {
            accept_prob = 1.0;
        } else {
            const ratio: f64 = p_x / q_x;
            accept_prob = if (ratio >= 1.0) 1.0 else ratio;
        }

        const u_acc: f32 = rng.nextUniformF32();
        if ((u_acc as f64) < accept_prob) {
            out.emitted_tokens[out.emitted_count] = tok;
            out.emitted_count += 1;
            out.accepted_draft_count += 1;
        } else {
            const u_res: f32 = rng.nextUniformF32();
            const corrective: u32 = sampleResidualDistributionF32(p_slice, q_slice, u_res) ?? return SpecRejectionOutcome{};
            out.emitted_tokens[out.emitted_count] = corrective;
            out.emitted_count += 1;
            out.rejected_at_idx = step;
            out.used_bonus_token = false;
            out.acceptance_rate_f32 = (out.accepted_draft_count as f32) / (k as f32);
            return out;
        }
    }

    // All k draft tokens accepted -> sample Bonus Token from the (k + 1)-th target distribution
    const u_bonus: f32 = rng.nextUniformF32();
    const bonus_tok: u32 = sampleFromValidProbsF32(target_probs_flat[bonus_start..bonus_end], bonus_sum, u_bonus);
    out.emitted_tokens[out.emitted_count] = bonus_tok;
    out.emitted_count += 1;
    out.rejected_at_idx = k;
    out.used_bonus_token = true;
    out.acceptance_rate_f32 = 1.0;
    return out;
}
