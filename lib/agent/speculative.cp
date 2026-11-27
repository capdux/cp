// Origin: Pure-CP Stdlib (AI Inference Speculative Decoding: Rejection Sampling & Adaptive Draft Length) | Created: 2026-11-22

fn isNaN(x: f64): bool {
    return x != x;
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return a + b;
}

fn nextUnitFloat(state: *u64): f64 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    z = z ^ (z >> 31);
    return ((z >> 11) as f64) * (1.0 / 9007199254740992.0);
}

fn sampleNormalized(probs: []const f64, sum: f64, state: *u64): usize {
    const u = nextUnitFloat(state) * sum;
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < probs.len) {
        acc += probs[i];
        if (u < acc) {
            return i;
        }
        i += 1;
    }
    return probs.len - 1;
}

fn validateAndSumRow(probs: []const f64, row_off: usize, vocab_size: usize): f64? {
    let sum: f64 = 0.0;
    let v: usize = 0;
    while (v < vocab_size) {
        const p = probs[row_off + v];
        if (isNaN(p) || p < 0.0) {
            return null;
        }
        sum += p;
        v += 1;
    }
    if (sum <= 0.0 || isNaN(sum)) {
        return null;
    }
    return sum;
}

fn argmaxRow(probs: []const f64, row_off: usize, vocab_size: usize): usize? {
    let best_idx: usize = 0;
    let best_val: f64 = -1.0;
    let v: usize = 0;
    while (v < vocab_size) {
        const p = probs[row_off + v];
        if (isNaN(p) || p < 0.0) {
            return null;
        }
        if (v == 0 || p > best_val) {
            best_idx = v;
            best_val = p;
        }
        v += 1;
    }
    return best_idx;
}

/// Verification result returned by `verifySpeculativeStep` and `verifySpeculativeGreedy`.
pub struct SpecVerifyResult {
    accepted_count: usize,
    emitted_count: usize,
    all_accepted: bool,
    rejected_at: usize,
    bonus_token: usize,
}

/// Lossless speculative decoding verification (Leviathan et al., 2023):
/// - `draft_probs`: flattened `K * vocab_size` probabilities from the draft model `q_i`.
/// - `target_probs`: flattened `(K + 1) * vocab_size` probabilities from the target model `p_i`.
/// - For each draft token `x_i`, accepts with probability `min(1, p_i(x_i) / q_i(x_i))`.
/// - On first rejection, samples a corrected token from `norm(max(0, p_i - q_i))` and stops.
/// - If all `K` tokens are accepted, samples an extra bonus token from `target_probs[K]`.
/// Mathematically guarantees the emitted token distribution is identical to sampling from `target_probs`.
pub fn verifySpeculativeStep(
    draft_tokens: []const usize,
    draft_probs: []const f64,
    target_probs: []const f64,
    vocab_size: usize,
    seed: u64,
    emitted_out: *[]usize,
    residual_scratch: *[]f64,
): SpecVerifyResult? {
    const k = draft_tokens.len;
    if (k == 0 || vocab_size == 0) {
        return null;
    }
    if (draft_probs.len != k * vocab_size || target_probs.len != (k + 1) * vocab_size) {
        return null;
    }
    if (emitted_out.*.len < k + 1 || residual_scratch.*.len < vocab_size) {
        return null;
    }

    let rng_state: u64 = seed;
    let i: usize = 0;
    while (i < k) {
        const tok = draft_tokens[i];
        if (tok >= vocab_size) {
            return null;
        }
        const row_off = i * vocab_size;
        const q = draft_probs[row_off + tok];
        const p = target_probs[row_off + tok];
        if (isNaN(q) || isNaN(p) || q < 0.0 || p < 0.0) {
            return null;
        }

        let accept: bool = false;
        if (p >= q) {
            accept = true;
        } else {
            const u = nextUnitFloat(&rng_state);
            accept = (u * q) < p;
        }

        if (accept) {
            emitted_out.*[i] = tok;
            i += 1;
            continue;
        }

        // Rejected at position i: sample correction token from norm(max(0, p_i - q_i))
        let res_sum: f64 = 0.0;
        let v: usize = 0;
        while (v < vocab_size) {
            const pv = target_probs[row_off + v];
            const qv = draft_probs[row_off + v];
            if (isNaN(pv) || isNaN(qv) || pv < 0.0 || qv < 0.0) {
                return null;
            }
            const diff: f64 = if (pv > qv) (pv - qv) else 0.0;
            residual_scratch.*[v] = diff;
            res_sum += diff;
            v += 1;
        }

        let correction: usize = 0;
        if (res_sum > 0.0) {
            correction = sampleNormalized(residual_scratch.*[0..vocab_size], res_sum, &rng_state);
        } else {
            const tsum = validateAndSumRow(target_probs, row_off, vocab_size) ?? return null;
            correction = sampleNormalized(target_probs[row_off..(row_off + vocab_size)], tsum, &rng_state);
        }

        emitted_out.*[i] = correction;
        return SpecVerifyResult {
            accepted_count: i,
            emitted_count: i + 1,
            all_accepted: false,
            rejected_at: i,
            bonus_token: correction,
        };
    }

    // All K draft tokens accepted -> sample bonus token from target_probs[K]
    const bonus_off = k * vocab_size;
    const bonus_sum = validateAndSumRow(target_probs, bonus_off, vocab_size) ?? return null;
    const bonus_tok = sampleNormalized(target_probs[bonus_off..(bonus_off + vocab_size)], bonus_sum, &rng_state);
    emitted_out.*[k] = bonus_tok;

    return SpecVerifyResult {
        accepted_count: k,
        emitted_count: k + 1,
        all_accepted: true,
        rejected_at: k,
        bonus_token: bonus_tok,
    };
}

