// Source: cp/cap/test/test_lsh_production_deep.cp (Production deep tests for cap:lsh)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LSH_NO_ENTRY,
    LshEntry,
    CosineLsh,
    EuclideanLsh,
    estimatedCosineFromHamming,
} from "cap:ml";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn hasId(ids: []const u32, count: usize, target: u32): bool {
    let i: usize = 0;
    while (i < count) {
        if (ids[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

test("1. CosineLsh 16D: high recall on near neighbors, low/zero collision on orthogonal & opposite vectors", () => {
    // dim=16, num_tables=10, num_bits=8, num_buckets=64
    let planes_arr: [1280]f64 = std.mem.zeroes([1280]f64);
    let heads_arr: [640]u32 = std.mem.zeroes([640]u32);
    let entries_arr: [200]LshEntry = std.mem.zeroes([200]LshEntry);
    let planes: []f64 = planes_arr[0..];
    let heads: []u32 = heads_arr[0..];
    let entries: []LshEntry = entries_arr[0..];

    let idx = CosineLsh.init(16, 10, 8, 64, 2026, &planes, &heads, &entries) ?? unreachable;
    expect_eq(heads_arr[0], LSH_NO_ENTRY);

    const base: [16]f64 = [
        1.0, 0.8, 0.6, 0.4, 0.2, 0.1, 0.3, 0.5,
        0.7, 0.9, 0.4, 0.2, 0.6, 0.8, 0.1, 0.3,
    ];

    // Insert 4 near neighbors (ids 0..3)
    let near: [16]f64 = std.mem.zeroes([16]f64);
    let n_idx: usize = 0;
    while (n_idx < 4) {
        let j: usize = 0;
        while (j < 16) {
            const delta = (((n_idx + j) % 3) as f64 - 1.0) * 0.03;
            near[j] = base[j] + delta;
            j += 1;
        }
        expect_eq(idx.insert(n_idx as u32, near[0..]), true);
        n_idx += 1;
    }

    // Insert opposite vector (id 10)
    let opp: [16]f64 = std.mem.zeroes([16]f64);
    let j: usize = 0;
    while (j < 16) {
        opp[j] = 0.0 - base[j];
        j += 1;
    }
    expect_eq(idx.insert(10, opp[0..]), true);

    // Insert orthogonal vector (id 11): pair-wise swapped with alternating sign so dot(base, ortho) == 0
    const ortho: [16]f64 = [
        -0.8, 1.0, -0.4, 0.6, -0.1, 0.2, -0.5, 0.3,
        -0.9, 0.7, -0.2, 0.4, -0.8, 0.6, -0.3, 0.1,
    ];
    expect_eq(idx.insert(11, ortho[0..]), true);

    let out_arr: [32]u32 = std.mem.zeroes([32]u32);
    let seen_arr: [8]u8 = std.mem.zeroes([8]u8);
    let out_ids: []u32 = out_arr[0..];
    let seen: []u8 = seen_arr[0..];

    const hits = idx.queryCandidates(base[0..], &out_ids, &seen) ?? unreachable;
    let near_hits: usize = 0;
    let k: u32 = 0;
    while (k < 4) {
        if (hasId(out_arr[0..], hits, k)) {
            near_hits += 1;
        }
        k += 1;
    }
    expect_eq(near_hits >= 3, true);
    expect_eq(hasId(out_arr[0..], hits, 10), false);
});

test("2. estimatedCosineFromHamming with 64-bit signatures approximates true cosine", () => {
    // dim=16, num_tables=1, num_bits=64, num_buckets=16
    let planes_arr: [1024]f64 = std.mem.zeroes([1024]f64);
    let heads_arr: [16]u32 = std.mem.zeroes([16]u32);
    let entries_arr: [8]LshEntry = std.mem.zeroes([8]LshEntry);
    let planes: []f64 = planes_arr[0..];
    let heads: []u32 = heads_arr[0..];
    let entries: []LshEntry = entries_arr[0..];

    const idx = CosineLsh.init(16, 1, 64, 16, 777, &planes, &heads, &entries) ?? unreachable;

    const e0: [16]f64 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const neg_e0: [16]f64 = [-1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const e1: [16]f64 = [0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    // cos(60 deg) = 0.5, sin(60 deg) = 0.8660254
    const deg60: [16]f64 = [0.5, 0.8660254, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];

    const s_e0 = idx.hashVector(e0[0..], 0) ?? unreachable;
    const s_neg = idx.hashVector(neg_e0[0..], 0) ?? unreachable;
    const s_e1 = idx.hashVector(e1[0..], 0) ?? unreachable;
    const s_60 = idx.hashVector(deg60[0..], 0) ?? unreachable;

    expect_eq(absF64(estimatedCosineFromHamming(s_e0, s_e0, 64) - 1.0) < 1.0e-12, true);
    expect_eq(absF64(CosineLsh.estimatedCosineFromHamming(s_e0, s_neg, 64) - (-1.0)) < 1.0e-12, true);
    expect_eq(absF64(estimatedCosineFromHamming(s_e0, s_e1, 64)) < 0.25, true);
    expect_eq(absF64(estimatedCosineFromHamming(s_e0, s_60, 64) - 0.5) < 0.25, true);
});

test("3. EuclideanLsh (E2LSH) 8D: recalls same-cluster neighbors and filters distant clusters", () => {
    // dim=8, num_tables=8, num_proj=4, bucket_width=4.0, num_buckets=64
    let proj_a_arr: [256]f64 = std.mem.zeroes([256]f64); // 8 * 4 * 8
    let proj_b_arr: [32]f64 = std.mem.zeroes([32]f64);   // 8 * 4
    let heads_arr: [512]u32 = std.mem.zeroes([512]u32);  // 8 * 64
    let entries_arr: [256]LshEntry = std.mem.zeroes([256]LshEntry);
    let proj_a: []f64 = proj_a_arr[0..];
    let proj_b: []f64 = proj_b_arr[0..];
    let heads: []u32 = heads_arr[0..];
    let entries: []LshEntry = entries_arr[0..];

    let idx = EuclideanLsh.init(8, 8, 4, 4.0, 64, 9001, &proj_a, &proj_b, &heads, &entries) ?? unreachable;

    // Cluster 0 around (0, ..., 0): ids 0..3
    let v: [8]f64 = std.mem.zeroes([8]f64);
    let i: usize = 0;
    while (i < 4) {
        let j: usize = 0;
        while (j < 8) {
            v[j] = (((i + j) % 3) as f64 - 1.0) * 0.2;
            j += 1;
        }
        expect_eq(idx.insert(i as u32, v[0..]), true);
        i += 1;
    }

    // Cluster 1 around (30, ..., 30): ids 10..13
    i = 0;
    while (i < 4) {
        let j: usize = 0;
        while (j < 8) {
            v[j] = 30.0 + (((i + j) % 3) as f64 - 1.0) * 0.2;
            j += 1;
        }
        expect_eq(idx.insert((10 + i) as u32, v[0..]), true);
        i += 1;
    }

    // Cluster 2 around (-30, ..., -30): ids 20..23
    i = 0;
    while (i < 4) {
        let j: usize = 0;
        while (j < 8) {
            v[j] = -30.0 + (((i + j) % 3) as f64 - 1.0) * 0.2;
            j += 1;
        }
        expect_eq(idx.insert((20 + i) as u32, v[0..]), true);
        i += 1;
    }

    const query_c0: [8]f64 = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    let out_arr: [32]u32 = std.mem.zeroes([32]u32);
    let seen_arr: [8]u8 = std.mem.zeroes([8]u8);
    let out_ids: []u32 = out_arr[0..];
    let seen: []u8 = seen_arr[0..];

    const hits = idx.queryCandidates(query_c0[0..], &out_ids, &seen) ?? unreachable;
    let c0_hits: usize = 0;
    let k: u32 = 0;
    while (k < 4) {
        if (hasId(out_arr[0..], hits, k)) {
            c0_hits += 1;
        }
        expect_eq(hasId(out_arr[0..], hits, 10 + k), false);
        expect_eq(hasId(out_arr[0..], hits, 20 + k), false);
        k += 1;
    }
    expect_eq(c0_hits >= 3, true);
});

test("4. Cross-table deduplication guarantees unique candidate IDs", () => {
    let planes_arr: [128]f64 = std.mem.zeroes([128]f64); // 8 tables * 4 bits * 4 dim
    let heads_arr: [128]u32 = std.mem.zeroes([128]u32);  // 8 tables * 16 buckets
    let entries_arr: [64]LshEntry = std.mem.zeroes([64]LshEntry);
    let planes: []f64 = planes_arr[0..];
    let heads: []u32 = heads_arr[0..];
    let entries: []LshEntry = entries_arr[0..];

    let idx = CosineLsh.init(4, 8, 4, 16, 42, &planes, &heads, &entries) ?? unreachable;
    const v = [1.0, 2.0, 3.0, 4.0];
    expect_eq(idx.insert(5, v[0..]), true);
    expect_eq(idx.insert(12, v[0..]), true);
    expect_eq(idx.insert(19, v[0..]), true);

    let out_arr: [32]u32 = std.mem.zeroes([32]u32);
    let seen_arr: [8]u8 = std.mem.zeroes([8]u8);
    let out_ids: []u32 = out_arr[0..];
    let seen: []u8 = seen_arr[0..];

    // Every item collides across all 8 tables, yet must appear exactly once.
    const hits = idx.queryCandidates(v[0..], &out_ids, &seen) ?? unreachable;
    expect_eq(hits, 3);
    expect_eq(hasId(out_arr[0..], hits, 5), true);
    expect_eq(hasId(out_arr[0..], hits, 12), true);
    expect_eq(hasId(out_arr[0..], hits, 19), true);
});

test("5. Bit-exact determinism with identical seed for CosineLsh & EuclideanLsh", () => {
    const v = [0.5, -1.2, 2.3, -0.7, 1.1, 0.4, -0.9, 1.8];

    // CosineLsh x2
    let p1_arr: [256]f64 = std.mem.zeroes([256]f64);
    let h1_arr: [64]u32 = std.mem.zeroes([64]u32);
    let e1_arr: [16]LshEntry = std.mem.zeroes([16]LshEntry);
    let p1: []f64 = p1_arr[0..];
    let h1: []u32 = h1_arr[0..];
    let e1: []LshEntry = e1_arr[0..];

    let p2_arr: [256]f64 = std.mem.zeroes([256]f64);
    let h2_arr: [64]u32 = std.mem.zeroes([64]u32);
    let e2_arr: [16]LshEntry = std.mem.zeroes([16]LshEntry);
    let p2: []f64 = p2_arr[0..];
    let h2: []u32 = h2_arr[0..];
    let e2: []LshEntry = e2_arr[0..];

    const c1 = CosineLsh.init(8, 4, 8, 16, 0xABCDEF123456, &p1, &h1, &e1) ?? unreachable;
    const c2 = CosineLsh.init(8, 4, 8, 16, 0xABCDEF123456, &p2, &h2, &e2) ?? unreachable;
    let t: usize = 0;
    while (t < 4) {
        expect_eq(c1.hashVector(v[0..], t) ?? unreachable, c2.hashVector(v[0..], t) ?? unreachable);
        t += 1;
    }

    // EuclideanLsh x2
    let a1_arr: [128]f64 = std.mem.zeroes([128]f64);
    let b1_arr: [16]f64 = std.mem.zeroes([16]f64);
    let eh1_arr: [64]u32 = std.mem.zeroes([64]u32);
    let ee1_arr: [16]LshEntry = std.mem.zeroes([16]LshEntry);
    let a1: []f64 = a1_arr[0..];
    let b1: []f64 = b1_arr[0..];
    let eh1: []u32 = eh1_arr[0..];
    let ee1: []LshEntry = ee1_arr[0..];

    let a2_arr: [128]f64 = std.mem.zeroes([128]f64);
    let b2_arr: [16]f64 = std.mem.zeroes([16]f64);
    let eh2_arr: [64]u32 = std.mem.zeroes([64]u32);
    let ee2_arr: [16]LshEntry = std.mem.zeroes([16]LshEntry);
    let a2: []f64 = a2_arr[0..];
    let b2: []f64 = b2_arr[0..];
    let eh2: []u32 = eh2_arr[0..];
    let ee2: []LshEntry = ee2_arr[0..];

    const e_idx1 = EuclideanLsh.init(8, 4, 4, 2.5, 16, 0x13579BDF, &a1, &b1, &eh1, &ee1) ?? unreachable;
    const e_idx2 = EuclideanLsh.init(8, 4, 4, 2.5, 16, 0x13579BDF, &a2, &b2, &eh2, &ee2) ?? unreachable;
    t = 0;
    while (t < 4) {
        expect_eq(e_idx1.hashVector(v[0..], t) ?? unreachable, e_idx2.hashVector(v[0..], t) ?? unreachable);
        t += 1;
    }
});

test("6. Defensive guards: invalid bits/width, pool exhaustion, small buffers, and NaN", () => {
    let planes_arr: [64]f64 = std.mem.zeroes([64]f64);
    let heads_arr: [32]u32 = std.mem.zeroes([32]u32);
    let entries_arr: [2]LshEntry = std.mem.zeroes([2]LshEntry);
    let short_planes_arr: [4]f64 = std.mem.zeroes([4]f64);
    let planes: []f64 = planes_arr[0..];
    let heads: []u32 = heads_arr[0..];
    let entries: []LshEntry = entries_arr[0..];
    let short_planes: []f64 = short_planes_arr[0..];

    // CosineLsh invalid num_bits (0, 65) & small planes buffer
    expect_eq(CosineLsh.init(4, 2, 0, 8, 1, &planes, &heads, &entries) == null, true);
    expect_eq(CosineLsh.init(4, 2, 65, 8, 1, &planes, &heads, &entries) == null, true);
    expect_eq(CosineLsh.init(4, 2, 4, 8, 1, &short_planes, &heads, &entries) == null, true);

    // Pool exhaustion: entries has capacity 2, num_tables = 2 -> 1st insert ok, 2nd insert false
    let c_idx = CosineLsh.init(4, 2, 4, 8, 1, &planes, &heads, &entries) ?? unreachable;
    const v_ok = [1.0, 2.0, 3.0, 4.0];
    const zero_f: f64 = 0.0;
    const nan_f: f64 = zero_f / zero_f;
    const v_nan = [1.0, nan_f, 3.0, 4.0];

    expect_eq(c_idx.insert(0, v_nan[0..]), false);
    expect_eq(c_idx.hashVector(v_nan[0..], 0) == null, true);
    expect_eq(c_idx.insert(0, v_ok[0..]), true);
    expect_eq(c_idx.insert(1, v_ok[0..]), false);

    // EuclideanLsh invalid bucket_width (<= 0, NaN) & small proj_b buffer
    let pa_arr: [64]f64 = std.mem.zeroes([64]f64);
    let pb_arr: [8]f64 = std.mem.zeroes([8]f64);
    let pb_short_arr: [1]f64 = std.mem.zeroes([1]f64);
    let pa: []f64 = pa_arr[0..];
    let pb: []f64 = pb_arr[0..];
    let pb_short: []f64 = pb_short_arr[0..];

    expect_eq(EuclideanLsh.init(4, 2, 2, 0.0, 8, 1, &pa, &pb, &heads, &entries) == null, true);
    expect_eq(EuclideanLsh.init(4, 2, 2, -1.0, 8, 1, &pa, &pb, &heads, &entries) == null, true);
    expect_eq(EuclideanLsh.init(4, 2, 2, nan_f, 8, 1, &pa, &pb, &heads, &entries) == null, true);
    expect_eq(EuclideanLsh.init(4, 2, 2, 1.0, 8, 1, &pa, &pb_short, &heads, &entries) == null, true);

    let e_idx = EuclideanLsh.init(4, 2, 2, 1.0, 8, 1, &pa, &pb, &heads, &entries) ?? unreachable;
    expect_eq(e_idx.insert(0, v_nan[0..]), false);
    expect_eq(e_idx.hashVector(v_nan[0..], 0) == null, true);
    let out_arr: [4]u32 = std.mem.zeroes([4]u32);
    let seen_arr: [4]u8 = std.mem.zeroes([4]u8);
    let out_ids: []u32 = out_arr[0..];
    let seen: []u8 = seen_arr[0..];
    expect_eq(e_idx.queryCandidates(v_nan[0..], &out_ids, &seen) == null, true);
    expect_eq(e_idx.insert(0, v_ok[0..]), true);
    expect_eq(e_idx.insert(1, v_ok[0..]), false);
});
