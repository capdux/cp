// Origin: Pure-CP Stdlib Test Suite (lib/attention/spec_rejection.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Leviathan Lossless Speculative Decoding:
//   1. Identical distributions (p == q): 100% acceptance + Bonus Token (k + 1 emitted tokens)
//   2. Zero target support p(x) == 0: guaranteed rejection at step 0 + exact residual resampling
//   3. 4000-seed Monte Carlo proof of lossless equivalence (|empirical - p| < 0.025 for all tokens)
//   4. Partial acceptance mid-stream rejection: exact rejected_at_idx & emitted_count == accepted + 1
//   5. SpecStepScheduler: EWMA grow to max_k, shrink to min_k, deadband hold & clamping
//   6. SpecStepSchedulerConfig invalid parameter guards
//   7. Malformed input guards: empty/oversized draft, dim mismatch, OOB token, NaN/Inf/neg/zero probs

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SPEC_REJECT_MAX_DRAFT_K,
    SpecRejectionOutcome,
    SpecStepSchedulerConfig,
    SpecStepScheduler,
    sampleResidualDistributionF32,
    verifySpeculativeRejectionF32,
} from "cap:attention";

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn testSplitMixNext(state: *u64): u64 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn testSampleFromQ4(q: [4]f32, u: f32): u32 {
    let cdf: f32 = 0.0;
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        cdf += q[i];
        if (u < cdf) return i as u32;
    }
    return 3;
}

// ============================================================================
// Test 1: Identical Distributions (p == q) -> 100% Acceptance + Bonus Token
// ============================================================================

fn testAllAcceptedWithBonusToken()! {
    const draft_tokens: [3]u32 = [1, 3, 0];
    const q_flat: [12]f32 = [
        0.10, 0.60, 0.20, 0.10,
        0.05, 0.15, 0.10, 0.70,
        0.80, 0.05, 0.10, 0.05,
    ];
    // Target matches draft on steps 0..2, plus deterministic bonus token 2 on step 3
    const p_flat: [16]f32 = [
        0.10, 0.60, 0.20, 0.10,
        0.05, 0.15, 0.10, 0.70,
        0.80, 0.05, 0.10, 0.05,
        0.00, 0.00, 1.00, 0.00,
    ];

    let seed: u64 = 1;
    while (seed <= 32) : (seed += 1) {
        const res: SpecRejectionOutcome = cap.attention.verifySpeculativeRejectionF32(
            draft_tokens[0..],
            q_flat[0..],
            p_flat[0..],
            4,
            seed * 1337,
        );
        expect_eq(res.ok, true);
        expect_eq(res.accepted_draft_count, 3);
        expect_eq(res.emitted_count, 4);
        expect_eq(res.rejected_at_idx, 3);
        expect_eq(res.used_bonus_token, true);
        expect_eq(absF32(res.acceptance_rate_f32 - 1.0) < 1.0e-6, true);
        expect_eq(res.emitted_tokens[0], 1);
        expect_eq(res.emitted_tokens[1], 3);
        expect_eq(res.emitted_tokens[2], 0);
        expect_eq(res.emitted_tokens[3], 2);
    }
}

// ============================================================================
// Test 2: Zero Target Probability p(x) == 0 -> Forced Rejection & Residual Resampling
// ============================================================================

