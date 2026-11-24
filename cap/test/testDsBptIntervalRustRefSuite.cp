// rust-lang/rust BTree & rust-bio IntervalTree (MIT OR Apache-2.0) Reference-Hardened Test Suite
// Covers:
//   - B+Tree CRUD, duplicate updates, multi-level splits, borrow/merge/root-collapse, rangeScan, verifyInvariants
//   - IntervalTree low>high strict rejection, empty tree, point/range queries, max_high invariant, brute-force diff
//   - 4-lane @Vector(4, i64) SIMD vs scalar 0-discrepancy parity across lengths 0..129
//   - 1,800-iteration deterministic PRNG differential fuzzing (900 B+Tree + 900 IntervalTree)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BPlusTree,
    scalarBptCountLessI64,
    simdBptCountLessI64,
} from "../../lib/ds/bptree.cp";
import {
    IntervalTree,
    scalarIntervalCountOverlaps,
    simdIntervalCountOverlaps,
} from "../../lib/ds/intervalTree.cp";

fn xorshift64(state: *u64): u64 {
    let x: u64 = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn nextRangeI64(state: *u64, lo: i64, span: u64): i64 {
    const off: i64 = @intCast(xorshift64(state) % span);
    return lo + off;
}

fn checkItInsertErr(tree: *IntervalTree(i64, i64), lo: i64, hi: i64, val: i64): bool {
    tree.insert(lo, hi, val) catch return true;
    return false;
}

fn checkItQueryErr(tree: IntervalTree(i64, i64), alloc: std.mem.Allocator, lo: i64, hi: i64): bool {
    const res = tree.queryOverlap(alloc, lo, hi) catch return true;
    alloc.free(res);
    return false;
}

fn checkItHasErr(tree: IntervalTree(i64, i64), lo: i64, hi: i64): bool {
    const r = tree.hasOverlap(lo, hi) catch return true;
    return if (r) false else false;
}

fn checkItCountErr(tree: IntervalTree(i64, i64), lo: i64, hi: i64): bool {
    const r = tree.countOverlaps(lo, hi) catch return true;
    return if (r == 999999) true else false;
}

fn runTest01BptCrudSplitsMerges(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    let tree = BPlusTree(i64, i64).init(alloc);
    defer tree.deinit();

    // Empty tree guards
    if (tree.len() != 0 or !tree.isEmpty()) return false;
    if (tree.get(42) != null or tree.remove(42) != null) return false;
    if (!tree.verifyInvariants()) return false;
    const empty_scan = tree.rangeScan(alloc, -10, 10) catch return false;
    defer alloc.free(empty_scan);
    if (empty_scan.len != 0) return false;

    // Insert 200 keys in scrambled order to exercise multi-level splits
    let i: usize = 0;
    while (i < 200) : (i += 1) {
        const k: i64 = @as(i64, @intCast((i * 73) % 200)) - 100;
        const prev = tree.insert(k, k * 10) catch return false;
        if (prev != null) return false;
    }
    if (tree.len() != 200 or tree.isEmpty() or !tree.verifyInvariants()) return false;

    // Duplicate key update returns old value and preserves count
    const old_v = tree.insert(0, 9999) catch return false;
    if (old_v == null or old_v.? != 0) return false;
    if (tree.len() != 200 or tree.get(0).? != 9999) return false;
    const restored = tree.insert(0, 0) catch return false;
    if (restored == null or restored.? != 9999) return false;
    if (!tree.verifyInvariants()) return false;

    // Inverted rangeScan returns empty safely
    const inv_scan = tree.rangeScan(alloc, 50, -50) catch return false;
    defer alloc.free(inv_scan);
    if (inv_scan.len != 0) return false;

    // Range scan [-25, 25] -> exact 51 entries in strict ascending order
    const mid_scan = tree.rangeScan(alloc, -25, 25) catch return false;
    defer alloc.free(mid_scan);
    if (mid_scan.len != 51) return false;
    let idx: usize = 0;
    while (idx < mid_scan.len) : (idx += 1) {
        const exp_k: i64 = -25 + @as(i64, @intCast(idx));
        if (mid_scan[idx].key != exp_k or mid_scan[idx].value != exp_k * 10) return false;
    }

    // Removing missing keys safely returns null
    if (tree.remove(-999) != null or tree.remove(999) != null or tree.len() != 200) return false;

    // Remove 180 keys (triggering borrowFromLeft, borrowFromRight, mergeChildren)
    let r: usize = 0;
    while (r < 180) : (r += 1) {
        const k: i64 = @as(i64, @intCast((r * 37) % 200)) - 100;
        const rem = tree.remove(k);
        if (rem == null or rem.? != k * 10) return false;
        if ((r % 15) == 0 and !tree.verifyInvariants()) return false;
    }
    if (tree.len() != 20 or !tree.verifyInvariants()) return false;

    // Remove remaining 20 keys down to empty tree
    while (r < 200) : (r += 1) {
        const k: i64 = @as(i64, @intCast((r * 37) % 200)) - 100;
        const rem = tree.remove(k);
        if (rem == null or rem.? != k * 10) return false;
    }
    return tree.len() == 0 and tree.isEmpty() and tree.verifyInvariants();
}

fn runTest02IntervalTreeBoundsAndInvariants(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    let tree = IntervalTree(i64, i64).init(alloc);
    defer tree.deinit();

    // Empty tree safety
    if (tree.len() != 0 or !tree.isEmpty() or !tree.verifyInvariants()) return false;
    if (tree.hasOverlap(0, 10) catch return false) return false;
    if ((tree.countOverlaps(0, 10) catch return false) != 0) return false;
    const e_res = tree.queryPoint(alloc, 5) catch return false;
    defer alloc.free(e_res);
    if (e_res.len != 0) return false;

    // Strict low > high rejection on all entry points
    if (!checkItInsertErr(*tree, 10, 5, 1)) return false;
    if (!checkItQueryErr(tree, alloc, 10, 5)) return false;
    if (!checkItHasErr(tree, 10, 5)) return false;
    if (!checkItCountErr(tree, 10, 5)) return false;
    if (tree.len() != 0) return false;

    // Insert intervals including negative, point, nested, touching, duplicate
    const lows: [8]i64 = [10, -20, 5, 20, 15, -5, 20, 100];
    const highs: [8]i64 = [20, -10, 25, 20, 30, 5, 35, 200];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        tree.insert(lows[i], highs[i], @as(i64, @intCast(i))) catch return false;
        if (!tree.verifyInvariants()) return false;
    }
    if (tree.len() != 8) return false;

    // Point query at 20: overlaps [10,20], [5,25], [20,20], [15,30], [20,35] => 5
    const p20 = tree.queryPoint(alloc, 20) catch return false;
    defer alloc.free(p20);
    if (p20.len != 5) return false;
    if ((tree.countOverlaps(20, 20) catch return false) != 5) return false;
    if (!(tree.hasOverlap(20, 20) catch return false)) return false;

    // Gap query [50, 99]: 0 overlaps
    const gap = tree.queryOverlap(alloc, 50, 99) catch return false;
    defer alloc.free(gap);
    if (gap.len != 0) return false;
    if ((tree.countOverlaps(50, 99) catch return false) != 0) return false;
    if (tree.hasOverlap(50, 99) catch return false) return false;

    // Touching endpoint query [-10, -5]: overlaps [-20,-10] and [-5,5] => 2
    const touch = tree.queryOverlap(alloc, -10, -5) catch return false;
    defer alloc.free(touch);
    if (touch.len != 2) return false;
    if ((tree.countOverlaps(-10, -5) catch return false) != 2) return false;
    return true;
}

