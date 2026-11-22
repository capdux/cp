// Origin: Pure-CP Stdlib Test Suite (lib/attention/nucleus_sampler.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc LLM Logit Sampling Pipeline (Penalties + Top-K + Min-P + Top-P):
//   1. temperature == 0 greedy fast path picks exact max logit & breaks ties by ascending token_id
//   2. applyTokenPenaltiesInPlace: Keskar positive/negative repeat_penalty + OpenAI freq & presence penalties
//   3. Min-P adaptive truncation eliminates low-probability long tail while preserving high-mass head
//   4. Top-P (Nucleus) cumulative probability truncation, renormalization to 1.0 & min-1-candidate invariant
//   5. Multi-seed empirical sampling distribution converges to exact Softmax probabilities
//   6. Temperature-entropy monotonicity (low temp -> low entropy, high temp -> high entropy)
//   7. Boundary guards: NaN / -Inf / +Inf dirty logits, all-non-finite vocab, empty vocab & invalid configs

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SAMPLER_MAX_CANDIDATES,
    NucleusSamplerConfig,
    SamplerCandidate,
    NucleusSampleOutcome,
    applyTokenPenaltiesInPlace,
    selectTopKCandidates,
    softmaxAndTruncateNucleus,
    sampleFromLogits,
} from "cap:attention";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: Greedy Fast Path (temperature == 0) & Ascending token_id Tie-Break
// ============================================================================
test("nucleus_sampler: temperature == 0 greedy fast path & deterministic tie-break", () => {
    expect_eq(SAMPLER_MAX_CANDIDATES, 256);

    let logits_buf: [6]f32 = [1.0, 4.5, 2.0, 4.5, -1.0, 3.0];
    let logits: []f32 = logits_buf[0..];
    const empty_recent: [1]u32 = [0];

    let cfg = NucleusSamplerConfig.defaults();
    cfg.temperature = 0.0;

    const out: NucleusSampleOutcome = sampleFromLogits(logits, empty_recent[0..0], cfg, 42);
    expect_eq(out.ok, true);
    expect_eq(out.token_id, 1); // ties at 4.5 (idx 1 & 3) -> ascending token_id picks 1
    expect_eq(approxEqF32(out.selected_prob, 1.0, 0.0001), true);
    expect_eq(out.kept_candidates, 1);
    expect_eq(approxEqF32(out.entropy_nats, 0.0, 0.0001), true);
});

// ============================================================================
// Test 2: Repetition (Pos/Neg), Frequency & Presence Penalties Exact Calculation
// ============================================================================
test("nucleus_sampler: Keskar repeat_penalty (pos/neg) + OpenAI freq & presence penalties", () => {
    // token 0: +4.0 appears 2x -> 4.0 / 2.0 - (0.5 * 2 + 0.25) = 2.0 - 1.25 = 0.75
    // token 1: -2.0 appears 1x -> -2.0 * 2.0 - (0.5 * 1 + 0.25) = -4.0 - 0.75 = -4.75
    // token 2: +3.0 not in recent -> untouched (+3.0)
    // recent also includes out-of-bounds token 999 -> safely ignored
    let logits_buf: [3]f32 = [4.0, -2.0, 3.0];
    let logits: []f32 = logits_buf[0..];
    const recent: [4]u32 = [0, 1, 0, 999];

    applyTokenPenaltiesInPlace(logits, recent[0..], 2.0, 0.5, 0.25);
    expect_eq(approxEqF32(logits_buf[0], 0.75, 0.0001), true);
    expect_eq(approxEqF32(logits_buf[1], -4.75, 0.0001), true);
    expect_eq(approxEqF32(logits_buf[2], 3.0, 0.0001), true);
});

