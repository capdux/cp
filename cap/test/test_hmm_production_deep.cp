// Source: cp/cap/test/test_hmm_production_deep.cp (Production deep tests for cap:hmm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    HmmModel,
    ViterbiResult,
    viterbiDecode,
    forwardScaled,
    backwardScaled,
    posteriorStates,
    baumWelchStep,
} from "cap:math";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn pathJointProb(model: HmmModel, obs: []const usize, states: []const usize): f64 {
    const n: usize = model.num_states;
    const m: usize = model.num_obs;
    let p: f64 = model.init_prob[states[0]] * model.emit_prob[states[0] * m + obs[0]];
    let t: usize = 1;
    while (t < obs.len) {
        p *= model.trans_prob[states[t - 1] * n + states[t]] * model.emit_prob[states[t] * m + obs[t]];
        t += 1;
    }
    return p;
}

test("1. Classic Rainy/Sunny HMM Viterbi decoding matches exhaustive 2^4 path optimum", () => {
    // States: 0 = Rainy, 1 = Sunny
    // Obs:    0 = Walk,  1 = Shop, 2 = Clean
    const pi: [2]f64 = [0.6, 0.4];
    const trans: [4]f64 = [
        0.7, 0.3,
        0.4, 0.6,
    ];
    const emit: [6]f64 = [
        0.1, 0.4, 0.5,
        0.6, 0.3, 0.1,
    ];
    const model: HmmModel = HmmModel {
        num_states: 2,
        num_obs: 3,
        init_prob: pi[0..],
        trans_prob: trans[0..],
        emit_prob: emit[0..],
    };
    expect_eq(model.validate(), true);

    const obs: [4]usize = [0, 1, 2, 2]; // Walk, Shop, Clean, Clean
    let path_b: [4]usize = std.mem.zeroes([4]usize);
    let dp0_b: [2]f64 = std.mem.zeroes([2]f64);
    let dp1_b: [2]f64 = std.mem.zeroes([2]f64);
    let bp_b: [8]usize = std.mem.zeroes([8]usize);

    let path: []usize = path_b[0..];
    let dp0: []f64 = dp0_b[0..];
    let dp1: []f64 = dp1_b[0..];
    let bp: []usize = bp_b[0..];

    const vres: ViterbiResult = viterbiDecode(model, obs[0..], *path, *dp0, *dp1, *bp) ?? unreachable;

    // Brute-force all 2^4 = 16 state sequences
    let best_p: f64 = -1.0;
    let best_mask: usize = 0;
    let mask: usize = 0;
    while (mask < 16) {
        const seq: [4]usize = [
            (mask >> 0) & 1,
            (mask >> 1) & 1,
            (mask >> 2) & 1,
            (mask >> 3) & 1,
        ];
        const jp: f64 = pathJointProb(model, obs[0..], seq[0..]);
        if (jp > best_p) {
            best_p = jp;
            best_mask = mask;
        }
        mask += 1;
    }

    let t: usize = 0;
    while (t < 4) {
        const exp_s: usize = (best_mask >> t) & 1;
        expect_eq(path[t], exp_s);
        t += 1;
    }
    expect_eq(absF64(std.math.exp(vres.log_prob) - best_p) < 1.0e-12, true);
});

