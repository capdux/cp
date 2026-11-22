// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc LLM Logit Sampling Pipeline: Penalties + Top-K + Min-P + Top-P Nucleus) | Created: 2026-11-22
// Pure CP zero-heap-allocation llama.cpp / vLLM / SGLang style logit post-processing and sampling pipeline:
//   - Keskar repetition penalty (positive divide / negative multiply) + OpenAI frequency & presence penalties
//   - O(V log K) min-heap Top-K candidate selection filtering NaN / ±Inf with deterministic token_id tie-break
//   - Numerically stable temperature Softmax -> Min-P adaptive tail truncation -> Top-P Nucleus truncation -> renorm
//   - Deterministic 53-bit SplitMix64 sampling with Shannon entropy (nats) telemetry & zero-temp greedy fast path

const std = @import("std");

// ============================================================================
// 1. Constants & Internal Numeric / Heap Helpers
// ============================================================================

/// Maximum candidate tokens retained on stack during Top-K / Min-P / Top-P sampling (zero heap).
pub const SAMPLER_MAX_CANDIDATES: usize = 256;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn splitMix64(seed: u64): u64 {
    let z: u64 = seed +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

/// Strict total order `(logit desc, token_id asc)`: returns `true` if `a` ranks ahead of `b`.
fn candBetter(a: SamplerCandidate, b: SamplerCandidate): bool {
    if (a.logit > b.logit) return true;
    if (a.logit < b.logit) return false;
    return a.token_id < b.token_id;
}

/// Min-heap sift-up where the root holds the worst kept candidate.
fn minHeapSiftUp(heap: *[]SamplerCandidate, idx_in: usize) {
    let idx: usize = idx_in;
    while (idx > 0) {
        const parent: usize = (idx - 1) / 2;
        if (candBetter(heap[parent], heap[idx])) {
            const tmp = heap[parent];
            heap[parent] = heap[idx];
            heap[idx] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

/// Min-heap sift-down where the root holds the worst kept candidate.
fn minHeapSiftDown(heap: *[]SamplerCandidate, count: usize, start_idx: usize) {
    let root: usize = start_idx;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= count) break;
        let worst: usize = root;
        if (candBetter(heap[worst], heap[left])) {
            worst = left;
        }
        const right: usize = left + 1;
        if (right < count && candBetter(heap[worst], heap[right])) {
            worst = right;
        }
        if (worst == root) break;
        const tmp = heap[root];
        heap[root] = heap[worst];
        heap[worst] = tmp;
        root = worst;
    }
}

/// Ensures `cands[0..n]` is sorted strictly descending by `(logit desc, token_id asc)`.
fn ensureSortedDesc(cands: *[]SamplerCandidate, n: usize) {
    let i: usize = 1;
    while (i < n) : (i += 1) {
        const key = cands[i];
        let pos: usize = i;
        while (pos > 0 && candBetter(key, cands[pos - 1])) {
            cands[pos] = cands[pos - 1];
            pos -= 1;
        }
        cands[pos] = key;
    }
}

// ============================================================================
// 2. Public Models: Config, Candidate & Sample Outcome
// ============================================================================

/// Configuration for the zero-alloc LLM logit sampling pipeline.
pub struct NucleusSamplerConfig {
    temperature: f32 = 1.0;
    top_k: usize = 40;
    top_p: f32 = 0.95;
    min_p: f32 = 0.05;
    repeat_penalty: f32 = 1.0;
    frequency_penalty: f32 = 0.0;
    presence_penalty: f32 = 0.0;

    /// Returns production defaults (`temperature=1.0`, `top_k=40`, `top_p=0.95`, `min_p=0.05`).
    static fn defaults(): NucleusSamplerConfig {
        return NucleusSamplerConfig{
            temperature: 1.0,
            top_k: 40,
            top_p: 0.95,
            min_p: 0.05,
            repeat_penalty: 1.0,
            frequency_penalty: 0.0,
            presence_penalty: 0.0,
        };
    }

    /// Validates all numeric bounds and finiteness invariants.
    fn isValid(): bool {
        if (!isFiniteF32(_.temperature) || _.temperature < 0.0) return false;
        if (!isFiniteF32(_.top_p) || _.top_p <= 0.0 || _.top_p > 1.0) return false;
        if (!isFiniteF32(_.min_p) || _.min_p < 0.0 || _.min_p > 1.0) return false;
        if (!isFiniteF32(_.repeat_penalty) || _.repeat_penalty < 1.0) return false;
        if (!isFiniteF32(_.frequency_penalty) || !isFiniteF32(_.presence_penalty)) return false;
        return true;
    }
}

/// Single candidate token entry with raw logit and post-softmax normalized probability.
pub struct SamplerCandidate {
    token_id: u32 = 0;
    logit: f32 = 0.0;
    prob: f32 = 0.0;
}

/// Full outcome returned by `sampleFromLogits`, including selected token and distribution telemetry.
pub struct NucleusSampleOutcome {
    ok: bool = false;
    token_id: u32 = 0;
    selected_prob: f32 = 0.0;
    kept_candidates: usize = 0;
    entropy_nats: f32 = 0.0;
}

// ============================================================================
// 3. Zero-Alloc Logit Penalty, Top-K, Min-P / Top-P & Sampling Kernels
// ============================================================================

/// Applies Keskar repetition penalty and OpenAI frequency/presence penalties to `logits` in place
/// with zero dynamic heap allocation:
/// - Positive logits are divided by `repeat_penalty`; non-positive logits are multiplied by `repeat_penalty`.
/// - Subtracts `freq_penalty * count + presence_penalty` per unique in-bounds token in `recent_tokens`.
/// - Safely ignores out-of-bounds token IDs and non-finite logits.
pub fn applyTokenPenaltiesInPlace(
    logits: *[]f32,
    recent_tokens: []const u32,
    repeat_penalty: f32,
    freq_penalty: f32,
    presence_penalty: f32,
) {
    if (logits.len == 0 || recent_tokens.len == 0) return;
    if (!isFiniteF32(repeat_penalty) || repeat_penalty < 1.0) return;
    if (!isFiniteF32(freq_penalty) || !isFiniteF32(presence_penalty)) return;

    let i: usize = 0;
    while (i < recent_tokens.len) : (i += 1) {
        const tid: u32 = recent_tokens[i];
        const tid_usize: usize = tid as usize;
        if (tid_usize >= logits.len) continue;

        // Skip if already processed at an earlier index in recent_tokens
        let seen_earlier: bool = false;
        let prev: usize = 0;
        while (prev < i) : (prev += 1) {
            if (recent_tokens[prev] == tid) {
                seen_earlier = true;
                break;
            }
        }
        if (seen_earlier) continue;

        // Count total occurrences of tid in recent_tokens
        let count: u32 = 1;
        let j: usize = i + 1;
        while (j < recent_tokens.len) : (j += 1) {
            if (recent_tokens[j] == tid) {
                count += 1;
            }
        }

        let val: f32 = logits[tid_usize];
        if (!isFiniteF32(val)) continue;

        if (repeat_penalty > 1.0) {
            if (val > 0.0) {
                val = val / repeat_penalty;
            } else {
                val = val * repeat_penalty;
            }
        }
        val = val - (freq_penalty * (count as f32) + presence_penalty);
        logits[tid_usize] = val;
    }
}

/// Extracts up to `top_k` finite candidates (`0` means up to `min(out_cands.len, SAMPLER_MAX_CANDIDATES)`)
/// in strict descending order of `(logit desc, token_id asc)`, automatically filtering `NaN` and `±Inf`.
pub fn selectTopKCandidates(
    logits: []const f32,
    top_k: usize,
    out_cands: *[]SamplerCandidate,
): usize {
    if (logits.len == 0 || out_cands.len == 0) return 0;

    let k_limit: usize = out_cands.len;
    if (top_k > 0 && top_k < k_limit) {
        k_limit = top_k;
    }
    if (k_limit > SAMPLER_MAX_CANDIDATES) {
        k_limit = SAMPLER_MAX_CANDIDATES;
    }

    let count: usize = 0;
    let i: usize = 0;
    while (i < logits.len) : (i += 1) {
        const val: f32 = logits[i];
        if (!isFiniteF32(val)) continue;

        const cand = SamplerCandidate{
            token_id: @intCast(i),
            logit: val,
            prob: 0.0,
        };
        if (count < k_limit) {
            out_cands[count] = cand;
            minHeapSiftUp(out_cands, count);
            count += 1;
        } else if (candBetter(cand, out_cands[0])) {
            out_cands[0] = cand;
            minHeapSiftDown(out_cands, count, 0);
        }
    }

    // Heap-sort min-heap in place into descending order (best candidate at index 0)
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = out_cands[0];
        out_cands[0] = out_cands[end];
        out_cands[end] = tmp;
        minHeapSiftDown(out_cands, end, 0);
    }

    return count;
}

/// Performs numerically stable temperature-scaled Softmax followed by:
/// 1. Min-P adaptive tail truncation (`prob >= min_p * max_prob`, keeping at least 1 candidate),
/// 2. Top-P (Nucleus) cumulative probability truncation (keeping at least 1 candidate),
/// 3. Exact probability renormalization so `sum(cands[0..kept].prob) == 1.0`.
/// Returns the number of surviving candidates (`0` on invalid input).
pub fn softmaxAndTruncateNucleus(
    cands: *[]SamplerCandidate,
    count: usize,
    temperature: f32,
    top_p: f32,
    min_p: f32,
): usize {
    const n: usize = if (count > cands.len) cands.len else count;
    if (n == 0) return 0;
    if (!isFiniteF32(temperature) || temperature <= 0.0) return 0;
    if (!isFiniteF32(top_p) || top_p <= 0.0 || top_p > 1.0) return 0;
    if (!isFiniteF32(min_p) || min_p < 0.0 || min_p > 1.0) return 0;

    // Ensure descending order by (logit desc, token_id asc)
    ensureSortedDesc(cands, n);

    const max_logit: f64 = cands[0].logit as f64;
    const inv_temp: f64 = 1.0 / (temperature as f64);

    // Numerically stable Softmax in f64
    let sum_exp: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const scaled: f64 = ((cands[i].logit as f64) - max_logit) * inv_temp;
        const e: f64 = std.math.exp(scaled);
        cands[i].prob = @floatCast(e);
        sum_exp += e;
    }
    if (sum_exp <= 0.0 || std.math.isNan(sum_exp) || std.math.isInf(sum_exp)) {
        return 0;
    }

    let j: usize = 0;
    while (j < n) : (j += 1) {
        cands[j].prob = @floatCast((cands[j].prob as f64) / sum_exp);
    }

    // 1. Min-P adaptive truncation: retain prob >= min_p * max_prob (always keep index 0)
    const max_prob: f64 = cands[0].prob as f64;
    const min_p_thresh: f64 = (min_p as f64) * max_prob;
    let min_p_kept: usize = 1;
    if (min_p > 0.0) {
        while (min_p_kept < n && (cands[min_p_kept].prob as f64) >= min_p_thresh) : (min_p_kept += 1) {}
    } else {
        min_p_kept = n;
    }

    // 2. Top-P (Nucleus) cumulative probability truncation (always keep at least 1)
    let nuc_kept: usize = 0;
    let cum_prob: f64 = 0.0;
    while (nuc_kept < min_p_kept) {
        cum_prob += cands[nuc_kept].prob as f64;
        nuc_kept += 1;
        if (cum_prob >= (top_p as f64)) break;
    }
    if (nuc_kept == 0) {
        nuc_kept = 1;
    }

    // 3. Renormalize surviving distribution to sum to 1.0
    let norm_sum: f64 = 0.0;
    let k: usize = 0;
    while (k < nuc_kept) : (k += 1) {
        norm_sum += cands[k].prob as f64;
    }
    if (norm_sum > 0.0) {
        let m: usize = 0;
        while (m < nuc_kept) : (m += 1) {
            cands[m].prob = @floatCast((cands[m].prob as f64) / norm_sum);
        }
    } else {
        cands[0].prob = 1.0;
        nuc_kept = 1;
    }

    return nuc_kept;
}

