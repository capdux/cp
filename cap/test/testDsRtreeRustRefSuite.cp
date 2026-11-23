// Wave 29 R10 (Lane 2B) Rust Reference Hardening Test Suite for lib/ds/rtree.cp
// Reference benchmark: georust/rstar (MIT/Apache-2.0)
// Covers:
//   1. 4-Lane SIMD + FMA Batch Kernels (`minDistSqBatchSimd`, `overlapBatchSimd`) vs Scalar (< 1e-4 / exact)
//   2. STR Bulk Load (`bulkLoad` / `bulkLoadWithCapacity`) vs Incremental Insert + Structural Invariants
//   3. R*-Tree Underflow-Reinsertion `remove` + Single-Element / Empty Tree / Root Collapse
//   4. Degenerate Geometry (zero-area points, coincident points, negative/huge coords, NaN/Inf rejection)
//   5. 1,300 Deterministic PRNG Differential Ops (insert/remove/bulkLoad/intersect/contain/radius/KNN) vs Brute-Force Oracle

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    RTreeError,
    Rect,
    RTreeEntry,
    NearestRect,
    minDistSqBatchScalar,
    minDistSqBatchSimd,
    overlapBatchScalar,
    overlapBatchSimd,
    RTree,
} from "../../lib/ds/rtree.cp";

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

struct XorShift64 {
    state: u64 = 0x1E37_79B9_7F4A_7C15;

    fn next(*_): u64 {
        let x: u64 = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn range(*_, bound: usize): usize {
        if (bound == 0) return 0;
        return @as(usize, @intCast(_.next() % @as(u64, @intCast(bound))));
    }

    fn nextF64(*_, lo: f64, hi: f64): f64 {
        const u: u64 = _.next() & 0xFFFF_FFFF;
        const frac: f64 = @as(f64, @floatFromInt(u)) / 4294967295.0;
        return lo + frac * (hi - lo);
    }
}

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn sortU64(arr: []u64) {
    let i: usize = 1;
    while (i < arr.len) : (i += 1) {
        const key: u64 = arr[i];
        let j: usize = i;
        while (j > 0 && arr[j - 1] > key) : (j -= 1) {
            arr[j] = arr[j - 1];
        }
        arr[j] = key;
    }
}

fn sortF64(arr: []f64) {
    let i: usize = 1;
    while (i < arr.len) : (i += 1) {
        const key: f64 = arr[i];
        let j: usize = i;
        while (j > 0 && arr[j - 1] > key) : (j -= 1) {
            arr[j] = arr[j - 1];
        }
        arr[j] = key;
    }
}

// ============================================================================
// Test 1: 4-Lane SIMD + FMA Batch Kernels vs Scalar Reference
// ============================================================================
fn runTestSimdBatchKernels(): void! {
    let rng: XorShift64 = XorShift64{ state: 0x3141_5926_5358_9793 };
    let min_x: [68]f32 = [0.0; 68];
    let min_y: [68]f32 = [0.0; 68];
    let max_x: [68]f32 = [0.0; 68];
    let max_y: [68]f32 = [0.0; 68];
    let d_simd: [68]f32 = [0.0; 68];
    let d_ref: [68]f32 = [0.0; 68];
    let h_simd: [68]u8 = [0; 68];
    let h_ref: [68]u8 = [0; 68];

    // Sweep lengths 0..67 (including 0, 1, 3, 4, 5, 7, 8, 15, 16, 67)
    let len: usize = 0;
    while (len <= 67) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            const x0: f32 = @floatCast(rng.nextF64(-100.0, 100.0));
            const y0: f32 = @floatCast(rng.nextF64(-100.0, 100.0));
            const w: f32 = if (rng.range(5) == 0) 0.0 else @floatCast(rng.nextF64(0.0, 25.0));
            const h: f32 = if (rng.range(5) == 0) 0.0 else @floatCast(rng.nextF64(0.0, 25.0));
            min_x[i] = x0;
            min_y[i] = y0;
            max_x[i] = x0 + w;
            max_y[i] = y0 + h;
        }
        const qx: f32 = @floatCast(rng.nextF64(-120.0, 120.0));
        const qy: f32 = @floatCast(rng.nextF64(-120.0, 120.0));