test("2. Scaled Forward log-likelihood & Forward-Backward posteriors match exact brute-force marginals", () => {
    const pi: [2]f64 = [0.6, 0.4];
    const trans: [4]f64 = [0.7, 0.3, 0.4, 0.6];
    const emit: [6]f64 = [0.1, 0.4, 0.5, 0.6, 0.3, 0.1];
    const model: HmmModel = HmmModel {
        num_states: 2, num_obs: 3,
        init_prob: pi[0..], trans_prob: trans[0..], emit_prob: emit[0..],
    };
    const obs: [4]usize = [0, 1, 2, 0];

    let alpha_b: [8]f64 = std.mem.zeroes([8]f64);
    let beta_b: [8]f64 = std.mem.zeroes([8]f64);
    let scale_b: [4]f64 = std.mem.zeroes([4]f64);
    let gamma_b: [8]f64 = std.mem.zeroes([8]f64);

    let alpha: []f64 = alpha_b[0..];
    let beta: []f64 = beta_b[0..];
    let scale: []f64 = scale_b[0..];
    let gamma: []f64 = gamma_b[0..];

    const ll: f64 = forwardScaled(model, obs[0..], *alpha, *scale) ?? unreachable;
    expect_eq(backwardScaled(model, obs[0..], scale, *beta), true);
    expect_eq(posteriorStates(alpha, beta, 4, 2, *gamma), true);

    // Brute-force total likelihood and state marginals over all 16 paths
    let total_p: f64 = 0.0;
    let marg_s0: [4]f64 = std.mem.zeroes([4]f64);
    let mask: usize = 0;
    while (mask < 16) {
        const seq: [4]usize = [
            (mask >> 0) & 1,
            (mask >> 1) & 1,
            (mask >> 2) & 1,
            (mask >> 3) & 1,
        ];
        const jp: f64 = pathJointProb(model, obs[0..], seq[0..]);
        total_p += jp;
        let t: usize = 0;
        while (t < 4) {
            if (seq[t] == 0) {
                marg_s0[t] += jp;
            }
            t += 1;
        }
        mask += 1;
    }

    expect_eq(absF64(std.math.exp(ll) - total_p) < 1.0e-12, true);

    // Every time step sums to 1.0 and matches exact brute-force posterior
    let t: usize = 0;
    while (t < 4) {
        const g0: f64 = gamma[t * 2 + 0];
        const g1: f64 = gamma[t * 2 + 1];
        expect_eq(absF64((g0 + g1) - 1.0) < 1.0e-12, true);
        expect_eq(absF64(g0 - (marg_s0[t] / total_p)) < 1.0e-12, true);
        t += 1;
    }
});

test("3. Baum-Welch EM monotonically increases log-likelihood & maintains valid normalized model", () => {
    // Observation sequence with strong two-regime switching pattern
    const obs: [20]usize = [
        0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
        2, 2, 1, 2, 2, 2, 1, 2, 2, 2,
    ];

    let cur_pi: [2]f64 = [0.55, 0.45];
    let cur_tr: [4]f64 = [0.6, 0.4, 0.4, 0.6];
    let cur_em: [6]f64 = [0.45, 0.30, 0.25, 0.25, 0.30, 0.45];

    let nxt_pi_b: [2]f64 = std.mem.zeroes([2]f64);
    let nxt_tr_b: [4]f64 = std.mem.zeroes([4]f64);
    let nxt_em_b: [6]f64 = std.mem.zeroes([6]f64);
    let alpha_b: [40]f64 = std.mem.zeroes([40]f64);
    let beta_b: [40]f64 = std.mem.zeroes([40]f64);
    let scale_b: [20]f64 = std.mem.zeroes([20]f64);

    let nxt_pi: []f64 = nxt_pi_b[0..];
    let nxt_tr: []f64 = nxt_tr_b[0..];
    let nxt_em: []f64 = nxt_em_b[0..];
    let alpha: []f64 = alpha_b[0..];
    let beta: []f64 = beta_b[0..];
    let scale: []f64 = scale_b[0..];

    let first_ll: f64 = 0.0;
    let prev_ll: f64 = 0.0;
    let iter: usize = 0;
    while (iter < 10) {
        const m: HmmModel = HmmModel {
            num_states: 2,
            num_obs: 3,
            init_prob: cur_pi[0..],
            trans_prob: cur_tr[0..],
            emit_prob: cur_em[0..],
        };
        expect_eq(m.validate(), true);

        const ll: f64 = baumWelchStep(m, obs[0..], *nxt_pi, *nxt_tr, *nxt_em, *alpha, *beta, *scale) ?? unreachable;
        if (iter == 0) {
            first_ll = ll;
        } else {
            // EM guarantee: log-likelihood is monotonically non-decreasing
            expect_eq(ll >= prev_ll - 1.0e-11, true);
        }
        prev_ll = ll;

        // Copy updated parameters back into cur_*
        let i: usize = 0;
        while (i < 2) { cur_pi[i] = nxt_pi[i]; i += 1; }
        let j: usize = 0;
        while (j < 4) { cur_tr[j] = nxt_tr[j]; j += 1; }
        let k: usize = 0;
        while (k < 6) { cur_em[k] = nxt_em[k]; k += 1; }
        iter += 1;
    }

    const final_m: HmmModel = HmmModel {
        num_states: 2, num_obs: 3,
        init_prob: cur_pi[0..], trans_prob: cur_tr[0..], emit_prob: cur_em[0..],
    };
    expect_eq(final_m.validate(), true);
    const final_ll: f64 = forwardScaled(final_m, obs[0..], *alpha, *scale) ?? unreachable;
    expect_eq(final_ll >= prev_ll - 1.0e-11, true);
    expect_eq(final_ll > first_ll + 1.0, true);
});

