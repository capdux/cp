// Source: cp/cap/test/test_consistent_jump_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    JumpError,
    WeightedNode,
    jumpConsistentHash,
    jumpConsistentHashBytes,
    hrwSelect,
    hrwSelectWeighted,
} from "cap:hash";

test "consistent_jump: Google Jump Hash monotonicity & minimal movement (10->11 buckets)" {
    let moved: usize = 0;
    let i: u64 = 0;
    while (i < 1100) : (i += 1) {
        const b10 = try jumpConsistentHash(i, 10);
        const b11 = try jumpConsistentHash(i, 11);
        try expect_eq(b10 < 10 and b11 < 11, true);
        if (b10 != b11) {
            // Monotonic property: moved keys MUST only land in new bucket 10
            try expect_eq(b11, 10);
            moved += 1;
        }
    }
    // Expected ~100 (1/11), allow [60, 150]
    try expect_eq(moved >= 60 and moved <= 150, true);
}

test "consistent_jump: uniform distribution across 8 shards" {
    let counts: [8]usize = [ 0, 0, 0, 0, 0, 0, 0, 0 ];
    let key_buf: [24]u8 = undefined;
    let i: usize = 0;
    while (i < 1600) : (i += 1) {
        const k = try std.fmt.bufPrint(&key_buf, "user:{d}", .{i});
        const b = try jumpConsistentHashBytes(k, 8);
        const b_idx: usize = @intCast(b);
        counts[b_idx] += 1;
    }
    for (c of counts) {
        try expect_eq(c >= 140 and c <= 260, true);
    }
}

test "consistent_jump: Rendezvous HRW minimal disruption on node removal" {
    const nodes3: [3][]const u8 = [ "n1", "n2", "n3" ];
    const nodes2: [2][]const u8 = [ "n1", "n3" ];

    let i: usize = 0;
    let key_buf: [24]u8 = undefined;
    while (i < 300) : (i += 1) {
        const k = try std.fmt.bufPrint(&key_buf, "obj:{d}", .{i});
        const sel3 = try hrwSelect(k, &nodes3);
        const sel2 = try hrwSelect(k, &nodes2);
        // If sel3 wasn't the removed node "n2", it MUST remain unchanged!
        if (!std.mem.eql(u8, sel3, "n2")) {
            try expect_eq(std.mem.eql(u8, sel3, sel2), true);
        }
    }
}

test "consistent_jump: weighted HRW respects 3:1 capacity ratio & error guards" {
    const nodes: [2]WeightedNode = [
        WeightedNode{ .id = "heavy", .weight = 3.0 },
        WeightedNode{ .id = "light", .weight = 1.0 },
    ];
    let heavy_cnt: usize = 0;
    let key_buf: [24]u8 = undefined;
    let i: usize = 0;
    while (i < 1000) : (i += 1) {
        const k = try std.fmt.bufPrint(&key_buf, "req:{d}", .{i});
        const sel = try hrwSelectWeighted(k, &nodes);
        if (std.mem.eql(u8, sel, "heavy")) { heavy_cnt += 1; }
    }
    // Expect ~750
    try expect_eq(heavy_cnt >= 680 and heavy_cnt <= 820, true);

    try expect_eq(jumpConsistentHash(1, 0) == JumpError.InvalidBucketCount, true);
    const empty: [0][]const u8 = [];
    try expect_eq(hrwSelect("k", &empty) == JumpError.EmptyNodes, true);
}

test "consistent_jump: cap.consistent_jump dynamic mount" {
    const b = try cap.consistent_jump.jumpConsistentHash(42, 1);
    try expect_eq(b, 0);
}
