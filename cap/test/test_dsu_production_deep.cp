// Source: cp/cap/test/test_dsu_production_deep.cp
// Deep Production Verification Suite for cap:dsu & cap.dsu
// Covers:
//   1. DisjointSet large-scale connectivity, path compression, componentCount/componentSize/groups & reset
//   2. WeightedDsu transitive difference inference diff(a, c) & ConflictConstraint rejection
//   3. RollbackDsu multi-level snapshot()/rollbackTo()/undo() state recovery & NothingToRollback
//   4. IndexOutOfBounds rejection & n=0 empty-set boundaries across all 3 DSU variants
//   5. `import cap from "cap"; cap.dsu` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DsuError,
    DisjointSet,
    WeightedDsu,
    RollbackDsu,
} from "cap:ds";

test("cap:dsu - DisjointSet large-scale merges, path compression, groups & reset") {
    const alloc = std.testing.allocator;
    const n: usize = 2000;
    let dsu = DisjointSet.init(alloc, n) catch unreachable;
    defer dsu.deinit();

    expect_eq(dsu.componentCount(), n);
    expect_eq(dsu.componentSize(0) catch unreachable, 1);

    // Chain-merge even numbers and odd numbers separately to build deep trees before compression
    let i: usize = 2;
    while (i < n) {
        expect_eq(dsu.unite(i - 2, i) catch unreachable, true);
        i += 2;
    }
    i = 3;
    while (i < n) {
        expect_eq(dsu.unite(i - 2, i) catch unreachable, true);
        i += 2;
    }

    // Duplicate merge must return false and keep componentCount unchanged
    expect_eq(dsu.unite(0, 1998) catch unreachable, false);
    expect_eq(dsu.componentCount(), 2);
    expect_eq(dsu.componentSize(0) catch unreachable, 1000);
    expect_eq(dsu.componentSize(1) catch unreachable, 1000);
    expect_eq(dsu.connected(0, 1998) catch unreachable, true);
    expect_eq(dsu.connected(1, 1999) catch unreachable, true);
    expect_eq(dsu.connected(0, 1) catch unreachable, false);

    // Path compression verification: after find(1998), parent[1998] points directly to root
    const r_even = dsu.find(1998) catch unreachable;
    expect_eq(dsu.parent[1998], r_even);

    // Groups partition verification
    const grps = dsu.groups(alloc) catch unreachable;
    defer {
        for (g of grps) {
            alloc.free(g);
        }
        alloc.free(grps);
    }
    expect_eq(grps.len, 2);
    expect_eq(grps[0].len, 1000);
    expect_eq(grps[1].len, 1000);

    // Reset restores all n isolated singletons
    dsu.reset();
    expect_eq(dsu.componentCount(), n);
    expect_eq(dsu.connected(0, 2) catch unreachable, false);
    expect_eq(dsu.componentSize(0) catch unreachable, 1);
}

test("cap:dsu - WeightedDsu transitive difference inference & ConflictConstraint rejection") {
    const alloc = std.testing.allocator;
    let wdsu = WeightedDsu.init(alloc, 6) catch unreachable;
    defer wdsu.deinit();

    // Unconnected diff must return null
    expect_eq((wdsu.diff(0, 1) catch unreachable) == null, true);

    // val[0] - val[1] = 10, val[1] - val[2] = -3 => val[0] - val[2] = 7
    expect_eq(wdsu.relate(0, 1, 10) catch unreachable, true);
    expect_eq(wdsu.relate(1, 2, -3) catch unreachable, true);
    expect_eq((wdsu.diff(0, 2) catch unreachable).?, 7);
    expect_eq((wdsu.diff(2, 0) catch unreachable).?, -7);

    // Build second component: val[3] - val[4] = 5, val[4] - val[5] = 2 => val[3] - val[5] = 7
    expect_eq(wdsu.relate(3, 4, 5) catch unreachable, true);
    expect_eq(wdsu.relate(4, 5, 2) catch unreachable, true);
    expect_eq(wdsu.componentCount(), 2);

    // Merge two components via val[2] - val[3] = 4 => val[0] - val[5] = 7 + 4 + 7 = 18
    expect_eq(wdsu.relate(2, 3, 4) catch unreachable, true);
    expect_eq(wdsu.componentCount(), 1);
    expect_eq((wdsu.diff(0, 5) catch unreachable).?, 18);

    // Consistent redundant constraint returns false
    expect_eq(wdsu.relate(0, 5, 18) catch unreachable, false);

    // Contradictory constraint must return ConflictConstraint and preserve state
    let conflict_caught = false;
    wdsu.relate(0, 5, 19) catch |e| {
        if (e == DsuError.ConflictConstraint) conflict_caught = true;
    };
    expect_eq(conflict_caught, true);
    expect_eq((wdsu.diff(0, 5) catch unreachable).?, 18);
}

