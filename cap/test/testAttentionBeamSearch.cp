// Origin: Pure-CP Stdlib Test Suite (lib/attention/beamSearch.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Beam Search & Diverse Beam Decoding:
//   1. Standard Beam Search global optimum recovery (defeating greedy trap) & dense logits Log-Softmax step
//   2. Google NMT length penalty ((5 + len) / 6)^alpha exact math & short-vs-long hypothesis reranking
//   3. Exact no_repeat_ngram_size (unigram/bigram/trigram) blocking & telemetry counters
//   4. Diverse Beam Search multi-group Hamming diversity penalty forcing inter-group token separation
//   5. EOS archiving, min_len guard, early_stopping vs non-early-stopping convergence & topHypotheses ranking
//   6. Max sequence length termination, active-beam fallback & zero-alloc reset() session reuse
//   7. Full malformed config / NaN / Inf / truncated slice boundary guards & cap.attention facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    BEAM_MAX_WIDTH,
    BEAM_MAX_SEQ_LEN,
    BEAM_MAX_GROUPS,
    BEAM_MAX_FINISHED,
    beamLengthPenalty,
    beamNormalizedScore,
    beamHasRepeatedNgram,
    BeamCandidate,
    BeamSearchConfig,
    BeamHypothesis,
    BeamStepOutcome,
    BeamSearchStats,
    BeamSearchDecoder,
} from "cap:attention";

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

test("BeamSearch: standard beam search recovers global optimum over greedy trap + stepFromLogits") {
    let cfg = BeamSearchConfig.defaults();
    cfg.beam_width = 2;
    cfg.max_seq_len = 16;
    cfg.length_penalty_alpha = 0.0;
    cfg.early_stopping = true;
    cfg.eos_token_id = 99;

    const prompt: [1]u32 = [1];
    let decoder = BeamSearchDecoder.init(cfg, prompt[0..]) orelse {
        expect_true(false);
        return;
    };
    expect_eq(decoder.activeCount(), 1 as usize);

    // Step 1: Token 10 (-0.2) looks better greedily than Token 20 (-0.5),
    // but Token 10 leads to a poor continuation while Token 20 leads to a strong continuation.
    const step1_cands: [3]BeamCandidate = [
        BeamCandidate{ token_id: 10, log_prob: -0.2 },
        BeamCandidate{ token_id: 20, log_prob: -0.5 },
        BeamCandidate{ token_id: 30, log_prob: -2.0 },
    ];
    const out1 = decoder.step(step1_cands[0..], 3, 99) orelse {
        expect_true(false);
        return;
    };
    expect_eq(out1.step_index, 1 as usize);
    expect_eq(out1.active_beams, 2 as usize);
    expect_eq(out1.stopped, false);

    const b0 = decoder.activeBeam(0) orelse {
        expect_true(false);
        return;
    };
    const b1 = decoder.activeBeam(1) orelse {
        expect_true(false);
        return;
    };
    expect_eq(b0.tokens[1], 10 as u32);
    expect_eq(b1.tokens[1], 20 as u32);

    // Step 2: Beam 0 (token 10) has weak continuations (-2.0, -2.5);
    //         Beam 1 (token 20) has strong continuation token 21 (-0.1) -> total -0.6!
    const step2_cands: [4]BeamCandidate = [
        // Row 0 (parent = [1, 10], lp = -0.2)
        BeamCandidate{ token_id: 11, log_prob: -2.0 },
        BeamCandidate{ token_id: 12, log_prob: -2.5 },
        // Row 1 (parent = [1, 20], lp = -0.5)
        BeamCandidate{ token_id: 21, log_prob: -0.1 },
        BeamCandidate{ token_id: 22, log_prob: -0.4 },
    ];
    const out2 = decoder.step(step2_cands[0..], 2, 99) orelse {
        expect_true(false);
        return;
    };
    expect_eq(out2.active_beams, 2 as usize);

    const best_active = decoder.bestHypothesis() orelse {
        expect_true(false);
        return;
    };
    expect_eq(best_active.len, 3 as usize);
    expect_eq(best_active.tokens[0], 1 as u32);
    expect_eq(best_active.tokens[1], 20 as u32);
    expect_eq(best_active.tokens[2], 21 as u32);
    expect_true(absF32(best_active.log_prob - (-0.6)) < 1e-5);

    // Step 3 via stepFromLogits (vocab_size = 4, eos = 3):
    // Both active beams strongly prefer EOS (token 3).
    const logits: [8]f32 = [
        // Beam 0 ([1, 20, 21]): token 3 dominates
        0.0, 0.0, 0.0, 10.0,
        // Beam 1 ([1, 20, 22]): token 3 dominates
        0.0, 0.0, 0.0, 10.0,
    ];
    const out3 = decoder.stepFromLogits(logits[0..], 4, 3) orelse {
        expect_true(false);
        return;
    };
    expect_eq(out3.newly_finished, 2 as usize);
    expect_eq(out3.total_finished, 2 as usize);
    expect_eq(out3.stopped, true);
    expect_true(decoder.isDone());

    const final_best = decoder.bestHypothesis() orelse {
        expect_true(false);
        return;
    };
    expect_true(final_best.finished);
    expect_eq(final_best.len, 4 as usize);
    expect_eq(final_best.tokens[1], 20 as u32);
    expect_eq(final_best.tokens[2], 21 as u32);
    expect_eq(final_best.tokens[3], 3 as u32);
}

