// Source: cp/cap/test/test_radix_production_deep.cp
// Deep Production Verification Suite for cap:radix & cap.radix
// Covers:
//   1. Insert, prefix split, exact get/contains, DuplicateKey, upsert & empty-key "" boundary
//   2. Delete & automatic single-child merge compression (nodeCount & verifyInvariants)
//   3. longestPrefix (CIDR / rule matching) & lexicographical scanPrefix
//   4. RadixRouter static, :param, *wildcard routes, priority resolution & parameter extraction
//   5. `import cap from "cap"; cap.radix` dynamic mounting & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RadixError,
    RadixMatch,
    RadixEntry,
    RadixTree,
    RouteParam,
    RouteMatch,
    RadixRouter,
} from "cap:radix";

test("cap:radix - insert, split, exact get, DuplicateKey, upsert & empty-key boundary") {
    const alloc = std.testing.allocator;
    let tree = RadixTree<u32>.init(alloc);
    defer tree.deinit();

    expect_eq(tree.isEmpty(), true);
    expect_eq(tree.count(), 0);
    expect_eq(tree.verifyInvariants(), true);

    // Empty-string key boundary
    tree.insert("", 999) catch unreachable;
    expect_eq(tree.isEmpty(), false);
    expect_eq(tree.count(), 1);
    expect_eq(tree.get("").?, 999);
    expect_eq(tree.contains(""), true);

    // Common-prefix splitting: "romane", "romanus", "romulus", "rubens", "ruber", "rubicon", "rubicundus"
    tree.insert("romane", 1) catch unreachable;
    tree.insert("romanus", 2) catch unreachable;
    tree.insert("romulus", 3) catch unreachable;
    tree.insert("rubens", 4) catch unreachable;
    tree.insert("ruber", 5) catch unreachable;
    tree.insert("rubicon", 6) catch unreachable;
    tree.insert("rubicundus", 7) catch unreachable;
    // Also insert a prefix of an existing edge ("rom") to test mid-edge split-terminal
    tree.insert("rom", 8) catch unreachable;

    expect_eq(tree.count(), 9);
    expect_eq(tree.verifyInvariants(), true);

    expect_eq(tree.get("romane").?, 1);
    expect_eq(tree.get("romanus").?, 2);
    expect_eq(tree.get("romulus").?, 3);
    expect_eq(tree.get("rubens").?, 4);
    expect_eq(tree.get("ruber").?, 5);
    expect_eq(tree.get("rubicon").?, 6);
    expect_eq(tree.get("rubicundus").?, 7);
    expect_eq(tree.get("rom").?, 8);
    expect_eq(tree.get("roman") == null, true);
    expect_eq(tree.get("missing") == null, true);

    // DuplicateKey on insert
    let dup_err = false;
    tree.insert("romane", 100) catch |e| {
        if (e == RadixError.DuplicateKey) dup_err = true;
    };
    expect_eq(dup_err, true);
    expect_eq(tree.get("romane").?, 1);

    // Upsert updates existing and inserts new
    tree.upsert("romane", 11) catch unreachable;
    tree.upsert("ruby", 12) catch unreachable;
    expect_eq(tree.get("romane").?, 11);
    expect_eq(tree.get("ruby").?, 12);
    expect_eq(tree.count(), 10);
    expect_eq(tree.verifyInvariants(), true);
}

