// Source: cp/cap/test/test_roaring_production_deep.cp
// Deep Production Verification Suite for cap:roaring & cap.roaring
// Covers:
//   1. Sparse ArrayContainer add/dedup/removeVal/minimum/maximum/rank & empty set boundaries
//   2. >4096 auto-upgrade to BitsetContainer, downgrade back to ArrayContainer on removeVal, & cross-block addRange
//   3. Mixed sparse/dense bitAnd/bitOr/bitAndNot/bitXor, andCardinality/intersects, & toSlice ordering
//   4. serialize/deserialize binary roundtrip & malformed magic/truncated/corrupt payload InvalidSerializedData
//   5. `import cap from "cap"; cap.roaring` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RoaringError,
    RoaringBitmap,
} from "cap:ds";

test("cap:roaring - sparse ArrayContainer ops, dedup, min/max, rank & empty boundaries") {
    const alloc = std.testing.allocator;
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();

    expect_eq(bm.isEmpty(), true);
    expect_eq(bm.cardinality(), 0);
    expect_eq(bm.minimum() == null, true);
    expect_eq(bm.maximum() == null, true);
    expect_eq(bm.rank(100), 0);
    expect_eq(bm.contains(0), false);
    expect_eq(bm.removeVal(0), false);

    // Insert sparse values across multiple high-16-bit keys (out of order)
    expect_eq(bm.add(100000) catch unreachable, true);
    expect_eq(bm.add(5) catch unreachable, true);
    expect_eq(bm.add(65536) catch unreachable, true);
    expect_eq(bm.add(65535) catch unreachable, true);
    // Duplicate insert returns false
    expect_eq(bm.add(65535) catch unreachable, false);

    expect_eq(bm.isEmpty(), false);
    expect_eq(bm.cardinality(), 4);
    expect_eq(bm.contains(5), true);
    expect_eq(bm.contains(65535), true);
    expect_eq(bm.contains(65536), true);
    expect_eq(bm.contains(100000), true);
    expect_eq(bm.contains(65534), false);

    expect_eq(bm.minimum().?, 5);
    expect_eq(bm.maximum().?, 100000);

    expect_eq(bm.rank(4), 0);
    expect_eq(bm.rank(5), 1);
    expect_eq(bm.rank(65535), 2);
    expect_eq(bm.rank(65536), 3);
    expect_eq(bm.rank(99999), 3);
    expect_eq(bm.rank(100000), 4);
    expect_eq(bm.rank(0xFFFFFFFF), 4);

    // Remove and verify container cleanup
    expect_eq(bm.removeVal(65536), true);
    expect_eq(bm.removeVal(65536), false);
    expect_eq(bm.cardinality(), 3);
    expect_eq(bm.rank(100000), 3);

    expect_eq(bm.removeVal(5), true);
    expect_eq(bm.minimum().?, 65535);
    expect_eq(bm.removeVal(100000), true);
    expect_eq(bm.maximum().?, 65535);
    expect_eq(bm.removeVal(65535), true);
    expect_eq(bm.isEmpty(), true);
    expect_eq(bm.minimum() == null, true);
    expect_eq(bm.maximum() == null, true);
}

