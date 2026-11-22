// Source: cp/cap/test/test_agent_speculative_production_deep.cp (Production deep tests for cap:agent speculative)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SpecVerifyResult,
    verifySpeculativeStep,
    verifySpeculativeGreedy,
    SpecStats,
    AdaptiveDraftController,
} from "cap:agent";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

fn drawFromQ(q: []const f64, seed: u64): usize {
    let z: u64 = seed +% 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    z = z ^ (z >> 31);
    const u = ((z >> 11) as f64) * (1.0 / 9007199254740992.0);
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < q.len) {
        acc += q[i];
        if (u < acc) return i;
        i += 1;
    }
    return q.len - 1;
}

test("1. Identical draft and target distributions accept 100% of K tokens and emit K+1 with bonus", () => {
    const draft_tokens: [3]usize = [0, 2, 1];
    // K=3, V=3
    const draft_probs: [9]f64 = [
        0.6, 0.3, 0.1,
        0.1, 0.2, 0.7,
        0.2, 0.5, 0.3,
    ];
    // K+1=4 rows; row 3 has 100% mass on token 2 as deterministic bonus
    const target_probs: [12]f64 = [
        0.6, 0.3, 0.1,
        0.1, 0.2, 0.7,
        0.2, 0.5, 0.3,
        0.0, 0.0, 1.0,
    ];
    let emitted: [4]usize = std.mem.zeroes([4]usize);
    let residual: [3]f64 = std.mem.zeroes([3]f64);
    let em_s: []usize = emitted[0..];
    let res_s: []f64 = residual[0..];

    const res = verifySpeculativeStep(
        draft_tokens[0..], draft_probs[0..], target_probs[0..], 3, 12345, &em_s, &res_s,
    ) ?? unreachable;

    expect_eq(res.all_accepted, true);
    expect_eq(res.accepted_count, 3);
    expect_eq(res.emitted_count, 4);
    expect_eq(res.rejected_at, 3);
    expect_eq(res.bonus_token, 2);
    expect_eq(em_s[0], 0);
    expect_eq(em_s[1], 2);
    expect_eq(em_s[2], 1);
    expect_eq(em_s[3], 2);
});

test("2. Monte Carlo 5000-trial proof: rejection sampling output matches target distribution p within 0.025", () => {
    // Deliberately mismatched q vs p (V=4, K=1)
    const q_row: [4]f64 = [0.60, 0.25, 0.10, 0.05];
    const p_rows: [8]f64 = [
        0.20, 0.45, 0.25, 0.10, // target p_0
        0.25, 0.25, 0.25, 0.25, // bonus row
    ];

    let counts: [4]usize = [0, 0, 0, 0];
    let draft_tok: [1]usize = [0];
    let emitted: [2]usize = [0, 0];
    let residual: [4]f64 = [0.0, 0.0, 0.0, 0.0];
    let em_s: []usize = emitted[0..];
    let res_s: []f64 = residual[0..];

    const trials: usize = 5000;
    let t: usize = 0;
    while (t < trials) {
        // Sample draft token x ~ q using independent seed stream
        draft_tok[0] = drawFromQ(q_row[0..], (t as u64) * 2 + 17);
        const res = verifySpeculativeStep(
            draft_tok[0..], q_row[0..], p_rows[0..], 4, (t as u64) * 2 + 999991, &em_s, &res_s,
        ) ?? unreachable;
        * = res;
        counts[em_s[0]] += 1;
        t += 1;
    }

    // Empirical frequencies must match target p = [0.20, 0.45, 0.25, 0.10] within < 0.025
    expect_eq(approxEq((counts[0] as f64) / (trials as f64), 0.20, 0.025), true);
    expect_eq(approxEq((counts[1] as f64) / (trials as f64), 0.45, 0.025), true);
    expect_eq(approxEq((counts[2] as f64) / (trials as f64), 0.25, 0.025), true);
    expect_eq(approxEq((counts[3] as f64) / (trials as f64), 0.10, 0.025), true);
});

test("3. verifySpeculativeGreedy truncates at first mismatch and emits bonus on full match", () => {
    // K=3, V=3; target argmax per row: [1, 2, 0, 1]
    const target_probs: [12]f64 = [
        0.1, 0.8, 0.1, // argmax = 1
        0.2, 0.1, 0.7, // argmax = 2
        0.6, 0.3, 0.1, // argmax = 0
        0.1, 0.5, 0.4, // bonus argmax = 1
    ];
    let emitted: [4]usize = std.mem.zeroes([4]usize);
    let em_s: []usize = emitted[0..];

    // Mismatch at index 1 (drafted [1, 0, 0] vs target [1, 2, 0])
    const draft_miss: [3]usize = [1, 0, 0];
    const r_miss = verifySpeculativeGreedy(draft_miss[0..], target_probs[0..], 3, &em_s) ?? unreachable;
    expect_eq(r_miss.all_accepted, false);
    expect_eq(r_miss.accepted_count, 1);
    expect_eq(r_miss.emitted_count, 2);
    expect_eq(r_miss.rejected_at, 1);
    expect_eq(r_miss.bonus_token, 2);
    expect_eq(em_s[0], 1);
    expect_eq(em_s[1], 2);

    // Full match -> emits all 3 + bonus 1
    const draft_ok: [3]usize = [1, 2, 0];
    const r_ok = verifySpeculativeGreedy(draft_ok[0..], target_probs[0..], 3, &em_s) ?? unreachable;
    expect_eq(r_ok.all_accepted, true);
    expect_eq(r_ok.accepted_count, 3);
    expect_eq(r_ok.emitted_count, 4);
    expect_eq(r_ok.bonus_token, 1);
    expect_eq(em_s[3], 1);
});

