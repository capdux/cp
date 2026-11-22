// Source: cp/cap/test/test_skiplist_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:skiplist & cap.skiplist

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { SkipList, SkipEntry, SkipListError } from "cap:ds";

test("1. out-of-order 1024-key insertion, strict lexicographical rangeScan, upsert overwrite and key ownership") {
    const alloc = std.testing.allocator;
    let sl = SkipList<i64>.init(alloc)?;
    defer sl.deinit();

    // Ownership safety: mutate caller buffer immediately after upsert
    let key_buf: [16]u8 = undefined;
    @memcpy(key_buf[0..6], "k:0000");
    const first_ins = sl.upsert(key_buf[0..6], 999)?;
    expect_eq(first_ins, true);
    @memcpy(key_buf[0..6], "XXXXXX");
    expect_eq(sl.get("k:0000").?, 999);
    expect_eq(sl.contains("XXXXXX"), false);

    // Overwrite existing key returns false
    const overwrite_res = sl.upsert("k:0000", 0)?;
    expect_eq(overwrite_res, false);
    expect_eq(sl.get("k:0000").?, 0);
    expect_eq(sl.len(), 1);

    // Insert 1023 more keys in deterministic pseudo-random out-of-order sequence
    // Using stride 373 (coprime to 1024) to visit 1..1023 in scrambled order
    let step: usize = 1;
    let fmt_buf: [16]u8 = undefined;
    while (step < 1024) : (step += 1) {
        const idx = (step * 373) % 1024;
        if (idx == 0) continue;
        const k = std.fmt.bufPrint(&fmt_buf, "k:{d:0>4}", .{idx}) catch unreachable;
        const inserted = sl.upsert(k, idx as i64)?;
        expect_eq(inserted, true);
    }
    // Note: since 373 is coprime to 1024, step 1..1023 visits all 1..1023 except idx=0 (which was already inserted)
    // Wait: step*373 mod 1024 == 0 only when step == 0 or 1024, so all 1023 values are inserted!
    expect_eq(sl.len(), 1024);
    expect_eq(sl.isEmpty(), false);

    // Overwrite a subset of keys and verify len stays 1024
    expect_eq(sl.upsert("k:0500", 50000)?, false);
    expect_eq(sl.upsert("k:0999", 99900)?, false);
    expect_eq(sl.len(), 1024);
    expect_eq(sl.get("k:0500").?, 50000);
    expect_eq(sl.get("k:0999").?, 99900);

    // Full rangeScan [k:0000, k:9999) must yield all 1024 entries in strict ascending order
    const all_entries = sl.rangeScan(alloc, "k:0000", "k:9999")?;
    defer alloc.free(all_entries);
    expect_eq(all_entries.len, 1024);

    let i: usize = 0;
    while (i < all_entries.len) : (i += 1) {
        if (i > 0) {
            expect_eq(std.mem.order(u8, all_entries[i - 1].key, all_entries[i].key) == .lt, true);
        }
    }
    expect_eq(std.mem.eql(u8, all_entries[0].key, "k:0000"), true);
    expect_eq(std.mem.eql(u8, all_entries[1023].key, "k:1023"), true);

    // Sub-range scan [k:0100, k:0200) -> exactly 100 entries
    const sub = sl.rangeScan(alloc, "k:0100", "k:0200")?;
    defer alloc.free(sub);
    expect_eq(sub.len, 100);
    expect_eq(std.mem.eql(u8, sub[0].key, "k:0100"), true);
    expect_eq(std.mem.eql(u8, sub[99].key, "k:0199"), true);
}

test("2. lowerBound, first, last, empty table and boundary key lookups") {
    const alloc = std.testing.allocator;
    let sl = SkipList<i32>.init(alloc)?;
    defer sl.deinit();

    // Empty table boundary checks
    expect_eq(sl.isEmpty(), true);
    expect_eq(sl.len(), 0);
    expect_eq(sl.first() == null, true);
    expect_eq(sl.last() == null, true);
    expect_eq(sl.lowerBound("a") == null, true);
    expect_eq(sl.get("a") == null, true);
    expect_eq(sl.contains("a"), false);

    * = sl.upsert("b", 20)?;
    * = sl.upsert("d", 40)?;
    * = sl.upsert("f", 60)?;

    const f_entry = sl.first().?;
    expect_eq(std.mem.eql(u8, f_entry.key, "b"), true);
    expect_eq(f_entry.value, 20);

    const l_entry = sl.last().?;
    expect_eq(std.mem.eql(u8, l_entry.key, "f"), true);
    expect_eq(l_entry.value, 60);

    // lowerBound before first
    const lb_a = sl.lowerBound("a").?;
    expect_eq(std.mem.eql(u8, lb_a.key, "b"), true);
    expect_eq(lb_a.value, 20);

    // lowerBound exact match
    const lb_d = sl.lowerBound("d").?;
    expect_eq(std.mem.eql(u8, lb_d.key, "d"), true);
    expect_eq(lb_d.value, 40);

    // lowerBound between keys
    const lb_e = sl.lowerBound("e").?;
    expect_eq(std.mem.eql(u8, lb_e.key, "f"), true);
    expect_eq(lb_e.value, 60);

    // lowerBound past last -> null
    expect_eq(sl.lowerBound("z") == null, true);
}

