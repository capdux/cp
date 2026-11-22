// Source: cp/cap/test/test_agent_beam_production_deep.cp (Production deep tests for cap:agent beam)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    gnmtLengthPenalty,
    normalizedBeamScore,
    wouldRepeatNgram,
    BeamConfig,
    BeamHyp,
    BeamCandidate,
    BeamSearch,
} from "cap:agent";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

test("1. 2-step delayed high-score path beats greedy trap under BeamSearch width 2", () => {
    const cfg = BeamConfig {
        beam_width: 2,
        max_len: 4,
        eos_token: 0,
        length_penalty_alpha: 0.0,
        no_repeat_ngram: 0,
    };
    let ta: [8]u32 = std.mem.zeroes([8]u32);
    let tb: [8]u32 = std.mem.zeroes([8]u32);
    let ha: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let hb: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let ft: [8]u32 = std.mem.zeroes([8]u32);
    let fh: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let ta_s: []u32 = ta[0..];
    let tb_s: []u32 = tb[0..];
    let ha_s: []BeamHyp = ha[0..];
    let hb_s: []BeamHyp = hb[0..];
    let ft_s: []u32 = ft[0..];
    let fh_s: []BeamHyp = fh[0..];

    let bs = BeamSearch.init(cfg, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) ?? unreachable;
    let cands: [4]BeamCandidate = std.mem.zeroes([4]BeamCandidate);
    let cand_s: []BeamCandidate = cands[0..];

    // Step 1 (1 active beam, V=4): tok1 = -0.2 (greedy trap), tok2 = -0.5 (delayed winner)
    const lp1: [4]f64 = [-10.0, -0.2, -0.5, -5.0];
    expect_eq(bs.stepLogProbs(lp1[0..], 4, &cand_s), false);
    expect_eq(bs.activeCount(), 2);
    expect_eq(bs.activeBeamTokens(0)[0], 1);
    expect_eq(bs.activeBeamTokens(1)[0], 2);

    // Step 2 (2 active beams, V=4):
    // Beam 0 (tok1): all continuations poor (-5.0) -> total -5.2
    // Beam 1 (tok2): tok3 is -0.1 -> total -0.6 (wins!)
    const lp2: [8]f64 = [
        -5.0, -5.0, -5.0, -5.0, // beam 0
        -5.0, -4.0, -4.0, -0.1, // beam 1
    ];
    expect_eq(bs.stepLogProbs(lp2[0..], 4, &cand_s), false);

    let out: [4]u32 = std.mem.zeroes([4]u32);
    let out_s: []u32 = out[0..];
    const best = bs.bestSequence(&out_s) ?? unreachable;
    expect_eq(best.len, 2);
    expect_eq(best.finished, false);
    expect_eq(out_s[0], 2);
    expect_eq(out_s[1], 3);
    expect_eq(approxEq(best.score_sum, -0.6, 1e-12), true);
});

test("2. gnmtLengthPenalty and normalizedBeamScore favor longer vs shorter sequences by alpha", () => {
    // alpha = 0 -> penalty always 1.0
    expect_eq(gnmtLengthPenalty(1, 0.0), 1.0);
    expect_eq(gnmtLengthPenalty(10, 0.0), 1.0);
    // alpha = 1.0, len = 7 -> (5 + 7)/6 = 2.0
    expect_eq(approxEq(gnmtLengthPenalty(7, 1.0), 2.0, 1e-12), true);

    // Short seq (len=1, sum=-1.0) vs long seq (len=7, sum=-1.8)
    // With alpha=1.0: short norm = -1.0, long norm = -1.8 / 2.0 = -0.9 -> long wins!
    const short_pos = normalizedBeamScore(-1.0, 1, 1.0);
    const long_pos = normalizedBeamScore(-1.8, 7, 1.0);
    expect_eq(long_pos > short_pos, true);

    // With alpha=-1.0: short norm = -1.0, long norm = -1.8 / 0.5 = -3.6 -> short wins!
    const short_neg = normalizedBeamScore(-1.0, 1, -1.0);
    const long_neg = normalizedBeamScore(-1.8, 7, -1.0);
    expect_eq(short_neg > long_neg, true);
});