test("4. 200-step Occasionally Dishonest Casino sequence runs without numerical underflow", () => {
    // States: 0 = Fair die, 1 = Loaded die (rolls 5 -> face '6' with prob 0.5)
    const pi: [2]f64 = [0.5, 0.5];
    const trans: [4]f64 = [0.95, 0.05, 0.10, 0.90];
    const emit: [12]f64 = [
        1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0, 1.0 / 6.0,
        0.1, 0.1, 0.1, 0.1, 0.1, 0.5,
    ];
    const model: HmmModel = HmmModel {
        num_states: 2, num_obs: 6,
        init_prob: pi[0..], trans_prob: trans[0..], emit_prob: emit[0..],
    };
    expect_eq(model.validate(), true);

    // Construct 200 observations: first 100 uniform, next 100 heavy on face 5 ('6')
    let obs: [200]usize = std.mem.zeroes([200]usize);
    let t: usize = 0;
    while (t < 100) {
        obs[t] = t % 6;
        t += 1;
    }
    let t2: usize = 100;
    while (t2 < 200) {
        obs[t2] = if ((t2 % 2) == 0) (5 as usize) else (t2 % 5);
        t2 += 1;
    }

    let alpha_b: [400]f64 = std.mem.zeroes([400]f64);
    let beta_b: [400]f64 = std.mem.zeroes([400]f64);
    let scale_b: [200]f64 = std.mem.zeroes([200]f64);
    let gamma_b: [400]f64 = std.mem.zeroes([400]f64);
    let alpha: []f64 = alpha_b[0..];
    let beta: []f64 = beta_b[0..];
    let scale: []f64 = scale_b[0..];
    let gamma: []f64 = gamma_b[0..];

    const ll: f64 = forwardScaled(model, obs[0..], *alpha, *scale) ?? unreachable;
    // Unscaled probability ~ 6^-200 ~ 10^-155 would risk underflow; scaled log-lik is finite ~ -320
    expect_eq(ll < -200.0 && ll > -450.0, true);
    expect_eq(backwardScaled(model, obs[0..], scale, *beta), true);
    expect_eq(posteriorStates(alpha, beta, 200, 2, *gamma), true);

    // Check normalization at start, mid, and end; loaded state posterior dominates late segment
    expect_eq(absF64((gamma[0] + gamma[1]) - 1.0) < 1.0e-12, true);
    expect_eq(absF64((gamma[199 * 2] + gamma[199 * 2 + 1]) - 1.0) < 1.0e-12, true);
    expect_eq(gamma[150 * 2 + 1] > 0.8, true);

    let path_b: [200]usize = std.mem.zeroes([200]usize);
    let dp0_b: [2]f64 = std.mem.zeroes([2]f64);
    let dp1_b: [2]f64 = std.mem.zeroes([2]f64);
    let bp_b: [400]usize = std.mem.zeroes([400]usize);
    let path: []usize = path_b[0..];
    let dp0: []f64 = dp0_b[0..];
    let dp1: []f64 = dp1_b[0..];
    let bp: []usize = bp_b[0..];

    const vres: ViterbiResult = viterbiDecode(model, obs[0..], *path, *dp0, *dp1, *bp) ?? unreachable;
    expect_eq(vres.log_prob <= ll + 1.0e-9, true);
    expect_eq(path[50], 0 as usize);  // Fair segment
    expect_eq(path[150], 1 as usize); // Loaded segment
});