// ============================================================================
// Test 3: Min-P Adaptive Tail Truncation
// ============================================================================
test("nucleus_sampler: Min-P truncates low-probability tail relative to max_prob", () => {
    // Logits: [4.0, 3.5, 0.0, -2.0] at T=1.0
    // exp deltas from 4.0: [1.0, 0.6065, 0.0183, 0.0025] -> sum ≈ 1.6273
    // probs ≈ [0.6145, 0.3727, 0.0112, 0.0015]
    // With min_p = 0.10, threshold = 0.10 * 0.6145 = 0.06145 -> keeps only top 2 (tokens 0 & 1)!
    const logits: [4]f32 = [4.0, 3.5, 0.0, -2.0];
    let cands_buf: [4]SamplerCandidate = [SamplerCandidate{}; 4];
    let cands: []SamplerCandidate = cands_buf[0..];

    const n = selectTopKCandidates(logits[0..], 4, cands);
    expect_eq(n, 4);

    const kept = softmaxAndTruncateNucleus(cands, n, 1.0, 1.0, 0.10);
    expect_eq(kept, 2);
    expect_eq(cands_buf[0].token_id, 0);
    expect_eq(cands_buf[1].token_id, 1);
    // Surviving probabilities must renormalize to 1.0
    expect_eq(approxEqF32(cands_buf[0].prob + cands_buf[1].prob, 1.0, 0.0001), true);
});

// ============================================================================
// Test 4: Top-P (Nucleus) Cumulative Truncation & Min-1-Candidate Guarantee
// ============================================================================
test("nucleus_sampler: Top-P nucleus truncation & minimum 1 candidate guarantee", () => {
    // Equal logits [1.0, 1.0, 1.0, 1.0] -> probs [0.25, 0.25, 0.25, 0.25]
    const logits: [4]f32 = [1.0, 1.0, 1.0, 1.0];
    let cands_buf: [4]SamplerCandidate = [SamplerCandidate{}; 4];
    let cands: []SamplerCandidate = cands_buf[0..];

    const n = selectTopKCandidates(logits[0..], 4, cands);
    // top_p = 0.45 -> 0.25 < 0.45, 0.25 + 0.25 = 0.50 >= 0.45 -> keeps exactly 2
    const kept2 = softmaxAndTruncateNucleus(cands, n, 1.0, 0.45, 0.0);
    expect_eq(kept2, 2);
    expect_eq(approxEqF32(cands_buf[0].prob, 0.5, 0.0001), true);
    expect_eq(approxEqF32(cands_buf[1].prob, 0.5, 0.0001), true);

    // Very small top_p = 0.01 (< max_prob 0.25) -> MUST still keep at least 1 candidate
    const n2 = selectTopKCandidates(logits[0..], 4, cands);
    const kept1 = softmaxAndTruncateNucleus(cands, n2, 1.0, 0.01, 0.0);
    expect_eq(kept1, 1);
    expect_eq(approxEqF32(cands_buf[0].prob, 1.0, 0.0001), true);
});

// ============================================================================
// Test 5: Multi-Seed Sampling Distribution Converges to True Softmax
// ============================================================================
test("nucleus_sampler: multi-seed empirical distribution converges to exact Softmax", () => {
    // Logits: [ln(7), ln(3)] -> exact Softmax at T=1.0 is [0.70, 0.30]
    const ln7: f32 = 1.9459101;
    const ln3: f32 = 1.0986123;
    const empty_recent: [1]u32 = [0];

    let cfg = NucleusSamplerConfig.defaults();
    cfg.temperature = 1.0;
    cfg.top_k = 0;
    cfg.top_p = 1.0;
    cfg.min_p = 0.0;

    let c0: usize = 0;
    let c1: usize = 0;
    let seed: u64 = 1;
    while (seed <= 4000) : (seed += 1) {
        let logits_buf: [2]f32 = [ln7, ln3];
        let logits: []f32 = logits_buf[0..];
        const res = sampleFromLogits(logits, empty_recent[0..0], cfg, seed);
        expect_eq(res.ok, true);
        if (res.token_id == 0) {
            c0 += 1;
        } else {
            c1 += 1;
        }
    }

    // Expected: c0 ≈ 2800 (70%), c1 ≈ 1200 (30%) out of 4000 (+/- 150)
    expect_eq(c0 >= 2650 && c0 <= 2950, true);
    expect_eq(c1 >= 1050 && c1 <= 1350, true);
});