test("cap:dsu - RollbackDsu multi-level snapshot, rollbackTo, undo & NothingToRollback") {
    const alloc = std.testing.allocator;
    let rdsu = RollbackDsu.init(alloc, 5) catch unreachable;
    defer rdsu.deinit();

    // Empty history undo returns NothingToRollback
    let empty_undo_caught = false;
    rdsu.undo() catch |e| {
        if (e == DsuError.NothingToRollback) empty_undo_caught = true;
    };
    expect_eq(empty_undo_caught, true);

    const s0 = rdsu.snapshot();
    expect_eq(rdsu.unite(0, 1) catch unreachable, true);
    expect_eq(rdsu.unite(1, 2) catch unreachable, true);
    // Redundant unite should not push to history
    expect_eq(rdsu.unite(0, 2) catch unreachable, false);
    expect_eq(rdsu.componentCount(), 3);

    const s1 = rdsu.snapshot();
    expect_eq(rdsu.unite(3, 4) catch unreachable, true);
    expect_eq(rdsu.unite(0, 4) catch unreachable, true);
    expect_eq(rdsu.componentCount(), 1);
    expect_eq(rdsu.connected(2, 4) catch unreachable, true);

    // Roll back to s1 -> (3,4) and (0,4) undone, (0,1,2) remains connected
    rdsu.rollbackTo(s1);
    expect_eq(rdsu.componentCount(), 3);
    expect_eq(rdsu.connected(0, 2) catch unreachable, true);
    expect_eq(rdsu.connected(3, 4) catch unreachable, false);
    expect_eq(rdsu.connected(0, 4) catch unreachable, false);

    // Single undo -> (1,2) undone, only (0,1) remains
    rdsu.undo() catch unreachable;
    expect_eq(rdsu.componentCount(), 4);
    expect_eq(rdsu.connected(0, 1) catch unreachable, true);
    expect_eq(rdsu.connected(0, 2) catch unreachable, false);

    // Roll back to s0 -> all isolated
    rdsu.rollbackTo(s0);
    expect_eq(rdsu.componentCount(), 5);
    expect_eq(rdsu.connected(0, 1) catch unreachable, false);
}

test("cap:dsu - IndexOutOfBounds rejection & n=0 empty-set boundaries") {
    const alloc = std.testing.allocator;

    // Empty set n = 0 across all 3 structures
    let empty_dsu = DisjointSet.init(alloc, 0) catch unreachable;
    defer empty_dsu.deinit();
    expect_eq(empty_dsu.componentCount(), 0);
    const empty_grps = empty_dsu.groups(alloc) catch unreachable;
    expect_eq(empty_grps.len, 0);

    let oob_dsu = false;
    * = empty_dsu.find(0) catch |e| {
        if (e == DsuError.IndexOutOfBounds) oob_dsu = true;
    };
    expect_eq(oob_dsu, true);

    let empty_wdsu = WeightedDsu.init(alloc, 0) catch unreachable;
    defer empty_wdsu.deinit();
    expect_eq(empty_wdsu.componentCount(), 0);
    let oob_wdsu = false;
    * = empty_wdsu.diff(0, 0) catch |e| {
        if (e == DsuError.IndexOutOfBounds) oob_wdsu = true;
    };
    expect_eq(oob_wdsu, true);

    let empty_rdsu = RollbackDsu.init(alloc, 0) catch unreachable;
    defer empty_rdsu.deinit();
    expect_eq(empty_rdsu.componentCount(), 0);
    let empty_undo = false;
    empty_rdsu.undo() catch |e| {
        if (e == DsuError.NothingToRollback) empty_undo = true;
    };
    expect_eq(empty_undo, true);

    // Non-empty out-of-bounds checks
    let dsu3 = DisjointSet.init(alloc, 3) catch unreachable;
    defer dsu3.deinit();
    let oob_unite = false;
    * = dsu3.unite(1, 3) catch |e| {
        if (e == DsuError.IndexOutOfBounds) oob_unite = true;
    };
    expect_eq(oob_unite, true);
}

test("cap.dsu - dynamic cap.dsu namespace mounting & std.testing.allocator zero leaks") {
    const alloc = std.testing.allocator;

    let dsu = cap.ds.DisjointSet.init(alloc, 4) catch unreachable;
    defer dsu.deinit();
    expect_eq(dsu.unite(0, 3) catch unreachable, true);
    expect_eq(dsu.connected(0, 3) catch unreachable, true);

    let wdsu = cap.ds.WeightedDsu.init(alloc, 3) catch unreachable;
    defer wdsu.deinit();
    expect_eq(wdsu.relate(0, 2, 42) catch unreachable, true);
    expect_eq((wdsu.diff(0, 2) catch unreachable).?, 42);

    let rdsu = cap.ds.RollbackDsu.init(alloc, 3) catch unreachable;
    defer rdsu.deinit();
    const snap = rdsu.snapshot();
    expect_eq(rdsu.unite(1, 2) catch unreachable, true);
    expect_eq(rdsu.componentCount(), 2);
    rdsu.rollbackTo(snap);
    expect_eq(rdsu.componentCount(), 3);
}
