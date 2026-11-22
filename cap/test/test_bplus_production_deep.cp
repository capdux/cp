// Source: cp/cap/test/test_bplus_production_deep.cp
// Deep Production Verification Suite for cap:bplus & cap.bplus
// Covers:
//   1. Out-of-order insertion of 1000 entries (>= 3-level height split), verifyInvariants(), 100% get() hit rate
//   2. Overwrite update, rangeScan inclusive/exclusive exact boundaries, prefixScan & lowerBound
//   3. Large-scale remove (90% deletion triggering borrow/merge/height contraction) + full drain with verifyInvariants()
//   4. Boundary guards (InvalidOrder, InvalidRange, empty range) & std.testing.allocator zero memory leaks
//   5. `import cap from "cap"; cap.bplus` dynamic mounting verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BPlusError,
    BPlusEntry,
    BPlusTree,
} from "cap:db";

test("cap:bplus - 1000 out-of-order inserts, multi-level splits & verifyInvariants") {
    const alloc = std.testing.allocator;
    let tree = BPlusTree.init(alloc, 4) catch unreachable;
    defer tree.deinit();

    expect_eq(tree.len(), 0);
    expect_eq(tree.height(), 0);
    expect_eq(tree.verifyInvariants(), true);
    expect_eq(tree.minEntry() == null, true);
    expect_eq(tree.maxEntry() == null, true);

    let key_buf: [16]u8 = undefined;
    let val_buf: [16]u8 = undefined;

    // 263 is coprime to 1000 -> visits every integer in 0..1000 in pseudo-random order
    for (step of 0..1000) {
        const id = (step * 263 + 17) % 1000;
        const k = std.fmt.bufPrint(key_buf[0..], "k:{d:0>4}", .{id}) catch unreachable;
        const v = std.fmt.bufPrint(val_buf[0..], "v:{d:0>4}", .{id}) catch unreachable;
        const inserted = tree.put(k, v) catch unreachable;
        expect_eq(inserted, true);

        if (step % 50 == 0 || step == 999) {
            expect_eq(tree.verifyInvariants(), true);
        }
    }

    expect_eq(tree.len(), 1000);
    expect_eq(tree.height() >= 3, true);
    expect_eq(tree.verifyInvariants(), true);

    // 100% get() hit verification
    for (id of 0..1000) {
        const k = std.fmt.bufPrint(key_buf[0..], "k:{d:0>4}", .{id}) catch unreachable;
        const expected_v = std.fmt.bufPrint(val_buf[0..], "v:{d:0>4}", .{id}) catch unreachable;
        const got = tree.get(k);
        expect_eq(got != null, true);
        expect_eq(std.mem.eql(u8, got.?, expected_v), true);
    }
    expect_eq(tree.get("k:9999") == null, true);

    const min_e = tree.minEntry().?;
    const max_e = tree.maxEntry().?;
    expect_eq(std.mem.eql(u8, min_e.key, "k:0000"), true);
    expect_eq(std.mem.eql(u8, max_e.key, "k:0999"), true);
}

