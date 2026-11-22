// Source: cp/cap/test/test_agent_sampler_production_deep.cp (Production deep tests for cap:agent sampler)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SamplerConfig,
    SampledToken,
    applyRepetitionPenalties,
    greedyArgmax,
    sampleToken,
} from "cap:agent";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

test("1. Greedy decoding and greedyArgmax select highest non-NaN logit deterministically", () => {
    const nan_v: f64 = std.math.nan(f64);
    const logits: [5]f64 = [nan_v, 2.5, 4.0, 4.0, -1.0];
    expect_eq(greedyArgmax(logits[0..]) ?? 99, 2); // skips NaN, lowest id on tie

    const empty_recent: [0]usize = [];
    let wl: [5]f64 = std.mem.zeroes([5]f64);
    let wi: [5]usize = std.mem.zeroes([5]usize);
    let wp: [5]f64 = std.mem.zeroes([5]f64);
    let wl_s: []f64 = wl[0..];
    let wi_s: []usize = wi[0..];
    let wp_s: []f64 = wp[0..];

    const tok = sampleToken(logits[0..], empty_recent[0..], SamplerConfig.greedy(), &wl_s, &wi_s, &wp_s) ?? unreachable;
    expect_eq(tok.token_id, 2);
    expect_eq(tok.prob, 1.0);
    expect_eq(tok.logit, 4.0);
    expect_eq(tok.candidates_considered, 1);
});

test("2. Repetition, frequency, and presence penalties scale positive/negative logits accurately", () => {
    let logits: [4]f64 = [4.0, -2.0, 3.0, 1.0];
    let ls: []f64 = logits[0..];
    // token 0 appears twice, token 1 appears once
    const recent: [3]usize = [0, 1, 0];

    // repeat_penalty=2.0, freq_penalty=0.5, presence_penalty=0.25
    // token 0 (pos 4.0, cnt=2): 4.0/2.0 - 0.5*2 - 0.25 = 2.0 - 1.25 = 0.75
    // token 1 (neg -2.0, cnt=1): -2.0*2.0 - 0.5*1 - 0.25 = -4.75
    // token 2 & 3 untouched: 3.0, 1.0
    expect_eq(applyRepetitionPenalties(&ls, recent[0..], 2.0, 0.5, 0.25), true);
    expect_eq(approxEq(ls[0], 0.75, 1e-12), true);
    expect_eq(approxEq(ls[1], -4.75, 1e-12), true);
    expect_eq(approxEq(ls[2], 3.0, 1e-12), true);
    expect_eq(approxEq(ls[3], 1.0, 1e-12), true);

    // Via sampleToken greedy: token 0 suppressed below token 2 -> selects token 2
    const orig: [4]f64 = [4.0, -2.0, 3.0, 1.0];
    let wl: [4]f64 = std.mem.zeroes([4]f64);
    let wi: [4]usize = std.mem.zeroes([4]usize);
    let wp: [4]f64 = std.mem.zeroes([4]f64);
    let wl_s: []f64 = wl[0..];
    let wi_s: []usize = wi[0..];
    let wp_s: []f64 = wp[0..];
    let cfg = SamplerConfig.greedy();
    cfg.repeat_penalty = 2.0;
    cfg.frequency_penalty = 0.5;
    cfg.presence_penalty = 0.25;

    const tok = sampleToken(orig[0..], recent[0..], cfg, &wl_s, &wi_s, &wp_s) ?? unreachable;
    expect_eq(tok.token_id, 2);
});