test("3. wouldRepeatNgram and BeamSearch no_repeat_ngram = 2 strictly block repetition loops", () => {
    const hist: [4]u32 = [1, 2, 3, 1];
    expect_eq(wouldRepeatNgram(hist[0..], 2, 0), false); // disabled
    expect_eq(wouldRepeatNgram(hist[0..], 2, 1), true);  // unigram repeat
    expect_eq(wouldRepeatNgram(hist[0..], 4, 1), false);
    expect_eq(wouldRepeatNgram(hist[0..], 2, 2), true);  // [1, 2] already occurred
    expect_eq(wouldRepeatNgram(hist[0..], 3, 2), false); // [1, 3] never occurred
    const hist3: [5]u32 = [1, 2, 3, 1, 2];
    expect_eq(wouldRepeatNgram(hist3[0..], 3, 3), true); // [1, 2, 3] already occurred
    expect_eq(wouldRepeatNgram(hist3[0..], 4, 3), false);

    // BeamSearch with width=1 and no_repeat_ngram=2:
    // Step 1 -> 1, Step 2 -> 2, Step 3 -> 1, Step 4 wants 2 (-0.01) but [1, 2] is BLOCKED -> must pick 3 (-0.5)!
    const cfg = BeamConfig {
        beam_width: 1,
        max_len: 4,
        eos_token: 0,
        length_penalty_alpha: 0.0,
        no_repeat_ngram: 2,
    };
    let ta: [4]u32 = std.mem.zeroes([4]u32);
    let tb: [4]u32 = std.mem.zeroes([4]u32);
    let ha: [1]BeamHyp = std.mem.zeroes([1]BeamHyp);
    let hb: [1]BeamHyp = std.mem.zeroes([1]BeamHyp);
    let ft: [4]u32 = std.mem.zeroes([4]u32);
    let fh: [1]BeamHyp = std.mem.zeroes([1]BeamHyp);
    let ta_s: []u32 = ta[0..];
    let tb_s: []u32 = tb[0..];
    let ha_s: []BeamHyp = ha[0..];
    let hb_s: []BeamHyp = hb[0..];
    let ft_s: []u32 = ft[0..];
    let fh_s: []BeamHyp = fh[0..];

    let bs = BeamSearch.init(cfg, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) ?? unreachable;
    let cands: [2]BeamCandidate = std.mem.zeroes([2]BeamCandidate);
    let cand_s: []BeamCandidate = cands[0..];

    const s1: [4]f64 = [-10.0, -0.1, -2.0, -3.0]; // picks 1
    const s2: [4]f64 = [-10.0, -2.0, -0.1, -3.0]; // picks 2 -> [1, 2]
    const s3: [4]f64 = [-10.0, -0.1, -2.0, -3.0]; // picks 1 -> [1, 2, 1]
    const s4: [4]f64 = [-10.0, -2.0, -0.01, -0.5]; // 2 is blocked! picks 3 -> [1, 2, 1, 3]

    expect_eq(bs.stepLogProbs(s1[0..], 4, &cand_s), false);
    expect_eq(bs.stepLogProbs(s2[0..], 4, &cand_s), false);
    expect_eq(bs.stepLogProbs(s3[0..], 4, &cand_s), false);
    expect_eq(bs.stepLogProbs(s4[0..], 4, &cand_s), true); // max_len=4 reached

    let out: [4]u32 = std.mem.zeroes([4]u32);
    let out_s: []u32 = out[0..];
    const best = bs.bestSequence(&out_s) ?? unreachable;
    expect_eq(best.len, 4);
    expect_eq(out_s[0], 1);
    expect_eq(out_s[1], 2);
    expect_eq(out_s[2], 1);
    expect_eq(out_s[3], 3);
});

