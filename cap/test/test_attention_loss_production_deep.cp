// Origin: Pure-CP Stdlib Test Suite (Training & Alignment Losses: CE, PPL, KL, DPO & InfoNCE) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/loss.cp
// Covers:
//   1. Uniform logits: crossEntropyLoss == ln(V), label smoothing exact check, sequencePerplexity == V & ignore_index skip
//   2. klDivergenceFromLogits: D_KL(p || p) == 0.0, strict non-negativity & analytical verification
//   3. dpoLoss: positive margin -> accuracy == true, negative extreme margin (-1000.0) overflow immunity
//   4. infoNceLoss: temperature sharpening & exact equivalence with crossEntropyLoss(sim / tau)
//   5. Invalid parameters, all-ignored targets, empty/mismatched slices & NaN/Inf defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DpoResult,
    logSumExp,
    crossEntropyLoss,
    sequencePerplexity,
    klDivergenceFromLogits,
    dpoLoss,
    infoNceLoss,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testCrossEntropyLabelSmoothingAndPerplexity()! {
    // 1a. Uniform logits over V=4 -> crossEntropyLoss == ln(4)
    const uni4: [4]f64 = [2.0, 2.0, 2.0, 2.0];
    if (lse := logSumExp(uni4[0..])) {
        expect_eq(approxF64(lse, 2.0 + @log(4.0 as f64), EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    if (ce := crossEntropyLoss(uni4[0..], 2, 0.0)) {
        expect_eq(approxF64(ce, @log(4.0 as f64), EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // 1b. Label smoothing on non-uniform logits
    const lg: [4]f64 = [3.0, 1.0, 0.0, -1.0];
    if (ce_ls := crossEntropyLoss(lg[0..], 0, 0.1)) {
        const lse = logSumExp(lg[0..]) ?? 0.0;
        const mean_lg = (3.0 + 1.0 + 0.0 - 1.0) / 4.0;
        const expected = lse - 0.9 * 3.0 - 0.1 * mean_lg;
        expect_eq(approxF64(ce_ls, expected, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // 1c. sequencePerplexity on uniform logits (V=4) with ignore_index = -100 skipped -> exact PPL == 4.0
    const seq_logits: [12]f64 = [
        0.0, 0.0, 0.0, 0.0,      // t=0 active -> CE = ln(4)
        100.0, -100.0, 0.0, 0.0, // t=1 ignored (-100)
        5.0, 5.0, 5.0, 5.0,      // t=2 active -> CE = ln(4)
    ];
    const targets: [3]i32 = [1, -100, 3];
    if (ppl := sequencePerplexity(seq_logits[0..], targets[0..], 3, 4, -100)) {
        expect_eq(approxF64(ppl, 4.0, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }
}

fn testKlDivergenceProperties()! {
    const p: [4]f64 = [1.5, -0.5, 0.25, 2.0];
    const q: [4]f64 = [0.5, 0.5, -1.0, 1.0];

    // D_KL(P || P) == 0.0
    if (kl_self := klDivergenceFromLogits(p[0..], p[0..])) {
        expect_eq(approxF64(kl_self, 0.0, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // Shift invariance: adding constant +50 to p_logits does not change D_KL
    const p_shifted: [4]f64 = [51.5, 49.5, 50.25, 52.0];
    if (kl1 := klDivergenceFromLogits(p[0..], q[0..])) {
        if (kl2 := klDivergenceFromLogits(p_shifted[0..], q[0..])) {
            expect_eq(kl1 > 0.0, true);
            expect_eq(approxF64(kl1, kl2, EPS_F64), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Analytical 2-class check: P = [0.5, 0.5], Q = [1/(1+e), e/(1+e)]
    const p2: [2]f64 = [0.0, 0.0];
    const q2: [2]f64 = [0.0, 1.0];
    if (kl_2 := klDivergenceFromLogits(p2[0..], q2[0..])) {
        const lse_q2 = @log(1.0 + std.math.exp(1.0));
        const expected = 0.5 * (@log(0.5 as f64) - (0.0 - lse_q2)) + 0.5 * (@log(0.5 as f64) - (1.0 - lse_q2));
        expect_eq(approxF64(kl_2, expected, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }
}

fn testDpoLossAndExtremeMarginStability()! {
    // 3a. Correct preference: policy boosts chosen (+2.0) and lowers rejected (-3.0) vs ref
    // beta = 0.5 -> chosen_reward = 1.0, rejected_reward = -1.5, margin = 2.5
    if (res := dpoLoss(-10.0, -15.0, -12.0, -12.0, 0.5)) {
        expect_eq(approxF64(res.chosen_reward, 1.0, EPS_F64), true);
        expect_eq(approxF64(res.rejected_reward, -1.5, EPS_F64), true);
        expect_eq(approxF64(res.reward_margin, 2.5, EPS_F64), true);
        expect_eq(res.accuracy, true);
        const neg_margin: f64 = -2.5;
        const expected_loss = std.math.log1p(std.math.exp(neg_margin));
        expect_eq(approxF64(res.loss, expected_loss, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // 3b. Extreme negative margin (-1000.0): must NOT overflow exp(+1000), loss == 1000.0
    if (res_neg := dpoLoss(-1000.0, 0.0, 0.0, 0.0, 1.0)) {
        expect_eq(res_neg.accuracy, false);
        expect_eq(approxF64(res_neg.reward_margin, -1000.0, EPS_F64), true);
        expect_eq(approxF64(res_neg.loss, 1000.0, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // 3c. Extreme positive margin (+1000.0): loss == 0.0 without NaN
    if (res_pos := dpoLoss(0.0, -1000.0, 0.0, 0.0, 1.0)) {
        expect_eq(res_pos.accuracy, true);
        expect_eq(approxF64(res_pos.loss, 0.0, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }
}

fn testInfoNceTemperatureSharpeningAndEquivalence()! {
    const sims: [4]f64 = [0.9, 0.4, 0.2, -0.1];
    const tau_hi: f64 = 0.5;
    const tau_lo: f64 = 0.05;

    if (loss_hi := infoNceLoss(sims[0..], 0, tau_hi)) {
        if (loss_lo := infoNceLoss(sims[0..], 0, tau_lo)) {
            // Lower temperature sharpens distribution around positive_idx=0 -> lower loss
            expect_eq(loss_lo < loss_hi, true);

            // Exact equivalence with crossEntropyLoss(sims / tau_lo, 0, 0.0)
            const scaled: [4]f64 = [
                sims[0] / tau_lo,
                sims[1] / tau_lo,
                sims[2] / tau_lo,
                sims[3] / tau_lo,
            ];
            const ce_ref = crossEntropyLoss(scaled[0..], 0, 0.0) ?? -1.0;
            expect_eq(approxF64(loss_lo, ce_ref, EPS_F64), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_val = std.math.nan(f64);
    const inf_val = std.math.inf(f64);
    const ok4: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const nan4: [4]f64 = [1.0, nan_val, 3.0, 4.0];
    const inf4: [4]f64 = [1.0, inf_val, 3.0, 4.0];

    expect_eq(logSumExp(ok4[0..0]) == null, true);
    expect_eq(logSumExp(nan4[0..]) == null, true);
    expect_eq(logSumExp(inf4[0..]) == null, true);

    expect_eq(crossEntropyLoss(ok4[0..], 4, 0.0) == null, true);   // OOB target
    expect_eq(crossEntropyLoss(ok4[0..], 0, -0.1) == null, true);  // negative smoothing
    expect_eq(crossEntropyLoss(ok4[0..], 0, 1.0) == null, true);   // smoothing == 1.0
    expect_eq(crossEntropyLoss(nan4[0..], 0, 0.0) == null, true);

    const all_ignored: [2]i32 = [-100, -100];
    const oob_tgt: [2]i32 = [0, 4];
    expect_eq(sequencePerplexity(ok4[0..], all_ignored[0..], 1, 4, -100) == null, true);
    expect_eq(sequencePerplexity(ok4[0..], oob_tgt[1..2], 1, 4, -100) == null, true);
    expect_eq(sequencePerplexity(ok4[0..], oob_tgt[0..1], 0, 4, -100) == null, true);

    expect_eq(klDivergenceFromLogits(ok4[0..], ok4[0..3]) == null, true);
    expect_eq(klDivergenceFromLogits(ok4[0..0], ok4[0..0]) == null, true);
    expect_eq(klDivergenceFromLogits(nan4[0..], ok4[0..]) == null, true);

    expect_eq(dpoLoss(-1.0, -2.0, -1.0, -2.0, 0.0) == null, true);
    expect_eq(dpoLoss(-1.0, -2.0, -1.0, -2.0, -0.1) == null, true);
    expect_eq(dpoLoss(nan_val, -2.0, -1.0, -2.0, 0.1) == null, true);

    expect_eq(infoNceLoss(ok4[0..], 4, 0.1) == null, true);
    expect_eq(infoNceLoss(ok4[0..], 0, 0.0) == null, true);
    expect_eq(infoNceLoss(ok4[0..], 0, -0.5) == null, true);
    expect_eq(infoNceLoss(nan4[0..], 0, 0.1) == null, true);
}

fn main() {
    test("1. loss: uniform CE==ln(V), label smoothing & sequencePerplexity==V with ignore_index", () => {
        testCrossEntropyLabelSmoothingAndPerplexity()?;
    });
    test("2. loss: klDivergenceFromLogits self==0, shift invariance & analytical check", () => {
        testKlDivergenceProperties()?;
    });
    test("3. loss: dpoLoss reward margin, accuracy & extreme -1000/+1000 stability", () => {
        testDpoLossAndExtremeMarginStability()?;
    });
    test("4. loss: infoNceLoss temperature sharpening & crossEntropyLoss equivalence", () => {
        testInfoNceTemperatureSharpeningAndEquivalence()?;
    });
    test("5. loss: invalid params, all-ignored targets, empty/mismatch & NaN defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