fn testZeroTargetSupportRejectsAndResamplesResidual()! {
    // Draft proposes token 0, where p(0) == 0.0; residual max(0, p - q) has mass only on tokens 2 & 3:
    //   q = [0.70, 0.20, 0.05, 0.05]
    //   p = [0.00, 0.20, 0.35, 0.45]
    //   max(0, p - q) = [0.00, 0.00, 0.30, 0.40] -> normalized [0, 0, 3/7, 4/7]
    const draft_tokens: [2]u32 = [0, 1];
    const q_flat: [8]f32 = [
        0.70, 0.20, 0.05, 0.05,
        0.25, 0.25, 0.25, 0.25,
    ];
    const p_flat: [12]f32 = [
        0.00, 0.20, 0.35, 0.45,
        0.25, 0.25, 0.25, 0.25,
        0.25, 0.25, 0.25, 0.25,
    ];

    let count_tok2: u64 = 0;
    let count_tok3: u64 = 0;
    let seed: u64 = 1;
    while (seed <= 200) : (seed += 1) {
        const res = verifySpeculativeRejectionF32(draft_tokens[0..], q_flat[0..], p_flat[0..], 4, seed);
        expect_eq(res.ok, true);
        expect_eq(res.accepted_draft_count, 0);
        expect_eq(res.rejected_at_idx, 0);
        expect_eq(res.emitted_count, 1);
        expect_eq(res.used_bonus_token, false);
        expect_eq(absF32(res.acceptance_rate_f32) < 1.0e-6, true);
        // Must ONLY emit token 2 or token 3 (never 0 or 1!)
        const tok = res.emitted_tokens[0];
        expect_eq(tok == 2 || tok == 3, true);
        if (tok == 2) count_tok2 += 1;
        if (tok == 3) count_tok3 += 1;
    }
    expect_eq(count_tok2 > 50, true);
    expect_eq(count_tok3 > 70, true);

    // Direct residual helper boundary: when p == q (residual mass == 0), falls back safely to target_probs
    const same: [4]f32 = [0.0, 0.0, 1.0, 0.0];
    const fb_tok = sampleResidualDistributionF32(same[0..], same[0..], 0.5) ?? return error.FallbackFailed;
    expect_eq(fb_tok, 2);
}

// ============================================================================
// Test 3: 4000-Seed Monte Carlo Proof of Lossless Target Distribution Equivalence
// ============================================================================

fn testMonteCarloLosslessDistributionEquivalence()! {
    // Non-trivial mismatched distributions:
    //   q = [0.50, 0.25, 0.15, 0.10]  (over-samples token 0, under-samples tokens 1, 2, 3)
    //   p = [0.10, 0.40, 0.30, 0.20]
    // Leviathan theorem guarantees: P(emitted_tokens[0] == v) == p[v] EXACTLY!
    const q_dist: [4]f32 = [0.50, 0.25, 0.15, 0.10];
    const p_flat: [8]f32 = [
        0.10, 0.40, 0.30, 0.20,
        0.25, 0.25, 0.25, 0.25, // bonus distribution (unused for emitted_tokens[0])
    ];

    let counts: [4]u64 = [0, 0, 0, 0];
    let draft_rng: u64 = 0xCAFEBABE12345678;
    const TRIALS: u64 = 4000;

    let t: u64 = 0;
    while (t < TRIALS) : (t += 1) {
        const u_draft: f32 = ((testSplitMixNext(&draft_rng) >> 40) as u32 as f32) * (1.0 / 16777216.0);
        const draft_tok: u32 = testSampleFromQ4(q_dist, u_draft);
        const draft_arr: [1]u32 = [draft_tok];

        const res = verifySpeculativeRejectionF32(
            draft_arr[0..],
            q_dist[0..],
            p_flat[0..],
            4,
            0x9001_0000 +% (t *% 7919),
        );
        expect_eq(res.ok, true);
        expect_eq(res.emitted_count >= 1, true);
        const out_tok: usize = res.emitted_tokens[0] as usize;
        expect_eq(out_tok < 4, true);
        counts[out_tok] += 1;
    }

    const expected: [4]f64 = [0.10, 0.40, 0.30, 0.20];
    let v: usize = 0;
    while (v < 4) : (v += 1) {
        const empirical: f64 = (counts[v] as f64) / (TRIALS as f64);
        expect_eq(absF64(empirical - expected[v]) < 0.025, true);
    }
}

// ============================================================================
// Test 4: Partial Acceptance Mid-Stream Rejection
// ============================================================================

