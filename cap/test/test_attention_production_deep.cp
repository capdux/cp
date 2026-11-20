// Source: cp/cap/test/test_attention_production_deep.cp (Production Deep Tests for cap:attention)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AttnError,
    scaledDotProductAttention,
    groupedQueryAttention,
    PagedKvSlot,
    KvCache,
    SamplingConfig,
    sampleGreedy,
    sampleToken,
} from "cap:attention";

test ("sdpa causal mask and numerical ground truth") {
    const alloc = std.testing.allocator;

    const q: [4]f32 = [1.0, 0.0, 0.0, 1.0];
    const k: [4]f32 = [1.0, 0.0, 0.0, 1.0];
    const v1: [4]f32 = [10.0, 20.0, 30.0, 40.0];
    const v2: [4]f32 = [10.0, 20.0, 999.0, -999.0];

    const out1 = try scaledDotProductAttention(alloc, &q, &k, &v1, 2, 2, 2, true, 0);
    defer alloc.free(out1);
    const out2 = try scaledDotProductAttention(alloc, &q, &k, &v2, 2, 2, 2, true, 0);
    defer alloc.free(out2);

    // Token 0 only sees Key 0 -> output is exactly v[0] = [10, 20]
    try std.testing.expect(@abs(out1[0] - 10.0) < 1e-5);
    try std.testing.expect(@abs(out1[1] - 20.0) < 1e-5);
    try std.testing.expect(@abs(out1[0] - out2[0]) < 1e-6);
    try std.testing.expect(@abs(out1[1] - out2[1]) < 1e-6);

    // Token 1 sees Key 0 (dot=0) and Key 1 (dot=1/sqrt(2))
    const inv_sqrt2: f32 = 1.0 / @sqrt(@as(f32, 2.0));
    const w0: f32 = 1.0;
    const w1: f32 = @exp(inv_sqrt2);
    const sum_w: f32 = w0 + w1;
    const expected_x: f32 = (w0 * 10.0 + w1 * 30.0) / sum_w;
    const expected_y: f32 = (w0 * 20.0 + w1 * 40.0) / sum_w;
    try std.testing.expect(@abs(out1[2] - expected_x) < 1e-4);
    try std.testing.expect(@abs(out1[3] - expected_y) < 1e-4);
}

test ("gqa 8q 2kv with kv cache append and truncateTo rollback") {
    const alloc = std.testing.allocator;
    const num_layers: usize = 2;
    const num_q_heads: usize = 8;
    const num_kv_heads: usize = 2;
    const head_dim: usize = 2;
    const max_seq_len: usize = 4;

    let cache = try KvCache.init(alloc, num_layers, num_kv_heads, head_dim, max_seq_len);
    defer cache.deinit();

    const k0: [4]f32 = [1.0, 0.0, 0.0, 1.0];
    const v0: [4]f32 = [2.0, 3.0, 4.0, 5.0];
    const slot0: PagedKvSlot = try cache.appendStep(0, &k0, &v0);
    expect_eq(slot0.seq_offset, @as(usize, 0));
    expect_eq(cache.seqLen(0), @as(usize, 1));

    const k1: [4]f32 = [0.0, 1.0, 1.0, 0.0];
    const v1: [4]f32 = [6.0, 7.0, 8.0, 9.0];
    const slot1 = try cache.appendStep(0, &k1, &v1);
    expect_eq(slot1.seq_offset, @as(usize, 1));
    expect_eq(cache.seqLen(0), @as(usize, 2));

    const k2: [4]f32 = [5.0, 5.0, 5.0, 5.0];
    const v2: [4]f32 = [100.0, 100.0, 100.0, 100.0];
    * = try cache.appendStep(0, &k2, &v2);
    expect_eq(cache.seqLen(0), @as(usize, 3));

    cache.truncateTo(2);
    expect_eq(cache.seqLen(0), @as(usize, 2));

    const q: [16]f32 = [
        1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0,
        0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
    ];
    const out = try groupedQueryAttention(
        alloc,
        &q,
        cache.keysSlice(0),
        cache.valuesSlice(0),
        num_q_heads,
        num_kv_heads,
        cache.seqLen(0),
        head_dim,
    );
    defer alloc.free(out);

    try std.testing.expect(@abs(out[0] - out[2]) < 1e-6);
    try std.testing.expect(@abs(out[1] - out[3]) < 1e-6);
    try std.testing.expect(@abs(out[8] - out[10]) < 1e-6);
    try std.testing.expect(@abs(out[9] - out[11]) < 1e-6);

    cache.reset();
    expect_eq(cache.seqLen(0), @as(usize, 0));
}