test("4. AdaptiveDraftController scales draft_k up on high acceptance, down on low acceptance, within [min_k, max_k]", () => {
    let ctrl = AdaptiveDraftController.init(2, 5, 3, 0.5) ?? unreachable;
    expect_eq(ctrl.currentK(), 3);

    // High acceptance (100%) ramps up: 3 -> 4 -> 5 -> clamped at 5
    expect_eq(ctrl.recordStep(3, 3, 4), 4);
    expect_eq(ctrl.recordStep(4, 4, 5), 5);
    expect_eq(ctrl.recordStep(5, 5, 6), 5);

    // Low acceptance (0%) ramps down: EMA decays 1.0 -> 0.5 -> 0.25 (<=0.40 -> 4) -> 0.125 (-> 3) -> 0.0625 (-> 2) -> clamped at 2
    * = ctrl.recordStep(5, 0, 1);
    expect_eq(ctrl.recordStep(5, 0, 1), 4);
    expect_eq(ctrl.recordStep(4, 0, 1), 3);
    expect_eq(ctrl.recordStep(3, 0, 1), 2);
    expect_eq(ctrl.recordStep(2, 0, 1), 2);

    const st = ctrl.stats();
    expect_eq(st.total_steps, 8);
    expect_eq(st.total_drafted, 31);
    expect_eq(st.total_accepted, 12);
    expect_eq(st.total_emitted, 20);
    expect_eq(approxEq(st.acceptance_rate, 12.0 / 31.0, 1e-12), true);
    expect_eq(approxEq(st.avg_tokens_per_step, 20.0 / 8.0, 1e-12), true);
});

test("5. Defensive guards for K=0, V=0, length mismatch, undersized buffers, OOB token, NaN, and bad controller init", () => {
    const empty_u: [0]usize = [];
    const empty_f: [0]f64 = [];
    let em: [4]usize = std.mem.zeroes([4]usize);
    let res: [4]f64 = std.mem.zeroes([4]f64);
    let em_s: []usize = em[0..];
    let res_s: []f64 = res[0..];

    // K == 0 or vocab_size == 0 -> null
    expect_eq(verifySpeculativeStep(empty_u[0..], empty_f[0..], empty_f[0..], 3, 1, &em_s, &res_s) == null, true);
    const d1: [1]usize = [0];
    const q1: [2]f64 = [0.5, 0.5];
    const p1: [4]f64 = [0.5, 0.5, 0.5, 0.5];
    expect_eq(verifySpeculativeStep(d1[0..], q1[0..], p1[0..], 0, 1, &em_s, &res_s) == null, true);

    // OOB draft token -> null
    const d_oob: [1]usize = [9];
    expect_eq(verifySpeculativeStep(d_oob[0..], q1[0..], p1[0..], 2, 1, &em_s, &res_s) == null, true);
    expect_eq(verifySpeculativeGreedy(d_oob[0..], p1[0..], 2, &em_s) == null, true);

    // Undersized emitted_out or residual_scratch -> null
    let tiny_em: [1]usize = [0];
    let tiny_em_s: []usize = tiny_em[0..];
    let tiny_res: [1]f64 = [0.0];
    let tiny_res_s: []f64 = tiny_res[0..];
    expect_eq(verifySpeculativeStep(d1[0..], q1[0..], p1[0..], 2, 1, &tiny_em_s, &res_s) == null, true);
    expect_eq(verifySpeculativeStep(d1[0..], q1[0..], p1[0..], 2, 1, &em_s, &tiny_res_s) == null, true);

    // NaN probability -> null
    const nan_p: [4]f64 = [std.math.nan(f64), 0.5, 0.5, 0.5];
    expect_eq(verifySpeculativeStep(d1[0..], q1[0..], nan_p[0..], 2, 1, &em_s, &res_s) == null, true);
    expect_eq(verifySpeculativeGreedy(d1[0..], nan_p[0..], 2, &em_s) == null, true);

    // Invalid AdaptiveDraftController init -> null
    expect_eq(AdaptiveDraftController.init(0, 4, 2, 0.3) == null, true);
    expect_eq(AdaptiveDraftController.init(4, 2, 3, 0.3) == null, true);
    expect_eq(AdaptiveDraftController.init(1, 4, 5, 0.3) == null, true);
    expect_eq(AdaptiveDraftController.init(1, 4, 2, 0.0) == null, true);
    expect_eq(AdaptiveDraftController.init(1, 4, 2, std.math.nan(f64)) == null, true);
});