test("3. Top-K, Top-P, and Min-P strictly truncate tail tokens over 1000 seeded draws", () => {
    // 6 tokens: top 2 are strong (5.0, 4.5), middle (2.0), tail 3 are weak (0.0, -1.0, -2.0)
    const logits: [6]f64 = [5.0, 4.5, 2.0, 0.0, -1.0, -2.0];
    const empty_recent: [0]usize = [];
    let wl: [6]f64 = std.mem.zeroes([6]f64);
    let wi: [6]usize = std.mem.zeroes([6]usize);
    let wp: [6]f64 = std.mem.zeroes([6]f64);
    let wl_s: []f64 = wl[0..];
    let wi_s: []usize = wi[0..];
    let wp_s: []f64 = wp[0..];

    // 3a. Top-K = 2 -> NEVER samples token >= 2 across 1000 draws
    let i: u64 = 0;
    while (i < 1000) {
        const cfg = SamplerConfig {
            temperature: 1.0,
            top_k: 2,
            top_p: 1.0,
            min_p: 0.0,
            repeat_penalty: 1.0,
            frequency_penalty: 0.0,
            presence_penalty: 0.0,
            seed: i + 1,
        };
        const tok = sampleToken(logits[0..], empty_recent[0..], cfg, &wl_s, &wi_s, &wp_s) ?? unreachable;
        expect_eq(tok.token_id < 2, true);
        expect_eq(tok.candidates_considered, 2);
        i += 1;
    }

    // 3b. Top-P = 0.90 (probs at T=1: tok0 ~0.60, tok1 ~0.36 -> cum ~0.96 >= 0.90 -> keeps only 2 tokens)
    let j: u64 = 0;
    while (j < 1000) {
        const cfg = SamplerConfig {
            temperature: 1.0,
            top_k: 0,
            top_p: 0.90,
            min_p: 0.0,
            repeat_penalty: 1.0,
            frequency_penalty: 0.0,
            presence_penalty: 0.0,
            seed: j + 5000,
        };
        const tok = sampleToken(logits[0..], empty_recent[0..], cfg, &wl_s, &wi_s, &wp_s) ?? unreachable;
        expect_eq(tok.token_id < 2, true);
        expect_eq(tok.candidates_considered, 2);
        j += 1;
    }

    // 3c. Min-P = 0.20 (peak tok0 ~0.60, thresh ~0.12 -> tok1 ~0.36 passes, tok2 ~0.03 fails -> keeps only 2)
    let k: u64 = 0;
    while (k < 1000) {
        const cfg = SamplerConfig {
            temperature: 1.0,
            top_k: 0,
            top_p: 1.0,
            min_p: 0.20,
            repeat_penalty: 1.0,
            frequency_penalty: 0.0,
            presence_penalty: 0.0,
            seed: k + 9000,
        };
        const tok = sampleToken(logits[0..], empty_recent[0..], cfg, &wl_s, &wi_s, &wp_s) ?? unreachable;
        expect_eq(tok.token_id < 2, true);
        expect_eq(tok.candidates_considered, 2);
        k += 1;
    }
});

test("4. Low temperature sharpens vs high temperature flattens distribution, and same seed is bit-identical", () => {
    const logits: [4]f64 = [2.0, 1.2, 0.8, 0.5];
    const empty_recent: [0]usize = [];
    let wl: [4]f64 = std.mem.zeroes([4]f64);
    let wi: [4]usize = std.mem.zeroes([4]usize);
    let wp: [4]f64 = std.mem.zeroes([4]f64);
    let wl_s: []f64 = wl[0..];
    let wi_s: []usize = wi[0..];
    let wp_s: []f64 = wp[0..];

    let low_t_top0: usize = 0;
    let high_t_top0: usize = 0;
    let i: u64 = 0;
    while (i < 500) {
        const low_cfg = SamplerConfig {
            temperature: 0.2, top_k: 0, top_p: 1.0, min_p: 0.0,
            repeat_penalty: 1.0, frequency_penalty: 0.0, presence_penalty: 0.0, seed: i + 100,
        };
        const high_cfg = SamplerConfig {
            temperature: 2.5, top_k: 0, top_p: 1.0, min_p: 0.0,
            repeat_penalty: 1.0, frequency_penalty: 0.0, presence_penalty: 0.0, seed: i + 100,
        };
        if ((sampleToken(logits[0..], empty_recent[0..], low_cfg, &wl_s, &wi_s, &wp_s) ?? unreachable).token_id == 0) {
            low_t_top0 += 1;
        }
        if ((sampleToken(logits[0..], empty_recent[0..], high_cfg, &wl_s, &wi_s, &wp_s) ?? unreachable).token_id == 0) {
            high_t_top0 += 1;
        }
        i += 1;
    }
    // Low T concentrates strongly on token 0 (> 90%), High T spreads out (< 50%)
    expect_eq(low_t_top0 > 450, true);
    expect_eq(high_t_top0 < 250, true);

    // Same seed determinism
    const det_cfg = SamplerConfig.default(424242);
    const a = sampleToken(logits[0..], empty_recent[0..], det_cfg, &wl_s, &wi_s, &wp_s) ?? unreachable;
    const b = sampleToken(logits[0..], empty_recent[0..], det_cfg, &wl_s, &wi_s, &wp_s) ?? unreachable;
    expect_eq(a.token_id, b.token_id);
    expect_eq(a.prob, b.prob);
    expect_eq(a.logit, b.logit);
});