test("BeamSearch: Google NMT length penalty exact formula and short-vs-long reranking") {
    // Exact Google NMT formula: ((5 + len) / 6)^alpha
    expect_true(absF32(beamLengthPenalty(1, 0.6) - 1.0) < 1e-6);
    expect_true(absF32(beamLengthPenalty(10, 0.0) - 1.0) < 1e-6);
    const lp7_1: f32 = beamLengthPenalty(7, 1.0); // (5 + 7) / 6 = 2.0
    expect_true(absF32(lp7_1 - 2.0) < 1e-5);
    const ns_test: f32 = beamNormalizedScore(-4.0, 7, 1.0); // -4.0 / 2.0 = -2.0
    expect_true(absF32(ns_test - (-2.0)) < 1e-5);

    // Compare alpha = 0.0 (favors short sequence) vs alpha = 1.5 (favors longer sequence)
    const prompt: [1]u32 = [1];

    // Case A: alpha = 0.0 -> short EOS sequence (len=2, lp=-1.5) beats longer sequence (len=5, lp=-2.0)
    let cfg_no_pen = BeamSearchConfig.defaults();
    cfg_no_pen.beam_width = 2;
    cfg_no_pen.max_seq_len = 12;
    cfg_no_pen.length_penalty_alpha = 0.0;
    cfg_no_pen.early_stopping = false;
    cfg_no_pen.eos_token_id = 2;

    let dec_a = BeamSearchDecoder.init(cfg_no_pen, prompt[0..]) orelse {
        expect_true(false);
        return;
    };
    // Step 1: EOS with -1.5 (archived, len=2, score=-1.5), token 10 with -0.5, token 11 with -3.0
    const s1: [3]BeamCandidate = [
        BeamCandidate{ token_id: 10, log_prob: -0.5 },
        BeamCandidate{ token_id: 2, log_prob: -1.5 },
        BeamCandidate{ token_id: 11, log_prob: -3.0 },
    ];
    * = dec_a.step(s1[0..], 3, 2);
    // Step 2: continue token 10 -> 12 (-0.5), token 11 -> 13 (-0.5)
    const s2: [4]BeamCandidate = [
        BeamCandidate{ token_id: 12, log_prob: -0.5 },
        BeamCandidate{ token_id: 14, log_prob: -4.0 },
        BeamCandidate{ token_id: 13, log_prob: -0.5 },
        BeamCandidate{ token_id: 15, log_prob: -4.0 },
    ];
    * = dec_a.step(s2[0..], 2, 2);
    // Step 3: continue token 12 -> 16 (-0.5)
    const s3: [4]BeamCandidate = [
        BeamCandidate{ token_id: 16, log_prob: -0.5 },
        BeamCandidate{ token_id: 17, log_prob: -4.0 },
        BeamCandidate{ token_id: 18, log_prob: -0.5 },
        BeamCandidate{ token_id: 19, log_prob: -4.0 },
    ];
    * = dec_a.step(s3[0..], 2, 2);
    // Step 4: emit EOS (-0.5) -> long seq len=5, total lp = -2.0
    const s4: [4]BeamCandidate = [
        BeamCandidate{ token_id: 2, log_prob: -0.5 },
        BeamCandidate{ token_id: 20, log_prob: -4.0 },
        BeamCandidate{ token_id: 2, log_prob: -0.5 },
        BeamCandidate{ token_id: 21, log_prob: -4.0 },
    ];
    * = dec_a.step(s4[0..], 2, 2);

    const best_a = dec_a.bestHypothesis() orelse {
        expect_true(false);
        return;
    };
    expect_eq(best_a.len, 2 as usize); // short seq (-1.5 > -2.0) wins when alpha=0.0

    // Case B: alpha = 1.5 -> same exact trajectory, now long seq (len=5, lp=-2.0) beats short seq (len=2, lp=-1.5)!
    // penalty(2, 1.5) = (7/6)^1.5 ~= 1.260 -> -1.5 / 1.260 ~= -1.190
    // penalty(5, 1.5) = (10/6)^1.5 ~= 2.152 -> -2.0 / 2.152 ~= -0.929 > -1.190
    let cfg_pen = cfg_no_pen;
    cfg_pen.length_penalty_alpha = 1.5;
    let dec_b = BeamSearchDecoder.init(cfg_pen, prompt[0..]) orelse {
        expect_true(false);
        return;
    };
    * = dec_b.step(s1[0..], 3, 2);
    * = dec_b.step(s2[0..], 2, 2);
    * = dec_b.step(s3[0..], 2, 2);
    * = dec_b.step(s4[0..], 2, 2);

    const best_b = dec_b.bestHypothesis() orelse {
        expect_true(false);
        return;
    };
    expect_eq(best_b.len, 5 as usize);
    expect_true(best_b.normalized_score > -1.0);
}