test("cap:roaring - >4096 BitsetContainer upgrade/downgrade & cross-block addRange") {
    const alloc = std.testing.allocator;
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();

    // Insert 4096 elements in block 0 (stays ArrayContainer), then 4097th upgrades to BitsetContainer
    let i: u32 = 0;
    while (i < 4096) : (i += 1) {
        expect_eq(bm.add(i * 2) catch unreachable, true);
    }
    expect_eq(bm.cardinality(), 4096);
    expect_eq(bm.add(8192) catch unreachable, true);
    expect_eq(bm.cardinality(), 4097);
    expect_eq(bm.contains(0), true);
    expect_eq(bm.contains(8190), true);
    expect_eq(bm.contains(8192), true);
    expect_eq(bm.rank(8192), 4097);

    // Removing 1 element downgrades BitsetContainer back to ArrayContainer (<= 4096)
    expect_eq(bm.removeVal(8192), true);
    expect_eq(bm.cardinality(), 4096);
    expect_eq(bm.contains(8192), false);
    expect_eq(bm.contains(8190), true);
    expect_eq(bm.maximum().?, 8190);

    // Cross-block addRange [65000, 132000): spans keys 0, 1, 2
    let ranged = RoaringBitmap.init(alloc);
    defer ranged.deinit();

    // Empty range is no-op; inverted range triggers InvalidRange
    ranged.addRange(100, 100) catch unreachable;
    expect_eq(ranged.isEmpty(), true);

    let invalid_range = false;
    ranged.addRange(200, 100) catch |e| {
        if (e == RoaringError.InvalidRange) invalid_range = true;
    };
    expect_eq(invalid_range, true);

    ranged.addRange(65000, 132000) catch unreachable;
    expect_eq(ranged.cardinality(), 67000);
    expect_eq(ranged.minimum().?, 65000);
    expect_eq(ranged.maximum().?, 131999);
    expect_eq(ranged.contains(64999), false);
    expect_eq(ranged.contains(65000), true);
    expect_eq(ranged.contains(65535), true);
    expect_eq(ranged.contains(65536), true);
    expect_eq(ranged.contains(131071), true);
    expect_eq(ranged.contains(131072), true);
    expect_eq(ranged.contains(131999), true);
    expect_eq(ranged.contains(132000), false);
    expect_eq(ranged.rank(65535), 536);
    expect_eq(ranged.rank(131999), 67000);
}

test("cap:roaring - mixed sparse/dense set algebra, andCardinality, intersects & toSlice") {
    const alloc = std.testing.allocator;

    // A: sparse in key 0 ([10, 20, 30]) + dense in key 1 ([65536, 75536))
    let a = RoaringBitmap.init(alloc);
    defer a.deinit();
    * = a.add(10) catch unreachable;
    * = a.add(20) catch unreachable;
    * = a.add(30) catch unreachable;
    a.addRange(65536, 75536) catch unreachable;

    // B: sparse in key 0 ([20, 40]) + sparse in key 1 ([65536, 70000, 80000]) + key 3 ([200000])
    let b = RoaringBitmap.init(alloc);
    defer b.deinit();
    * = b.add(20) catch unreachable;
    * = b.add(40) catch unreachable;
    * = b.add(65536) catch unreachable;
    * = b.add(70000) catch unreachable;
    * = b.add(80000) catch unreachable;
    * = b.add(200000) catch unreachable;

    expect_eq(a.intersects(b), true);
    expect_eq(a.andCardinality(b), 3); // 20, 65536, 70000

    let disjoint = RoaringBitmap.init(alloc);
    defer disjoint.deinit();
    * = disjoint.add(999999) catch unreachable;
    expect_eq(a.intersects(disjoint), false);
    expect_eq(a.andCardinality(disjoint), 0);

    // Intersection
    let inter = a.bitAnd(alloc, b) catch unreachable;
    defer inter.deinit();
    expect_eq(inter.cardinality(), 3);
    const inter_slice = inter.toSlice(alloc) catch unreachable;
    defer alloc.free(inter_slice);
    expect_eq(inter_slice.len, 3);
    expect_eq(inter_slice[0], 20);
    expect_eq(inter_slice[1], 65536);
    expect_eq(inter_slice[2], 70000);

    // Union
    let uni = a.bitOr(alloc, b) catch unreachable;
    defer uni.deinit();
    expect_eq(uni.cardinality(), 10003 + 6 - 3); // 10006
    expect_eq(uni.contains(10), true);
    expect_eq(uni.contains(40), true);
    expect_eq(uni.contains(80000), true);
    expect_eq(uni.contains(200000), true);

    // Difference (a \ b)
    let diff = a.bitAndNot(alloc, b) catch unreachable;
    defer diff.deinit();
    expect_eq(diff.cardinality(), 10003 - 3);
    expect_eq(diff.contains(10), true);
    expect_eq(diff.contains(20), false);
    expect_eq(diff.contains(65536), false);
    expect_eq(diff.contains(65537), true);
    expect_eq(diff.contains(70000), false);

    // Symmetric difference (a ^ b)
    let sym = a.bitXor(alloc, b) catch unreachable;
    defer sym.deinit();
    expect_eq(sym.cardinality(), 10003 + 6 - 2 * 3); // 10003
    expect_eq(sym.contains(10), true);
    expect_eq(sym.contains(20), false);
    expect_eq(sym.contains(40), true);
    expect_eq(sym.contains(65536), false);
    expect_eq(sym.contains(80000), true);
    expect_eq(sym.contains(200000), true);
}