test("3. bulk removeKey structural integrity, tail tracking and clear reset") {
    const alloc = std.testing.allocator;
    let sl = SkipList<usize>.initWithSeed(alloc, 16, 12345)?;
    defer sl.deinit();

    let fmt_buf: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 200) : (i += 1) {
        const k = std.fmt.bufPrint(&fmt_buf, "item:{d:0>3}", .{i}) catch unreachable;
        * = sl.upsert(k, i * 10)?;
    }
    expect_eq(sl.len(), 200);

    // Remove non-existent key
    expect_eq(sl.removeKey("item:999") == null, true);
    expect_eq(sl.len(), 200);

    // Remove all even keys (0, 2, ..., 198)
    i = 0;
    while (i < 200) : (i += 2) {
        const k = std.fmt.bufPrint(&fmt_buf, "item:{d:0>3}", .{i}) catch unreachable;
        const removed = sl.removeKey(k);
        expect_eq(removed.?, i * 10);
        expect_eq(sl.contains(k), false);
    }
    expect_eq(sl.len(), 100);

    // Remove tail ("item:199") and head ("item:001") and verify first/last update properly
    expect_eq(sl.removeKey("item:199").?, 1990);
    expect_eq(sl.removeKey("item:001").?, 10);
    expect_eq(sl.len(), 98);
    expect_eq(std.mem.eql(u8, sl.first().?.key, "item:003"), true);
    expect_eq(std.mem.eql(u8, sl.last().?.key, "item:197"), true);

    // Verify rangeScan consistency across remaining odd keys
    const rem = sl.rangeScan(alloc, "item:000", "item:200")?;
    defer alloc.free(rem);
    expect_eq(rem.len, 98);
    expect_eq(std.mem.eql(u8, rem[0].key, "item:003"), true);
    expect_eq(std.mem.eql(u8, rem[97].key, "item:197"), true);

    // clear() resets state and allows clean reuse
    sl.clear();
    expect_eq(sl.isEmpty(), true);
    expect_eq(sl.len(), 0);
    expect_eq(sl.first() == null, true);
    expect_eq(sl.last() == null, true);
    expect_eq(sl.get("item:003") == null, true);

    * = sl.upsert("reused", 777)?;
    expect_eq(sl.len(), 1);
    expect_eq(sl.get("reused").?, 777);
    expect_eq(std.mem.eql(u8, sl.last().?.key, "reused"), true);
}

test("4. InvalidMaxLevel, InvalidRange error guards and zero-length empty key boundary") {
    const alloc = std.testing.allocator;

    // InvalidMaxLevel: 0 and 33 (> 32)
    let err_zero = false;
    * = SkipList<i32>.initWithSeed(alloc, 0, 1) catch |e| {
        if (e == SkipListError.InvalidMaxLevel) err_zero = true;
    };
    expect_eq(err_zero, true);

    let err_too_high = false;
    * = SkipList<i32>.initWithSeed(alloc, 33, 1) catch |e| {
        if (e == SkipListError.InvalidMaxLevel) err_too_high = true;
    };
    expect_eq(err_too_high, true);

    let sl = SkipList<i32>.initWithSeed(alloc, 1, 7)?;
    defer sl.deinit();

    // Zero-length key "" edge case
    expect_eq(sl.upsert("", 42)?, true);
    * = sl.upsert("a", 100)?;
    expect_eq(sl.len(), 2);
    expect_eq(sl.get("").?, 42);
    expect_eq(std.mem.eql(u8, sl.first().?.key, ""), true);
    expect_eq(sl.lowerBound("").?.value, 42);

    // Half-open [k, k) yields empty slice
    const empty_range = sl.rangeScan(alloc, "a", "a")?;
    defer alloc.free(empty_range);
    expect_eq(empty_range.len, 0);

    // InvalidRange when start > end
    let invalid_range = false;
    * = sl.rangeScan(alloc, "z", "a") catch |e| {
        if (e == SkipListError.InvalidRange) invalid_range = true;
    };
    expect_eq(invalid_range, true);

    // Remove zero-length key
    expect_eq(sl.removeKey("").?, 42);
    expect_eq(sl.contains(""), false);
    expect_eq(std.mem.eql(u8, sl.first().?.key, "a"), true);
}

test("5. cap.skiplist dynamic mounting, 32-level tower and std.testing.allocator zero-leak check") {
    const alloc = std.testing.allocator;
    let sl = cap.ds.SkipList<i64>.initWithSeed(alloc, 32, 987654321)?;
    defer sl.deinit();

    * = sl.upsert("wal:003", 300)?;
    * = sl.upsert("wal:001", 100)?;
    * = sl.upsert("wal:002", 200)?;

    expect_eq(sl.len(), 3);
    expect_eq(sl.get("wal:002").?, 200);

    const scan = sl.rangeScan(alloc, "wal:001", "wal:003")?;
    defer alloc.free(scan);
    expect_eq(scan.len, 2);
    expect_eq(scan[0].value, 100);
    expect_eq(scan[1].value, 200);

    expect_eq(sl.removeKey("wal:001").?, 100);
    expect_eq(sl.removeKey("wal:003").?, 300);
    expect_eq(sl.len(), 1);
    expect_eq(std.mem.eql(u8, sl.first().?.key, "wal:002"), true);
    expect_eq(std.mem.eql(u8, sl.last().?.key, "wal:002"), true);
}