test("BeamSearch: no_repeat_ngram_size blocks repeated unigrams, bigrams and trigrams") {
    const hist: [5]u32 = [10, 20, 30, 10, 20];
    expect_eq(beamHasRepeatedNgram(hist[0..], 30, 0), false);
    expect_eq(beamHasRepeatedNgram(hist[0..], 10, 1), true);
    expect_eq(beamHasRepeatedNgram(hist[0..], 99, 1), false);
    // Last token is 20; bigram [20, 30] already appeared at indices [1, 2]
    expect_eq(beamHasRepeatedNgram(hist[0..], 30, 2), true);
    expect_eq(beamHasRepeatedNgram(hist[0..], 40, 2), false);
    // Last two tokens are [10, 20]; trigram [10, 20, 30] already appeared at indices [0, 1, 2]
    expect_eq(beamHasRepeatedNgram(hist[0..], 30, 3), true);
    expect_eq(beamHasRepeatedNgram(hist[0..], 10, 3), false);

    // End-to-end decoder blocking test with bigram blocking (no_repeat_ngram_size = 2)
    let cfg = BeamSearchConfig.defaults();
    cfg.beam_width = 2;
    cfg.max_seq_len = 16;
    cfg.no_repeat_ngram_size = 2;
    cfg.eos_token_id = 99;

    const prompt: [3]u32 = [10, 20, 10];
    let decoder = BeamSearchDecoder.init(cfg, prompt[0..]) orelse {
        expect_true(false);
        return;
    };

    // Appending 20 would form repeated bigram [10, 20] -> must be blocked despite highest log_prob (-0.01)!
    const cands: [3]BeamCandidate = [
        BeamCandidate{ token_id: 20, log_prob: -0.01 },
        BeamCandidate{ token_id: 30, log_prob: -0.50 },
        BeamCandidate{ token_id: 40, log_prob: -0.80 },
    ];
    const out = decoder.step(cands[0..], 3, 99) orelse {
        expect_true(false);
        return;
    };
    expect_eq(out.blocked_ngrams, 1 as usize);
    expect_eq(out.active_beams, 2 as usize);

    const b0 = decoder.activeBeam(0) orelse {
        expect_true(false);
        return;
    };
    const b1 = decoder.activeBeam(1) orelse {
        expect_true(false);
        return;
    };
    expect_eq(b0.tokens[3], 30 as u32);
    expect_eq(b1.tokens[3], 40 as u32);

    const st = decoder.stats();
    expect_eq(st.blocked_ngram_count, 1 as usize);
}