test("5. Zero-probability transitions & impossible observation sequences return null cleanly", () => {
    // Deterministic 2-state chain: starts in state 0, emits 0, transitions to 1, emits 1
    const pi: [2]f64 = [1.0, 0.0];
    const trans: [4]f64 = [0.0, 1.0, 0.0, 1.0];
    const emit: [4]f64 = [1.0, 0.0, 0.0, 1.0];
    const model: HmmModel = HmmModel {
        num_states: 2, num_obs: 2,
        init_prob: pi[0..], trans_prob: trans[0..], emit_prob: emit[0..],
    };
    expect_eq(model.validate(), true);

    let path_b: [2]usize = std.mem.zeroes([2]usize);
    let dp0_b: [2]f64 = std.mem.zeroes([2]f64);
    let dp1_b: [2]f64 = std.mem.zeroes([2]f64);
    let bp_b: [4]usize = std.mem.zeroes([4]usize);
    let alpha_b: [4]f64 = std.mem.zeroes([4]f64);
    let scale_b: [2]f64 = std.mem.zeroes([2]f64);
    let path: []usize = path_b[0..];
    let dp0: []f64 = dp0_b[0..];
    let dp1: []f64 = dp1_b[0..];
    let bp: []usize = bp_b[0..];
    let alpha: []f64 = alpha_b[0..];
    let scale: []f64 = scale_b[0..];

    // Valid sequence [0, 1] has probability 1.0 -> log_prob == 0.0
    const ok_obs: [2]usize = [0, 1];
    const vres: ViterbiResult = viterbiDecode(model, ok_obs[0..], *path, *dp0, *dp1, *bp) ?? unreachable;
    expect_eq(absF64(vres.log_prob) < 1.0e-12, true);
    expect_eq(path[0], 0 as usize);
    expect_eq(path[1], 1 as usize);

    // Impossible sequence [1, 0] -> null
    const bad_obs: [2]usize = [1, 0];
    expect_eq(viterbiDecode(model, bad_obs[0..], *path, *dp0, *dp1, *bp) == null, true);
    expect_eq(forwardScaled(model, bad_obs[0..], *alpha, *scale) == null, true);
});

test("6. Validation & defensive guards: unnormalized probs, OOB obs, empty seq & short buffers", () => {
    const ok_pi: [2]f64 = [0.5, 0.5];
    const bad_pi: [2]f64 = [0.6, 0.6]; // sums to 1.2
    const neg_pi: [2]f64 = [-0.2, 1.2];
    const ok_tr: [4]f64 = [0.5, 0.5, 0.5, 0.5];
    const ok_em: [4]f64 = [0.5, 0.5, 0.5, 0.5];

    const m_unnorm: HmmModel = HmmModel {
        num_states: 2, num_obs: 2,
        init_prob: bad_pi[0..], trans_prob: ok_tr[0..], emit_prob: ok_em[0..],
    };
    const m_neg: HmmModel = HmmModel {
        num_states: 2, num_obs: 2,
        init_prob: neg_pi[0..], trans_prob: ok_tr[0..], emit_prob: ok_em[0..],
    };
    const m_ok: HmmModel = HmmModel {
        num_states: 2, num_obs: 2,
        init_prob: ok_pi[0..], trans_prob: ok_tr[0..], emit_prob: ok_em[0..],
    };

    expect_eq(m_unnorm.validate(), false);
    expect_eq(m_neg.validate(), false);
    expect_eq(m_ok.validate(), true);

    let alpha_b: [6]f64 = std.mem.zeroes([6]f64);
    let scale_b: [3]f64 = std.mem.zeroes([3]f64);
    let alpha: []f64 = alpha_b[0..];
    let scale: []f64 = scale_b[0..];

    const empty_obs: [0]usize = [];
    const oob_obs: [2]usize = [0, 2]; // num_obs == 2, so 2 is out of range
    const valid_obs: [2]usize = [0, 1];

    expect_eq(forwardScaled(m_unnorm, valid_obs[0..], *alpha, *scale) == null, true);
    expect_eq(forwardScaled(m_ok, empty_obs[0..], *alpha, *scale) == null, true);
    expect_eq(forwardScaled(m_ok, oob_obs[0..], *alpha, *scale) == null, true);

    // Insufficient buffers
    let short_alpha: []f64 = alpha_b[0..3]; // needs 2 * 2 = 4
    let short_scale: []f64 = scale_b[0..1]; // needs 2
    expect_eq(forwardScaled(m_ok, valid_obs[0..], *short_alpha, *scale) == null, true);
    expect_eq(forwardScaled(m_ok, valid_obs[0..], *alpha, *short_scale) == null, true);
});
