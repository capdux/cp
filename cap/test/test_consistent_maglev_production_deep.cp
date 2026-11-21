// Source: native-cp-stdlib (Google Maglev Consistent Hashing)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DEFAULT_TABLE_SIZE,
    MaglevError,
    Backend,
    MaglevRouter,
} from "cap:hash";

test "maglev 3-backend O(1) lookup balance (>25% each)" {
    let router = try MaglevRouter.init(std.testing.allocator, 997);
    defer router.deinit();

    try router.addBackend("node-a", 1);
    try router.addBackend("node-b", 1);
    try router.addBackend("node-c", 1);

    try expect_eq(router.backendCount(), 3);
    try expect_eq(router.tableSize(), 997);

    let count_a: usize = 0;
    let count_b: usize = 0;
    let count_c: usize = 0;
    let buf: [32]u8 = undefined;

    for (i of 0..3000) {
        const key = try std.fmt.bufPrint(&buf, "req-key-{d}", .{i});
        const target = try router.lookup(key);
        if (std.mem.eql(u8, target, "node-a")) {
            count_a += 1;
        } else if (std.mem.eql(u8, target, "node-b")) {
            count_b += 1;
        } else if (std.mem.eql(u8, target, "node-c")) {
            count_c += 1;
        }
    }

    try expect_eq(count_a + count_b + count_c, 3000);
    try expect_eq(count_a > 750, true);
    try expect_eq(count_b > 750, true);
    try expect_eq(count_c > 750, true);
}

test "maglev weighted routing 2:1 ratio verification" {
    let router = try MaglevRouter.init(std.testing.allocator, DEFAULT_TABLE_SIZE);
    defer router.deinit();

    try router.addBackend("heavy-node", 2);
    try router.addBackend("light-node", 1);

    let heavy_count: usize = 0;
    let light_count: usize = 0;
    let buf: [32]u8 = undefined;

    for (i of 0..3000) {
        const key = try std.fmt.bufPrint(&buf, "weighted-key-{d}", .{i});
        const target = try router.lookup(key);
        if (std.mem.eql(u8, target, "heavy-node")) {
            heavy_count += 1;
        } else if (std.mem.eql(u8, target, "light-node")) {
            light_count += 1;
        }
    }

    try expect_eq(heavy_count + light_count, 3000);
    // Expected ~2000 heavy vs ~1000 light (ratio ~2.0)
    try expect_eq(heavy_count > 1800, true);
    try expect_eq(light_count > 850, true);
    try expect_eq(heavy_count > light_count, true);
}

test "maglev minimal key disruption on backend add and remove" {
    let router = try MaglevRouter.init(std.testing.allocator, 997);
    defer router.deinit();

    try router.addBackend("srv-1", 1);
    try router.addBackend("srv-2", 1);
    try router.addBackend("srv-3", 1);

    const num_keys: usize = 1000;
    let initial_owners = try std.testing.allocator.alloc(u8, num_keys);
    defer std.testing.allocator.free(initial_owners);

    let buf: [32]u8 = undefined;
    for (i of 0..num_keys) {
        const key = try std.fmt.bufPrint(&buf, "session-{d}", .{i});
        const target = try router.lookup(key);
        initial_owners[i] = target[target.len - 1];
    }

    // Add 4th backend: ~25% disruption expected, strictly < 40%
    try router.addBackend("srv-4", 1);
    let moved_on_add: usize = 0;
    for (i of 0..num_keys) {
        const key = try std.fmt.bufPrint(&buf, "session-{d}", .{i});
        const target = try router.lookup(key);
        if (target[target.len - 1] != initial_owners[i]) {
            moved_on_add += 1;
        }
    }
    try expect_eq(moved_on_add < 400, true);

    // Remove 4th backend: should restore 100% of original mappings
    try router.removeBackend("srv-4");
    let diff_after_restore: usize = 0;
    for (i of 0..num_keys) {
        const key = try std.fmt.bufPrint(&buf, "session-{d}", .{i});
        const target = try router.lookup(key);
        if (target[target.len - 1] != initial_owners[i]) {
            diff_after_restore += 1;
        }
    }
    try expect_eq(diff_after_restore, 0);
}

test "maglev error handling: empty, non-prime, duplicate, not found" {
    try std.testing.expectError(
        MaglevError.InvalidTableSize,
        MaglevRouter.init(std.testing.allocator, 100),
    );

    let router = try MaglevRouter.init(std.testing.allocator, 997);
    defer router.deinit();

    try std.testing.expectError(MaglevError.EmptyBackends, router.lookup("k1"));
    try std.testing.expectError(MaglevError.EmptyBackends, router.rebuild());

    try router.addBackend("alpha", 1);
    try std.testing.expectError(MaglevError.DuplicateBackend, router.addBackend("alpha", 2));
    try std.testing.expectError(MaglevError.BackendNotFound, router.removeBackend("non-existent"));

    try router.removeBackend("alpha");
    try expect_eq(router.backendCount(), 0);
    try std.testing.expectError(MaglevError.EmptyBackends, router.lookup("k1"));
}

test "maglev cap.consistent_maglev dynamic mount and zero-leak check" {
    let router = try cap.consistent_maglev.MaglevRouter.init(
        std.testing.allocator,
        cap.consistent_maglev.DEFAULT_TABLE_SIZE,
    );
    defer router.deinit();

    try router.addBackend("dyn-1", 1);
    try router.addBackend("dyn-2", 1);
    try router.rebuild();

    const target = try router.lookup("dynamic-mount-key");
    const is_valid = std.mem.eql(u8, target, "dyn-1") || std.mem.eql(u8, target, "dyn-2");
    try expect_eq(is_valid, true);
}
