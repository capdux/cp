// Source: cp/cap/test/test_zset_production_deep.cp
// Deep Production Verification Suite for cap:zset & cap.zset
// Covers:
//   1. zadd, zincrby, lexicographical tie-breaking on equal scores, NaN InvalidScore guard & re-sort on update
//   2. zrank / zrevrank (O(log N) span verification) & zrange(0, -1) / zrevrange(0, 2) negative slicing
//   3. zrangeByScore, zcount, zremRangeByScore & zpopMin/zpopMax consistency on 500-entry dataset
//   4. unionInto (Sum / Min / Max) aggregation strategies & clean memory deallocation
//   5. `import cap from "cap"; cap.zset` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ZSetError,
    ZSetEntry,
    ZSetAgg,
    SortedSet,
} from "cap:ds";

test("cap:zset - zadd, zincrby, tie-breaking, NaN guard & update re-sort") {
    const alloc = std.testing.allocator;
    let zs = SortedSet.init(alloc);
    defer zs.deinit();

    // Empty checks
    expect_eq(zs.zcard(), 0);
    expect_eq(zs.zscore("missing") == null, true);
    expect_eq(zs.zrem("missing"), false);

    // Equal scores -> lexicographical tie-breaking: "alpha" < "beta" < "gamma"
    expect_eq(zs.zadd("gamma", 100.0) catch unreachable, true);
    expect_eq(zs.zadd("alpha", 100.0) catch unreachable, true);
    expect_eq(zs.zadd("beta", 100.0) catch unreachable, true);
    expect_eq(zs.zcard(), 3);

    // Duplicate zadd with same score returns false
    expect_eq(zs.zadd("beta", 100.0) catch unreachable, false);
    expect_eq(zs.zcard(), 3);

    expect_eq(zs.zrank("alpha").?, 0);
    expect_eq(zs.zrank("beta").?, 1);
    expect_eq(zs.zrank("gamma").?, 2);

    // Update score -> re-sorts SkipList
    expect_eq(zs.zadd("gamma", 50.0) catch unreachable, false);
    expect_eq(zs.zrank("gamma").?, 0);
    expect_eq(zs.zrank("alpha").?, 1);
    expect_eq(zs.zrank("beta").?, 2);

    // zincrby on existing and new member
    const s_gamma = zs.zincrby("gamma", 75.0) catch unreachable;
    expect_eq(s_gamma, 125.0);
    expect_eq(zs.zrank("gamma").?, 2);

    const s_delta = zs.zincrby("delta", 10.5) catch unreachable;
    expect_eq(s_delta, 10.5);
    expect_eq(zs.zrank("delta").?, 0);
    expect_eq(zs.zscore("delta").?, 10.5);

    // NaN rejection
    let nan_caught = false;
    zs.zadd("bad", std.math.nan(f64)) catch |e| {
        if (e == ZSetError.InvalidScore) nan_caught = true;
    };
    expect_eq(nan_caught, true);

    let nan_incr_caught = false;
    zs.zincrby("alpha", std.math.nan(f64)) catch |e| {
        if (e == ZSetError.InvalidScore) nan_incr_caught = true;
    };
    expect_eq(nan_incr_caught, true);
}

test("cap:zset - zrank/zrevrank span & zrange/zrevrange negative slicing") {
    const alloc = std.testing.allocator;
    let zs = SortedSet.init(alloc);
    defer zs.deinit();

    const members = ["m0", "m1", "m2", "m3", "m4", "m5"];
    const scores = [50.0, 10.0, 40.0, 20.0, 60.0, 30.0];
    for (i of 0..members.len) {
        let * = zs.zadd(members[i], scores[i]) catch unreachable;
    }

    // Sorted order: m1(10), m3(20), m5(30), m2(40), m0(50), m4(60)
    expect_eq(zs.zrank("m1").?, 0);
    expect_eq(zs.zrank("m3").?, 1);
    expect_eq(zs.zrank("m5").?, 2);
    expect_eq(zs.zrank("m2").?, 3);
    expect_eq(zs.zrank("m0").?, 4);
    expect_eq(zs.zrank("m4").?, 5);

    expect_eq(zs.zrevrank("m4").?, 0);
    expect_eq(zs.zrevrank("m0").?, 1);
    expect_eq(zs.zrevrank("m1").?, 5);
    expect_eq(zs.zrank("none") == null, true);
    expect_eq(zs.zrevrank("none") == null, true);

    // zrange(0, -1) full ascending slice
    const full = zs.zrange(alloc, 0, -1) catch unreachable;
    defer alloc.free(full);
    expect_eq(full.len, 6);
    expect_eq(std.mem.eql(u8, full[0].member, "m1"), true);
    expect_eq(std.mem.eql(u8, full[5].member, "m4"), true);

    // zrange(-2, -1) last two ascending
    const tail2 = zs.zrange(alloc, -2, -1) catch unreachable;
    defer alloc.free(tail2);
    expect_eq(tail2.len, 2);
    expect_eq(std.mem.eql(u8, tail2[0].member, "m0"), true);
    expect_eq(std.mem.eql(u8, tail2[1].member, "m4"), true);

    // zrevrange(0, 2) top 3 descending
    const top3 = zs.zrevrange(alloc, 0, 2) catch unreachable;
    defer alloc.free(top3);
    expect_eq(top3.len, 3);
    expect_eq(std.mem.eql(u8, top3[0].member, "m4"), true);
    expect_eq(top3[0].score, 60.0);
    expect_eq(std.mem.eql(u8, top3[1].member, "m0"), true);
    expect_eq(std.mem.eql(u8, top3[2].member, "m2"), true);

    // Inverted/out-of-bounds range returns empty slice
    const empty_r = zs.zrange(alloc, 4, 2) catch unreachable;
    defer alloc.free(empty_r);
    expect_eq(empty_r.len, 0);

    // zrem updates span & ranks accurately
    expect_eq(zs.zrem("m5"), true);
    expect_eq(zs.zcard(), 5);
    expect_eq(zs.zrank("m2").?, 2);
    expect_eq(zs.zrank("m4").?, 4);
}