test("BeamSearch: Diverse Beam Search multi-group Hamming penalty enforces inter-group diversity") {
    let cfg = BeamSearchConfig.defaults();
    cfg.beam_width = 4;
    cfg.num_groups = 2; // 2 groups x 2 beams per group
    cfg.diversity_penalty = 2.0;
    cfg.max_seq_len = 16;
    cfg.length_penalty_alpha = 0.0;
    cfg.eos_token_id = 99;

    const prompt: [1]u32 = [1];
    let decoder = BeamSearchDecoder.init(cfg, prompt[0..]) orelse {
        expect_true(false);
        return;
    };
    // Initial active beams = num_groups = 2 (one seed beam per group)
    expect_eq(decoder.activeCount(), 2 as usize);

    // Identical candidate scores for both group 0 and group 1:
    // Without diversity penalty, both groups would pick tokens 10 and 11.
    // With diversity_penalty = 2.0, group 0 picks {10, 11}, so group 1 gets -2.0 on 10 and 11
    // and must select {12, 13} instead!
    const step1: [8]BeamCandidate = [
        // Group 0 seed beam
        BeamCandidate{ token_id: 10, log_prob: -0.1 },
        BeamCandidate{ token_id: 11, log_prob: -0.2 },
        BeamCandidate{ token_id: 12, log_prob: -0.3 },
        BeamCandidate{ token_id: 13, log_prob: -0.4 },
        // Group 1 seed beam
        BeamCandidate{ token_id: 10, log_prob: -0.1 },
        BeamCandidate{ token_id: 11, log_prob: -0.2 },
        BeamCandidate{ token_id: 12, log_prob: -0.3 },
        BeamCandidate{ token_id: 13, log_prob: -0.4 },
    ];
    const out1 = decoder.step(step1[0..], 4, 99) orelse {
        expect_true(false);
        return;
    };
    expect_eq(out1.active_beams, 4 as usize);

    const g0_b0 = decoder.activeBeam(0) orelse { expect_true(false); return; };
    const g0_b1 = decoder.activeBeam(1) orelse { expect_true(false); return; };
    const g1_b0 = decoder.activeBeam(2) orelse { expect_true(false); return; };
    const g1_b1 = decoder.activeBeam(3) orelse { expect_true(false); return; };

    expect_eq(g0_b0.group_id, 0 as u16);
    expect_eq(g0_b1.group_id, 0 as u16);
    expect_eq(g1_b0.group_id, 1 as u16);
    expect_eq(g1_b1.group_id, 1 as u16);

    expect_eq(g0_b0.tokens[1], 10 as u32);
    expect_eq(g0_b1.tokens[1], 11 as u32);
    expect_eq(g1_b0.tokens[1], 12 as u32);
    expect_eq(g1_b1.tokens[1], 13 as u32);

    // True cumulative log_probs are preserved uncorrupted for fair final scoring.
    expect_true(absF32(g1_b0.log_prob - (-0.3)) < 1e-5);
    expect_true(absF32(g1_b1.log_prob - (-0.4)) < 1e-5);

    const st = decoder.stats();
    expect_true(st.diversity_penalties_applied >= 2);
}