test("cap:bplus - overwrite update, rangeScan boundaries, prefixScan & lowerBound") {
    const alloc = std.testing.allocator;
    let tree = BPlusTree.init(alloc, 4) catch unreachable;
    defer tree.deinit();

    // Insert mixed prefixes
    let key_buf: [24]u8 = undefined;
    let val_buf: [24]u8 = undefined;
    for (i of 0..30) {
        const k = std.fmt.bufPrint(key_buf[0..], "user:{d:0>3}", .{i}) catch unreachable;
        const v = std.fmt.bufPrint(val_buf[0..], "uval:{d}", .{i}) catch unreachable;
        expect_eq(tree.put(k, v) catch unreachable, true);
    }
    expect_eq(tree.put("admin:01", "root") catch unreachable, true);
    expect_eq(tree.put("zeta:99", "last") catch unreachable, true);

    // Overwrite existing key returns false and updates value
    expect_eq(tree.put("user:015", "updated_15") catch unreachable, false);
    expect_eq(tree.len(), 32);
    expect_eq(std.mem.eql(u8, tree.get("user:015").?, "updated_15"), true);
    expect_eq(tree.verifyInvariants(), true);

    // Inclusive rangeScan [user:010, user:015] -> 6 items
    const inc_res = tree.rangeScan(alloc, "user:010", "user:015", true) catch unreachable;
    defer alloc.free(inc_res);
    expect_eq(inc_res.len, 6);
    expect_eq(std.mem.eql(u8, inc_res[0].key, "user:010"), true);
    expect_eq(std.mem.eql(u8, inc_res[5].key, "user:015"), true);
    expect_eq(std.mem.eql(u8, inc_res[5].value, "updated_15"), true);

    // Exclusive rangeScan [user:010, user:015) -> 5 items
    const exc_res = tree.rangeScan(alloc, "user:010", "user:015", false) catch unreachable;
    defer alloc.free(exc_res);
    expect_eq(exc_res.len, 5);
    expect_eq(std.mem.eql(u8, exc_res[0].key, "user:010"), true);
    expect_eq(std.mem.eql(u8, exc_res[4].key, "user:014"), true);

    // prefixScan("user:01") -> user:010 .. user:019 (10 items)
    const pfx_res = tree.prefixScan(alloc, "user:01") catch unreachable;
    defer alloc.free(pfx_res);
    expect_eq(pfx_res.len, 10);
    expect_eq(std.mem.eql(u8, pfx_res[0].key, "user:010"), true);
    expect_eq(std.mem.eql(u8, pfx_res[9].key, "user:019"), true);

    // prefixScan with no match
    const empty_pfx = tree.prefixScan(alloc, "nope:") catch unreachable;
    defer alloc.free(empty_pfx);
    expect_eq(empty_pfx.len, 0);

    // lowerBound checks
    const lb1 = tree.lowerBound("user:015").?;
    expect_eq(std.mem.eql(u8, lb1.key, "user:015"), true);
    const lb2 = tree.lowerBound("user:029z").?;
    expect_eq(std.mem.eql(u8, lb2.key, "zeta:99"), true);
    expect_eq(tree.lowerBound("zzzz") == null, true);
}

test("cap:bplus - 90% mass remove, borrow/merge, height contraction & full drain") {
    const alloc = std.testing.allocator;
    let tree = BPlusTree.init(alloc, 4) catch unreachable;
    defer tree.deinit();

    let key_buf: [16]u8 = undefined;
    let val_buf: [16]u8 = undefined;

    const total: usize = 500;
    for (step of 0..total) {
        const id = (step * 137 + 29) % total;
        const k = std.fmt.bufPrint(key_buf[0..], "item:{d:0>4}", .{id}) catch unreachable;
        const v = std.fmt.bufPrint(val_buf[0..], "val:{d:0>4}", .{id}) catch unreachable;
        * = tree.put(k, v) catch unreachable;
    }

    const peak_height = tree.height();
    expect_eq(peak_height >= 3, true);
    expect_eq(tree.verifyInvariants(), true);

    // Delete 90% of keys (keep only ids where id % 10 == 0)
    let removed_count: usize = 0;
    for (step of 0..total) {
        const id = (step * 193 + 7) % total;
        if (id % 10 != 0) {
            const k = std.fmt.bufPrint(key_buf[0..], "item:{d:0>4}", .{id}) catch unreachable;
            const ok = tree.remove(k);
            expect_eq(ok, true);
            removed_count += 1;

            if (removed_count % 25 == 0) {
                expect_eq(tree.verifyInvariants(), true);
            }
        }
    }

    expect_eq(removed_count, 450);
    expect_eq(tree.len(), 50);
    expect_eq(tree.height() < peak_height, true);
    expect_eq(tree.verifyInvariants(), true);

    // Removing non-existent key returns false
    expect_eq(tree.remove("item:0001"), false);

    // Remaining 10% keys are 100% queryable
    for (id of 0..total) {
        const k = std.fmt.bufPrint(key_buf[0..], "item:{d:0>4}", .{id}) catch unreachable;
        const got = tree.get(k);
        if (id % 10 == 0) {
            const expected_v = std.fmt.bufPrint(val_buf[0..], "val:{d:0>4}", .{id}) catch unreachable;
            expect_eq(got != null, true);
            expect_eq(std.mem.eql(u8, got.?, expected_v), true);
        } else {
            expect_eq(got == null, true);
        }
    }

    // Drain remaining 10% completely
    for (id of 0..total) {
        if (id % 10 == 0) {
            const k = std.fmt.bufPrint(key_buf[0..], "item:{d:0>4}", .{id}) catch unreachable;
            expect_eq(tree.remove(k), true);
            expect_eq(tree.verifyInvariants(), true);
        }
    }
    expect_eq(tree.len(), 0);
    expect_eq(tree.height(), 0);
    expect_eq(tree.minEntry() == null, true);
    expect_eq(tree.maxEntry() == null, true);
}