/// End-to-end zero-heap-allocation LLM sampling pipeline:
/// penalties -> greedy fast path (`temperature == 0`) -> Top-K -> Softmax + Min-P + Top-P ->
/// deterministic SplitMix64 sampling with Shannon entropy (nats) telemetry.
pub fn sampleFromLogits(
    logits: *[]f32,
    recent_tokens: []const u32,
    cfg: NucleusSamplerConfig,
    rng_seed: u64,
): NucleusSampleOutcome {
    if (logits.len == 0 || !cfg.isValid()) {
        return NucleusSampleOutcome{};
    }

    // 1. Apply repetition / frequency / presence penalties in place
    applyTokenPenaltiesInPlace(
        logits,
        recent_tokens,
        cfg.repeat_penalty,
        cfg.frequency_penalty,
        cfg.presence_penalty,
    );

    // 2. Greedy fast path when temperature == 0.0
    if (cfg.temperature == 0.0) {
        let found: bool = false;
        let best_id: u32 = 0;
        let best_logit: f32 = 0.0;
        let i: usize = 0;
        while (i < logits.len) : (i += 1) {
            const val: f32 = logits[i];
            if (!isFiniteF32(val)) continue;
            if (!found || val > best_logit) {
                found = true;
                best_logit = val;
                best_id = @intCast(i);
            }
        }
        if (!found) return NucleusSampleOutcome{};
        return NucleusSampleOutcome{
            ok: true,
            token_id: best_id,
            selected_prob: 1.0,
            kept_candidates: 1,
            entropy_nats: 0.0,
        };
    }

    // 3. Top-K -> Softmax + Min-P + Top-P
    let cands_buf: [SAMPLER_MAX_CANDIDATES]SamplerCandidate = [SamplerCandidate{}; SAMPLER_MAX_CANDIDATES];
    let cands_s: []SamplerCandidate = cands_buf[0..];
    const k_cnt: usize = selectTopKCandidates(logits[0..], cfg.top_k, cands_s);
    if (k_cnt == 0) return NucleusSampleOutcome{};

    const kept: usize = softmaxAndTruncateNucleus(cands_s, k_cnt, cfg.temperature, cfg.top_p, cfg.min_p);
    if (kept == 0) return NucleusSampleOutcome{};

    // 4. Compute Shannon entropy H = -sum(p * ln(p)) in nats
    let entropy: f64 = 0.0;
    let i: usize = 0;
    while (i < kept) : (i += 1) {
        const p: f64 = cands_buf[i].prob as f64;
        if (p > 0.0) {
            entropy -= p * std.math.log(f64, std.math.e, p);
        }
    }
    if (entropy < 0.0) {
        entropy = 0.0;
    }

    // 5. Deterministic SplitMix64 uniform [0, 1) draw
    const r_u64: u64 = splitMix64(rng_seed);
    const u: f64 = ((r_u64 >> 11) as f64) * (1.0 / 9007199254740992.0);

    let chosen_idx: usize = kept - 1;
    let acc: f64 = 0.0;
    let c: usize = 0;
    while (c < kept) : (c += 1) {
        acc += cands_buf[c].prob as f64;
        if (u < acc) {
            chosen_idx = c;
            break;
        }
    }

    return NucleusSampleOutcome{
        ok: true,
        token_id: cands_buf[chosen_idx].token_id,
        selected_prob: cands_buf[chosen_idx].prob,
        kept_candidates: kept,
        entropy_nats: @floatCast(entropy),
    };
}