        let ds_s: []f32 = d_simd[0..len];
        let dr_s: []f32 = d_ref[0..len];
        const n1: usize = minDistSqBatchSimd(qx, qy, min_x[0..len], min_y[0..len], max_x[0..len], max_y[0..len], ds_s);
        const n2: usize = minDistSqBatchScalar(qx, qy, min_x[0..len], min_y[0..len], max_x[0..len], max_y[0..len], dr_s);
        expect_eq(n1, len);
        expect_eq(n2, len);
        i = 0;
        while (i < len) : (i += 1) {
            const rel_tol: f32 = 1.0e-4 + 1.0e-5 * absF32(d_ref[i]);
            expect_true(absF32(d_simd[i] - d_ref[i]) <= rel_tol)?;
        }

        const dw: f32 = @floatCast(rng.nextF64(0.0, 40.0));
        const dh: f32 = @floatCast(rng.nextF64(0.0, 40.0));
        const qx1: f32 = qx + dw;
        const qy1: f32 = qy + dh;
        let hs_s: []u8 = h_simd[0..len];
        let hr_s: []u8 = h_ref[0..len];
        const c1: usize = overlapBatchSimd(qx, qy, qx1, qy1, min_x[0..len], min_y[0..len], max_x[0..len], max_y[0..len], hs_s);
        const c2: usize = overlapBatchScalar(qx, qy, qx1, qy1, min_x[0..len], min_y[0..len], max_x[0..len], max_y[0..len], hr_s);
        expect_eq(c1, c2);
        i = 0;
        while (i < len) : (i += 1) {
            expect_eq(h_simd[i], h_ref[i]);
        }
    }
}

// ============================================================================
// Test 2: STR Bulk Load vs Incremental Insert & Multi-Query Parity
// ============================================================================
fn runTestStrBulkLoadParity(): void! {
    const alloc = std.heap.page_allocator;
    let rng: XorShift64 = XorShift64{ state: 0x2718_2818_2845_9045 };

    const N: usize = 200;
    let entries: [200]RTreeEntry<u64> = undefined;
    let inc_tree = RTree<u64>.initWithCapacity(alloc, 8) catch { expect_true(false)?; return; };
    defer inc_tree.deinit();

    let i: usize = 0;
    while (i < N) : (i += 1) {
        const x0: f64 = rng.nextF64(-500.0, 500.0);
        const y0: f64 = rng.nextF64(-500.0, 500.0);
        const w: f64 = if (rng.range(4) == 0) 0.0 else rng.nextF64(0.0, 30.0);
        const h: f64 = if (rng.range(4) == 0) 0.0 else rng.nextF64(0.0, 30.0);
        const r: Rect = Rect.init(x0, y0, x0 + w, y0 + h) catch { expect_true(false)?; return; };
        entries[i] = RTreeEntry<u64>{ id: @as(u64, @intCast(i + 1)), box_rect: r, value: @as(u64, @intCast(i * 7)) };
        inc_tree.insert(entries[i].id, entries[i].box_rect, entries[i].value) catch { expect_true(false)?; return; };
    }

    let str_tree = RTree<u64>.bulkLoadWithCapacity(alloc, entries[0..], 8) catch { expect_true(false)?; return; };
    defer str_tree.deinit();

    expect_eq(str_tree.len(), N);
    expect_true(str_tree.verifyInvariants())?;
    expect_true(inc_tree.verifyInvariants())?;

    // Compare envelope intersecting, envelope contained, radius, and KNN (k=1, 5, 20, 250)
    const qrect: Rect = Rect.init(-120.0, -80.0, 160.0, 140.0) catch { expect_true(false)?; return; };
    const r_str_ov = str_tree.locateInEnvelopeIntersecting(alloc, qrect) catch { expect_true(false)?; return; };
    defer alloc.free(r_str_ov);
    const r_inc_ov = inc_tree.locateInEnvelopeIntersecting(alloc, qrect) catch { expect_true(false)?; return; };
    defer alloc.free(r_inc_ov);
    expect_eq(r_str_ov.len, r_inc_ov.len);

    const r_str_ct = str_tree.locateInEnvelopeContained(alloc, qrect) catch { expect_true(false)?; return; };
    defer alloc.free(r_str_ct);
    const r_inc_ct = inc_tree.locateInEnvelopeContained(alloc, qrect) catch { expect_true(false)?; return; };
    defer alloc.free(r_inc_ct);
    expect_eq(r_str_ct.len, r_inc_ct.len);
    expect_true(r_str_ct.len <= r_str_ov.len)?;

    const r_str_rad = str_tree.locateWithinDistance(alloc, 10.0, -20.0, 25000.0) catch { expect_true(false)?; return; };
    defer alloc.free(r_str_rad);
    const r_inc_rad = inc_tree.locateWithinDistance(alloc, 10.0, -20.0, 25000.0) catch { expect_true(false)?; return; };
    defer alloc.free(r_inc_rad);
    expect_eq(r_str_rad.len, r_inc_rad.len);

    const ks: [4]usize = [1, 5, 20, 250];
    let ki: usize = 0;
    while (ki < 4) : (ki += 1) {
        const k: usize = ks[ki];
        const nn_str = str_tree.nearestK(alloc, 25.0, -15.0, k) catch { expect_true(false)?; return; };
        defer alloc.free(nn_str);
        const nn_inc = inc_tree.nearestK(alloc, 25.0, -15.0, k) catch { expect_true(false)?; return; };
        defer alloc.free(nn_inc);
        const exp_len: usize = if (k < N) k else N;
        expect_eq(nn_str.len, exp_len);
        expect_eq(nn_inc.len, exp_len);
        let j: usize = 0;
        while (j < exp_len) : (j += 1) {
            expect_true(absF64(nn_str[j].dist_sq - nn_inc[j].dist_sq) <= 1.0e-9)?;
            if (j > 0) expect_true(nn_str[j].dist_sq >= nn_str[j - 1].dist_sq)?;
        }
    }
}

