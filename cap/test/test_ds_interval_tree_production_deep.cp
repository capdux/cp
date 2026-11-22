// Production Deep Test Suite for cap:ds Augmented Interval Tree (lib/ds/augmented_interval_tree.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    AUG_ITREE_MAX_NODES,
    IntervalSpan,
    coalesceIntervals,
    findGaps,
    AugmentedIntervalTree,
} from "cap:ds";

fn hasPayload(hits: []const IntervalSpan, id: u64): bool {
    let i: usize = 0;
    while (i < hits.len) : (i += 1) {
        if (hits[i].payload_id == id) return true;
    }
    return false;
}

// ============================================================================
// Test 1: buildFromSlice balanced tree, queryPoint stabbing & queryOverlap
// ============================================================================
test("interval_tree: buildFromSlice balanced tree, queryPoint & queryOverlap", fn() {
    const items: [6]IntervalSpan = [
        IntervalSpan{ start: 10, end: 20, payload_id: 1 },
        IntervalSpan{ start: 15, end: 30, payload_id: 2 },
        IntervalSpan{ start: 5,  end: 12, payload_id: 3 },
        IntervalSpan{ start: 25, end: 40, payload_id: 4 },
        IntervalSpan{ start: 50, end: 60, payload_id: 5 },
        IntervalSpan{ start: 18, end: 18, payload_id: 6 }, // point interval
    ];
    const tree = AugmentedIntervalTree.buildFromSlice(items[0..]) ?? return;
    const st = tree.stats();
    expect_eq(st.count, 6 as usize);
    expect_true(st.balanced);
    expect_eq(st.min_start, 5 as i64);
    expect_eq(st.max_end, 60 as i64);

    // Stabbing point 18 -> hits [10,20] (1), [15,30] (2), [18,18] (6)
    let hits: [8]IntervalSpan = [IntervalSpan{}; 8];
    let hits_s: []IntervalSpan = hits[0..];
    const q1 = tree.queryPoint(18, *hits_s);
    expect_eq(q1.written, 3 as usize);
    expect_eq(q1.total_matched, 3 as usize);
    expect_true(!q1.truncated);
    expect_true(hasPayload(hits[0..q1.written], 1));
    expect_true(hasPayload(hits[0..q1.written], 2));
    expect_true(hasPayload(hits[0..q1.written], 6));

    // Overlap [21, 26] -> hits [15,30] (2), [25,40] (4)
    const q2 = tree.queryOverlap(21, 26, *hits_s);
    expect_eq(q2.written, 2 as usize);
    expect_true(hasPayload(hits[0..q2.written], 2));
    expect_true(hasPayload(hits[0..q2.written], 4));

    // Disjoint query [41, 49] -> 0 hits
    const q3 = tree.queryOverlap(41, 49, *hits_s);
    expect_eq(q3.written, 0 as usize);
    expect_eq(q3.total_matched, 0 as usize);
});

// ============================================================================
// Test 2: queryContained & O(log N) hasAnyOverlap fast check
// ============================================================================
test("interval_tree: queryContained & O(log N) hasAnyOverlap fast check", fn() {
    const items: [5]IntervalSpan = [
        IntervalSpan{ start: 0,  end: 100, payload_id: 1 },
        IntervalSpan{ start: 10, end: 20,  payload_id: 2 },
        IntervalSpan{ start: 15, end: 25,  payload_id: 3 },
        IntervalSpan{ start: 20, end: 35,  payload_id: 4 },
        IntervalSpan{ start: 50, end: 60,  payload_id: 5 },
    ];
    const tree = AugmentedIntervalTree.buildFromSlice(items[0..]) ?? return;

    let hits: [8]IntervalSpan = [IntervalSpan{}; 8];
    let hits_s: []IntervalSpan = hits[0..];
    // Contained inside [10, 30] -> ONLY [10,20] (2) and [15,25] (3); excludes [0,100] and [20,35]
    const qc = tree.queryContained(10, 30, *hits_s);
    expect_eq(qc.written, 2 as usize);
    expect_true(hasPayload(hits[0..qc.written], 2));
    expect_true(hasPayload(hits[0..qc.written], 3));

    // hasAnyOverlap fast checks
    expect_true(tree.hasAnyOverlap(100, 120)); // touches [0,100] at 100
    expect_true(tree.hasAnyOverlap(55, 55));
    expect_true(!tree.hasAnyOverlap(101, 200));
    expect_true(!tree.hasAnyOverlap(-50, -1));
});

