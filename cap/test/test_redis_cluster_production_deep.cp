// Source: cp/cap/test/test_redis_cluster_production_deep.cp (Deep Production Tests for cap:redis_cluster)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    REDIS_CLUSTER_SLOTS,
    crc16Ccitt,
    extractHashTag,
    keySlot,
    ensureSameSlot,
    ClusterError,
    NodeEndpoint,
    RedirectKind,
    ClusterTopology,
    parseRedirectError,
} from "cap:db";

test("redis_cluster crc16 official vector, hash tag edge cases and keySlot") {
    try expect_eq(@as(u16, 16384), REDIS_CLUSTER_SLOTS);
    try expect_eq(@as(u16, 0x31C3), crc16Ccitt("123456789"));

    // Standard hash tag
    try std.testing.expectEqualStrings("bar", extractHashTag("foo{bar}"));
    try std.testing.expectEqualStrings("bar", extractHashTag("baz{bar}"));
    try expect_eq(keySlot("foo{bar}"), keySlot("baz{bar}"));

    // Edge cases: empty {}, multiple {a}{b}, nested {{a}, unclosed {abc
    try std.testing.expectEqualStrings("foo{}bar", extractHashTag("foo{}bar"));
    try std.testing.expectEqualStrings("a", extractHashTag("{a}{b}"));
    try std.testing.expectEqualStrings("{a", extractHashTag("{{a}"));
    try std.testing.expectEqualStrings("abc{def", extractHashTag("abc{def"));
}

test("redis_cluster topology 3-master full coverage, O(1) routing and bounds check") {
    let topo = ClusterTopology.init(std.testing.allocator);
    defer topo.deinit();

    const empty_replicas: [0]NodeEndpoint = [];
    const n1 = NodeEndpoint.init("10.0.0.1", 7001);
    const n2 = NodeEndpoint.init("10.0.0.2", 7002);
    const n3 = NodeEndpoint.init("10.0.0.3", 7003);

    try topo.addSlotRange(0, 5460, n1, &empty_replicas);
    try topo.addSlotRange(5461, 10922, n2, &empty_replicas);
    try std.testing.expect(!topo.isFullCoverage());

    try topo.addSlotRange(10923, 16383, n3, &empty_replicas);
    try std.testing.expect(topo.isFullCoverage());
    try expect_eq(@as(u32, 16384), topo.coveredSlots());

    const r0 = try topo.routeSlot(0);
    const r1 = try topo.routeSlot(5461);
    const r2 = try topo.routeSlot(16383);
    try std.testing.expect(r0.equals(n1));
    try std.testing.expect(r1.equals(n2));
    try std.testing.expect(r2.equals(n3));

    const slot_foo = keySlot("foo{bar}");
    const routed_foo = try topo.routeKey("foo{bar}");
    const routed_baz = try topo.routeKey("baz{bar}");
    try std.testing.expect(routed_foo.equals(routed_baz));
    * = slot_foo;

    // Bounds & invalid range errors
    try std.testing.expectError(ClusterError.InvalidSlot, topo.routeSlot(16384));
    try std.testing.expectError(ClusterError.InvalidSlotRange, topo.addSlotRange(100, 50, n1, &empty_replicas));
    try std.testing.expectError(ClusterError.InvalidSlotRange, topo.addSlotRange(0, 16384, n1, &empty_replicas));
}

test("redis_cluster parseRedirectError (MOVED/ASK/IPv6/ClusterErrors) and applyMovedRedirect") {
    const moved = try parseRedirectError("-MOVED 3999 127.0.0.1:7001\r\n");
    try expect_eq(RedirectKind.Moved, moved.kind);
    try expect_eq(@as(u16, 3999), moved.slot);
    try std.testing.expect(moved.endpoint != null);
    try std.testing.expectEqualStrings("127.0.0.1", moved.endpoint.?.host);
    try expect_eq(@as(u16, 7001), moved.endpoint.?.port);

    const ask_v6 = try parseRedirectError("ASK 12182 [::1]:7002");
    try expect_eq(RedirectKind.Ask, ask_v6.kind);
    try expect_eq(@as(u16, 12182), ask_v6.slot);
    try std.testing.expectEqualStrings("::1", ask_v6.endpoint.?.host);
    try expect_eq(@as(u16, 7002), ask_v6.endpoint.?.port);

    const down = try parseRedirectError("-CLUSTERDOWN The cluster is down");
    try expect_eq(RedirectKind.ClusterDown, down.kind);
    const try_again = try parseRedirectError("TRYAGAIN Multiple keys request during rehashing");
    try expect_eq(RedirectKind.TryAgain, try_again.kind);
    const cross = try parseRedirectError("CROSSSLOT Keys in request don't hash to the same slot");
    try expect_eq(RedirectKind.CrossSlot, cross.kind);

    // Hot-update routing table via applyMovedRedirect
    let topo = ClusterTopology.init(std.testing.allocator);
    defer topo.deinit();
    const empty_replicas: [0]NodeEndpoint = [];
    try topo.addSlotRange(0, 16383, NodeEndpoint.init("127.0.0.1", 7001), &empty_replicas);
    try topo.applyMovedRedirect(ask_v6.slot, ask_v6.endpoint.?.host, ask_v6.endpoint.?.port);
    const updated = try topo.routeSlot(12182);
    try std.testing.expectEqualStrings("::1", updated.host);
    try expect_eq(@as(u16, 7002), updated.port);
}

test("redis_cluster ensureSameSlot multi-key consistency") {
    const same_keys: [3][]const u8 = [
        "user:{42}:name",
        "user:{42}:email",
        "{42}",
    ];
    const slot = try ensureSameSlot(&same_keys);
    try expect_eq(keySlot("{42}"), slot);

    const cross_keys: [2][]const u8 = [
        "user:{42}:name",
        "user:{43}:name",
    ];
    try std.testing.expectError(error.CrossSlot, ensureSameSlot(&cross_keys));
}

test("redis_cluster cap.redis_cluster dynamic mount and zero-leak check") {
    const slot = cap.db.keySlot("session:{abc}:token");
    try expect_eq(keySlot("session:{abc}:token"), slot);

    let topo = cap.db.ClusterTopology.init(std.testing.allocator);
    defer topo.deinit();
    const empty_replicas: [0]cap.db.NodeEndpoint = [];
    try topo.addSlotRange(0, 16383, cap.db.NodeEndpoint.init("127.0.0.1", 6379), &empty_replicas);
    const ep = try topo.routeKey("session:{abc}:token");
    try std.testing.expectEqualStrings("127.0.0.1", ep.host);
    try expect_eq(@as(u16, 6379), ep.port);
}