fn runTest03Simd4I64ZeroDiscrepancy(): bool {
    const lens: [14]usize = [0, 1, 2, 3, 4, 5, 7, 8, 15, 16, 17, 31, 65, 129];
    let keys: [129]i64 = [0; 129];
    let lows: [129]i64 = [0; 129];
    let highs: [129]i64 = [0; 129];
    let rng: u64 = 0x4B71_51AD_0000_0042;
    let idx: usize = 0;
    while (idx < 129) : (idx += 1) {
        keys[idx] = -500 + @as(i64, @intCast(idx * 9));
        const a = nextRangeI64(*rng, -200, 400);
        const b = nextRangeI64(*rng, -200, 400);
        if ((idx % 19) == 0) {
            lows[idx] = if (a > b) a else b + 1;
            highs[idx] = if (a > b) b else a;
        } else {
            lows[idx] = if (a <= b) a else b;
            highs[idx] = if (a <= b) b else a;
        }
    }

    const targets: [5]i64 = [-1000, -200, 0, 250, 2000];
    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        let ti: usize = 0;
        while (ti < targets.len) : (ti += 1) {
            const t = targets[ti];
            if (simdBptCountLessI64(keys[0..n], t) != scalarBptCountLessI64(keys[0..n], t)) return false;
            if (simdIntervalCountOverlaps(lows[0..n], highs[0..n], t - 40, t + 40) !=
                scalarIntervalCountOverlaps(lows[0..n], highs[0..n], t - 40, t + 40))
            {
                return false;
            }
        }
        if (simdIntervalCountOverlaps(lows[0..n], highs[0..n], 50, -50) != 0) return false;
    }
    return simdIntervalCountOverlaps(lows[0..5], highs[0..4], -100, 100) == 0;
}