test("cap:radix - delete & automatic single-child merge compression") {
    const alloc = std.testing.allocator;
    let tree = RadixTree<i64>.init(alloc);
    defer tree.deinit();

    // Build shared-prefix branches:
    // root("") -> "foo" (non-terminal) -> ["ba" -> ["r", "z"], "qux"]
    tree.insert("foobar", 10) catch unreachable;
    tree.insert("foobaz", 20) catch unreachable;
    tree.insert("fooqux", 30) catch unreachable;
    expect_eq(tree.count(), 3);
    expect_eq(tree.verifyInvariants(), true);
    const initial_nodes = tree.nodeCount();
    // root("") + "foo" + "ba" + "r" + "z" + "qux" = 6 nodes
    expect_eq(initial_nodes, 6);

    // Deleting "foobar" leaves "ba" with single child "z" -> merges into "baz" (5 nodes)
    expect_eq(tree.delete("foobar").?, 10);
    expect_eq(tree.count(), 2);
    expect_eq(tree.get("foobar") == null, true);
    expect_eq(tree.get("foobaz").?, 20);
    expect_eq(tree.verifyInvariants(), true);
    expect_eq(tree.nodeCount(), 4); // root("") + "foo" + "baz" + "qux" = 4 nodes

    // Deleting "fooqux" leaves "foo" with single child "baz" -> merges into "foobaz" (2 nodes)
    expect_eq(tree.delete("fooqux").?, 30);
    expect_eq(tree.count(), 1);
    expect_eq(tree.get("foobaz").?, 20);
    expect_eq(tree.verifyInvariants(), true);
    expect_eq(tree.nodeCount(), 2); // root("") + "foobaz" = 2 nodes

    // Deleting absent key returns null
    expect_eq(tree.delete("nope") == null, true);

    // Deleting last key leaves only root
    expect_eq(tree.delete("foobaz").?, 20);
    expect_eq(tree.isEmpty(), true);
    expect_eq(tree.nodeCount(), 1);
    expect_eq(tree.verifyInvariants(), true);

    // Clear and reuse
    tree.upsert("alpha", 1) catch unreachable;
    tree.clear();
    expect_eq(tree.isEmpty(), true);
    expect_eq(tree.nodeCount(), 0);
}

test("cap:radix - longestPrefix matching & lexicographical scanPrefix") {
    const alloc = std.testing.allocator;
    let tree = RadixTree<u32>.init(alloc);
    defer tree.deinit();

    // CIDR / route / rule prefixes in out-of-order insertion
    tree.insert("10.", 100) catch unreachable;
    tree.insert("192.168.1.", 300) catch unreachable;
    tree.insert("192.168.", 200) catch unreachable;
    tree.insert("192.168.1.50", 400) catch unreachable;
    tree.insert("172.16.", 500) catch unreachable;
    tree.insert("192.168.2.1", 600) catch unreachable;
    expect_eq(tree.verifyInvariants(), true);

    // longestPrefix checks
    const m1 = tree.longestPrefix("192.168.1.50").?;
    expect_eq(std.mem.eql(u8, m1.key, "192.168.1.50"), true);
    expect_eq(m1.value, 400);

    const m2 = tree.longestPrefix("192.168.1.99").?;
    expect_eq(std.mem.eql(u8, m2.key, "192.168.1."), true);
    expect_eq(m2.value, 300);

    const m3 = tree.longestPrefix("192.168.99.1").?;
    expect_eq(std.mem.eql(u8, m3.key, "192.168."), true);
    expect_eq(m3.value, 200);

    expect_eq(tree.longestPrefix("8.8.8.8") == null, true);

    // scanPrefix("192.168.") -> strictly lexicographical order
    const entries = tree.scanPrefix(alloc, "192.168.") catch unreachable;
    defer alloc.free(entries);
    expect_eq(entries.len, 4);
    expect_eq(std.mem.eql(u8, entries[0].key, "192.168."), true);
    expect_eq(entries[0].value, 200);
    expect_eq(std.mem.eql(u8, entries[1].key, "192.168.1."), true);
    expect_eq(entries[1].value, 300);
    expect_eq(std.mem.eql(u8, entries[2].key, "192.168.1.50"), true);
    expect_eq(entries[2].value, 400);
    expect_eq(std.mem.eql(u8, entries[3].key, "192.168.2.1"), true);
    expect_eq(entries[3].value, 600);

    // scanPrefix mid-edge ("192.16") -> same 4 entries
    const mid_entries = tree.scanPrefix(alloc, "192.16") catch unreachable;
    defer alloc.free(mid_entries);
    expect_eq(mid_entries.len, 4);

    // scanPrefix with no match
    const empty_entries = tree.scanPrefix(alloc, "999.") catch unreachable;
    defer alloc.free(empty_entries);
    expect_eq(empty_entries.len, 0);
}

