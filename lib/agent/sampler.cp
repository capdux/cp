// Origin: Pure-CP Stdlib (AI Inference Logits Sampler: Temperature, Top-K, Top-P, Min-P & Penalties) | Created: 2026-11-22

import std from "std";

fn isNaN(x: f64): bool {
    return x != x;
}

fn splitMix64Next(seed: u64): u64 {
    let z: u64 = seed +% 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

/// Maps a 64-bit seed deterministically to a uniform `f64` in `[0.0, 1.0)`.
fn splitMix64UnitFloat(seed: u64): f64 {
    const bits = splitMix64Next(seed) >> 11; // top 53 bits for exact f64 mantissa
    return (bits as f64) * (1.0 / 9007199254740992.0);
}

/// Returns `true` if token index `a` should rank *after* `b` (min-heap comparator for descending sort).
fn ranksAfter(logits: []const f64, a: usize, b: usize): bool {
    const la = logits[a];
    const lb = logits[b];
    if (la != lb) {
        return la < lb;
    }
    return a > b;
}

fn siftDownDesc(indices: *[]usize, logits: []const f64, start: usize, end: usize) {
    let root: usize = start;
    while (root * 2 + 1 < end) {
        const left = root * 2 + 1;
        let swap_idx = root;
        if (ranksAfter(logits, indices.*[left], indices.*[swap_idx])) {
            swap_idx = left;
        }
        if (left + 1 < end && ranksAfter(logits, indices.*[left + 1], indices.*[swap_idx])) {
            swap_idx = left + 1;
        }
        if (swap_idx == root) {
            return;
        }
        const tmp = indices.*[root];
        indices.*[root] = indices.*[swap_idx];
        indices.*[swap_idx] = tmp;
        root = swap_idx;
    }
}

/// In-place zero-alloc heapsort of `indices[0..len]` in descending order of `logits[idx]` (ascending `idx` ties).
fn sortIndicesDesc(indices: *[]usize, logits: []const f64, len: usize) {
    if (len < 2) {
        return;
    }
    let start: usize = len / 2;
    while (start > 0) {
        start -= 1;
        siftDownDesc(indices, logits, start, len);
    }
    let end: usize = len;
    while (end > 1) {
        end -= 1;
        const tmp = indices.*[end];
        indices.*[end] = indices.*[0];
        indices.*[0] = tmp;
        siftDownDesc(indices, logits, 0, end);
    }
}

/// Configuration for LLM token sampling (`sampleToken`).
pub struct SamplerConfig {
    temperature: f64,
    top_k: usize,
    top_p: f64,
    min_p: f64,
    repeat_penalty: f64,
    frequency_penalty: f64,
    presence_penalty: f64,
    seed: u64,

    /// Deterministic greedy decoding (`temperature = 0.0`).
    fn greedy(): SamplerConfig {
        return SamplerConfig {
            temperature: 0.0,
            top_k: 0,
            top_p: 1.0,
            min_p: 0.0,
            repeat_penalty: 1.0,
            frequency_penalty: 0.0,
            presence_penalty: 0.0,
            seed: 0,
        };
    }

    /// Balanced default stochastic sampling configuration seeded with `seed`.
    fn default(seed: u64): SamplerConfig {
        return SamplerConfig {
            temperature: 0.8,
            top_k: 40,
            top_p: 0.95,
            min_p: 0.05,
            repeat_penalty: 1.0,
            frequency_penalty: 0.0,
            presence_penalty: 0.0,
            seed: seed,
        };
    }
}

/// Sampling outcome returned by `sampleToken`.
pub struct SampledToken {
    token_id: usize,
    prob: f64,
    logit: f64,
    candidates_considered: usize,
}

/// Applies Keskar repetition penalty (positive logits divided by `repeat_penalty`, non-positive
/// logits multiplied by `repeat_penalty`) plus OpenAI frequency and presence penalties in-place.
/// Returns `false` if `repeat_penalty <= 0.0` or any penalty parameter is `NaN`.
pub fn applyRepetitionPenalties(
    logits_inout: *[]f64,
    recent_tokens: []const usize,
    repeat_penalty: f64,
    freq_penalty: f64,
    presence_penalty: f64,
): bool {
    if (
        isNaN(repeat_penalty) || repeat_penalty <= 0.0 ||
        isNaN(freq_penalty) || isNaN(presence_penalty)
    ) {
        return false;
    }
    if (recent_tokens.len == 0 || logits_inout.*.len == 0) {
        return true;
    }

    let i: usize = 0;
    while (i < recent_tokens.len) {
        const tid = recent_tokens[i];
        if (tid < logits_inout.*.len) {
            let seen_before: bool = false;
            let p: usize = 0;
            while (p < i) {
                if (recent_tokens[p] == tid) {
                    seen_before = true;
                    break;
                }
                p += 1;
            }
            if (!seen_before) {
                let cnt: usize = 1;
                let j: usize = i + 1;
                while (j < recent_tokens.len) {
                    if (recent_tokens[j] == tid) {
                        cnt += 1;
                    }
                    j += 1;
                }
                let v = logits_inout.*[tid];
                if (!isNaN(v)) {
                    if (repeat_penalty != 1.0) {
                        if (v > 0.0) {
                            v = v / repeat_penalty;
                        } else {
                            v = v * repeat_penalty;
                        }
                    }
                    v = v - freq_penalty * (cnt as f64) - presence_penalty;
                    logits_inout.*[tid] = v;
                }
            }
        }
        i += 1;
    }
    return true;
}

/// Deterministically selects the token index with the highest non-`NaN` logit (smallest index breaks ties).
/// Returns `null` if `logits` is empty or contains only `NaN`.
pub fn greedyArgmax(logits: []const f64): usize? {
    let found: bool = false;
    let best_idx: usize = 0;
    let best_val: f64 = 0.0;

    let i: usize = 0;
    while (i < logits.len) {
        const v = logits[i];
        if (!isNaN(v)) {
            if (!found || v > best_val) {
                found = true;
                best_idx = i;
                best_val = v;
            }
        }
        i += 1;
    }

    if (!found) {
        return null;
    }
    return best_idx;
}

/// Full zero-allocation LLM token sampling pipeline:
/// copy -> repetition/frequency/presence penalties -> greedy (`temperature == 0`) or
/// temperature scaling + stable softmax -> Top-K -> Top-P -> Min-P -> normalize + SplitMix64 draw.
/// Returns `null` on empty logits, invalid config parameters, insufficient scratch buffers, or all-`NaN` logits.
pub fn sampleToken(
    logits: []const f64,
    recent_tokens: []const usize,
    cfg: SamplerConfig,
    work_logits: *[]f64,
    work_indices: *[]usize,
    work_probs: *[]f64,
): SampledToken? {
    const vocab = logits.len;
    if (vocab == 0) {
        return null;
    }
    if (work_logits.*.len < vocab || work_indices.*.len < vocab || work_probs.*.len < vocab) {
        return null;
    }
    if (
        isNaN(cfg.temperature) || cfg.temperature < 0.0 ||
        isNaN(cfg.top_p) || cfg.top_p <= 0.0 || cfg.top_p > 1.0 ||
        isNaN(cfg.min_p) || cfg.min_p < 0.0 || cfg.min_p > 1.0 ||
        isNaN(cfg.repeat_penalty) || cfg.repeat_penalty <= 0.0 ||
        isNaN(cfg.frequency_penalty) || isNaN(cfg.presence_penalty)
    ) {
        return null;
    }

    // 1. Copy logits to scratch workspace and apply penalties
    let i: usize = 0;
    while (i < vocab) {
        work_logits.*[i] = logits[i];
        i += 1;
    }
    let wl_slice: []f64 = work_logits.*[0..vocab];
    if (!applyRepetitionPenalties(&wl_slice, recent_tokens, cfg.repeat_penalty, cfg.frequency_penalty, cfg.presence_penalty)) {
        return null;
    }

    // 2. Collect non-NaN token indices
    let valid_count: usize = 0;
    let v_idx: usize = 0;
    while (v_idx < vocab) {
        if (!isNaN(work_logits.*[v_idx])) {
            work_indices.*[valid_count] = v_idx;
            valid_count += 1;
        }
        v_idx += 1;
    }
    if (valid_count == 0) {
        return null;
    }

    // 3. Greedy path when temperature == 0.0
    if (cfg.temperature == 0.0) {
        const best_id = greedyArgmax(work_logits.*[0..vocab]) ?? return null;
        return SampledToken {
            token_id: best_id,
            prob: 1.0,
            logit: work_logits.*[best_id],
            candidates_considered: 1,
        };
    }

    // 4. Sort valid candidate indices descending by penalized logit
    sortIndicesDesc(work_indices, work_logits.*[0..vocab], valid_count);

    // 5. Top-K truncation
    let count: usize = valid_count;
    if (cfg.top_k > 0 && cfg.top_k < count) {
        count = cfg.top_k;
    }

    // 6. Numerically stable temperature-scaled softmax over top-K candidates
    const max_logit = work_logits.*[work_indices.*[0]];
    let sum_exp: f64 = 0.0;
    let k: usize = 0;
    while (k < count) {
        const scaled = (work_logits.*[work_indices.*[k]] - max_logit) / cfg.temperature;
        const e = std.math.exp(scaled);
        work_probs.*[k] = e;
        sum_exp += e;
        k += 1;
    }
    if (sum_exp <= 0.0 || isNaN(sum_exp)) {
        const fallback_id = work_indices.*[0];
        return SampledToken {
            token_id: fallback_id,
            prob: 1.0,
            logit: work_logits.*[fallback_id],
            candidates_considered: 1,
        };
    }
    let n1: usize = 0;
    while (n1 < count) {
        work_probs.*[n1] /= sum_exp;
        n1 += 1;
    }

    // 7. Top-P (nucleus) truncation (always retain at least 1 token)
    if (cfg.top_p < 1.0 && count > 1) {
        let cum: f64 = 0.0;
        let keep: usize = 0;
        while (keep < count) {
            cum += work_probs.*[keep];
            keep += 1;
            if (cum >= cfg.top_p) {
                break;
            }
        }
        count = keep;
    }

    // 8. Min-P truncation relative to peak probability (always retain at least 1 token)
    if (cfg.min_p > 0.0 && count > 1) {
        const thresh = cfg.min_p * work_probs.*[0];
        let keep: usize = 1;
        while (keep < count && work_probs.*[keep] >= thresh) {
            keep += 1;
        }
        count = keep;
    }

    // 9. Renormalize truncated distribution
    let trunc_sum: f64 = 0.0;
    let r: usize = 0;
    while (r < count) {
        trunc_sum += work_probs.*[r];
        r += 1;
    }
    let r2: usize = 0;
    while (r2 < count) {
        work_probs.*[r2] /= trunc_sum;
        r2 += 1;
    }

    // 10. Deterministic SplitMix64 draw
    const u = splitMix64UnitFloat(cfg.seed);
    let acc: f64 = 0.0;
    let chosen_pos: usize = count - 1;
    let s: usize = 0;
    while (s < count) {
        acc += work_probs.*[s];
        if (u < acc) {
            chosen_pos = s;
            break;
        }
        s += 1;
    }

    const chosen_id = work_indices.*[chosen_pos];
    return SampledToken {
        token_id: chosen_id,
        prob: work_probs.*[chosen_pos],
        logit: work_logits.*[chosen_id],
        candidates_considered: count,
    };
}