// ============================================================================
// Test 3: Incremental insert + rebuildBalanced parity with negative i64 coords
// ============================================================================
test("interval_tree: incremental insert + rebuildBalanced with negative i64 coords", fn() {
    let tree = AugmentedIntervalTree.init();
    expect_true(tree.insert(-100, -50, 10));
    expect_true(tree.insert(-60, 10, 20));
    expect_true(tree.insert(0, 25, 30));
    expect_true(tree.insert(-200, -150, 40));
    expect_true(!tree.stats().balanced);

    let hits_before: [8]IntervalSpan = [IntervalSpan{}; 8];
    let hb_s: []IntervalSpan = hits_before[0..];
    const qb = tree.queryPoint(-55, *hb_s);
    expect_eq(qb.total_matched, 2 as usize);
    expect_true(hasPayload(hits_before[0..qb.written], 10));
    expect_true(hasPayload(hits_before[0..qb.written], 20));

    // Rebuild into strictly balanced augmented BST -> identical query results
    tree.rebuildBalanced();
    expect_true(tree.stats().balanced);
    let hits_after: [8]IntervalSpan = [IntervalSpan{}; 8];
    let ha_s: []IntervalSpan = hits_after[0..];
    const qa = tree.queryPoint(-55, *ha_s);
    expect_eq(qa.total_matched, 2 as usize);
    expect_true(hasPayload(hits_after[0..qa.written], 10));
    expect_true(hasPayload(hits_after[0..qa.written], 20));
});

// ============================================================================
// Test 4: coalesceIntervals overlapping vs adjacent merging
// ============================================================================
test("interval_tree: coalesceIntervals overlapping vs adjacent merging", fn() {
    let arr1: [5]IntervalSpan = [
        IntervalSpan{ start: 20, end: 30, payload_id: 1 },
        IntervalSpan{ start: 1,  end: 5,  payload_id: 2 },
        IntervalSpan{ start: 6,  end: 10, payload_id: 3 }, // adjacent to [1,5]
        IntervalSpan{ start: 8,  end: 15, payload_id: 4 }, // overlaps [6,10]
        IntervalSpan{ start: 25, end: 28, payload_id: 5 }, // nested in [20,30]
    ];
    let s1: []IntervalSpan = arr1[0..];

    // merge_adjacent = false -> [1,5], [6,15], [20,30]
    const n_no_adj = coalesceIntervals(*s1, 5, false) ?? 0;
    expect_eq(n_no_adj, 3 as usize);
    expect_eq(arr1[0].start, 1 as i64);
    expect_eq(arr1[0].end, 5 as i64);
    expect_eq(arr1[1].start, 6 as i64);
    expect_eq(arr1[1].end, 15 as i64);
    expect_eq(arr1[2].start, 20 as i64);
    expect_eq(arr1[2].end, 30 as i64);

    // merge_adjacent = true -> [1,15], [20,30]
    const n_adj = coalesceIntervals(*s1, n_no_adj, true) ?? 0;
    expect_eq(n_adj, 2 as usize);
    expect_eq(arr1[0].start, 1 as i64);
    expect_eq(arr1[0].end, 15 as i64);
    expect_eq(arr1[1].start, 20 as i64);
    expect_eq(arr1[1].end, 30 as i64);
});