test("cap:radix - RadixRouter static, :param, *wildcard & priority resolution") {
    const alloc = std.testing.allocator;
    let router = RadixRouter<u32>.init(alloc);
    defer router.deinit();

    expect_eq(router.isEmpty(), true);
    router.addRoute("/", 1) catch unreachable;
    router.addRoute("/users/profile", 2) catch unreachable;
    router.addRoute("/users/:id", 3) catch unreachable;
    router.addRoute("/users/:id/posts/:post_id", 4) catch unreachable;
    router.addRoute("/static/*filepath", 5) catch unreachable;
    router.addRoute("/users/*rest", 6) catch unreachable;
    expect_eq(router.count(), 6);

    // Root match
    if (m0 := router.matchRoute(alloc, "/") catch unreachable) {
        defer m0.deinit();
        expect_eq(m0.value, 1);
        expect_eq(m0.params.len, 0);
    } else {
        expect_eq(false, true);
    }

    // Static beats :param ("/users/profile" > "/users/:id")
    if (m1 := router.matchRoute(alloc, "/users/profile") catch unreachable) {
        defer m1.deinit();
        expect_eq(m1.value, 2);
        expect_eq(m1.params.len, 0);
    } else {
        expect_eq(false, true);
    }

    // :param match
    if (m2 := router.matchRoute(alloc, "/users/42") catch unreachable) {
        defer m2.deinit();
        expect_eq(m2.value, 3);
        expect_eq(m2.params.len, 1);
        expect_eq(std.mem.eql(u8, m2.getParam("id").?, "42"), true);
    } else {
        expect_eq(false, true);
    }

    // Multi-param match
    if (m3 := router.matchRoute(alloc, "/users/42/posts/9001") catch unreachable) {
        defer m3.deinit();
        expect_eq(m3.value, 4);
        expect_eq(m3.params.len, 2);
        expect_eq(std.mem.eql(u8, m3.getParam("id").?, "42"), true);
        expect_eq(std.mem.eql(u8, m3.getParam("post_id").?, "9001"), true);
    } else {
        expect_eq(false, true);
    }

    // Wildcard match
    if (m4 := router.matchRoute(alloc, "/static/css/app.min.css") catch unreachable) {
        defer m4.deinit();
        expect_eq(m4.value, 5);
        expect_eq(std.mem.eql(u8, m4.getParam("filepath").?, "css/app.min.css"), true);
    } else {
        expect_eq(false, true);
    }

    // Param backtracking to wildcard when deeper static segment fails
    if (m5 := router.matchRoute(alloc, "/users/42/unknown/deep") catch unreachable) {
        defer m5.deinit();
        expect_eq(m5.value, 6);
        expect_eq(std.mem.eql(u8, m5.getParam("rest").?, "42/unknown/deep"), true);
    } else {
        expect_eq(false, true);
    }

    // Non-matching route
    const m_none = router.matchRoute(alloc, "/not/found") catch unreachable;
    expect_eq(m_none == null, true);
}

test("cap:radix - dynamic cap.radix mount & zero memory leaks") {
    const alloc = std.testing.allocator;
    let tree = cap.radix.RadixTree<u64>.init(alloc);
    defer tree.deinit();

    let buf: [32]u8 = undefined;
    for (i of 0..200) {
        const k = std.fmt.bufPrint(buf[0..], "svc/v1/node-{d:0>3}", .{i}) catch unreachable;
        tree.insert(k, i as u64) catch unreachable;
    }
    expect_eq(tree.count(), 200);
    expect_eq(tree.verifyInvariants(), true);

    for (i of 0..100) {
        const k = std.fmt.bufPrint(buf[0..], "svc/v1/node-{d:0>3}", .{i * 2}) catch unreachable;
        const removed = tree.delete(k);
        expect_eq(removed.?, (i * 2) as u64);
    }
    expect_eq(tree.count(), 100);
    expect_eq(tree.verifyInvariants(), true);

    let dyn_router = cap.radix.RadixRouter<u64>.init(alloc);
    defer dyn_router.deinit();
    dyn_router.addRoute("/api/:ver/health", 200) catch unreachable;
    if (rm := dyn_router.matchRoute(alloc, "/api/v2/health") catch unreachable) {
        defer rm.deinit();
        expect_eq(rm.value, 200);
        expect_eq(std.mem.eql(u8, rm.getParam("ver").?, "v2"), true);
    } else {
        expect_eq(false, true);
    }
}