test("cap:zset - 500-entry zrangeByScore, zcount, zremRangeByScore & zpopMin/zpopMax") {
    const alloc = std.testing.allocator;
    let zs = SortedSet.init(alloc);
    defer zs.deinit();

    // Insert 500 items with shuffled pseudo-random scores in [0, 499]
    let i: usize = 0;
    while (i < 500) : (i += 1) {
        const perm = (i * 137 + 29) % 500;
        const name = std.fmt.allocPrint(alloc, "item_{d:0>3}", .{perm}) catch unreachable;
        defer alloc.free(name);
        expect_eq(zs.zadd(name, perm as f64) catch unreachable, true);
    }
    expect_eq(zs.zcard(), 500);

    // Verify zcount & zrangeByScore on [100.0, 199.0]
    expect_eq(zs.zcount(100.0, 199.0), 100);
    const slice_100 = zs.zrangeByScore(alloc, 100.0, 199.0) catch unreachable;
    defer alloc.free(slice_100);
    expect_eq(slice_100.len, 100);
    expect_eq(slice_100[0].score, 100.0);
    expect_eq(slice_100[99].score, 199.0);

    // Verify zrank across 500 items
    expect_eq(zs.zrank("item_000").?, 0);
    expect_eq(zs.zrank("item_250").?, 250);
    expect_eq(zs.zrank("item_499").?, 499);
    expect_eq(zs.zrevrank("item_499").?, 0);

    // Remove [100.0, 199.0] -> 400 remain
    const removed = zs.zremRangeByScore(100.0, 199.0);
    expect_eq(removed, 100);
    expect_eq(zs.zcard(), 400);
    expect_eq(zs.zcount(100.0, 199.0), 0);
    expect_eq(zs.zrank("item_200").?, 100);

    // zpopMin(10) -> pops 0..9
    const mins = zs.zpopMin(alloc, 10) catch unreachable;
    defer SortedSet.freeEntries(alloc, mins);
    expect_eq(mins.len, 10);
    expect_eq(mins[0].score, 0.0);
    expect_eq(mins[9].score, 9.0);
    expect_eq(zs.zcard(), 390);

    // zpopMax(10) -> pops 499..490
    const maxs = zs.zpopMax(alloc, 10) catch unreachable;
    defer SortedSet.freeEntries(alloc, maxs);
    expect_eq(maxs.len, 10);
    expect_eq(maxs[0].score, 499.0);
    expect_eq(maxs[9].score, 490.0);
    expect_eq(zs.zcard(), 380);
    expect_eq(zs.zrank("item_010").?, 0);
    expect_eq(zs.zrevrank("item_489").?, 0);
}

test("cap:zset - unionInto (Sum / Min / Max) & clean memory release") {
    const alloc = std.testing.allocator;
    let a = SortedSet.init(alloc);
    defer a.deinit();
    let b = SortedSet.init(alloc);
    defer b.deinit();

    let * = a.zadd("alice", 10.0) catch unreachable;
    let * = a.zadd("bob", 20.0) catch unreachable;

    let * = b.zadd("bob", 5.0) catch unreachable;
    let * = b.zadd("carol", 30.0) catch unreachable;

    const inputs: [2]*SortedSet = [&a, &b];

    // Sum
    let u_sum = SortedSet.unionInto(alloc, inputs[0..], ZSetAgg.Sum) catch unreachable;
    defer u_sum.deinit();
    expect_eq(u_sum.zcard(), 3);
    expect_eq(u_sum.zscore("alice").?, 10.0);
    expect_eq(u_sum.zscore("bob").?, 25.0);
    expect_eq(u_sum.zscore("carol").?, 30.0);

    // Min
    let u_min = SortedSet.unionInto(alloc, inputs[0..], ZSetAgg.Min) catch unreachable;
    defer u_min.deinit();
    expect_eq(u_min.zscore("bob").?, 5.0);
    expect_eq(u_min.zrank("bob").?, 0);

    // Max
    let u_max = SortedSet.unionInto(alloc, inputs[0..], ZSetAgg.Max) catch unreachable;
    defer u_max.deinit();
    expect_eq(u_max.zscore("bob").?, 20.0);
    expect_eq(u_max.zrank("bob").?, 1);
}

test("cap.zset - dynamic mounting via import cap from \"cap\"") {
    const alloc = std.testing.allocator;
    let zs = cap.ds.SortedSet.init(alloc);
    defer zs.deinit();

    expect_eq(zs.zadd("task_b", 200.0) catch unreachable, true);
    expect_eq(zs.zadd("task_a", 100.0) catch unreachable, true);
    expect_eq(zs.zcard(), 2);
    expect_eq(zs.zrank("task_a").?, 0);
    expect_eq(zs.zrank("task_b").?, 1);

    const popped = zs.zpopMin(alloc, 1) catch unreachable;
    defer cap.ds.SortedSet.freeEntries(alloc, popped);
    expect_eq(popped.len, 1);
    expect_eq(std.mem.eql(u8, popped[0].member, "task_a"), true);
    expect_eq(popped[0].score, 100.0);
}