// ============================================================================
// Test 3: R*-Tree Underflow-Reinsertion remove() + Root Collapse & Re-Insert
// ============================================================================
fn runTestRemoveUnderflowReinsert(): void! {
    const alloc = std.heap.page_allocator;
    let tree = RTree<u64>.initWithCapacity(alloc, 6) catch { expect_true(false)?; return; };
    defer tree.deinit();

    // Remove on empty tree -> null
    const empty_rm: RTreeEntry<u64>? = tree.remove(999);
    expect_true(empty_rm == null)?;

    // Single-element insert & remove
    const r0: Rect = Rect.init(1.0, 2.0, 3.0, 4.0) catch { expect_true(false)?; return; };
    tree.insert(42, r0, 100) catch { expect_true(false)?; return; };
    expect_eq(tree.len(), 1);
    const rm_one: RTreeEntry<u64>? = tree.remove(42);
    expect_true(rm_one != null)?;
    expect_eq(tree.len(), 0);
    expect_true(tree.isEmpty())?;
    expect_true(tree.bounds() == null)?;
    expect_true(tree.verifyInvariants())?;

    // Insert 90 rects -> remove 75 -> verify invariants & exact survivors -> remove remaining 15
    let i: usize = 0;
    while (i < 90) : (i += 1) {
        const fx: f64 = @as(f64, @floatFromInt(i % 15)) * 10.0;
        const fy: f64 = @as(f64, @floatFromInt(i / 15)) * 10.0;
        const r: Rect = Rect.init(fx, fy, fx + 5.0, fy + 5.0) catch { expect_true(false)?; return; };
        tree.insert(@as(u64, @intCast(i + 1)), r, @as(u64, @intCast(i))) catch { expect_true(false)?; return; };
    }
    expect_true(tree.verifyInvariants())?;

    i = 0;
    while (i < 75) : (i += 1) {
        const got = tree.remove(@as(u64, @intCast(i + 1)));
        expect_true(got != null)?;
        if (i % 10 == 0) {
            expect_true(tree.verifyInvariants())?;
        }
    }
    expect_eq(tree.len(), 15);
    expect_true(tree.verifyInvariants())?;

    // Removing already-removed ID returns null
    expect_true(tree.remove(1) == null)?;

    // Remove final 15 entries down to empty tree
    while (i < 90) : (i += 1) {
        const got = tree.remove(@as(u64, @intCast(i + 1)));
        expect_true(got != null)?;
    }
    expect_eq(tree.len(), 0);
    expect_true(tree.isEmpty())?;
    expect_true(tree.verifyInvariants())?;
}

