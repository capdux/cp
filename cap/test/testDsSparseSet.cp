// Origin: Pure-CP Stdlib Test Suite (lib/ds/sparseSet.cp) | Created: 2026-11-23
// Production Deep Verification Suite for O(1) SparseSet & ECS Dense SparseMapU64:
//   1. O(1) insert, idempotent duplicate rejection, universe boundary guards & telemetry stats
//   2. O(1) swap-remove preserving tightly packed `denseSlice()` with zero holes
//   3. O(1) zero-touch `clear()` immunity to stale `sparse` slots across subsequent re-insertions
//   4. Set algebra: `intersectInto`, `unionInto`, `differenceInto`, and `isSubsetOf`
//   5. `SparseMapU64`: `put`, overwrite, `get`, `contains` & out-of-range guards
//   6. `SparseMapU64`: swap-remove maintaining strict 1-to-1 alignment between `denseIds()` and `denseValues()`
//   7. `cap.ds` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SPARSE_SET_UNIVERSE,
    SPARSE_SET_INVALID_IDX,
    SparseSet,
    SparseMapU64,
} from "cap:ds";

test("SparseSet: O(1) insert, idempotency, out-of-universe bounds & stats") {
    let s = SparseSet.init();
    expect_true(s.isEmpty());
    expect_eq(s.len(), @as(usize, 0));

    expect_true(s.insert(0));
    expect_true(s.insert(42));
    expect_true(s.insert(1023));
    // Duplicate insert is idempotent (returns false, does not grow len)
    expect_true(!s.insert(42));
    // Out-of-universe IDs rejected safely
    expect_true(!s.insert(1024));
    expect_true(!s.insert(65535));
    expect_true(!s.contains(1024));

    expect_eq(s.len(), @as(usize, 3));
    expect_true(!s.isEmpty());
    expect_true(s.contains(0));
    expect_true(s.contains(42));
    expect_true(s.contains(1023));
    expect_true(!s.contains(1));

    const st = s.stats();
    expect_eq(st.len, @as(usize, 3));
    expect_eq(st.universe, SPARSE_SET_UNIVERSE);
    expect_eq(st.insert_ops, @as(u64, 3));
}

test("SparseSet: O(1) swap-remove preserves tightly packed denseSlice") {
    let s = SparseSet.init();
    * = s.insert(10);
    * = s.insert(20);
    * = s.insert(30);
    * = s.insert(40);

    // Removing 20 swaps tail (40) into index 1 -> dense = [10, 40, 30]
    expect_true(s.remove(20));
    expect_true(!s.remove(20)); // removing absent returns false
    expect_true(!s.remove(1024));
    expect_eq(s.len(), @as(usize, 3));
    expect_true(!s.contains(20));
    expect_true(s.contains(10));
    expect_true(s.contains(30));
    expect_true(s.contains(40));

    const slice = s.denseSlice();
    expect_eq(slice.len, @as(usize, 3));
    expect_eq(slice[0], @as(u16, 10));
    expect_eq(slice[1], @as(u16, 40));
    expect_eq(slice[2], @as(u16, 30));

    // Remove remaining elements down to empty
    expect_true(s.remove(30));
    expect_true(s.remove(10));
    expect_true(s.remove(40));
    expect_true(s.isEmpty());
    expect_eq(s.stats().remove_ops, @as(u64, 4));
}

test("SparseSet: O(1) clear() immunity to stale sparse slots") {
    let s = SparseSet.init();
    * = s.insert(5);
    * = s.insert(100);
    * = s.insert(500);
    expect_eq(s.len(), @as(usize, 3));

    // O(1) clear leaves sparse[5]=0, sparse[100]=1, sparse[500]=2 untouched
    s.clear();
    expect_true(s.isEmpty());
    expect_true(!s.contains(5));
    expect_true(!s.contains(100));
    expect_true(!s.contains(500));
    expect_eq(s.stats().clear_ops, @as(u64, 1));

    // Re-insert completely different IDs at dense indices 0, 1, 2
    // Even though sparse[5]==0 < dense_len, dense[0]==7 != 5 so contains(5) MUST be false!
    expect_true(s.insert(7));
    expect_true(s.insert(8));
    expect_true(s.insert(9));
    expect_true(!s.contains(5));
    expect_true(!s.contains(100));
    expect_true(!s.contains(500));
    expect_true(s.contains(7));
    expect_true(s.contains(8));
    expect_true(s.contains(9));
}