test("4. EOS termination archives finished beams and triggers early stopping", () => {
    const cfg = BeamConfig {
        beam_width: 2,
        max_len: 6,
        eos_token: 0,
        length_penalty_alpha: 0.6,
        no_repeat_ngram: 0,
    };
    let ta: [12]u32 = std.mem.zeroes([12]u32);
    let tb: [12]u32 = std.mem.zeroes([12]u32);
    let ha: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let hb: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let ft: [12]u32 = std.mem.zeroes([12]u32);
    let fh: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let ta_s: []u32 = ta[0..];
    let tb_s: []u32 = tb[0..];
    let ha_s: []BeamHyp = ha[0..];
    let hb_s: []BeamHyp = hb[0..];
    let ft_s: []u32 = ft[0..];
    let fh_s: []BeamHyp = fh[0..];

    let bs = BeamSearch.init(cfg, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) ?? unreachable;
    let cands: [4]BeamCandidate = std.mem.zeroes([4]BeamCandidate);
    let cand_s: []BeamCandidate = cands[0..];

    // Step 1: pick tok1 (-0.3) and tok2 (-0.1)
    const lp1: [3]f64 = [-10.0, -0.3, -0.1];
    expect_eq(bs.stepLogProbs(lp1[0..], 3, &cand_s), false);

    // Step 2: both beams emit EOS (token 0) with top scores -> finished_count == 2 == beam_width -> early stop!
    const lp2: [6]f64 = [
        -0.05, -2.0, -2.0, // beam 0 (tok2, -0.1) + EOS -> sum -0.15
        -0.20, -2.0, -2.0, // beam 1 (tok1, -0.3) + EOS -> sum -0.50
    ];
    expect_eq(bs.stepLogProbs(lp2[0..], 3, &cand_s), true);
    expect_eq(bs.finishedCount(), 2);

    let out: [6]u32 = std.mem.zeroes([6]u32);
    let out_s: []u32 = out[0..];
    const best = bs.bestSequence(&out_s) ?? unreachable;
    expect_eq(best.finished, true);
    expect_eq(best.len, 2);
    expect_eq(out_s[0], 2);
    expect_eq(out_s[1], 0);
});

test("5. Defensive guards for zero beam_width/max_len, undersized buffers, and NaN scores", () => {
    let ta: [4]u32 = std.mem.zeroes([4]u32);
    let tb: [4]u32 = std.mem.zeroes([4]u32);
    let ha: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let hb: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let ft: [4]u32 = std.mem.zeroes([4]u32);
    let fh: [2]BeamHyp = std.mem.zeroes([2]BeamHyp);
    let ta_s: []u32 = ta[0..];
    let tb_s: []u32 = tb[0..];
    let ha_s: []BeamHyp = ha[0..];
    let hb_s: []BeamHyp = hb[0..];
    let ft_s: []u32 = ft[0..];
    let fh_s: []BeamHyp = fh[0..];

    // Invalid beam_width=0, max_len=0, NaN alpha, or undersized token buffer (needs 2*4=8 > 4)
    expect_eq(BeamSearch.init(BeamConfig { beam_width: 0, max_len: 2, eos_token: 0, length_penalty_alpha: 0.0, no_repeat_ngram: 0 }, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) == null, true);
    expect_eq(BeamSearch.init(BeamConfig { beam_width: 2, max_len: 0, eos_token: 0, length_penalty_alpha: 0.0, no_repeat_ngram: 0 }, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) == null, true);
    expect_eq(BeamSearch.init(BeamConfig { beam_width: 2, max_len: 2, eos_token: 0, length_penalty_alpha: std.math.nan(f64), no_repeat_ngram: 0 }, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) == null, true);
    expect_eq(BeamSearch.init(BeamConfig { beam_width: 2, max_len: 4, eos_token: 0, length_penalty_alpha: 0.0, no_repeat_ngram: 0 }, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) == null, true);

    // Valid init (2*2=4), NaN log_probs safely skipped
    let bs = BeamSearch.init(BeamConfig { beam_width: 2, max_len: 2, eos_token: 0, length_penalty_alpha: 0.0, no_repeat_ngram: 0 }, &ta_s, &tb_s, &ha_s, &hb_s, &ft_s, &fh_s) ?? unreachable;
    let cands: [4]BeamCandidate = std.mem.zeroes([4]BeamCandidate);
    let cand_s: []BeamCandidate = cands[0..];
    const lp_nan: [3]f64 = [std.math.nan(f64), -0.5, std.math.nan(f64)];
    expect_eq(bs.stepLogProbs(lp_nan[0..], 3, &cand_s), false);
    expect_eq(bs.activeCount(), 1);
    expect_eq(bs.activeBeamTokens(0)[0], 1);

    // Undersized out_tokens rejected
    let tiny_out: [0]u32 = [];
    let tiny_s: []u32 = tiny_out[0..];
    expect_eq(bs.bestSequence(&tiny_s) == null, true);
});
