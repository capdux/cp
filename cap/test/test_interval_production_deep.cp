// Source: cp/cap/test/test_interval_production_deep.cp
// Deep Production Verification Suite for cap:interval & cap.interval
// Covers:
//   1. Interval primitives & IntervalSet overlap/adjacent merge, subtract split & gaps extraction
//   2. Linear O(M+N) unionWith / intersectWith / differenceWith algebra & containsRange / totalLength
//   3. Generic IntervalTree<V> overlapping multi-interval queryPoint / queryOverlap with max_end pruning
//   4. Boundary guards: start > end -> InvalidInterval & empty [x, x) semantics + EmptyInterval
//   5. `import cap from "cap"; cap.interval` dynamic mounting & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    IntervalError,
    Interval,
    IntervalSet,
    IntervalEntry,
    IntervalTree,
} from "cap:time";

test("cap:interval - IntervalSet merge, subtract split & gaps extraction") {
    const alloc = std.testing.allocator;

    // Interval primitive checks
    const a = Interval.init(10, 20) catch unreachable;
    const b = Interval.init(20, 30) catch unreachable;
    const c = Interval.init(15, 25) catch unreachable;
    expect_eq(a.len(), 10);
    expect_eq(a.isEmpty(), false);
    expect_eq(a.containsPoint(10), true);
    expect_eq(a.containsPoint(20), false);
    expect_eq(a.touches(b), true);
    expect_eq(a.overlaps(b), false);
    expect_eq(a.overlaps(c), true);
    const inter = a.intersect(c).?;
    expect_eq(inter.start, 15);
    expect_eq(inter.end, 20);
    expect_eq(a.intersect(b) == null, true);
    const h = a.hull(b);
    expect_eq(h.start, 10);
    expect_eq(h.end, 30);

    let set = IntervalSet.init(alloc);
    defer set.deinit();

    // Adjacent and overlapping merges: [0, 5) + [5, 10) + [20, 30) + [8, 22) -> [0, 30)
    * = set.add(0, 5) catch unreachable;
    * = set.add(5, 10) catch unreachable;
    expect_eq(set.items().len, 1);
    expect_eq(set.items()[0].start, 0);
    expect_eq(set.items()[0].end, 10);

    * = set.add(20, 30) catch unreachable;
    expect_eq(set.items().len, 2);

    * = set.add(8, 22) catch unreachable;
    expect_eq(set.items().len, 1);
    expect_eq(set.items()[0].start, 0);
    expect_eq(set.items()[0].end, 30);
    expect_eq(set.totalLength(), 30);

    // Subtract middle hole -> splits [0, 30) into [0, 10) and [20, 30)
    * = set.subtract(10, 20) catch unreachable;
    expect_eq(set.items().len, 2);
    expect_eq(set.items()[0].start, 0);
    expect_eq(set.items()[0].end, 10);
    expect_eq(set.items()[1].start, 20);
    expect_eq(set.items()[1].end, 30);

    // Subtract another hole and verify left/right truncation + gaps
    * = set.subtract(25, 40) catch unreachable;
    * = set.subtract(-5, 2) catch unreachable;
    expect_eq(set.items().len, 2);
    expect_eq(set.items()[0].start, 2);
    expect_eq(set.items()[0].end, 10);
    expect_eq(set.items()[1].start, 20);
    expect_eq(set.items()[1].end, 25);

    * = set.add(35, 40) catch unreachable;
    const g = set.gaps(alloc) catch unreachable;
    defer alloc.free(g);
    expect_eq(g.len, 2);
    expect_eq(g[0].start, 10);
    expect_eq(g[0].end, 20);
    expect_eq(g[1].start, 25);
    expect_eq(g[1].end, 35);

    // clone & clear
    let cloned = set.clone(alloc) catch unreachable;
    defer cloned.deinit();
    expect_eq(cloned.totalLength(), set.totalLength());
    set.clear();
    expect_eq(set.items().len, 0);
    expect_eq(set.totalLength(), 0);
    expect_eq(cloned.items().len, 3);
}