fn runTest04BptFuzz900(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    let tree = BPlusTree(i64, i64).init(alloc);
    defer tree.deinit();

    let ref_present: [128]bool = [false; 128];
    let ref_val: [128]i64 = [0; 128];
    let ref_count: usize = 0;
    let rng: u64 = 0x1B97_F022_0000_0900;

    let iter: usize = 0;
    while (iter < 900) : (iter += 1) {
        const op = xorshift64(*rng) % 10;
        const slot: usize = @intCast(xorshift64(*rng) % 128);
        const k: i64 = @as(i64, @intCast(slot)) - 64;

        if (op < 5) {
            const v: i64 = nextRangeI64(*rng, -10000, 20000);
            const got_old = tree.insert(k, v) catch return false;
            if (ref_present[slot]) {
                if (got_old == null or got_old.? != ref_val[slot]) return false;
            } else {
                if (got_old != null) return false;
                ref_present[slot] = true;
                ref_count += 1;
            }
            ref_val[slot] = v;
        } else if (op < 8) {
            const got_rem = tree.remove(k);
            if (ref_present[slot]) {
                if (got_rem == null or got_rem.? != ref_val[slot]) return false;
                ref_present[slot] = false;
                ref_count -= 1;
            } else {
                if (got_rem != null) return false;
            }
        } else {
            const got_v = tree.get(k);
            if (ref_present[slot]) {
                if (got_v == null or got_v.? != ref_val[slot]) return false;
            } else {
                if (got_v != null) return false;
            }

            const a = nextRangeI64(*rng, -70, 140);
            const b = nextRangeI64(*rng, -70, 140);
            const scan = tree.rangeScan(alloc, a, b) catch return false;
            defer alloc.free(scan);

            let exp_idx: usize = 0;
            if (a <= b) {
                let s: usize = 0;
                while (s < 128) : (s += 1) {
                    const sk: i64 = @as(i64, @intCast(s)) - 64;
                    if (ref_present[s] and sk >= a and sk <= b) {
                        if (exp_idx >= scan.len) return false;
                        if (scan[exp_idx].key != sk or scan[exp_idx].value != ref_val[s]) return false;
                        exp_idx += 1;
                    }
                }
            }
            if (scan.len != exp_idx) return false;
        }

        if (tree.len() != ref_count) return false;
        if ((iter % 15) == 0 and !tree.verifyInvariants()) return false;
    }
    return tree.verifyInvariants();
}