test("cap:roaring - serialize/deserialize roundtrip & InvalidSerializedData validation") {
    const alloc = std.testing.allocator;

    // Empty roundtrip
    let empty_bm = RoaringBitmap.init(alloc);
    defer empty_bm.deinit();
    const empty_bytes = empty_bm.serialize(alloc) catch unreachable;
    defer alloc.free(empty_bytes);
    let empty_restored = RoaringBitmap.deserialize(alloc, empty_bytes) catch unreachable;
    defer empty_restored.deinit();
    expect_eq(empty_restored.isEmpty(), true);

    // Mixed Array + Bitset roundtrip
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();
    * = bm.add(1) catch unreachable;
    * = bm.add(42) catch unreachable;
    bm.addRange(65536, 72000) catch unreachable;
    * = bm.add(0xFFFFFFFE) catch unreachable;

    const bytes = bm.serialize(alloc) catch unreachable;
    defer alloc.free(bytes);

    let restored = RoaringBitmap.deserialize(alloc, bytes) catch unreachable;
    defer restored.deinit();
    expect_eq(restored.cardinality(), bm.cardinality());
    expect_eq(restored.minimum().?, 1);
    expect_eq(restored.maximum().?, 0xFFFFFFFE);
    expect_eq(restored.contains(42), true);
    expect_eq(restored.contains(70000), true);
    expect_eq(restored.andCardinality(bm), bm.cardinality());

    // Malformed magic
    let bad_magic = alloc.alloc(u8, bytes.len) catch unreachable;
    defer alloc.free(bad_magic);
    @memcpy(bad_magic, bytes);
    bad_magic[0] = 'X';
    let caught_magic = false;
    * = RoaringBitmap.deserialize(alloc, bad_magic) catch |e| {
        if (e == RoaringError.InvalidSerializedData) caught_magic = true;
    };
    expect_eq(caught_magic, true);

    // Truncated payload
    let caught_trunc = false;
    * = RoaringBitmap.deserialize(alloc, bytes[0..bytes.len - 5]) catch |e| {
        if (e == RoaringError.InvalidSerializedData) caught_trunc = true;
    };
    expect_eq(caught_trunc, true);

    // Too short header
    let caught_short = false;
    * = RoaringBitmap.deserialize(alloc, "ROAR") catch |e| {
        if (e == RoaringError.InvalidSerializedData) caught_short = true;
    };
    expect_eq(caught_short, true);
}

test("cap:roaring - cap.roaring dynamic mounting & clone isolation") {
    const alloc = std.testing.allocator;
    let a = cap.ds.RoaringBitmap.init(alloc);
    defer a.deinit();

    a.addRange(1000, 6000) catch unreachable;
    expect_eq(a.cardinality(), 5000);

    let b = a.clone(alloc) catch unreachable;
    defer b.deinit();
    expect_eq(b.removeVal(1500), true);
    expect_eq(a.contains(1500), true);
    expect_eq(b.contains(1500), false);
    expect_eq(a.cardinality(), 5000);
    expect_eq(b.cardinality(), 4999);
}
