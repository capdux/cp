// Origin: Pure-CP Stdlib Test Suite (lib/attention/sliding_window.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation Sliding Window Attention (SWA) and
// StreamingLLM Attention Sink RingKvCache: pure SWA window/causal masking, permanent sink
// anchoring under ring wrap-around, unbounded streaming equivalence with contiguous SWA,
// numerical equivalence with local SDPA, and invalid dimension/empty-cache/NaN guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SwaConfig,
    RingKvCache,
    swaAttentionSingleQuery,
    ringKvAttentionSingleQuery,
} from "cap:attention";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn referenceLocalSdpa(
    query: []const f32,
    keys: []const f32,
    vals: []const f32,
    indices: []const usize,
    head_dim: usize,
    out_ref: *[]f32,
) {
    let max_s: f64 = -(std.math.inf(f64));
    const inv_sqrt_d: f64 = 1.0 / @sqrt(head_dim as f64);
    let scores: [32]f64 = undefined;

    let k: usize = 0;
    while (k < indices.len) : (k += 1) {
        const base: usize = indices[k] * head_dim;
        let dot: f64 = 0.0;
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            dot += (query[d] as f64) * (keys[base + d] as f64);
        }
        scores[k] = dot * inv_sqrt_d;
        if (scores[k] > max_s) {
            max_s = scores[k];
        }
    }

    let sum_e: f64 = 0.0;
    let weights: [32]f64 = undefined;
    k = 0;
    while (k < indices.len) : (k += 1) {
        weights[k] = std.math.exp(scores[k] - max_s);
        sum_e += weights[k];
    }

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        let acc: f64 = 0.0;
        k = 0;
        while (k < indices.len) : (k += 1) {
            const base: usize = indices[k] * head_dim;
            acc += (weights[k] / sum_e) * (vals[base + d] as f64);
        }
        out_ref[d] = acc as f32;
    }
}

fn testPureSwaWindowMaskingExactness()! {
    // 6 tokens, head_dim = 4, sink = 0, window = 3, causal = true, query_pos = 4.
    // Must attend ONLY to tokens [2, 3, 4], strictly masking old tokens [0, 1] and future token [5].
    const head_dim: usize = 4;
    const query: [4]f32 = [1.0, 0.5, -0.5, 0.25];
    const keys: [24]f32 = [
        100.0, 100.0, 100.0, 100.0, // token 0 (out of window - must be masked)
        100.0, 100.0, 100.0, 100.0, // token 1 (out of window - must be masked)
          0.8,   0.2,  -0.3,   0.1, // token 2 (in window)
         -0.4,   0.9,   0.2,   0.5, // token 3 (in window)
          0.5,  -0.1,  -0.6,   0.4, // token 4 (in window, query_pos)
        100.0, 100.0, 100.0, 100.0, // token 5 (future - must be masked)
    ];
    const vals: [24]f32 = [
        999.0, 999.0, 999.0, 999.0, // token 0 poison
        999.0, 999.0, 999.0, 999.0, // token 1 poison
          1.0,   2.0,  -1.0,   0.5, // token 2
         -0.5,   1.5,   2.0,  -1.0, // token 3
          0.25, -0.75,  0.5,   3.0, // token 4
        999.0, 999.0, 999.0, 999.0, // token 5 poison
    ];

    const cfg = SwaConfig{
        sink_tokens: 0,
        window_size: 3,
        head_dim: head_dim,
        causal: true,
    };

    let out_swa: [4]f32 = undefined;
    let out_ref: [4]f32 = undefined;
    let out_swa_s: []f32 = out_swa[0..];
    let out_ref_s: []f32 = out_ref[0..];

    expect_eq(
        cap.attention.swaAttentionSingleQuery(query[0..], keys[0..], vals[0..], 6, 4, cfg, out_swa_s),
        true,
    );

    const expected_indices: [3]usize = [2, 3, 4];
    referenceLocalSdpa(query[0..], keys[0..], vals[0..], expected_indices[0..], head_dim, out_ref_s);

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        expect_eq(absF64((out_swa[d] as f64) - (out_ref[d] as f64)) < 0.00001, true);
    }
}