test("BeamSearch: EOS archiving, min_len guard, early_stopping & topHypotheses extraction") {
    let cfg = BeamSearchConfig.defaults();
    cfg.beam_width = 2;
    cfg.max_seq_len = 10;
    cfg.length_penalty_alpha = 0.0;
    cfg.early_stopping = true;
    cfg.eos_token_id = 2;
    cfg.min_len = 3; // prompt(1) + at least 2 generated tokens before EOS allowed

    const prompt: [1]u32 = [1];
    let decoder = BeamSearchDecoder.init(cfg, prompt[0..]) orelse {
        expect_true(false);
        return;
    };

    // Step 1 (resulting len = 2 < min_len = 3): EOS token 2 must be ignored!
    const s1: [3]BeamCandidate = [
        BeamCandidate{ token_id: 2, log_prob: -0.01 },
        BeamCandidate{ token_id: 10, log_prob: -0.4 },
        BeamCandidate{ token_id: 11, log_prob: -0.6 },
    ];
    const o1 = decoder.step(s1[0..], 3, 2) orelse { expect_true(false); return; };
    expect_eq(o1.newly_finished, 0 as usize);
    expect_eq(o1.total_finished, 0 as usize);
    expect_eq(o1.active_beams, 2 as usize);

    // Step 2 (resulting len = 3 >= min_len = 3): both beams emit EOS -> archived & early stopped!
    const s2: [4]BeamCandidate = [
        BeamCandidate{ token_id: 2, log_prob: -0.2 }, // total -0.6
        BeamCandidate{ token_id: 20, log_prob: -1.5 },
        BeamCandidate{ token_id: 2, log_prob: -0.1 }, // total -0.7
        BeamCandidate{ token_id: 21, log_prob: -1.5 },
    ];
    const o2 = decoder.step(s2[0..], 2, 2) orelse { expect_true(false); return; };
    expect_eq(o2.newly_finished, 2 as usize);
    expect_eq(o2.total_finished, 2 as usize);
    expect_eq(o2.stopped, true);
    expect_true(decoder.stats().early_stopped);

    // Idempotent step after done
    const o3 = decoder.step(s2[0..], 2, 2) orelse { expect_true(false); return; };
    expect_eq(o3.stopped, true);
    expect_eq(o3.step_index, 2 as usize);

    // Extract top hypotheses & verify descending order
    let out_hyps: [4]BeamHypothesis = [BeamHypothesis.empty(); 4];
    let out_s: []BeamHypothesis = out_hyps[0..];
    const n_fin: usize = decoder.finishedHypotheses(*out_s);
    expect_eq(n_fin, 2 as usize);
    expect_true(out_hyps[0].normalized_score >= out_hyps[1].normalized_score);
    expect_eq(out_hyps[0].tokens[1], 10 as u32);
    expect_eq(out_hyps[1].tokens[1], 11 as u32);

    const n_top: usize = decoder.topHypotheses(*out_s);
    expect_true(n_top >= 2);
    expect_eq(out_hyps[0].finished, true);
}

test("BeamSearch: max_seq_len termination, active-beam fallback & zero-alloc reset reuse") {
    let cfg = BeamSearchConfig.defaults();
    cfg.beam_width = 2;
    cfg.max_seq_len = 3;
    cfg.length_penalty_alpha = 0.6;
    cfg.eos_token_id = 99;

    const prompt: [1]u32 = [5];
    let decoder = BeamSearchDecoder.init(cfg, prompt[0..]) orelse {
        expect_true(false);
        return;
    };

    const s1: [2]BeamCandidate = [
        BeamCandidate{ token_id: 10, log_prob: -0.2 },
        BeamCandidate{ token_id: 11, log_prob: -0.5 },
    ];
    const o1 = decoder.step(s1[0..], 2, 99) orelse { expect_true(false); return; };
    expect_eq(o1.stopped, false);

    const s2: [4]BeamCandidate = [
        BeamCandidate{ token_id: 20, log_prob: -0.3 },
        BeamCandidate{ token_id: 21, log_prob: -0.8 },
        BeamCandidate{ token_id: 22, log_prob: -0.1 },
        BeamCandidate{ token_id: 23, log_prob: -0.9 },
    ];
    const o2 = decoder.step(s2[0..], 2, 99) orelse { expect_true(false); return; };
    expect_eq(o2.stopped, true);
    expect_true(decoder.stats().reached_max_len);
    expect_eq(decoder.stats().finished_count, 0 as usize);

    // Fallback to best active beam when no hypothesis emitted EOS
    const best = decoder.bestHypothesis() orelse { expect_true(false); return; };
    expect_eq(best.finished, false);
    expect_eq(best.len, 3 as usize);
    expect_eq(best.tokens[1], 10 as u32);
    expect_eq(best.tokens[2], 20 as u32);

    // Reset and reuse same stack decoder cleanly
    const prompt2: [2]u32 = [7, 8];
    expect_true(decoder.reset(prompt2[0..]));
    expect_eq(decoder.isDone(), false);
    expect_eq(decoder.stats().steps, 0 as usize);
    expect_eq(decoder.stats().finished_count, 0 as usize);
    expect_eq(decoder.stats().reached_max_len, false);
    expect_eq(decoder.activeCount(), 1 as usize);
}