test("cap:interval - linear O(M+N) unionWith, intersectWith, differenceWith & containsRange") {
    const alloc = std.testing.allocator;

    let s1 = IntervalSet.init(alloc);
    defer s1.deinit();
    * = s1.add(0, 10) catch unreachable;
    * = s1.add(20, 30) catch unreachable;
    * = s1.add(40, 50) catch unreachable;

    let s2 = IntervalSet.init(alloc);
    defer s2.deinit();
    * = s2.add(5, 25) catch unreachable;
    * = s2.add(30, 45) catch unreachable;

    // Union: [0, 50) (since [0,10) U [5,25) U [20,30) U [30,45) U [40,50) = [0, 50))
    let u = s1.unionWith(alloc, s2) catch unreachable;
    defer u.deinit();
    expect_eq(u.items().len, 1);
    expect_eq(u.items()[0].start, 0);
    expect_eq(u.items()[0].end, 50);
    expect_eq(u.totalLength(), 50);
    expect_eq(u.contains(0), true);
    expect_eq(u.contains(49), true);
    expect_eq(u.contains(50), false);
    expect_eq(u.containsRange(10, 40) catch unreachable, true);
    expect_eq(u.containsRange(10, 51) catch unreachable, false);

    // Intersection: [5, 10), [20, 25), [40, 45)
    let inter = s1.intersectWith(alloc, s2) catch unreachable;
    defer inter.deinit();
    expect_eq(inter.items().len, 3);
    expect_eq(inter.items()[0].start, 5);
    expect_eq(inter.items()[0].end, 10);
    expect_eq(inter.items()[1].start, 20);
    expect_eq(inter.items()[1].end, 25);
    expect_eq(inter.items()[2].start, 40);
    expect_eq(inter.items()[2].end, 45);
    expect_eq(inter.totalLength(), 15);

    // Difference s1 \ s2: [0, 5), [25, 30), [45, 50)
    let diff = s1.differenceWith(alloc, s2) catch unreachable;
    defer diff.deinit();
    expect_eq(diff.items().len, 3);
    expect_eq(diff.items()[0].start, 0);
    expect_eq(diff.items()[0].end, 5);
    expect_eq(diff.items()[1].start, 25);
    expect_eq(diff.items()[1].end, 30);
    expect_eq(diff.items()[2].start, 45);
    expect_eq(diff.items()[2].end, 50);
    expect_eq(diff.totalLength(), 15);
    expect_eq(diff.containsRange(0, 5) catch unreachable, true);
    expect_eq(diff.containsRange(0, 6) catch unreachable, false);
}

test("cap:interval - IntervalTree<V> overlapping queries & max_end pruning") {
    const alloc = std.testing.allocator;
    let tree = IntervalTree<i32>.init(alloc);
    defer tree.deinit();

    // Insert 40 intervals (including negative, overlapping, and nested) to trigger AVL rotations
    let i: i64 = 0;
    while (i < 20) : (i += 1) {
        const s = i * 10 - 50;
        const e = s + 15;
        * = tree.insert(s, e, @intCast(i + 1)) catch unreachable;
    }
    // Add broad spanning intervals
    * = tree.insert(-100, 200, 999) catch unreachable;
    * = tree.insert(5, 25, 500) catch unreachable;
    * = tree.insert(10, 12, 501) catch unreachable;
    expect_eq(tree.count(), 23);

    // Query point 11:
    // Loop i where s <= 11 < s + 15 => i*10 - 50 <= 11 < i*10 - 35 => i*10 in (46, 61] => i=5 ([0,15)), i=6 ([10,25))
    // Plus 999 ([-100,200)), 500 ([5,25)), 501 ([10,12)) => total 5 matches
    const pt_hits = tree.queryPoint(alloc, 11) catch unreachable;
    defer alloc.free(pt_hits);
    expect_eq(pt_hits.len, 5);

    let sum_vals: i32 = 0;
    for (entry of pt_hits) {
        expect_eq(entry.start <= 11 && 11 < entry.end, true);
        sum_vals += entry.value;
    }
    // 6 + 7 + 999 + 500 + 501 = 2013
    expect_eq(sum_vals, 2013);

    // Query overlap [12, 13):
    // 501 ([10, 12)) does NOT overlap [12, 13) because half-open!
    // Matches: i=5 ([0,15)), i=6 ([10,25)), 999, 500 => 4 matches
    const ov_hits = tree.queryOverlap(alloc, 12, 13) catch unreachable;
    defer alloc.free(ov_hits);
    expect_eq(ov_hits.len, 4);

    // Disjoint query far away -> pruned immediately
    const far_hits = tree.queryOverlap(alloc, 500, 600) catch unreachable;
    defer alloc.free(far_hits);
    expect_eq(far_hits.len, 0);
}