fn testAttentionSinkPermanentRetentionInRingCache()! {
    const cfg = SwaConfig{
        sink_tokens: 2,
        window_size: 3,
        head_dim: 4,
        causal: true,
    };
    let k_buf: [20]f32 = undefined; // (2 + 3) * 4
    let v_buf: [20]f32 = undefined;
    let k_s: []f32 = k_buf[0..];
    let v_s: []f32 = v_buf[0..];

    const cache_opt = cap.attention.RingKvCache.init(cfg, k_s, v_s);
    expect_eq(cache_opt != null, true);
    if (init_cache := cache_opt) {
        let cache = init_cache;
        // Token 0 & 1 are anchored Attention Sinks with distinct strong keys and sentinel values.
        const sink0_k: [4]f32 = [10.0, 0.0, 0.0, 0.0];
        const sink0_v: [4]f32 = [42.0, 0.0, 0.0, 0.0];
        const sink1_k: [4]f32 = [0.0, 10.0, 0.0, 0.0];
        const sink1_v: [4]f32 = [0.0, 84.0, 0.0, 0.0];

        expect_eq(cache.appendToken(sink0_k[0..], sink0_v[0..]), true);
        expect_eq(cache.appendToken(sink1_k[0..], sink1_v[0..]), true);

        // Stream 10 more tokens (3+ full wrap-arounds of the 3-slot window).
        let step: usize = 0;
        while (step < 10) : (step += 1) {
            const wk: [4]f32 = [0.0, 0.0, 1.0, (step as f32) * 0.1];
            const wv: [4]f32 = [0.0, 0.0, (step as f32), 1.0];
            expect_eq(cache.appendToken(wk[0..], wv[0..]), true);
        }

        expect_eq(cache.activeLen(), 5);
        expect_eq(cache.totalSeen(), 12);

        // Query aligned with Sink 0 must still recover ~42.0 after 10 streaming evictions.
        let out: [4]f32 = undefined;
        let out_s: []f32 = out[0..];
        const q_sink0: [4]f32 = [10.0, 0.0, 0.0, 0.0];
        expect_eq(ringKvAttentionSingleQuery(q_sink0[0..], cache, out_s), true);
        expect_eq(absF64((out[0] as f64) - 42.0) < 0.001, true);

        // Query aligned with Sink 1 must still recover ~84.0.
        const q_sink1: [4]f32 = [0.0, 10.0, 0.0, 0.0];
        expect_eq(ringKvAttentionSingleQuery(q_sink1[0..], cache, out_s), true);
        expect_eq(absF64((out[1] as f64) - 84.0) < 0.001, true);

        // clear() resets occupancy and stream counters cleanly.
        cache.clear();
        expect_eq(cache.activeLen(), 0);
        expect_eq(cache.totalSeen(), 0);
    }
}

