// Source: native-cp-stdlib (Compressed Radix / Patricia Trie)

import std from "std";
import { RadixTree, StrSlice } from "../../lib/ds/trie_radix_.cp";
import { test, expect_eq } from "cap:test";

fn containsKey(keys: []StrSlice, target: []const u8) -> bool {
    for (k of keys) {
        if (std.mem.eql(u8, k, target)) {
            return true;
        }
    }
    return false;
}

test "radix tree split insert overwrite and exact get" {
    const alloc = std.testing.allocator;
    let tree = try RadixTree<i32>.init(alloc);
    defer tree.deinit();

    expect_eq(tree.len(), 0);
    expect_eq(try tree.insert("romane", 1), null);
    expect_eq(try tree.insert("romanus", 2), null);
    expect_eq(try tree.insert("romulus", 3), null);
    expect_eq(try tree.insert("rubens", 4), null);
    expect_eq(try tree.insert("ruber", 5), null);
    expect_eq(try tree.insert("rubicon", 6), null);
    expect_eq(try tree.insert("rubicundus", 7), null);
    expect_eq(try tree.insert("rom", 8), null);
    expect_eq(tree.len(), 8);

    // Overwrite existing key and verify old value returned
    const prev = try tree.insert("romanus", 22);
    expect_eq(prev.?, 2);
    expect_eq(tree.len(), 8);

    expect_eq(tree.get("rom").?, 8);
    expect_eq(tree.get("romane").?, 1);
    expect_eq(tree.get("romanus").?, 22);
    expect_eq(tree.get("romulus").?, 3);
    expect_eq(tree.get("rubens").?, 4);
    expect_eq(tree.get("ruber").?, 5);
    expect_eq(tree.get("rubicon").?, 6);
    expect_eq(tree.get("rubicundus").?, 7);
    expect_eq(tree.get("roma") == null, true);
    expect_eq(tree.get("unknown") == null, true);
}

test "radix tree longestPrefixMatch routing scenario" {
    const alloc = std.testing.allocator;
    let tree = try RadixTree<u32>.init(alloc);
    defer tree.deinit();

    * = try tree.insert("/", 1);
    * = try tree.insert("/api", 2);
    * = try tree.insert("/api/v1/users", 3);
    * = try tree.insert("/api/v1/users/profile", 4);
    * = try tree.insert("/static/", 5);

    const m1 = tree.longestPrefixMatch("/api/v1/users/42/settings");
    expect_eq(m1 != null, true);
    expect_eq(std.mem.eql(u8, m1.?.key, "/api/v1/users"), true);
    expect_eq(m1.?.value, 3);

    const m2 = tree.longestPrefixMatch("/api/v1/users/profile");
    expect_eq(m2 != null, true);
    expect_eq(std.mem.eql(u8, m2.?.key, "/api/v1/users/profile"), true);
    expect_eq(m2.?.value, 4);

    const m3 = tree.longestPrefixMatch("/api/v2/items");
    expect_eq(m3 != null, true);
    expect_eq(std.mem.eql(u8, m3.?.key, "/api"), true);
    expect_eq(m3.?.value, 2);

    const m4 = tree.longestPrefixMatch("/about");
    expect_eq(m4 != null, true);
    expect_eq(std.mem.eql(u8, m4.?.key, "/"), true);
    expect_eq(m4.?.value, 1);

    const m5 = tree.longestPrefixMatch("other");
    expect_eq(m5 == null, true);
}

test "radix tree keysWithPrefix returns all matching keys" {
    const alloc = std.testing.allocator;
    let tree = try RadixTree<u32>.init(alloc);
    defer tree.deinit();

    * = try tree.insert("app.config.host", 1);
    * = try tree.insert("app.config.port", 2);
    * = try tree.insert("app.cache.ttl", 3);
    * = try tree.insert("app.cache.size", 4);
    * = try tree.insert("sys.log.level", 5);

    const cfg_keys = try tree.keysWithPrefix(alloc, "app.co");
    defer {
        for (k of cfg_keys) {
            alloc.free(k);
        }
        alloc.free(cfg_keys);
    }
    expect_eq(cfg_keys.len, 2);
    expect_eq(containsKey(cfg_keys, "app.config.host"), true);
    expect_eq(containsKey(cfg_keys, "app.config.port"), true);

    const app_keys = try tree.keysWithPrefix(alloc, "app.");
    defer {
        for (k of app_keys) {
            alloc.free(k);
        }
        alloc.free(app_keys);
    }
    expect_eq(app_keys.len, 4);
    expect_eq(containsKey(app_keys, "app.cache.ttl"), true);
    expect_eq(containsKey(app_keys, "app.cache.size"), true);

    const empty_keys = try tree.keysWithPrefix(alloc, "none.");
    defer alloc.free(empty_keys);
    expect_eq(empty_keys.len, 0);
}

test "radix tree remove merges single child compressed edge" {
    const alloc = std.testing.allocator;
    let tree = try RadixTree<i32>.init(alloc);
    defer tree.deinit();

    * = try tree.insert("test", 10);
    * = try tree.insert("team", 20);
    * = try tree.insert("toast", 30);
    * = try tree.insert("toaster", 40);
    expect_eq(tree.len(), 4);

    // Removing "team" leaves "te" with single child "st" -> merges back into "test"
    const r1 = tree.remove("team");
    expect_eq(r1.?, 20);
    expect_eq(tree.len(), 3);
    expect_eq(tree.get("team") == null, true);
    expect_eq(tree.get("test").?, 10);

    // Removing "toast" leaves "toast" node with no value and single child "er" -> merges into "toaster"
    const r2 = tree.remove("toast");
    expect_eq(r2.?, 30);
    expect_eq(tree.len(), 2);
    expect_eq(tree.get("toast") == null, true);
    expect_eq(tree.get("toaster").?, 40);

    // Re-insert after merge works cleanly
    * = try tree.insert("teammate", 50);
    expect_eq(tree.len(), 3);
    expect_eq(tree.get("teammate").?, 50);
    expect_eq(tree.get("test").?, 10);
    expect_eq(tree.remove("nonexistent") == null, true);
}

test "cap.trie_radix dynamic mount and zero leak verification" {
    const alloc = std.testing.allocator;
    let tree = try RadixTree<u64>.init(alloc);
    defer tree.deinit();

    * = try tree.insert("", 999);
    * = try tree.insert("alpha", 111);
    * = try tree.insert("alphabet", 222);
    expect_eq(tree.len(), 3);

    expect_eq(tree.get("").?, 999);
    expect_eq(tree.get("alpha").?, 111);
    expect_eq(tree.get("alphabet").?, 222);

    const m = tree.longestPrefixMatch("beta");
    expect_eq(m != null, true);
    expect_eq(m.?.key.len, 0);
    expect_eq(m.?.value, 999);

    expect_eq(tree.remove("alpha").?, 111);
    expect_eq(tree.remove("").?, 999);
    expect_eq(tree.len(), 1);
    expect_eq(tree.get("alphabet").?, 222);
}