fn testPartialAcceptanceMidStreamRejection()! {
    // k = 4: steps 0 & 1 have p == q (guaranteed accept), step 2 has p(x_2) == 0 (guaranteed reject -> residual token 3)
    const draft_tokens: [4]u32 = [1, 2, 0, 3];
    const q_flat: [16]f32 = [
        0.10, 0.70, 0.10, 0.10,
        0.10, 0.10, 0.70, 0.10,
        0.90, 0.05, 0.05, 0.00,
        0.25, 0.25, 0.25, 0.25,
    ];
    const p_flat: [20]f32 = [
        0.10, 0.70, 0.10, 0.10, // accept 1
        0.10, 0.10, 0.70, 0.10, // accept 2
        0.00, 0.05, 0.05, 0.90, // reject 0 -> residual mass 100% on token 3
        0.25, 0.25, 0.25, 0.25,
        0.25, 0.25, 0.25, 0.25,
    ];

    const res = verifySpeculativeRejectionF32(draft_tokens[0..], q_flat[0..], p_flat[0..], 4, 2026);
    expect_eq(res.ok, true);
    expect_eq(res.accepted_draft_count, 2);
    expect_eq(res.rejected_at_idx, 2);
    expect_eq(res.emitted_count, 3); // accepted + 1
    expect_eq(res.used_bonus_token, false);
    expect_eq(absF32(res.acceptance_rate_f32 - 0.5) < 1.0e-6, true);
    expect_eq(res.emitted_tokens[0], 1);
    expect_eq(res.emitted_tokens[1], 2);
    expect_eq(res.emitted_tokens[2], 3);
}

// ============================================================================
// Test 5: SpecStepScheduler EWMA Auto-Grow, Auto-Shrink & Bounds Clamping
// ============================================================================

fn testSpecStepSchedulerGrowShrinkClamping()! {
    const cfg = SpecStepSchedulerConfig{
        min_k: 2,
        max_k: 6,
        initial_k: 4,
        ewma_alpha: 0.50,
        grow_threshold: 0.75,
        shrink_threshold: 0.45,
    };
    let sched = SpecStepScheduler.init(cfg) ?? return error.SchedulerInitFailed;
    expect_eq(sched.current_k(), 4);

    // High acceptance rounds (100%) -> grows 4 -> 5 -> 6 and clamps at max_k = 6
    expect_eq(sched.observe_round(4, 4), 5);
    expect_eq(sched.observe_round(5, 5), 6);
    expect_eq(sched.observe_round(6, 6), 6); // clamped at max_k
    expect_eq(sched.ewma_acceptance_rate() >= 0.99, true);

    // Low acceptance rounds (0%) -> EWMA decays below 0.45 and shrinks down to min_k = 2
    let r: usize = 0;
    while (r < 10) : (r += 1) {
        * = sched.observe_round(0, sched.current_k());
    }
    expect_eq(sched.current_k(), 2); // clamped at min_k
    expect_eq(sched.ewma_acceptance_rate() < 0.05, true);

    // Zero proposed_k is a no-op
    expect_eq(sched.observe_round(0, 0), 2);
}

// ============================================================================
// Test 6: SpecStepSchedulerConfig Invalid Parameter Guards
// ============================================================================

fn testSchedulerInvalidConfigGuards()! {
    const nan_f: f32 = std.math.nan(f32);
    expect_eq(SpecStepSchedulerConfig.defaults().isValid(), true);

    const bad_min = SpecStepSchedulerConfig{ min_k: 0, max_k: 4, initial_k: 2 };
    const bad_max = SpecStepSchedulerConfig{ min_k: 1, max_k: SPEC_REJECT_MAX_DRAFT_K + 1, initial_k: 4 };
    const bad_init = SpecStepSchedulerConfig{ min_k: 2, max_k: 6, initial_k: 7 };
    const bad_thresh = SpecStepSchedulerConfig{ grow_threshold: 0.50, shrink_threshold: 0.60 };
    const bad_alpha = SpecStepSchedulerConfig{ ewma_alpha: nan_f };

    expect_eq(SpecStepScheduler.init(bad_min) == null, true);
    expect_eq(SpecStepScheduler.init(bad_max) == null, true);
    expect_eq(SpecStepScheduler.init(bad_init) == null, true);
    expect_eq(SpecStepScheduler.init(bad_thresh) == null, true);
    expect_eq(SpecStepScheduler.init(bad_alpha) == null, true);
}

// ============================================================================
// Test 7: Malformed Input, OOB Token & NaN/Inf/Negative Probability Guards
// ============================================================================

