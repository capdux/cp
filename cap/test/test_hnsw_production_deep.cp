// Source: cp/cap/test/test_hnsw_production_deep.cp (Production deep test suite for cap:hnsw)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { HnswIndex, HnswConfig, DistanceMetric, HnswError, SearchHit, computeDistance } from "cap:hnsw";
import cap from "cap";

fn approxEq(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

fn nextRandF32(state: *u64): f32 {
    state.* = state.* *% 6364136223846793005 +% 1442695040888963407;
    const u: u32 = @truncate(state.* >> 40);
    return @as(f32, @floatFromInt(u)) / 16777216.0;
}

test("1. Distance metrics (L2Squared, Cosine, InnerProduct) and validation") {
    const a: [3]f32 = [1.0, 0.0, 0.0];
    const b: [3]f32 = [0.0, 1.0, 0.0];
    const c: [3]f32 = [2.0, 0.0, 0.0];
    const d: [3]f32 = [1.0, 2.0, 3.0];
    const e: [3]f32 = [3.0, 4.0, 0.0];

    const l2 = try computeDistance(.L2Squared, &a, &b);
    expect_eq(approxEq(l2, 2.0, 1.0e-5), true);

    const cos_orth = try computeDistance(.Cosine, &a, &b);
    expect_eq(approxEq(cos_orth, 1.0, 1.0e-5), true);

    const cos_same = try computeDistance(.Cosine, &a, &c);
    expect_eq(approxEq(cos_same, 0.0, 1.0e-5), true);

    const ip = try computeDistance(.InnerProduct, &d, &e);
    expect_eq(approxEq(ip, -11.0, 1.0e-5), true);

    const bad_dim: [2]f32 = [1.0, 2.0];
    try std.testing.expectError(HnswError.DimensionMismatch, computeDistance(.L2Squared, &a, &bad_dim));

    const empty: [0]f32 = [];
    try std.testing.expectError(HnswError.EmptyVector, computeDistance(.L2Squared, &empty, &empty));

    const bad_cfg = HnswConfig{ .m = 1, .m0 = 2, .ef_construction = 10 };
    try std.testing.expectError(HnswError.InvalidConfig, HnswIndex.init(std.testing.allocator, 3, .L2Squared, bad_cfg));
}

test("2. 16D 120-vector cluster dataset HNSW insert and Recall@5 >= 0.9") {
    const alloc = std.testing.allocator;
    const dim: usize = 16;
    const n_vecs: usize = 120;
    const top_k: usize = 5;

    let idx = try HnswIndex.init(alloc, dim, .L2Squared, HnswConfig{
        .m = 12,
        .m0 = 24,
        .ef_construction = 64,
        .seed = 42,
    });
    defer idx.deinit();

    let dataset = try alloc.alloc(f32, n_vecs * dim);
    defer alloc.free(dataset);

    let rng: u64 = 123456789;
    let i: usize = 0;
    while (i < n_vecs) {
        const cluster: f32 = @floatFromInt(i % 4);
        let d: usize = 0;
        while (d < dim) {
            const base: f32 = if (d < 4) cluster * 2.5 else 0.0;
            dataset[i * dim + d] = base + nextRandF32(&rng) * 0.6;
            d += 1;
        }
        try idx.insert(@intCast(i + 1), dataset[i * dim .. (i + 1) * dim]);
        i += 1;
    }

    expect_eq(idx.len(), n_vecs);

    let total_hits: usize = 0;
    let total_expected: usize = 0;
    let q_idx: usize = 0;
    while (q_idx < 10) {
        let query: [16]f32 = undefined;
        const cluster: f32 = @floatFromInt(q_idx % 4);
        let d: usize = 0;
        while (d < dim) {
            const base: f32 = if (d < 4) cluster * 2.5 else 0.0;
            query[d] = base + nextRandF32(&rng) * 0.6;
            d += 1;
        }

        // Brute-force ground truth Top-K
        let gt_ids: [120]u64 = undefined;
        let gt_dists: [120]f32 = undefined;
        let vi: usize = 0;
        while (vi < n_vecs) {
            gt_ids[vi] = @intCast(vi + 1);
            gt_dists[vi] = try computeDistance(.L2Squared, &query, dataset[vi * dim .. (vi + 1) * dim]);
            vi += 1;
        }
        // Selection sort first top_k
        let k: usize = 0;
        while (k < top_k) {
            let best = k;
            let j = k + 1;
            while (j < n_vecs) {
                if (gt_dists[j] < gt_dists[best]) {
                    best = j;
                }
                j += 1;
            }
            const tmp_d = gt_dists[k];
            gt_dists[k] = gt_dists[best];
            gt_dists[best] = tmp_d;
            const tmp_id = gt_ids[k];
            gt_ids[k] = gt_ids[best];
            gt_ids[best] = tmp_id;
            k += 1;
        }

        const hits = try idx.search(alloc, &query, top_k, 48);
        defer alloc.free(hits);
        expect_eq(hits.len, top_k);

        // Verify sorted ascending
        let s: usize = 1;
        while (s < hits.len) {
            expect_eq(hits[s - 1].distance <= hits[s].distance, true);
            s += 1;
        }

        let hi: usize = 0;
        while (hi < hits.len) {
            let gi: usize = 0;
            while (gi < top_k) {
                if (hits[hi].id == gt_ids[gi]) {
                    total_hits += 1;
                    break;
                }
                gi += 1;
            }
            hi += 1;
        }
        total_expected += top_k;
        q_idx += 1;
    }

    // Recall@5 >= 0.9 (at least 45 / 50)
    expect_eq(total_hits * 10 >= total_expected * 9, true);
}

test("3. markDeleted soft-delete filtering and DuplicateId / IdNotFound") {
    const alloc = std.testing.allocator;
    let idx = try HnswIndex.init(alloc, 3, .L2Squared, HnswConfig{ .m = 4, .m0 = 8, .ef_construction = 16, .seed = 7 });
    defer idx.deinit();

    const v1: [3]f32 = [0.0, 0.0, 0.0];
    const v2: [3]f32 = [1.0, 0.0, 0.0];
    const v3: [3]f32 = [5.0, 0.0, 0.0];
    try idx.insert(10, &v1);
    try idx.insert(20, &v2);
    try idx.insert(30, &v3);

    try std.testing.expectError(HnswError.DuplicateId, idx.insert(10, &v2));
    try std.testing.expectError(HnswError.IdNotFound, idx.markDeleted(999));

    expect_eq(idx.len(), 3);
    try idx.markDeleted(10);
    expect_eq(idx.len(), 2);
    expect_eq(idx.totalNodes(), 3);

    const hits = try idx.search(alloc, &v1, 2, 16);
    defer alloc.free(hits);
    expect_eq(hits.len, 2);
    expect_eq(hits[0].id, 20);
    expect_eq(hits[1].id, 30);
}

test("4. saveBytes / loadBytes binary snapshot round-trip and CorruptSnapshot") {
    const alloc = std.testing.allocator;
    let orig = try HnswIndex.init(alloc, 8, .Cosine, HnswConfig{ .m = 8, .m0 = 16, .ef_construction = 32, .seed = 99 });
    defer orig.deinit();

    let rng: u64 = 987654321;
    let i: usize = 0;
    while (i < 30) {
        let vec: [8]f32 = undefined;
        let d: usize = 0;
        while (d < 8) {
            vec[d] = nextRandF32(&rng) + 0.1;
            d += 1;
        }
        try orig.insert(@intCast(1000 + i), &vec);
        i += 1;
    }
    try orig.markDeleted(1002);
    try orig.markDeleted(1015);

    const snap = try orig.saveBytes(alloc);
    defer alloc.free(snap);

    let loaded = try HnswIndex.loadBytes(alloc, snap);
    defer loaded.deinit();

    expect_eq(loaded.len(), orig.len());
    expect_eq(loaded.totalNodes(), orig.totalNodes());
    expect_eq(loaded.maxLevel(), orig.maxLevel());

    const query: [8]f32 = [0.5, 0.4, 0.3, 0.2, 0.6, 0.7, 0.1, 0.8];
    const hits_orig = try orig.search(alloc, &query, 5, 24);
    defer alloc.free(hits_orig);
    const hits_loaded = try loaded.search(alloc, &query, 5, 24);
    defer alloc.free(hits_loaded);

    expect_eq(hits_orig.len, hits_loaded.len);
    let hi: usize = 0;
    while (hi < hits_orig.len) {
        expect_eq(hits_orig[hi].id, hits_loaded[hi].id);
        expect_eq(approxEq(hits_orig[hi].distance, hits_loaded[hi].distance, 1.0e-6), true);
        hi += 1;
    }

    try std.testing.expectError(HnswError.CorruptSnapshot, HnswIndex.loadBytes(alloc, "BAD!"));
    try std.testing.expectError(HnswError.CorruptSnapshot, HnswIndex.loadBytes(alloc, snap[0..12]));
}

test("5. cap.hnsw dynamic mount and std.testing.allocator zero-leak check") {
    const alloc = std.testing.allocator;
    let idx = try cap.hnsw.HnswIndex.init(
        alloc,
        4,
        cap.hnsw.DistanceMetric.InnerProduct,
        cap.hnsw.HnswConfig{ .m = 4, .m0 = 8, .ef_construction = 16, .seed = 1 },
    );
    defer idx.deinit();

    const a: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    const b: [4]f32 = [0.1, 0.1, 0.1, 0.1];
    try idx.insert(1, &a);
    try idx.insert(2, &b);

    const hits = try idx.search(alloc, &a, 1, 8);
    defer alloc.free(hits);
    expect_eq(hits.len, 1);
    expect_eq(hits[0].id, 1);
}