test("5. Defensive guards for empty/all-NaN vocab, invalid config params, and undersized scratch buffers", () => {
    const empty_f: [0]f64 = [];
    const empty_recent: [0]usize = [];
    let wl: [4]f64 = std.mem.zeroes([4]f64);
    let wi: [4]usize = std.mem.zeroes([4]usize);
    let wp: [4]f64 = std.mem.zeroes([4]f64);
    let wl_s: []f64 = wl[0..];
    let wi_s: []usize = wi[0..];
    let wp_s: []f64 = wp[0..];

    expect_eq(greedyArgmax(empty_f[0..]) == null, true);
    expect_eq(sampleToken(empty_f[0..], empty_recent[0..], SamplerConfig.default(1), &wl_s, &wi_s, &wp_s) == null, true);

    const nan_v: f64 = std.math.nan(f64);
    const all_nan: [2]f64 = [nan_v, nan_v];
    expect_eq(greedyArgmax(all_nan[0..]) == null, true);
    expect_eq(sampleToken(all_nan[0..], empty_recent[0..], SamplerConfig.default(1), &wl_s, &wi_s, &wp_s) == null, true);

    const ok_logits: [3]f64 = [1.0, 2.0, 3.0];

    // Negative temperature, top_p <= 0, repeat_penalty <= 0, NaN temperature -> null
    let bad_t = SamplerConfig.default(1);
    bad_t.temperature = -0.1;
    expect_eq(sampleToken(ok_logits[0..], empty_recent[0..], bad_t, &wl_s, &wi_s, &wp_s) == null, true);

    let bad_p = SamplerConfig.default(1);
    bad_p.top_p = 0.0;
    expect_eq(sampleToken(ok_logits[0..], empty_recent[0..], bad_p, &wl_s, &wi_s, &wp_s) == null, true);

    let bad_rp = SamplerConfig.default(1);
    bad_rp.repeat_penalty = 0.0;
    expect_eq(sampleToken(ok_logits[0..], empty_recent[0..], bad_rp, &wl_s, &wi_s, &wp_s) == null, true);

    let nan_cfg = SamplerConfig.default(1);
    nan_cfg.temperature = nan_v;
    expect_eq(sampleToken(ok_logits[0..], empty_recent[0..], nan_cfg, &wl_s, &wi_s, &wp_s) == null, true);

    // Undersized scratch buffer -> null
    let tiny_wl: [2]f64 = std.mem.zeroes([2]f64);
    let tiny_wl_s: []f64 = tiny_wl[0..];
    expect_eq(sampleToken(ok_logits[0..], empty_recent[0..], SamplerConfig.default(1), &tiny_wl_s, &wi_s, &wp_s) == null, true);
});