fn testMalformedInputAndNanGuards()! {
    const empty_toks: [0]u32 = [];
    const ok_tok: [1]u32 = [0];
    const oob_tok: [1]u32 = [4]; // >= vocab_size 4
    const q4: [4]f32 = [0.25, 0.25, 0.25, 0.25];
    const p8: [8]f32 = [0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25];
    const nan_f: f32 = std.math.nan(f32);
    const inf_f: f32 = std.math.inf(f32);

    // Empty draft tokens
    expect_eq(verifySpeculativeRejectionF32(empty_toks[0..], q4[0..], p8[0..], 4, 1).ok, false);

    // Oversized draft (k = 17 > SPEC_REJECT_MAX_DRAFT_K)
    const big_toks: [17]u32 = [0; 17];
    expect_eq(verifySpeculativeRejectionF32(big_toks[0..], q4[0..], p8[0..], 4, 1).ok, false);

    // Zero vocab_size & mismatched flat dimensions
    expect_eq(verifySpeculativeRejectionF32(ok_tok[0..], q4[0..], p8[0..], 0, 1).ok, false);
    expect_eq(verifySpeculativeRejectionF32(ok_tok[0..], q4[0..3], p8[0..], 4, 1).ok, false);
    expect_eq(verifySpeculativeRejectionF32(ok_tok[0..], q4[0..], p8[0..4], 4, 1).ok, false);

    // Out-of-bounds draft token ID
    expect_eq(verifySpeculativeRejectionF32(oob_tok[0..], q4[0..], p8[0..], 4, 1).ok, false);

    // NaN, Inf, negative, and all-zero probability distributions
    const q_nan: [4]f32 = [nan_f, 0.5, 0.5, 0.0];
    const p_inf: [8]f32 = [inf_f, 0.5, 0.5, 0.0, 0.25, 0.25, 0.25, 0.25];
    const q_neg: [4]f32 = [-0.1, 0.5, 0.3, 0.3];
    const p_zero: [8]f32 = [0.0, 0.0, 0.0, 0.0, 0.25, 0.25, 0.25, 0.25];

    expect_eq(verifySpeculativeRejectionF32(ok_tok[0..], q_nan[0..], p8[0..], 4, 1).ok, false);
    expect_eq(verifySpeculativeRejectionF32(ok_tok[0..], q4[0..], p_inf[0..], 4, 1).ok, false);
    expect_eq(verifySpeculativeRejectionF32(ok_tok[0..], q_neg[0..], p8[0..], 4, 1).ok, false);
    expect_eq(verifySpeculativeRejectionF32(ok_tok[0..], q4[0..], p_zero[0..], 4, 1).ok, false);

    // Direct sampleResidualDistributionF32 guards
    expect_eq(sampleResidualDistributionF32(q_nan[0..], q4[0..], 0.5) == null, true);
    expect_eq(sampleResidualDistributionF32(q4[0..], q4[0..3], 0.5) == null, true);
    expect_eq(sampleResidualDistributionF32(q4[0..], q4[0..], nan_f) == null, true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("attention spec_rejection: p == q accepts all drafts and emits k + 1 tokens with bonus", testAllAcceptedWithBonusToken);
test("attention spec_rejection: p(x) == 0 rejects at step 0 and resamples residual max(0, p - q)", testZeroTargetSupportRejectsAndResamplesResidual);
test("attention spec_rejection: 4000-seed Monte Carlo lossless equivalence to target distribution p", testMonteCarloLosslessDistributionEquivalence);
test("attention spec_rejection: partial acceptance records exact rejected_at_idx and emitted_count", testPartialAcceptanceMidStreamRejection);
test("attention spec_rejection: SpecStepScheduler auto-grows, auto-shrinks, and clamps K", testSpecStepSchedulerGrowShrinkClamping);
test("attention spec_rejection: SpecStepSchedulerConfig rejects invalid bounds and thresholds", testSchedulerInvalidConfigGuards);
test("attention spec_rejection: guards empty/oversized draft, dim mismatch, OOB token & NaN/Inf probs", testMalformedInputAndNanGuards);
