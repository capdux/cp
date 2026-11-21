// Source: native-cp-stdlib (Ordered SkipList Key-Value Engine)

import { test, expect_eq } from "cap:test";
import { SkipList } from "cap:skip_list";
import cap from "cap";

const std = @import("std");

test "skip_list out-of-order 200 inserts and ordered items/rangeQuery" {
    const alloc = std.testing.allocator;
    let sl = SkipList(i64, i64).init(alloc);
    defer sl.deinit();

    let i: i64 = 0;
    while (i < 200) {
        const k: i64 = @mod(i * 73 + 19, 200);
        const old = try sl.insert(k, k * 10);
        expect_eq(old == null, true);
        i += 1;
    }

    expect_eq(sl.len(), @as(usize, 200));

    const all_items = try sl.items(alloc);
    defer alloc.free(all_items);
    expect_eq(all_items.len, @as(usize, 200));

    let idx: usize = 0;
    while (idx < all_items.len) {
        const expected_k: i64 = @intCast(idx);
        expect_eq(all_items[idx].key, expected_k);
        expect_eq(all_items[idx].value, expected_k * 10);
        idx += 1;
    }

    const sub = try sl.rangeQuery(alloc, 45, 95);
    defer alloc.free(sub);
    expect_eq(sub.len, @as(usize, 51));
    expect_eq(sub[0].key, @as(i64, 45));
    expect_eq(sub[50].key, @as(i64, 95));
}

test "skip_list upsert overwrites and returns old value" {
    const alloc = std.testing.allocator;
    let sl = SkipList(i32, i32).init(alloc);
    defer sl.deinit();

    const first = try sl.upsert(42, 100);
    expect_eq(first == null, true);
    expect_eq(sl.get(42).?, @as(i32, 100));

    const second = try sl.upsert(42, 999);
    expect_eq(second.?, @as(i32, 100));
    expect_eq(sl.get(42).?, @as(i32, 999));
    expect_eq(sl.len(), @as(usize, 1));
}

test "skip_list random half removal preserves structure integrity" {
    const alloc = std.testing.allocator;
    let sl = SkipList(i64, i64).init(alloc);
    defer sl.deinit();

    let i: i64 = 0;
    while (i < 120) {
        * = try sl.insert(i, i + 1000);
        i += 1;
    }

    let prng = std.Random.DefaultPrng.init(0x12345678);
    let rand = prng.random();
    let removed_count: usize = 0;
    let j: i64 = 0;
    while (j < 120) {
        if ((rand.int(u8) & 1) == 0) {
            const rem = sl.remove(j);
            expect_eq(rem.?, j + 1000);
            removed_count += 1;
        }
        j += 1;
    }

    expect_eq(sl.len(), @as(usize, 120) - removed_count);
    expect_eq(sl.remove(9999) == null, true);

    const remaining = try sl.items(alloc);
    defer alloc.free(remaining);
    expect_eq(remaining.len, sl.len());

    let prev_k: i64 = -1;
    for (entry of remaining) {
        expect_eq(entry.key > prev_k, true);
        expect_eq(sl.get(entry.key).?, entry.value);
        prev_k = entry.key;
    }
}

test "skip_list lowerBound floor ceiling boundary lookups" {
    const alloc = std.testing.allocator;
    let sl = SkipList(i64, i64).init(alloc);
    defer sl.deinit();

    * = try sl.insert(10, 1);
    * = try sl.insert(20, 2);
    * = try sl.insert(30, 3);
    * = try sl.insert(40, 4);

    expect_eq(sl.lowerBound(5).?.key, @as(i64, 10));
    expect_eq(sl.lowerBound(20).?.key, @as(i64, 20));
    expect_eq(sl.lowerBound(25).?.key, @as(i64, 30));
    expect_eq(sl.lowerBound(45) == null, true);

    expect_eq(sl.ceiling(25).?.key, @as(i64, 30));
    expect_eq(sl.ceiling(40).?.key, @as(i64, 40));

    expect_eq(sl.floor(5) == null, true);
    expect_eq(sl.floor(10).?.key, @as(i64, 10));
    expect_eq(sl.floor(25).?.key, @as(i64, 20));
    expect_eq(sl.floor(99).?.key, @as(i64, 40));
}

test "skip_list cap.skip_list dynamic mount and zero leak with testing allocator" {
    const alloc = std.testing.allocator;
    let sl = cap.skip_list.SkipList([]const u8, i32).init(alloc);
    defer sl.deinit();

    * = try sl.insert("banana", 2);
    * = try sl.insert("apple", 1);
    * = try sl.insert("cherry", 3);

    expect_eq(sl.len(), @as(usize, 3));
    expect_eq(sl.get("apple").?, @as(i32, 1));
    expect_eq(sl.lowerBound("apricot").?.key[0], @as(u8, 'b'));
    expect_eq(sl.floor("blueberry").?.key[0], @as(u8, 'b'));

    const range = try sl.rangeQuery(alloc, "apple", "cherry");
    defer alloc.free(range);
    expect_eq(range.len, @as(usize, 3));
    expect_eq(range[0].value, @as(i32, 1));
    expect_eq(range[1].value, @as(i32, 2));
    expect_eq(range[2].value, @as(i32, 3));
}