// ============================================================================
// Test 4: Degenerate Geometry & NaN/Inf/Negative Distance Boundary Guards
// ============================================================================
fn runTestDegenerateAndNumericalGuards(): void! {
    const alloc = std.heap.page_allocator;
    const nan_val: f64 = std.math.nan(f64);
    const inf_val: f64 = std.math.inf(f64);

    // NaN / Inf / inverted rect rejected
    let bad_nan: bool = false;
    * = Rect.init(nan_val, 0.0, 1.0, 1.0) catch |e| { if (e == RTreeError.InvalidBox) bad_nan = true; };
    expect_true(bad_nan)?;

    let bad_inf: bool = false;
    * = Rect.init(0.0, 0.0, inf_val, 1.0) catch |e| { if (e == RTreeError.InvalidBox) bad_inf = true; };
    expect_true(bad_inf)?;

    let bad_inv: bool = false;
    * = Rect.init(5.0, 0.0, 4.0, 1.0) catch |e| { if (e == RTreeError.InvalidBox) bad_inv = true; };
    expect_true(bad_inv)?;

    // 12 coincident zero-area points at (0,0) + extreme negative & positive coordinates
    let pts: [14]RTreeEntry<u64> = undefined;
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        const p: Rect = Rect.init(0.0, 0.0, 0.0, 0.0) catch { expect_true(false)?; return; };
        pts[i] = RTreeEntry<u64>{ id: @as(u64, @intCast(i + 1)), box_rect: p, value: @as(u64, @intCast(i)) };
    }
    pts[12] = RTreeEntry<u64>{
        id: 13,
        box_rect: Rect.init(-1_000_000.0, -1_000_000.0, -999_999.0, -999_999.0) catch { expect_true(false)?; return; },
        value: 13,
    };
    pts[13] = RTreeEntry<u64>{
        id: 14,
        box_rect: Rect.init(999_999.0, 999_999.0, 1_000_000.0, 1_000_000.0) catch { expect_true(false)?; return; },
        value: 14,
    };

    let tree = RTree<u64>.bulkLoadWithCapacity(alloc, pts[0..], 4) catch { expect_true(false)?; return; };
    defer tree.deinit();
    expect_true(tree.verifyInvariants())?;

    const at_origin = tree.locateWithinDistance(alloc, 0.0, 0.0, 0.0) catch { expect_true(false)?; return; };
    defer alloc.free(at_origin);
    expect_eq(at_origin.len, 12);

    // Negative max_dist_sq -> InvalidDistance
    let bad_dist: bool = false;
    * = tree.locateWithinDistance(alloc, 0.0, 0.0, -1.0) catch |e| {
        if (e == RTreeError.InvalidDistance) bad_dist = true;
    };
    expect_true(bad_dist)?;

    // NaN query point -> InvalidBox
    let bad_q: bool = false;
    * = tree.nearestK(alloc, nan_val, 0.0, 5) catch |e| {
        if (e == RTreeError.InvalidBox) bad_q = true;
    };
    expect_true(bad_q)?;

    // Bulk load with NaN entry -> InvalidBox
    let bad_entries: [1]RTreeEntry<u64> = [RTreeEntry<u64>{
        id: 1,
        box_rect: Rect{ min_x: nan_val, min_y: 0.0, max_x: 1.0, max_y: 1.0 },
        value: 1,
    }];
    let bad_bulk: bool = false;
    * = RTree<u64>.bulkLoad(alloc, bad_entries[0..]) catch |e| {
        if (e == RTreeError.InvalidBox) bad_bulk = true;
    };
    expect_true(bad_bulk)?;
}