test ("sampler greedy, repetition penalty, top-k, top-p and deterministic seed") {
    const alloc = std.testing.allocator;
    const logits: [5]f32 = [1.0, 4.0, 3.5, 0.2, -1.0];

    const g = try sampleGreedy(&logits);
    expect_eq(g, @as(usize, 1));

    const recent: [1]usize = [1];
    const pen_cfg = SamplingConfig{
        .temperature = 0.0,
        .repetition_penalty = 2.0,
        .seed = 99,
    };
    const t_pen = try sampleToken(alloc, &logits, &recent, pen_cfg);
    expect_eq(t_pen, @as(usize, 2));

    const cfg = SamplingConfig{
        .temperature = 0.8,
        .top_k = 2,
        .top_p = 0.95,
        .repetition_penalty = 1.0,
        .seed = 7777,
    };
    const empty_recent: [0]usize = [];
    const s1 = try sampleToken(alloc, &logits, &empty_recent, cfg);
    const s2 = try sampleToken(alloc, &logits, &empty_recent, cfg);
    expect_eq(s1, s2);
    try std.testing.expect(s1 == 1 or s1 == 2);
}

test ("error handling for invalid dims, head mismatch, full cache, and bad temp") {
    const alloc = std.testing.allocator;
    const q: [2]f32 = [1.0, 0.0];
    const k: [2]f32 = [1.0, 0.0];
    const v: [2]f32 = [1.0, 0.0];

    try std.testing.expectError(
        AttnError.InvalidDimension,
        scaledDotProductAttention(alloc, &q, &k, &v, 2, 1, 2, false, 0)
    );

    const q_gqa: [6]f32 = [1.0, 0.0, 1.0, 0.0, 1.0, 0.0];
    const k_gqa: [4]f32 = [1.0, 0.0, 1.0, 0.0];
    try std.testing.expectError(
        AttnError.HeadMismatch,
        groupedQueryAttention(alloc, &q_gqa, &k_gqa, &k_gqa, 3, 2, 1, 2)
    );

    let cache = try KvCache.init(alloc, 1, 1, 2, 1);
    defer cache.deinit();
    * = try cache.appendStep(0, &k, &v);
    try std.testing.expectError(AttnError.CacheFull, cache.appendStep(0, &k, &v));

    const logits: [2]f32 = [0.5, 1.5];
    const empty_recent: [0]usize = [];
    const bad_cfg = SamplingConfig{ .temperature = -0.5 };
    try std.testing.expectError(
        AttnError.InvalidTemperature,
        sampleToken(alloc, &logits, &empty_recent, bad_cfg)
    );
}

test ("cap.attention dynamic mount and zero memory leak") {
    const alloc = std.testing.allocator;
    const logits: [3]f32 = [0.1, 2.5, 1.0];
    const idx = try cap.attention.sampleGreedy(&logits);
    expect_eq(idx, @as(usize, 1));

    const q: [2]f32 = [1.0, 0.0];
    const k: [2]f32 = [1.0, 0.0];
    const v: [2]f32 = [3.0, 4.0];
    const out = try cap.attention.scaledDotProductAttention(alloc, &q, &k, &v, 1, 1, 2, false, 0);
    defer alloc.free(out);
    try std.testing.expect(@abs(out[0] - 3.0) < 1e-6);
    try std.testing.expect(@abs(out[1] - 4.0) < 1e-6);
}