test("cap:interval - InvalidInterval & empty [x, x) boundary semantics") {
    const alloc = std.testing.allocator;

    // Interval.init(10, 5) -> InvalidInterval
    let saw_invalid_iv = false;
    * = Interval.init(10, 5) catch |e| {
        if (e == IntervalError.InvalidInterval) {
            saw_invalid_iv = true;
        }
    };
    expect_eq(saw_invalid_iv, true);

    // Empty interval [7, 7)
    const empty_iv = Interval.init(7, 7) catch unreachable;
    expect_eq(empty_iv.isEmpty(), true);
    expect_eq(empty_iv.len(), 0);
    expect_eq(empty_iv.containsPoint(7), false);

    // IntervalSet boundaries
    let set = IntervalSet.init(alloc);
    defer set.deinit();

    let saw_add_invalid = false;
    set.add(5, 4) catch |e| {
        if (e == IntervalError.InvalidInterval) {
            saw_add_invalid = true;
        }
    };
    expect_eq(saw_add_invalid, true);

    let saw_sub_invalid = false;
    set.subtract(5, 4) catch |e| {
        if (e == IntervalError.InvalidInterval) {
            saw_sub_invalid = true;
        }
    };
    expect_eq(saw_sub_invalid, true);

    let saw_rng_invalid = false;
    * = set.containsRange(5, 4) catch |e| {
        if (e == IntervalError.InvalidInterval) {
            saw_rng_invalid = true;
        }
    };
    expect_eq(saw_rng_invalid, true);

    // Empty [7, 7) on IntervalSet is no-op / vacuous true
    * = set.add(7, 7) catch unreachable;
    expect_eq(set.items().len, 0);
    * = set.subtract(7, 7) catch unreachable;
    expect_eq(set.containsRange(7, 7) catch unreachable, true);

    // IntervalTree boundaries
    let tree = IntervalTree<u8>.init(alloc);
    defer tree.deinit();

    let saw_tree_invalid = false;
    tree.insert(9, 2, 1) catch |e| {
        if (e == IntervalError.InvalidInterval) {
            saw_tree_invalid = true;
        }
    };
    expect_eq(saw_tree_invalid, true);

    let saw_tree_empty = false;
    tree.insert(9, 9, 1) catch |e| {
        if (e == IntervalError.EmptyInterval) {
            saw_tree_empty = true;
        }
    };
    expect_eq(saw_tree_empty, true);

    let saw_q_invalid = false;
    * = tree.queryOverlap(alloc, 8, 3) catch |e| {
        if (e == IntervalError.InvalidInterval) {
            saw_q_invalid = true;
        }
    };
    expect_eq(saw_q_invalid, true);

    const empty_q = tree.queryOverlap(alloc, 5, 5) catch unreachable;
    defer alloc.free(empty_q);
    expect_eq(empty_q.len, 0);
}

test("cap.interval - dynamic mounting via import cap from \"cap\" & zero leaks") {
    const alloc = std.testing.allocator;

    let set = cap.time.IntervalSet.init(alloc);
    defer set.deinit();
    * = set.add(100, 150) catch unreachable;
    * = set.add(150, 200) catch unreachable;
    expect_eq(set.totalLength(), 100);
    expect_eq(set.containsRange(110, 190) catch unreachable, true);

    let tree = cap.time.IntervalTree<i32>.init(alloc);
    defer tree.deinit();
    * = tree.insert(10, 50, 101) catch unreachable;
    * = tree.insert(30, 70, 102) catch unreachable;

    const hits = tree.queryPoint(alloc, 35) catch unreachable;
    defer alloc.free(hits);
    expect_eq(hits.len, 2);
}