fn testUnboundedStreamingEquivalenceWithContiguousSwa()! {
    const seq_total: usize = 16;
    const head_dim: usize = 4;
    const cfg = SwaConfig{
        sink_tokens: 2,
        window_size: 4,
        head_dim: head_dim,
        causal: true,
    };

    // Synthesize 16 deterministic KV tokens.
    let all_k: [64]f32 = undefined;
    let all_v: [64]f32 = undefined;
    let t: usize = 0;
    while (t < seq_total) : (t += 1) {
        const tf: f32 = (t as f32) + 1.0;
        all_k[t * 4 + 0] = 0.2 * tf - 1.5;
        all_k[t * 4 + 1] = -0.15 * tf + 1.0;
        all_k[t * 4 + 2] = 0.1 * ((t % 5) as f32) - 0.2;
        all_k[t * 4 + 3] = 0.3 * ((t % 3) as f32) - 0.3;

        all_v[t * 4 + 0] = tf * 0.5;
        all_v[t * 4 + 1] = -0.25 * tf;
        all_v[t * 4 + 2] = ((t % 4) as f32) + 0.5;
        all_v[t * 4 + 3] = 1.0 / tf;
    }

    let k_ring: [24]f32 = undefined; // (2 + 4) * 4
    let v_ring: [24]f32 = undefined;
    let k_ring_s: []f32 = k_ring[0..];
    let v_ring_s: []f32 = v_ring[0..];

    if (init_cache := RingKvCache.init(cfg, k_ring_s, v_ring_s)) {
        let cache = init_cache;
        const query: [4]f32 = [0.6, -0.4, 0.8, -0.2];
        let out_ring: [4]f32 = undefined;
        let out_swa: [4]f32 = undefined;
        let out_ring_s: []f32 = out_ring[0..];
        let out_swa_s: []f32 = out_swa[0..];

        // Verify exact step-by-step equivalence before wrap, at capacity, and after 2+ wrap-arounds.
        t = 0;
        while (t < seq_total) : (t += 1) {
            const base: usize = t * head_dim;
            expect_eq(cache.appendToken(all_k[base .. base + head_dim], all_v[base .. base + head_dim]), true);
            expect_eq(ringKvAttentionSingleQuery(query[0..], cache, out_ring_s), true);
            expect_eq(
                swaAttentionSingleQuery(query[0..], all_k[0..], all_v[0..], seq_total, t, cfg, out_swa_s),
                true,
            );

            let d: usize = 0;
            while (d < head_dim) : (d += 1) {
                expect_eq(absF64((out_ring[d] as f64) - (out_swa[d] as f64)) < 0.00001, true);
            }
        }
        expect_eq(cache.activeLen(), 6);
        expect_eq(cache.totalSeen(), 16);
    } else {
        expect_eq(true, false);
    }
}

fn testSinkPlusWindowNumericalEquivalenceWithLocalSdpa()! {
    // 8 tokens, sink = 2 ([0, 1]), window = 3, query_pos = 6 -> attends [0, 1] U [4, 5, 6].
    // Tokens 2, 3 (evicted gap) and 7 (future) must be excluded.
    const head_dim: usize = 4;
    const query: [4]f32 = [0.5, -0.5, 1.0, 0.2];
    let keys: [32]f32 = undefined;
    let vals: [32]f32 = undefined;

    let t: usize = 0;
    while (t < 8) : (t += 1) {
        const tf: f32 = (t as f32) + 1.0;
        keys[t * 4 + 0] = 0.3 * tf;
        keys[t * 4 + 1] = -0.2 * tf;
        keys[t * 4 + 2] = 0.5;
        keys[t * 4 + 3] = -0.1 * tf;

        vals[t * 4 + 0] = tf;
        vals[t * 4 + 1] = -0.5 * tf;
        vals[t * 4 + 2] = 2.0 * tf;
        vals[t * 4 + 3] = 0.25 * tf;
    }

    const cfg = SwaConfig{
        sink_tokens: 2,
        window_size: 3,
        head_dim: head_dim,
        causal: true,
    };

    let out_swa: [4]f32 = undefined;
    let out_ref: [4]f32 = undefined;
    let out_swa_s: []f32 = out_swa[0..];
    let out_ref_s: []f32 = out_ref[0..];

    expect_eq(swaAttentionSingleQuery(query[0..], keys[0..], vals[0..], 8, 6, cfg, out_swa_s), true);

    const attended_indices: [5]usize = [0, 1, 4, 5, 6];
    referenceLocalSdpa(query[0..], keys[0..], vals[0..], attended_indices[0..], head_dim, out_ref_s);

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        expect_eq(absF64((out_swa[d] as f64) - (out_ref[d] as f64)) < 0.00001, true);
    }
}

