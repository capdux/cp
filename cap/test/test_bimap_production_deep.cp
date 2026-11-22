// Source: native-cp-stdlib (Bidirectional 1-to-1 Map)
import * as zstd from "zig:std";
import { BiMap, BiMapError, StringBiMap } from "cap:ds";
import cap from "cap";
import { test, expect_eq } from "cap:test";

test "BiMap(u32, u64) bidirectional O(1) insert and lookup" {
    const alloc = zstd.testing.allocator;
    let bm = BiMap(u32, u64).init(alloc);
    defer bm.deinit();

    try bm.insert(1, 1001);
    try bm.insert(2, 2002);
    try bm.insert(3, 3003);

    expect_eq(bm.len(), 3);
    expect_eq(bm.containsLeft(2), true);
    expect_eq(bm.containsRight(2002), true);
    expect_eq(bm.getRight(1).?, 1001);
    expect_eq(bm.getLeft(2002).?, 2);
    expect_eq(bm.getRight(99) == null, true);
    expect_eq(bm.getLeft(9999) == null, true);

    const removed_r = bm.removeByLeft(2);
    expect_eq(removed_r.?, 2002);
    expect_eq(bm.len(), 2);
    expect_eq(bm.containsLeft(2), false);
    expect_eq(bm.containsRight(2002), false);

    const removed_l = bm.removeByRight(3003);
    expect_eq(removed_l.?, 3);
    expect_eq(bm.len(), 1);

    bm.clear();
    expect_eq(bm.len(), 0);
}

test "BiMap insert rejects DuplicateLeft and DuplicateRight" {
    const alloc = zstd.testing.allocator;
    let bm = BiMap(u32, u64).init(alloc);
    defer bm.deinit();

    try bm.insert(10, 100);

    let got_dup_left = false;
    bm.insert(10, 200) catch |e| {
        if (e == BiMapError.DuplicateLeft) {
            got_dup_left = true;
        }
    };
    expect_eq(got_dup_left, true);

    let got_dup_right = false;
    bm.insert(20, 100) catch |e| {
        if (e == BiMapError.DuplicateRight) {
            got_dup_right = true;
        }
    };
    expect_eq(got_dup_right, true);
    expect_eq(bm.len(), 1);
    expect_eq(bm.getRight(10).?, 100);
}

test "BiMap forceInsert overwrites and cleans old bidirectional mappings" {
    const alloc = zstd.testing.allocator;
    let bm = BiMap(u32, u64).init(alloc);
    defer bm.deinit();

    try bm.insert(1, 100);
    try bm.insert(2, 200);

    // Force-inserting (1, 200) must remove old (1, 100) and (2, 200)
    try bm.forceInsert(1, 200);

    expect_eq(bm.len(), 1);
    expect_eq(bm.getRight(1).?, 200);
    expect_eq(bm.getLeft(200).?, 1);
    expect_eq(bm.containsRight(100), false);
    expect_eq(bm.containsLeft(2), false);
}

test "StringBiMap deep copy management and removeByLeft/removeByRight" {
    const alloc = zstd.testing.allocator;
    let sbm = StringBiMap.init(alloc);
    defer sbm.deinit();

    let buf_l = try alloc.dupe(u8, "usr_1");
    defer alloc.free(buf_l);
    let buf_r = try alloc.dupe(u8, "alice");
    defer alloc.free(buf_r);
    try sbm.insert(buf_l, buf_r);

    // Mutate stack buffers to verify StringBiMap owns independent deep copies
    buf_l[0] = 'x';
    buf_r[0] = 'z';

    expect_eq(zstd.mem.eql(u8, sbm.getRight("usr_1").?, "alice"), true);
    expect_eq(zstd.mem.eql(u8, sbm.getLeft("alice").?, "usr_1"), true);

    try sbm.insert("usr_2", "bob");
    try sbm.forceInsert("usr_1", "bob");

    expect_eq(sbm.len(), 1);
    expect_eq(sbm.containsRight("alice"), false);
    expect_eq(sbm.containsLeft("usr_2"), false);
    expect_eq(zstd.mem.eql(u8, sbm.getRight("usr_1").?, "bob"), true);

    expect_eq(sbm.removeByLeft("usr_1"), true);
    expect_eq(sbm.removeByLeft("usr_1"), false);
    expect_eq(sbm.len(), 0);

    try sbm.insert("k1", "v1");
    expect_eq(sbm.removeByRight("v1"), true);
    expect_eq(sbm.len(), 0);

    try sbm.insert("k2", "v2");
    sbm.clear();
    expect_eq(sbm.len(), 0);
}

test "cap.bimap dynamic mount and zero-leak verification" {
    const alloc = zstd.testing.allocator;
    let bm = cap.ds.BiMap(u32, u64).init(alloc);
    defer bm.deinit();

    try bm.insert(42, 4200);
    expect_eq(bm.getLeft(4200).?, 42);

    let sbm = cap.ds.StringBiMap.init(alloc);
    defer sbm.deinit();

    try sbm.insert("alpha", "beta");
    expect_eq(zstd.mem.eql(u8, sbm.getRight("alpha").?, "beta"), true);
    expect_eq(zstd.mem.eql(u8, sbm.getLeft("beta").?, "alpha"), true);
}
