// Source: cp/cap/test/test_treap_production_deep.cp (Production deep tests for cap:treap)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TREAP_NIL,
    TreapNode,
    TreapEntry,
    Treap,
} from "cap:ds";

test("1. Sequential, reverse & shuffled insertions + in-place updates with invariant checks", () => {
    expect_eq(TREAP_NIL, 0xFFFFFFFF as u32);
    let pool: [128]TreapNode = std.mem.zeroes([128]TreapNode);
    let t = Treap.init(pool[0..], 12345);

    expect_eq(t.capacity(), 128 as usize);
    expect_eq(t.isEmpty(), true);
    expect_eq(t.verifyInvariants(), true);

    // Ascending 0..63
    let i: i64 = 0;
    while (i < 64) {
        expect_eq(t.insert(i, i * 10), true);
        i += 1;
    }
    expect_eq(t.len(), 64 as usize);
    expect_eq(t.verifyInvariants(), true);

    // Descending 127..64
    i = 127;
    while (i >= 64) {
        expect_eq(t.insert(i, i * 10), true);
        i -= 1;
    }
    expect_eq(t.len(), 128 as usize);
    expect_eq(t.verifyInvariants(), true);

    // In-place update on existing keys even when pool is completely full
    expect_eq(t.insert(42, 9999), true);
    expect_eq(t.insert(0, -1), true);
    expect_eq(t.insert(127, 7777), true);
    expect_eq(t.len(), 128 as usize);
    expect_eq(t.get(42) ?? unreachable, 9999 as i64);
    expect_eq(t.get(0) ?? unreachable, -1 as i64);
    expect_eq(t.get(127) ?? unreachable, 7777 as i64);
    expect_eq(t.contains(63), true);
    expect_eq(t.contains(999), false);

    const mn = t.minEntry() ?? unreachable;
    const mx = t.maxEntry() ?? unreachable;
    expect_eq(mn.key, 0 as i64);
    expect_eq(mx.key, 127 as i64);
    expect_eq(t.verifyInvariants(), true);
});

test("2. O(log n) kth, rank, lowerBound, upperBound & rangeCount cross-checked against sorted array", () => {
    let pool: [64]TreapNode = std.mem.zeroes([64]TreapNode);
    let t = Treap.init(pool[0..], 987654321);

    // Insert 40 distinct keys in scrambled order: keys = (idx - 20) * 3 -> [-60, -57, ..., 57]
    let sorted: [40]i64 = std.mem.zeroes([40]i64);
    let idx: usize = 0;
    while (idx < 40) {
        sorted[idx] = ((idx as i64) - 20) * 3;
        idx += 1;
    }

    // Insert via coprime stride (stride 17 mod 40)
    let step: usize = 0;
    while (step < 40) {
        const pos = (step * 17) % 40;
        const k = sorted[pos];
        expect_eq(t.insert(k, k + 1000), true);
        step += 1;
    }
    expect_eq(t.len(), 40 as usize);
    expect_eq(t.verifyInvariants(), true);

    // Verify kth(0..39), rank, lowerBound, upperBound for every key and between keys
    idx = 0;
    while (idx < 40) {
        const expected_k = sorted[idx];
        const e = t.kth(idx) ?? unreachable;
        expect_eq(e.key, expected_k);
        expect_eq(e.value, expected_k + 1000);

        expect_eq(t.rank(expected_k), idx);
        expect_eq(t.rank(expected_k + 1), idx + 1);

        const lb_exact = t.lowerBound(expected_k) ?? unreachable;
        expect_eq(lb_exact.key, expected_k);

        const lb_minus1 = t.lowerBound(expected_k - 1) ?? unreachable;
        expect_eq(lb_minus1.key, expected_k);

        if (idx + 1 < 40) {
            const ub = t.upperBound(expected_k) ?? unreachable;
            expect_eq(ub.key, sorted[idx + 1]);
        } else {
            expect_eq(t.upperBound(expected_k) == null, true);
        }
        idx += 1;
    }

    expect_eq(t.kth(40) == null, true);
    expect_eq(t.lowerBound(100) == null, true);
    expect_eq(t.rank(-100), 0 as usize);
    expect_eq(t.rank(100), 40 as usize);

    // rangeCount checks
    expect_eq(t.rangeCount(-60, 57), 40 as usize);
    expect_eq(t.rangeCount(-60, -60), 1 as usize);
    expect_eq(t.rangeCount(-59, -58), 0 as usize);
    expect_eq(t.rangeCount(-30, 30), 21 as usize); // -30, -27, ..., 30 => 21 elements
    expect_eq(t.rangeCount(10, -10), 0 as usize);  // lo > hi
});