// ============================================================================
// Test 6: Temperature-Entropy Monotonicity (Low Temp < Mid Temp < High Temp)
// ============================================================================
test("nucleus_sampler: Shannon entropy increases monotonically with temperature", () => {
    const empty_recent: [1]u32 = [0];
    let cfg = NucleusSamplerConfig.defaults();
    cfg.top_k = 0;
    cfg.top_p = 1.0;
    cfg.min_p = 0.0;

    let l_low: [4]f32 = [3.0, 2.0, 1.0, 0.0];
    let l_mid: [4]f32 = [3.0, 2.0, 1.0, 0.0];
    let l_hi: [4]f32 = [3.0, 2.0, 1.0, 0.0];

    cfg.temperature = 0.25;
    const out_low = sampleFromLogits(l_low[0..], empty_recent[0..0], cfg, 100);
    cfg.temperature = 1.0;
    const out_mid = sampleFromLogits(l_mid[0..], empty_recent[0..0], cfg, 100);
    cfg.temperature = 3.0;
    const out_hi = sampleFromLogits(l_hi[0..], empty_recent[0..0], cfg, 100);

    expect_eq(out_low.ok, true);
    expect_eq(out_mid.ok, true);
    expect_eq(out_hi.ok, true);
    expect_eq(out_low.entropy_nats < out_mid.entropy_nats, true);
    expect_eq(out_mid.entropy_nats < out_hi.entropy_nats, true);
});

// ============================================================================
// Test 7: Dirty Logits (NaN / -Inf / +Inf), Empty Vocab & Invalid Config Guards
// ============================================================================
test("nucleus_sampler: NaN/-Inf/+Inf filtering, empty vocab & invalid config guards", () => {
    const nan_f32: f32 = std.math.nan(f32);
    const neg_inf: f32 = -std.math.inf(f32);
    const pos_inf: f32 = std.math.inf(f32);
    const empty_recent: [1]u32 = [0];

    // Dirty logits containing NaN, -Inf, +Inf alongside two valid finite logits (idx 2=1.5, idx 4=2.5)
    let dirty_buf: [5]f32 = [nan_f32, neg_inf, 1.5, pos_inf, 2.5];
    let dirty: []f32 = dirty_buf[0..];
    let cfg = NucleusSamplerConfig.defaults();
    cfg.top_k = 10;
    cfg.min_p = 0.0;
    cfg.top_p = 1.0;

    const out = sampleFromLogits(dirty, empty_recent[0..0], cfg, 777);
    expect_eq(out.ok, true);
    expect_eq(out.kept_candidates, 2);
    expect_eq(out.token_id == 2 || out.token_id == 4, true);

    // All-non-finite logits -> safely returns ok = false
    let all_bad: [2]f32 = [nan_f32, neg_inf];
    expect_eq(sampleFromLogits(all_bad[0..], empty_recent[0..0], cfg, 1).ok, false);

    // Empty logits slice -> ok = false
    expect_eq(sampleFromLogits(all_bad[0..0], empty_recent[0..0], cfg, 1).ok, false);

    // Invalid configs -> isValid() == false & sampleFromLogits returns ok = false
    let bad_temp = NucleusSamplerConfig.defaults();
    bad_temp.temperature = -0.1;
    expect_eq(bad_temp.isValid(), false);

    let bad_top_p = NucleusSamplerConfig.defaults();
    bad_top_p.top_p = 0.0;
    expect_eq(bad_top_p.isValid(), false);

    let bad_min_p = NucleusSamplerConfig.defaults();
    bad_min_p.min_p = 1.5;
    expect_eq(bad_min_p.isValid(), false);

    let bad_rep = NucleusSamplerConfig.defaults();
    bad_rep.repeat_penalty = 0.9;
    expect_eq(bad_rep.isValid(), false);

    let valid_l: [2]f32 = [1.0, 2.0];
    expect_eq(sampleFromLogits(valid_l[0..], empty_recent[0..0], bad_temp, 1).ok, false);
});