test("SparseSet: set algebra (intersectInto, unionInto, differenceInto, isSubsetOf)") {
    let a = SparseSet.init();
    let b = SparseSet.init();
    let out = SparseSet.init();

    // A = {1, 2, 3, 4}, B = {3, 4, 5, 6}
    * = a.insert(1);
    * = a.insert(2);
    * = a.insert(3);
    * = a.insert(4);

    * = b.insert(3);
    * = b.insert(4);
    * = b.insert(5);
    * = b.insert(6);

    // Intersection = {3, 4}
    expect_eq(a.intersectInto(b, *out), @as(usize, 2));
    expect_true(out.contains(3) && out.contains(4));
    expect_true(out.isSubsetOf(a));
    expect_true(out.isSubsetOf(b));
    expect_true(!a.isSubsetOf(b));

    // Union = {1, 2, 3, 4, 5, 6}
    expect_eq(a.unionInto(b, *out), @as(usize, 6));
    expect_true(a.isSubsetOf(out));
    expect_true(b.isSubsetOf(out));

    // Difference A \ B = {1, 2}
    expect_eq(a.differenceInto(b, *out), @as(usize, 2));
    expect_true(out.contains(1) && out.contains(2));
    expect_true(!out.contains(3));
}

test("SparseMapU64: put, overwrite, get & out-of-range guards") {
    let m = SparseMapU64.init();
    expect_eq(m.len(), @as(usize, 0));
    expect_true(m.get(10) == null);

    expect_true(m.put(10, 1000));
    expect_true(m.put(20, 2000));
    expect_true(!m.put(1024, 9999)); // out of universe

    expect_eq(m.get(10) ?? 0, @as(u64, 1000));
    expect_eq(m.get(20) ?? 0, @as(u64, 2000));

    // Overwrite existing key in-place
    expect_true(m.put(10, 1500));
    expect_eq(m.len(), @as(usize, 2));
    expect_eq(m.get(10) ?? 0, @as(u64, 1500));
}

test("SparseMapU64: swap-remove preserves strict denseIds / denseValues alignment & O(1) clear") {
    let m = SparseMapU64.init();
    * = m.put(101, 1111);
    * = m.put(202, 2222);
    * = m.put(303, 3333);
    * = m.put(404, 4444);

    // Remove 202 -> tail (404, 4444) swaps into slot 1
    const removed = m.remove(202) ?? 0;
    expect_eq(removed, @as(u64, 2222));
    expect_true(m.remove(202) == null);
    expect_true(!m.contains(202));
    expect_eq(m.len(), @as(usize, 3));

    const ids = m.denseIds();
    const vals = m.denseValues();
    expect_eq(ids.len, @as(usize, 3));
    expect_eq(vals.len, @as(usize, 3));

    // Verify 1-to-1 alignment between parallel slices and O(1) get()
    let i: usize = 0;
    while (i < ids.len) : (i += 1) {
        expect_eq(m.get(ids[i]) ?? 0, vals[i]);
    }
    expect_eq(ids[1], @as(u16, 404));
    expect_eq(vals[1], @as(u64, 4444));

    // O(1) clear leaves no false positives
    m.clear();
    expect_eq(m.len(), @as(usize, 0));
    expect_true(m.get(101) == null);
    expect_true(m.get(404) == null);
}

test("SparseSet: cap.ds facade parity") {
    expect_eq(cap.ds.SPARSE_SET_UNIVERSE, @as(usize, 1024));
    expect_eq(cap.ds.SPARSE_SET_INVALID_IDX, @as(u16, 0xFFFF));
    expect_eq(SPARSE_SET_UNIVERSE, @as(usize, 1024));
    expect_eq(SPARSE_SET_INVALID_IDX, @as(u16, 0xFFFF));
}
