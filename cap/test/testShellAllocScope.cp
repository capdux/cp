// cap/test/testShellAllocScope.cp
// Production-grade deep tests for cap:shell allocScope.cp (Bun shell/AllocScope tracker)

import { test, assert, assertEq } from "cap:test";
import {
    ALLOC_SCOPE_MAX_ENTRIES,
    AllocScopeError,
    AllocScopeReport,
    AllocScope,
} from "cap:shell";

test("AllocScope: clean alloc/free lifecycle & peak bytes tracking", () => {
    let scope = AllocScope.beginScope(true);
    assert(scope.isEnabled());
    assertEq(scope.liveCount(), @as(usize, 0));
    assertEq(scope.liveBytes(), @as(usize, 0));

    assert(scope.trackAlloc(101, 256, 1) catch false);
    assert(scope.trackAlloc(102, 512, 2) catch false);
    assertEq(scope.liveCount(), @as(usize, 2));
    assertEq(scope.liveBytes(), @as(usize, 768));
    assertEq(scope.peakBytes(), @as(usize, 768));

    assert(scope.assertInScope(101) catch false);
    assert(scope.isOwned(102));

    assertEq(scope.trackFree(101) catch 0, @as(usize, 256));
    assertEq(scope.liveCount(), @as(usize, 1));
    assertEq(scope.liveBytes(), @as(usize, 512));
    assertEq(scope.peakBytes(), @as(usize, 768));

    assertEq(scope.trackFree(102) catch 0, @as(usize, 512));
    const rep = scope.endScope() catch AllocScopeReport{ leaked_count: 99 };
    assert(rep.isClean());
    assertEq(rep.total_allocs, @as(usize, 2));
    assertEq(rep.total_frees, @as(usize, 2));
    assertEq(rep.external_transfers, @as(usize, 0));
    assertEq(rep.peak_bytes, @as(usize, 768));
});

test("AllocScope: leakSlice external ownership transfer & assertInScope", () => {
    let scope = AllocScope.beginScope(true);
    assert(scope.trackAlloc(201, 128, 7) catch false);
    assert(scope.trackAlloc(202, 64, 8) catch false);

    // Transfer 201 to external owner
    assertEq(scope.leakSlice(201) catch 0, @as(usize, 128));
    assert(!scope.isOwned(201));

    // assertInScope fails for transferred allocation
    let got_not_owned = false;
    * = scope.assertInScope(201) catch |e| {
        if (e == AllocScopeError.NotOwnedByScope) {
            got_not_owned = true;
        }
        false
    };
    assert(got_not_owned);

    assertEq(scope.trackFree(202) catch 0, @as(usize, 64));
    const rep = scope.endScope() catch AllocScopeReport{ leaked_count: 99 };
    assert(rep.isClean());
    assertEq(rep.external_transfers, @as(usize, 1));
    assertEq(rep.total_frees, @as(usize, 1));
});

test("AllocScope: endScope detects unfreed leaks accurately", () => {
    let scope = AllocScope.beginScope(true);
    assert(scope.trackAlloc(301, 100, 1) catch false);
    assert(scope.trackAlloc(302, 250, 1) catch false);
    assertEq(scope.trackFree(301) catch 0, @as(usize, 100));

    const snap = scope.report();
    assert(!snap.isClean());
    assertEq(snap.leaked_count, @as(usize, 1));
    assertEq(snap.leaked_bytes, @as(usize, 250));

    let got_leaks = false;
    * = scope.endScope() catch |e| {
        if (e == AllocScopeError.LeaksDetected) {
            got_leaks = true;
        }
        AllocScopeReport{}
    };
    assert(got_leaks);
});

test("AllocScope: InvalidAllocation / DuplicateAllocation / DoubleFree guards", () => {
    let scope = AllocScope.beginScope(true);

    let got_invalid = false;
    * = scope.trackAlloc(0, 16, 0) catch |e| {
        if (e == AllocScopeError.InvalidAllocation) {
            got_invalid = true;
        }
        false
    };
    assert(got_invalid);

    assert(scope.trackAlloc(401, 32, 0) catch false);
    let got_dup = false;
    * = scope.trackAlloc(401, 64, 0) catch |e| {
        if (e == AllocScopeError.DuplicateAllocation) {
            got_dup = true;
        }
        false
    };
    assert(got_dup);

    assertEq(scope.trackFree(401) catch 0, @as(usize, 32));
    let got_double_free = false;
    * = scope.trackFree(401) catch |e| {
        if (e == AllocScopeError.DoubleFree) {
            got_double_free = true;
        }
        0
    };
    assert(got_double_free);
});

test("AllocScope: ScopeTableFull limit & slot reuse after free", () => {
    let scope = AllocScope.beginScope(true);
    let i: usize = 0;
    while (i < ALLOC_SCOPE_MAX_ENTRIES) {
        assert(scope.trackAlloc(@as(u64, i + 1), 16, 0) catch false);
        i += 1;
    }
    assertEq(scope.liveCount(), ALLOC_SCOPE_MAX_ENTRIES);

    let got_full = false;
    * = scope.trackAlloc(9999, 16, 0) catch |e| {
        if (e == AllocScopeError.ScopeTableFull) {
            got_full = true;
        }
        false
    };
    assert(got_full);

    assertEq(scope.trackFree(10) catch 0, @as(usize, 16));
    assert(scope.trackAlloc(9999, 32, 0) catch false);
    assertEq(scope.liveCount(), ALLOC_SCOPE_MAX_ENTRIES);
});

test("AllocScope: disabled release mode is zero-cost no-op", () => {
    let scope = AllocScope.beginScope(false);
    assert(!scope.isEnabled());
    assert(!(scope.trackAlloc(501, 1024, 1) catch true));
    assert(scope.isOwned(501));
    assert(scope.assertInScope(501) catch false);
    assertEq(scope.trackFree(501) catch 99, @as(usize, 0));
    assertEq(scope.leakSlice(501) catch 99, @as(usize, 0));
    const rep = scope.endScope() catch AllocScopeReport{ leaked_count: 1 };
    assert(rep.isClean());
    assertEq(rep.total_allocs, @as(usize, 0));
});