// ============================================================================
// Test 5: 1,300 Deterministic PRNG Differential Ops vs Brute-Force Oracle
// ============================================================================
fn runTestDifferentialFuzz1300(): void! {
    const alloc = std.heap.page_allocator;
    let rng: XorShift64 = XorShift64{ state: 0x6A09_E667_F3BC_C908 };

    let tree = RTree<u64>.initWithCapacity(alloc, 6) catch { expect_true(false)?; return; };
    defer tree.deinit();

    let model: std.ArrayListUnmanaged(RTreeEntry<u64>) = .empty;
    defer model.deinit(alloc);

    let next_id: u64 = 1;
    let ids_tree: [256]u64 = [0; 256];
    let ids_ref: [256]u64 = [0; 256];
    let dist_ref: [256]f64 = [0.0; 256];

    let step: usize = 0;
    while (step < 1300) : (step += 1) {
        const op: usize = rng.range(100);

        if (op < 30 && model.items.len < 160) {
            // Insert random rect (including degenerate points)
            const x0: f64 = rng.nextF64(-200.0, 200.0);
            const y0: f64 = rng.nextF64(-200.0, 200.0);
            const w: f64 = if (rng.range(4) == 0) 0.0 else rng.nextF64(0.0, 40.0);
            const h: f64 = if (rng.range(4) == 0) 0.0 else rng.nextF64(0.0, 40.0);
            const r: Rect = Rect.init(x0, y0, x0 + w, y0 + h) catch { expect_true(false)?; return; };
            const e: RTreeEntry<u64> = RTreeEntry<u64>{ id: next_id, box_rect: r, value: next_id * 3 };
            next_id += 1;
            tree.insert(e.id, e.box_rect, e.value) catch { expect_true(false)?; return; };
            model.append(alloc, e) catch { expect_true(false)?; return; };
            expect_eq(tree.len(), model.items.len);
        } else if (op < 45 && model.items.len > 0) {
            // Remove random existing entry via R*-Tree CondenseTree remove()
            const pick: usize = rng.range(model.items.len);
            const target_id: u64 = model.items[pick].id;
            const rm = tree.remove(target_id);
            expect_true(rm != null)?;
            * = model.swapRemove(pick);
            expect_eq(tree.len(), model.items.len);
        } else if (op < 50) {
            // Rebuild via STR bulkLoadWithCapacity and verify invariants
            let rebuilt = RTree<u64>.bulkLoadWithCapacity(alloc, model.items, 6) catch { expect_true(false)?; return; };
            tree.deinit();
            tree = rebuilt;
            expect_eq(tree.len(), model.items.len);
            expect_true(tree.verifyInvariants())?;
        } else if (op < 68) {
            // Differential locateInEnvelopeIntersecting
            const qx: f64 = rng.nextF64(-220.0, 180.0);
            const qy: f64 = rng.nextF64(-220.0, 180.0);
            const qr: Rect = Rect.init(qx, qy, qx + rng.nextF64(0.0, 120.0), qy + rng.nextF64(0.0, 120.0)) catch { expect_true(false)?; return; };
            const res = tree.locateInEnvelopeIntersecting(alloc, qr) catch { expect_true(false)?; return; };
            defer alloc.free(res);

            let ref_n: usize = 0;
            for (me of model.items) {
                if (me.box_rect.overlaps(qr)) {
                    ids_ref[ref_n] = me.id;
                    ref_n += 1;
                }
            }
            expect_eq(res.len, ref_n);
            let i: usize = 0;
            while (i < res.len) : (i += 1) ids_tree[i] = res[i].id;
            sortU64(ids_tree[0..res.len]);
            sortU64(ids_ref[0..ref_n]);
            i = 0;
            while (i < ref_n) : (i += 1) expect_eq(ids_tree[i], ids_ref[i]);
        } else if (op < 80) {
            // Differential locateInEnvelopeContained
            const qx: f64 = rng.nextF64(-220.0, 150.0);
            const qy: f64 = rng.nextF64(-220.0, 150.0);
            const qr: Rect = Rect.init(qx, qy, qx + rng.nextF64(10.0, 180.0), qy + rng.nextF64(10.0, 180.0)) catch { expect_true(false)?; return; };
            const res = tree.locateInEnvelopeContained(alloc, qr) catch { expect_true(false)?; return; };
            defer alloc.free(res);

            let ref_n: usize = 0;
            for (me of model.items) {
                if (qr.containsRect(me.box_rect)) {
                    ids_ref[ref_n] = me.id;
                    ref_n += 1;
                }
            }
            expect_eq(res.len, ref_n);
            let i: usize = 0;
            while (i < res.len) : (i += 1) ids_tree[i] = res[i].id;
            sortU64(ids_tree[0..res.len]);
            sortU64(ids_ref[0..ref_n]);
            i = 0;
            while (i < ref_n) : (i += 1) expect_eq(ids_tree[i], ids_ref[i]);
        } else if (op < 90) {
            // Differential locateWithinDistance
            const qx: f64 = rng.nextF64(-200.0, 200.0);
            const qy: f64 = rng.nextF64(-200.0, 200.0);
            const r2: f64 = rng.nextF64(0.0, 15000.0);
            const res = tree.locateWithinDistance(alloc, qx, qy, r2) catch { expect_true(false)?; return; };
            defer alloc.free(res);

            let ref_n: usize = 0;
            for (me of model.items) {
                if (me.box_rect.distanceSqToPoint(qx, qy) <= r2) {
                    ids_ref[ref_n] = me.id;
                    ref_n += 1;
                }
            }
            expect_eq(res.len, ref_n);
            let i: usize = 0;
            while (i < res.len) : (i += 1) ids_tree[i] = res[i].entry.id;
            sortU64(ids_tree[0..res.len]);
            sortU64(ids_ref[0..ref_n]);
            i = 0;
            while (i < ref_n) : (i += 1) expect_eq(ids_tree[i], ids_ref[i]);
        } else {
            // Differential nearestK (k = 1, 5, 20, or > len)
            const qx: f64 = rng.nextF64(-200.0, 200.0);
            const qy: f64 = rng.nextF64(-200.0, 200.0);
            const k_choices: [4]usize = [1, 5, 20, 200];
            const k: usize = k_choices[rng.range(4)];
            const res = tree.nearestK(alloc, qx, qy, k) catch { expect_true(false)?; return; };
            defer alloc.free(res);

            let ref_n: usize = 0;
            for (me of model.items) {
                dist_ref[ref_n] = me.box_rect.distanceSqToPoint(qx, qy);
                ref_n += 1;
            }
            sortF64(dist_ref[0..ref_n]);
            const exp_k: usize = if (k < ref_n) k else ref_n;
            expect_eq(res.len, exp_k);
            let i: usize = 0;
            while (i < exp_k) : (i += 1) {
                expect_true(absF64(res[i].dist_sq - dist_ref[i]) <= 1.0e-9)?;
            }
        }
    }
    expect_true(tree.verifyInvariants())?;
}

fn main() {
    test("RTree SIMD: 4-lane @Vector(4,f32) + @mulAdd batch distance & overlap vs scalar", () => {
        runTestSimdBatchKernels()?;
    });
    test("RTree STR: bulkLoad vs incremental insert, invariants & multi-query parity", () => {
        runTestStrBulkLoadParity()?;
    });
    test("RTree Remove: CondenseTree underflow-reinsertion, single-element & root collapse", () => {
        runTestRemoveUnderflowReinsert()?;
    });
    test("RTree Guards: degenerate points, coincident coords, extreme values & NaN/Inf rejection", () => {
        runTestDegenerateAndNumericalGuards()?;
    });
    test("RTree Differential Fuzz: 1,300 PRNG spatial ops vs brute-force oracle", () => {
        runTestDifferentialFuzz1300()?;
    });
}