test("cap:bplus - InvalidOrder, InvalidRange, empty range & zero-leak check") {
    const alloc = std.testing.allocator;

    // InvalidOrder when order < 3
    let bad_order_0 = false;
    * = BPlusTree.init(alloc, 0) catch |e| {
        if (e == BPlusError.InvalidOrder) bad_order_0 = true;
    };
    expect_eq(bad_order_0, true);

    let bad_order_2 = false;
    * = BPlusTree.init(alloc, 2) catch |e| {
        if (e == BPlusError.InvalidOrder) bad_order_2 = true;
    };
    expect_eq(bad_order_2, true);

    // Order = 3 (minimum valid odd order) works cleanly
    let tree = BPlusTree.init(alloc, 3) catch unreachable;
    defer tree.deinit();

    // Empty tree rangeScan returns empty slice
    const empty_scan = tree.rangeScan(alloc, "a", "z", true) catch unreachable;
    defer alloc.free(empty_scan);
    expect_eq(empty_scan.len, 0);

    * = tree.put("b", "2") catch unreachable;
    * = tree.put("d", "4") catch unreachable;

    // InvalidRange when start_key > end_key
    let bad_range = false;
    * = tree.rangeScan(alloc, "z", "a", true) catch |e| {
        if (e == BPlusError.InvalidRange) bad_range = true;
    };
    expect_eq(bad_range, true);

    // Exclusive equal range [b, b) -> empty slice
    const eq_exc = tree.rangeScan(alloc, "b", "b", false) catch unreachable;
    defer alloc.free(eq_exc);
    expect_eq(eq_exc.len, 0);

    // Disjoint range [e, f] -> empty slice
    const disjoint = tree.rangeScan(alloc, "e", "f", true) catch unreachable;
    defer alloc.free(disjoint);
    expect_eq(disjoint.len, 0);
}

test("cap.bplus - dynamic mounting via import cap from \"cap\"") {
    const alloc = std.testing.allocator;
    let tree = cap.db.BPlusTree.init(alloc, 5) catch unreachable;
    defer tree.deinit();

    expect_eq(tree.put("alpha", "1") catch unreachable, true);
    expect_eq(tree.put("beta", "2") catch unreachable, true);
    expect_eq(tree.put("gamma", "3") catch unreachable, true);
    expect_eq(tree.verifyInvariants(), true);

    const res = tree.rangeScan(alloc, "alpha", "gamma", true) catch unreachable;
    defer alloc.free(res);
    expect_eq(res.len, 3);
    expect_eq(std.mem.eql(u8, res[1].key, "beta"), true);
    expect_eq(std.mem.eql(u8, res[1].value, "2"), true);
}
