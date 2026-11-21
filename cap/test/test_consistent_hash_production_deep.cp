// Source: cp/cap/test/test_consistent_hash_production_deep.cp
const std = @import("std");
type StrSlice = []const u8;
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { ConsistentHashError, HashRing } from "cap:consistent_hash";

test "consistent_hash: deterministic routing & load balance across 3 nodes" {
    const alloc = std.testing.allocator;
    let ring = try HashRing.init(alloc, 100);
    defer ring.deinit();

    try ring.addNode("node-a");
    try ring.addNode("node-b");
    try ring.addNode("node-c");
    try expect_eq(ring.nodeCount(), 3);

    let counts: [3]usize = [ 0, 0, 0 ];
    let i: usize = 0;
    let key_buf: [32]u8 = undefined;
    while (i < 600) : (i += 1) {
        const k = try std.fmt.bufPrint(&key_buf, "key:{d}", .{i});
        const owner = try ring.getNode(k);
        if (std.mem.eql(u8, owner, "node-a")) { counts[0] += 1; }
        else if (std.mem.eql(u8, owner, "node-b")) { counts[1] += 1; }
        else if (std.mem.eql(u8, owner, "node-c")) { counts[2] += 1; }
    }
    try expect_eq(counts[0] + counts[1] + counts[2], 600);
    // Each node should receive a reasonable share (> 100 out of 600)
    try expect_eq(counts[0] > 100 and counts[1] > 100 and counts[2] > 100, true);
}

test "consistent_hash: minimal key migration on node addition" {
    const alloc = std.testing.allocator;
    let ring = try HashRing.init(alloc, 128);
    defer ring.deinit();
    try ring.addNode("n1");
    try ring.addNode("n2");
    try ring.addNode("n3");

    let moved: usize = 0;
    let before = std.ArrayList(StrSlice).empty;
    defer {
        for (s of before.items) { alloc.free(s); }
        before.deinit(alloc);
    }

    let key_buf: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 300) : (i += 1) {
        const k = try std.fmt.bufPrint(&key_buf, "item-{d}", .{i});
        const owner = try ring.getNode(k);
        try before.append(alloc, try alloc.dupe(u8, owner));
    }

    try ring.addNode("n4");
    i = 0;
    while (i < 300) : (i += 1) {
        const k = try std.fmt.bufPrint(&key_buf, "item-{d}", .{i});
        const owner = try ring.getNode(k);
        if (!std.mem.eql(u8, owner, before.items[i])) {
            moved += 1;
        }
    }
    // Expect roughly ~25% (75) moved, definitely < 45% (135)
    try expect_eq(moved > 0 and moved < 135, true);
}

test "consistent_hash: getReplicas returns distinct nodes" {
    const alloc = std.testing.allocator;
    let ring = try HashRing.init(alloc, 64);
    defer ring.deinit();
    try ring.addNode("r1");
    try ring.addNode("r2");
    try ring.addNode("r3");

    const reps = try ring.getReplicas(alloc, "user:99", 3);
    defer alloc.free(reps);
    try expect_eq(reps.len, 3);
    try expect_eq(std.mem.eql(u8, reps[0], reps[1]), false);
    try expect_eq(std.mem.eql(u8, reps[1], reps[2]), false);
    try expect_eq(std.mem.eql(u8, reps[0], reps[2]), false);
}

test "consistent_hash: removeNode and error guards" {
    const alloc = std.testing.allocator;
    try expect_eq(HashRing.init(alloc, 0) == ConsistentHashError.InvalidVirtualNodes, true);

    let ring = try HashRing.init(alloc, 16);
    defer ring.deinit();
    try expect_eq(ring.getNode("k") == ConsistentHashError.EmptyRing, true);
    try expect_eq(ring.removeNode("missing") == ConsistentHashError.NodeNotFound, true);

    try ring.addNode("only");
    try expect_eq(std.mem.eql(u8, try ring.getNode("k"), "only"), true);
    try ring.removeNode("only");
    try expect_eq(ring.nodeCount(), 0);
}

test "consistent_hash: cap.consistent_hash dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let ring = try cap.consistent_hash.HashRing.init(alloc, 32);
    defer ring.deinit();
    try ring.addNode("shard-0");
    try expect_eq(std.mem.eql(u8, try ring.getNode("abc"), "shard-0"), true);
}