test("3. High-frequency insert/remove churn verifies 100% free-list reuse with zero leaks", () => {
    // Small pool of capacity 16, churn 240 insert+remove cycles
    let pool: [16]TreapNode = std.mem.zeroes([16]TreapNode);
    let t = Treap.init(pool[0..], 2026);

    let round: i64 = 0;
    while (round < 20) {
        // Fill all 16 slots
        let j: i64 = 0;
        while (j < 16) {
            const key = round * 100 + ((j * 7) % 16);
            expect_eq(t.insert(key, key * 2), true);
            j += 1;
        }
        expect_eq(t.len(), 16 as usize);
        expect_eq(t.verifyInvariants(), true);

        // Removing non-existent key returns false
        expect_eq(t.remove(-99999), false);

        // Remove all 16 in a different order (forcing root, leaf & two-child merges)
        j = 0;
        while (j < 16) {
            const key = round * 100 + ((j * 11) % 16);
            expect_eq(t.remove(key), true);
            expect_eq(t.contains(key), false);
            j += 1;
        }
        expect_eq(t.isEmpty(), true);
        expect_eq(t.verifyInvariants(), true);
        round += 1;
    }
});

test("4. Pool exhaustion returns false cleanly, allows in-place update & recovers after remove", () => {
    let pool: [4]TreapNode = std.mem.zeroes([4]TreapNode);
    let t = Treap.init(pool[0..], 77);

    expect_eq(t.insert(10, 1), true);
    expect_eq(t.insert(20, 2), true);
    expect_eq(t.insert(30, 3), true);
    expect_eq(t.insert(40, 4), true);

    // Pool full: new key rejected
    expect_eq(t.insert(50, 5), false);
    expect_eq(t.len(), 4 as usize);

    // Existing key update still succeeds when pool is full
    expect_eq(t.insert(20, 222), true);
    expect_eq(t.get(20) ?? unreachable, 222 as i64);

    // Remove one slot -> new key succeeds
    expect_eq(t.remove(30), true);
    expect_eq(t.insert(50, 5), true);
    expect_eq(t.len(), 4 as usize);
    expect_eq(t.get(50) ?? unreachable, 5 as i64);
    expect_eq(t.verifyInvariants(), true);
});

test("5. clear() reset, empty tree queries & zero-capacity pool safety", () => {
    let pool: [8]TreapNode = std.mem.zeroes([8]TreapNode);
    let t = Treap.init(pool[0..], 0); // seed == 0 falls back cleanly

    expect_eq(t.insert(5, 50), true);
    expect_eq(t.insert(1, 10), true);
    expect_eq(t.insert(9, 90), true);
    expect_eq(t.len(), 3 as usize);

    t.clear();
    expect_eq(t.isEmpty(), true);
    expect_eq(t.len(), 0 as usize);
    expect_eq(t.get(5) == null, true);
    expect_eq(t.remove(5), false);
    expect_eq(t.kth(0) == null, true);
    expect_eq(t.rank(5), 0 as usize);
    expect_eq(t.lowerBound(5) == null, true);
    expect_eq(t.upperBound(5) == null, true);
    expect_eq(t.minEntry() == null, true);
    expect_eq(t.maxEntry() == null, true);
    expect_eq(t.rangeCount(-100, 100), 0 as usize);
    expect_eq(t.verifyInvariants(), true);

    // Can refill full capacity after clear()
    let i: i64 = 0;
    while (i < 8) {
        expect_eq(t.insert(i, i), true);
        i += 1;
    }
    expect_eq(t.len(), 8 as usize);
    expect_eq(t.verifyInvariants(), true);

    // Zero-capacity pool never panics
    let empty_pool: [0]TreapNode = [];
    let tz = Treap.init(empty_pool[0..], 1);
    expect_eq(tz.capacity(), 0 as usize);
    expect_eq(tz.insert(1, 1), false);
    expect_eq(tz.verifyInvariants(), true);
});

test("6. Extreme i64 key boundaries (min/max rangeCount overflow immunity)", () => {
    let pool: [8]TreapNode = std.mem.zeroes([8]TreapNode);
    let t = Treap.init(pool[0..], 555);

    const min_i64: i64 = -9223372036854775807 - 1;
    const max_i64: i64 = 9223372036854775807;

    expect_eq(t.insert(min_i64, -1), true);
    expect_eq(t.insert(0, 0), true);
    expect_eq(t.insert(max_i64, 1), true);
    expect_eq(t.verifyInvariants(), true);

    expect_eq(t.rangeCount(min_i64, max_i64), 3 as usize);
    expect_eq(t.rangeCount(max_i64, max_i64), 1 as usize);
    expect_eq(t.rangeCount(min_i64, min_i64), 1 as usize);
    expect_eq((t.minEntry() ?? unreachable).key, min_i64);
    expect_eq((t.maxEntry() ?? unreachable).key, max_i64);
});