/// Deterministic greedy speculative verification: compares each `draft_tokens[i]` against
/// `argmax(target_probs[i])`, truncating and replacing at the first mismatch, or emitting
/// `argmax(target_probs[K])` as the bonus token if all `K` draft tokens match.
pub fn verifySpeculativeGreedy(
    draft_tokens: []const usize,
    target_probs: []const f64,
    vocab_size: usize,
    emitted_out: *[]usize,
): SpecVerifyResult? {
    const k = draft_tokens.len;
    if (k == 0 || vocab_size == 0) {
        return null;
    }
    if (target_probs.len != (k + 1) * vocab_size || emitted_out.*.len < k + 1) {
        return null;
    }

    let i: usize = 0;
    while (i < k) {
        if (draft_tokens[i] >= vocab_size) {
            return null;
        }
        const best = argmaxRow(target_probs, i * vocab_size, vocab_size) ?? return null;
        emitted_out.*[i] = best;
        if (draft_tokens[i] != best) {
            return SpecVerifyResult {
                accepted_count: i,
                emitted_count: i + 1,
                all_accepted: false,
                rejected_at: i,
                bonus_token: best,
            };
        }
        i += 1;
    }

    const bonus = argmaxRow(target_probs, k * vocab_size, vocab_size) ?? return null;
    emitted_out.*[k] = bonus;
    return SpecVerifyResult {
        accepted_count: k,
        emitted_count: k + 1,
        all_accepted: true,
        rejected_at: k,
        bonus_token: bonus,
    };
}

/// Cumulative speculative decoding telemetry snapshot.
pub struct SpecStats {
    total_steps: u64,
    total_drafted: u64,
    total_accepted: u64,
    total_emitted: u64,
    acceptance_rate: f64,
    avg_tokens_per_step: f64,
}

/// Zero-allocation adaptive draft-length (`K`) controller driven by EMA acceptance rate.
/// Increases `K` when EMA acceptance rate >= 0.75 and decreases `K` when <= 0.40, clamped to `[min_k, max_k]`.
pub struct AdaptiveDraftController {
    min_k: usize,
    max_k: usize,
    cur_k: usize,
    ema_alpha: f64,
    ema_accept_rate: f64,
    has_sample: bool,
    total_steps: u64,
    total_drafted: u64,
    total_accepted: u64,
    total_emitted: u64,

    /// Initializes the controller (`1 <= min_k <= init_k <= max_k`, `0 < ema_alpha <= 1`).
    static fn init(min_k: usize, max_k: usize, init_k: usize, ema_alpha: f64): AdaptiveDraftController? {
        if (
            min_k == 0 ||
            max_k < min_k ||
            init_k < min_k ||
            init_k > max_k ||
            isNaN(ema_alpha) ||
            ema_alpha <= 0.0 ||
            ema_alpha > 1.0
        ) {
            return null;
        }
        return AdaptiveDraftController {
            min_k: min_k,
            max_k: max_k,
            cur_k: init_k,
            ema_alpha: ema_alpha,
            ema_accept_rate: 0.0,
            has_sample: false,
            total_steps: 0,
            total_drafted: 0,
            total_accepted: 0,
            total_emitted: 0,
        };
    }

    /// Returns the currently recommended draft length `K`.
    fn currentK(): usize {
        return _.cur_k;
    }

    /// Records one speculative verification step and returns the updated draft length `K`.
    fn recordStep(*_, drafted: usize, accepted: usize, emitted: usize): usize {
        const clamped_acc = if (accepted > drafted) drafted else accepted;
        _.total_steps = satAddU64(_.total_steps, 1);
        _.total_drafted = satAddU64(_.total_drafted, drafted as u64);
        _.total_accepted = satAddU64(_.total_accepted, clamped_acc as u64);
        _.total_emitted = satAddU64(_.total_emitted, emitted as u64);

        if (drafted > 0) {
            const step_rate = (clamped_acc as f64) / (drafted as f64);
            if (!_.has_sample) {
                _.ema_accept_rate = step_rate;
                _.has_sample = true;
            } else {
                _.ema_accept_rate = _.ema_alpha * step_rate + (1.0 - _.ema_alpha) * _.ema_accept_rate;
            }

            if (_.ema_accept_rate >= 0.75 && _.cur_k < _.max_k) {
                _.cur_k += 1;
            } else if (_.ema_accept_rate <= 0.40 && _.cur_k > _.min_k) {
                _.cur_k -= 1;
            }
        }
        return _.cur_k;
    }

    /// Computes cumulative speculative decoding statistics.
    fn stats(): SpecStats {
        const acc_rate: f64 = if (_.total_drafted > 0)
            ((_.total_accepted as f64) / (_.total_drafted as f64))
        else
            0.0;
        const avg_emitted: f64 = if (_.total_steps > 0)
            ((_.total_emitted as f64) / (_.total_steps as f64))
        else
            0.0;
        return SpecStats {
            total_steps: _.total_steps,
            total_drafted: _.total_drafted,
            total_accepted: _.total_accepted,
            total_emitted: _.total_emitted,
            acceptance_rate: acc_rate,
            avg_tokens_per_step: avg_emitted,
        };
    }
}