fn testInvalidDimensionsEmptyCacheAndNanGuards()! {
    let k_buf: [16]f32 = undefined;
    let v_buf: [16]f32 = undefined;
    let out: [4]f32 = undefined;
    let k_s: []f32 = k_buf[0..];
    let v_s: []f32 = v_buf[0..];
    let out_s: []f32 = out[0..];

    const ok_vec: [4]f32 = [1.0, 0.5, -0.5, 0.25];
    const nan_vec: [4]f32 = [1.0, std.math.nan(f32), 0.5, 0.25];
    const inf_vec: [4]f32 = [1.0, std.math.inf(f32), 0.5, 0.25];

    // Invalid SwaConfig: head_dim == 0, sink + window == 0, or undersized backing buffer.
    expect_eq(RingKvCache.init(SwaConfig{ sink_tokens: 1, window_size: 2, head_dim: 0, causal: true }, k_s, v_s) == null, true);
    expect_eq(RingKvCache.init(SwaConfig{ sink_tokens: 0, window_size: 0, head_dim: 4, causal: true }, k_s, v_s) == null, true);
    expect_eq(RingKvCache.init(SwaConfig{ sink_tokens: 2, window_size: 4, head_dim: 4, causal: true }, k_s, v_s) == null, true);

    // Empty cache query rejected; NaN/Inf/wrong-len append rejected.
    const cfg_ok = SwaConfig{ sink_tokens: 2, window_size: 2, head_dim: 4, causal: true };
    if (init_cache := RingKvCache.init(cfg_ok, k_s, v_s)) {
        let cache = init_cache;
        expect_eq(ringKvAttentionSingleQuery(ok_vec[0..], cache, out_s), false);
        expect_eq(cache.appendToken(ok_vec[0..3], ok_vec[0..4]), false);
        expect_eq(cache.appendToken(nan_vec[0..], ok_vec[0..]), false);
        expect_eq(cache.appendToken(ok_vec[0..], inf_vec[0..]), false);
        expect_eq(cache.activeLen(), 0);

        expect_eq(cache.appendToken(ok_vec[0..], ok_vec[0..]), true);
        expect_eq(ringKvAttentionSingleQuery(nan_vec[0..], cache, out_s), false);
        expect_eq(ringKvAttentionSingleQuery(ok_vec[0..], cache, out[0..2]), false);
    } else {
        expect_eq(true, false);
    }

    // Sink-only cache (window_size == 0): fills sink then rejects further appends safely.
    const cfg_sink_only = SwaConfig{ sink_tokens: 1, window_size: 0, head_dim: 4, causal: true };
    if (init_so := RingKvCache.init(cfg_sink_only, k_s, v_s)) {
        let so_cache = init_so;
        expect_eq(so_cache.appendToken(ok_vec[0..], ok_vec[0..]), true);
        expect_eq(so_cache.appendToken(ok_vec[0..], ok_vec[0..]), false);
        expect_eq(so_cache.activeLen(), 1);
    } else {
        expect_eq(true, false);
    }

    // swaAttentionSingleQuery guards: seq_len == 0, query_pos >= seq_len, NaN query.
    expect_eq(swaAttentionSingleQuery(ok_vec[0..], k_buf[0..], v_buf[0..], 0, 0, cfg_ok, out_s), false);
    expect_eq(swaAttentionSingleQuery(ok_vec[0..], k_buf[0..], v_buf[0..], 4, 4, cfg_ok, out_s), false);
    expect_eq(swaAttentionSingleQuery(nan_vec[0..], k_buf[0..], v_buf[0..], 4, 2, cfg_ok, out_s), false);
}

fn main() {
    test("1. attention/sliding_window: pure SWA window & causal masking exactness", () => {
        testPureSwaWindowMaskingExactness()?;
    });

    test("2. attention/sliding_window: Attention Sink permanent retention under ring wrap-around", () => {
        testAttentionSinkPermanentRetentionInRingCache()?;
    });

    test("3. attention/sliding_window: unbounded RingKvCache streaming equivalence with contiguous SWA", () => {
        testUnboundedStreamingEquivalenceWithContiguousSwa()?;
    });

    test("4. attention/sliding_window: Sink + Window numerical equivalence with standard local SDPA", () => {
        testSinkPlusWindowNumericalEquivalenceWithLocalSdpa()?;
    });

    test("5. attention/sliding_window: invalid dimensions, empty cache, sink-only cap & NaN guards", () => {
        testInvalidDimensionsEmptyCacheAndNanGuards()?;
    });
}