fn runTest05IntervalFuzz900(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    let tree = IntervalTree(i64, i64).init(alloc);
    defer tree.deinit();

    let ref_lows: [300]i64 = [0; 300];
    let ref_highs: [300]i64 = [0; 300];
    let ref_len: usize = 0;
    let rng: u64 = 0x217E_F022_0000_0900;

    let iter: usize = 0;
    while (iter < 900) : (iter += 1) {
        const do_insert = (ref_len < 300) and ((iter % 3) == 0);
        if (do_insert) {
            const a = nextRangeI64(*rng, -500, 1000);
            const b = nextRangeI64(*rng, -500, 1000);
            if ((iter % 33) == 0 and a != b) {
                const hi_bad = if (a < b) a else b;
                const lo_bad = if (a < b) b else a;
                if (!checkItInsertErr(*tree, lo_bad, hi_bad, @as(i64, @intCast(iter)))) return false;
            } else {
                const lo = if (a <= b) a else b;
                const hi = if (a <= b) b else a;
                tree.insert(lo, hi, @as(i64, @intCast(ref_len))) catch return false;
                ref_lows[ref_len] = lo;
                ref_highs[ref_len] = hi;
                ref_len += 1;
            }
        }

        const qa = nextRangeI64(*rng, -550, 1100);
        const qb = nextRangeI64(*rng, -550, 1100);
        const qlo = if (qa <= qb) qa else qb;
        const qhi = if (qa <= qb) qb else qa;

        const ref_cnt = scalarIntervalCountOverlaps(ref_lows[0..ref_len], ref_highs[0..ref_len], qlo, qhi);
        const simd_cnt = simdIntervalCountOverlaps(ref_lows[0..ref_len], ref_highs[0..ref_len], qlo, qhi);
        if (simd_cnt != ref_cnt) return false;

        const tree_cnt = tree.countOverlaps(qlo, qhi) catch return false;
        if (tree_cnt != ref_cnt) return false;
        const has_ov = tree.hasOverlap(qlo, qhi) catch return false;
        if (has_ov != (ref_cnt > 0)) return false;

        if ((iter % 5) == 0) {
            const hits = tree.queryOverlap(alloc, qlo, qhi) catch return false;
            defer alloc.free(hits);
            if (hits.len != ref_cnt) return false;

            const pt_ref = scalarIntervalCountOverlaps(ref_lows[0..ref_len], ref_highs[0..ref_len], qlo, qlo);
            const pt_hits = tree.queryPoint(alloc, qlo) catch return false;
            defer alloc.free(pt_hits);
            if (pt_hits.len != pt_ref) return false;
        }

        if (tree.len() != ref_len) return false;
        if ((iter % 20) == 0 and !tree.verifyInvariants()) return false;
    }
    return tree.verifyInvariants();
}

fn main() {
    test("ds_ref_01_bptree_crud_splits_merges_rangescan_invariants", () => {
        expect_eq(runTest01BptCrudSplitsMerges(), true);
    });

    test("ds_ref_02_interval_tree_bounds_overlaps_point_invariants", () => {
        expect_eq(runTest02IntervalTreeBoundsAndInvariants(), true);
    });

    test("ds_ref_03_simd4_i64_zero_discrepancy_parity", () => {
        expect_eq(runTest03Simd4I64ZeroDiscrepancy(), true);
    });

    test("ds_ref_04_bptree_prng_fuzz_900_iters_differential", () => {
        expect_eq(runTest04BptFuzz900(), true);
    });

    test("ds_ref_05_interval_tree_prng_fuzz_900_iters_differential", () => {
        expect_eq(runTest05IntervalFuzz900(), true);
    });
}