// ============================================================================
// Test 5: findGaps & findGapsInWindow complement hole extraction
// ============================================================================
test("interval_tree: findGaps & findGapsInWindow complement hole extraction", fn() {
    const busy: [3]IntervalSpan = [
        IntervalSpan{ start: 10, end: 20, payload_id: 1 },
        IntervalSpan{ start: 18, end: 30, payload_id: 2 },
        IntervalSpan{ start: 50, end: 60, payload_id: 3 },
    ];
    const tree = AugmentedIntervalTree.buildFromSlice(busy[0..]) ?? return;

    let gaps: [8]IntervalSpan = [IntervalSpan{}; 8];
    let gaps_s: []IntervalSpan = gaps[0..];
    // Window [0, 100] -> gaps: [0, 9], [31, 49], [61, 100]
    const g1 = tree.findGapsInWindow(0, 100, *gaps_s);
    expect_eq(g1.written, 3 as usize);
    expect_eq(g1.total_matched, 3 as usize);
    expect_true(!g1.truncated);
    expect_eq(gaps[0].start, 0 as i64);
    expect_eq(gaps[0].end, 9 as i64);
    expect_eq(gaps[1].start, 31 as i64);
    expect_eq(gaps[1].end, 49 as i64);
    expect_eq(gaps[2].start, 61 as i64);
    expect_eq(gaps[2].end, 100 as i64);

    // Fully covered window [12, 28] -> 0 gaps
    const g2 = findGaps(busy[0..], 12, 28, *gaps_s);
    expect_eq(g2.written, 0 as usize);
    expect_eq(g2.total_matched, 0 as usize);

    // Completely uncovered window [70, 80] -> single gap [70, 80]
    const g3 = findGaps(busy[0..], 70, 80, *gaps_s);
    expect_eq(g3.written, 1 as usize);
    expect_eq(gaps[0].start, 70 as i64);
    expect_eq(gaps[0].end, 80 as i64);
});

// ============================================================================
// Test 6: Output buffer truncation accounting (total_matched vs written)
// ============================================================================
test("interval_tree: output buffer truncation accounting", fn() {
    const items: [4]IntervalSpan = [
        IntervalSpan{ start: 0, end: 10, payload_id: 1 },
        IntervalSpan{ start: 2, end: 12, payload_id: 2 },
        IntervalSpan{ start: 4, end: 14, payload_id: 3 },
        IntervalSpan{ start: 5, end: 15, payload_id: 4 },
    ];
    const tree = AugmentedIntervalTree.buildFromSlice(items[0..]) ?? return;

    // Small buffer of 2 cannot hold all 4 matches -> written=2, total_matched=4, truncated=true
    let tiny: [2]IntervalSpan = [IntervalSpan{}; 2];
    let tiny_s: []IntervalSpan = tiny[0..];
    const q = tree.queryPoint(6, *tiny_s);
    expect_eq(q.written, 2 as usize);
    expect_eq(q.total_matched, 4 as usize);
    expect_true(q.truncated);
});

// ============================================================================
// Test 7: Strict boundary guards (reversed start>end, empty tree, full cap)
// ============================================================================
test("interval_tree: strict boundary guards (reversed, empty tree, full 256 cap)", fn() {
    let empty_tree = AugmentedIntervalTree.init();
    let hits: [4]IntervalSpan = [IntervalSpan{}; 4];
    let hits_s: []IntervalSpan = hits[0..];

    // Empty tree & reversed query ranges
    expect_true(!empty_tree.hasAnyOverlap(0, 10));
    expect_eq(empty_tree.queryOverlap(0, 10, *hits_s).total_matched, 0 as usize);
    expect_eq(empty_tree.queryOverlap(20, 10, *hits_s).total_matched, 0 as usize);
    expect_eq(empty_tree.queryContained(20, 10, *hits_s).total_matched, 0 as usize);

    // Reject reversed interval on insert, buildFromSlice, and coalesceIntervals
    expect_true(!empty_tree.insert(10, 5, 1));
    const bad_arr: [1]IntervalSpan = [IntervalSpan{ start: 10, end: 5, payload_id: 1 }];
    expect_true(AugmentedIntervalTree.buildFromSlice(bad_arr[0..]) == null);

    let mut_bad: [1]IntervalSpan = bad_arr;
    let mb_s: []IntervalSpan = mut_bad[0..];
    expect_true(coalesceIntervals(*mb_s, 1, true) == null);
    expect_true(coalesceIntervals(*mb_s, 2, true) == null); // count > len

    // Fill to AUG_ITREE_MAX_NODES (256) and verify 257th rejected
    let i: usize = 0;
    while (i < AUG_ITREE_MAX_NODES) : (i += 1) {
        const base: i64 = (i as i64) * 10;
        expect_true(empty_tree.insert(base, base + 5, i as u64));
    }
    expect_true(!empty_tree.insert(9999, 10000, 999));
    expect_eq(empty_tree.stats().count, AUG_ITREE_MAX_NODES);
});