test("BeamSearch: malformed config, NaN/Inf candidates, truncated slices & cap.attention facade") {
    const prompt: [1]u32 = [1];

    // Invalid beam_width = 0 and > BEAM_MAX_WIDTH
    let bad1 = BeamSearchConfig.defaults();
    bad1.beam_width = 0;
    expect_true(BeamSearchDecoder.init(bad1, prompt[0..]) == null);

    let bad2 = BeamSearchConfig.defaults();
    bad2.beam_width = BEAM_MAX_WIDTH + 1;
    expect_true(BeamSearchDecoder.init(bad2, prompt[0..]) == null);

    // Invalid max_seq_len = 0 and > BEAM_MAX_SEQ_LEN
    let bad3 = BeamSearchConfig.defaults();
    bad3.max_seq_len = 0;
    expect_true(BeamSearchDecoder.init(bad3, prompt[0..]) == null);

    let bad4 = BeamSearchConfig.defaults();
    bad4.max_seq_len = BEAM_MAX_SEQ_LEN + 1;
    expect_true(BeamSearchDecoder.init(bad4, prompt[0..]) == null);

    // Invalid num_groups (not dividing beam_width or > BEAM_MAX_GROUPS)
    let bad5 = BeamSearchConfig.defaults();
    bad5.beam_width = 4;
    bad5.num_groups = 3;
    expect_true(BeamSearchDecoder.init(bad5, prompt[0..]) == null);

    let bad6 = BeamSearchConfig.defaults();
    bad6.beam_width = 16;
    bad6.num_groups = BEAM_MAX_GROUPS + 1;
    expect_true(BeamSearchDecoder.init(bad6, prompt[0..]) == null);

    // Negative / NaN penalties
    let bad7 = BeamSearchConfig.defaults();
    bad7.diversity_penalty = -0.1;
    expect_true(BeamSearchDecoder.init(bad7, prompt[0..]) == null);

    let bad8 = BeamSearchConfig.defaults();
    bad8.length_penalty_alpha = std.math.nan(f32);
    expect_true(BeamSearchDecoder.init(bad8, prompt[0..]) == null);

    // Prompt exceeds max_seq_len
    let small_cfg = BeamSearchConfig.defaults();
    small_cfg.max_seq_len = 2;
    const long_prompt: [3]u32 = [1, 2, 3];
    expect_true(BeamSearchDecoder.init(small_cfg, long_prompt[0..]) == null);

    // Runtime step guards: candidates_per_beam == 0, truncated slice, NaN/Inf filtering
    let ok_cfg = BeamSearchConfig.defaults();
    ok_cfg.beam_width = 2;
    let decoder = BeamSearchDecoder.init(ok_cfg, prompt[0..]) orelse {
        expect_true(false);
        return;
    };
    const one_cand: [1]BeamCandidate = [BeamCandidate{ token_id: 5, log_prob: -0.1 }];
    expect_true(decoder.step(one_cand[0..], 0, 2) == null);
    expect_true(decoder.step(one_cand[0..], 2, 2) == null);
    const dummy_logits: [1]f32 = [1.0];
    expect_true(decoder.stepFromLogits(dummy_logits[0..], 0, 2) == null);
    expect_true(decoder.stepFromLogits(dummy_logits[0..], 4, 2) == null);

    // NaN and +Inf candidates are filtered out cleanly; finite candidate survives
    const mixed_cands: [3]BeamCandidate = [
        BeamCandidate{ token_id: 7, log_prob: std.math.nan(f32) },
        BeamCandidate{ token_id: 8, log_prob: std.math.inf(f32) },
        BeamCandidate{ token_id: 9, log_prob: -0.25 },
    ];
    const out_mixed = decoder.step(mixed_cands[0..], 3, 2) orelse {
        expect_true(false);
        return;
    };
    expect_eq(out_mixed.active_beams, 1 as usize);
    const surviving = decoder.activeBeam(0) orelse { expect_true(false); return; };
    expect_eq(surviving.tokens[1], 9 as u32);

    // Verify cap.attention namespace facade parity
    expect_eq(cap.attention.BEAM_MAX_WIDTH, BEAM_MAX_WIDTH);
    expect_eq(cap.attention.BEAM_MAX_SEQ_LEN, BEAM_MAX_SEQ_LEN);
    expect_eq(cap.attention.BEAM_MAX_GROUPS, BEAM_MAX_GROUPS);
    expect_eq(cap.attention.BEAM_MAX_FINISHED, BEAM_MAX_FINISHED);
    const facade_pen: f32 = cap.attention.beamLengthPenalty(7, 1.0);
    expect_true(absF32(facade_pen - 2.0) < 1e-5);
}
