// Source: native-cp-stdlib (Ordered B+ Tree Index)
import std from "std";
import cap from "cap";
import { BPlusTree } from "cap:bptree";
import { test, expect_eq } from "cap:test";

test "bptree: out-of-order 300 inserts trigger multi-level splits and exact lookup" {
    const alloc = std.testing.allocator;
    let tree = BPlusTree(i64, i64).init(alloc);
    defer tree.deinit();

    // Insert 300 keys in deterministic pseudo-random permutation (stride coprime to 300)
    let i: usize = 0;
    while (i < 300) {
        const k: i64 = @intCast((i * 73 + 19) % 300);
        const v: i64 = k * 10 + 7;
        const prev = try tree.insert(k, v);
        expect_eq(prev == null, true);
        i += 1;
    }

    expect_eq(tree.len(), 300);

    let k_check: i64 = 0;
    while (k_check < 300) {
        const got = tree.get(k_check);
        expect_eq(got != null, true);
        expect_eq(got.?, k_check * 10 + 7);
        k_check += 1;
    }

    expect_eq(tree.get(-1) == null, true);
    expect_eq(tree.get(300) == null, true);
}

test "bptree: upsert overwrites existing value and returns old value" {
    const alloc = std.testing.allocator;
    let tree = BPlusTree(i64, i64).init(alloc);
    defer tree.deinit();

    let i: i64 = 0;
    while (i < 50) {
        const prev = try tree.insert(i, i * 2);
        expect_eq(prev == null, true);
        i += 1;
    }
    expect_eq(tree.len(), 50);

    const old1 = try tree.insert(25, 9999);
    expect_eq(old1 != null, true);
    expect_eq(old1.?, 50);
    expect_eq(tree.get(25).?, 9999);
    expect_eq(tree.len(), 50);

    const old2 = try tree.insert(25, 12345);
    expect_eq(old2.?, 9999);
    expect_eq(tree.get(25).?, 12345);
    expect_eq(tree.len(), 50);
}

test "bptree: leaf linked-list rangeScan returns ordered slice" {
    const alloc = std.testing.allocator;
    let tree = BPlusTree(i64, i64).init(alloc);
    defer tree.deinit();

    let i: usize = 0;
    while (i < 120) {
        const k: i64 = @intCast((i * 37) % 120);
        * = try tree.insert(k, k + 1000);
        i += 1;
    }

    const entries = try tree.rangeScan(alloc, 25, 75);
    defer alloc.free(entries);

    expect_eq(entries.len, 51);
    let idx: usize = 0;
    while (idx < entries.len) {
        const expected_k: i64 = @intCast(25 + idx);
        expect_eq(entries[idx].key, expected_k);
        expect_eq(entries[idx].value, expected_k + 1000);
        idx += 1;
    }

    const empty_entries = try tree.rangeScan(alloc, 500, 600);
    defer alloc.free(empty_entries);
    expect_eq(empty_entries.len, 0);
}

test "bptree: remove half of keys with borrow/merge and verify remaining & rangeScan" {
    const alloc = std.testing.allocator;
    let tree = BPlusTree(i64, i64).init(alloc);
    defer tree.deinit();

    let i: i64 = 0;
    while (i < 200) {
        * = try tree.insert(i, i * 3);
        i += 1;
    }
    expect_eq(tree.len(), 200);

    // Delete all even keys (100 keys)
    let d: i64 = 0;
    while (d < 200) {
        const removed = tree.remove(d);
        expect_eq(removed != null, true);
        expect_eq(removed.?, d * 3);
        d += 2;
    }
    expect_eq(tree.len(), 100);
    expect_eq(tree.remove(42) == null, true);

    // Verify all odd keys remain and even keys are gone
    let check: i64 = 0;
    while (check < 200) {
        if (@mod(check, 2) == 0) {
            expect_eq(tree.get(check) == null, true);
        } else {
            expect_eq(tree.get(check).?, check * 3);
        }
        check += 1;
    }

    const scan = try tree.rangeScan(alloc, 10, 30);
    defer alloc.free(scan);
    expect_eq(scan.len, 10);
    let s_idx: usize = 0;
    while (s_idx < scan.len) {
        const expected_k: i64 = @intCast(11 + s_idx * 2);
        expect_eq(scan[s_idx].key, expected_k);
        expect_eq(scan[s_idx].value, expected_k * 3);
        s_idx += 1;
    }
}

test "bptree: cap.bptree dynamic mount and testing allocator zero-leak" {
    const alloc = std.testing.allocator;
    let tree = cap.bptree.BPlusTree(u32, u64).init(alloc);
    defer tree.deinit();

    let i: u32 = 1;
    while (i <= 100) {
        * = try tree.insert(101 - i, @as(u64, i) * 100);
        i += 1;
    }
    expect_eq(tree.len(), 100);

    let r: u32 = 1;
    while (r <= 100) {
        const removed = tree.remove(r);
        expect_eq(removed != null, true);
        r += 1;
    }
    expect_eq(tree.len(), 0);
    expect_eq(tree.get(1) == null, true);
}
