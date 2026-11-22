// Source: native-cp-stdlib (Roaring Bitmap Compressed Set)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { RoaringBitmap, ARRAY_MAX_SIZE } from "cap:db";
import cap from "cap";

test "roaring: auto upgrade ArrayContainer to BitsetContainer and downgrade back" {
    const alloc = std.testing.allocator;
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();

    // Insert 4096 elements into bucket 0 -> stays ArrayContainer
    let i: u32 = 0;
    while (i < 4096) {
        const added = try bm.add(i);
        expect_eq(added, true);
        i += 1;
    }
    expect_eq(bm.buckets.items.len, 1);
    expect_eq(bm.cardinality(), 4096);
    switch (bm.buckets.items[0].container) {
        .array => {},
        .bitset => expect_eq(true, false),
    }

    // Duplicate add returns false and does not upgrade
    expect_eq(try bm.add(100), false);
    expect_eq(bm.cardinality(), 4096);

    // 4097th element triggers upgrade to BitsetContainer
    expect_eq(try bm.add(4096), true);
    expect_eq(bm.cardinality(), 4097);
    switch (bm.buckets.items[0].container) {
        .array => expect_eq(true, false),
        .bitset => {},
    }
    expect_eq(bm.contains(0), true);
    expect_eq(bm.contains(4096), true);
    expect_eq(bm.contains(5000), false);

    // Remove 1 element -> 4096 elements -> downgrades back to ArrayContainer
    expect_eq(try bm.remove(4096), true);
    expect_eq(bm.cardinality(), 4096);
    switch (bm.buckets.items[0].container) {
        .array => {},
        .bitset => expect_eq(true, false),
    }
    expect_eq(bm.contains(4096), false);
    expect_eq(bm.contains(4095), true);
}

test "roaring: unionWith, intersectWith, differenceWith across Array and Bitset containers" {
    const alloc = std.testing.allocator;
    let a = RoaringBitmap.init(alloc);
    defer a.deinit();
    let b = RoaringBitmap.init(alloc);
    defer b.deinit();

    // Bucket 0: Array (a: 0..100) vs Array (b: 50..150)
    let i: u32 = 0;
    while (i < 100) {
        * = try a.add(i);
        i += 1;
    }
    i = 50;
    while (i < 150) {
        * = try b.add(i);
        i += 1;
    }

    // Bucket 1 (offset 65536): Bitset in a (5000 elems), Array in b (100 elems)
    const b1: u32 = 65536;
    i = 0;
    while (i < 5000) {
        * = try a.add(b1 + i * 2);
        i += 1;
    }
    i = 0;
    while (i < 100) {
        * = try b.add(b1 + i * 2);
        i += 1;
    }

    // Bucket 2 (offset 131072): Bitset in a (4500 elems), Bitset in b (4500 elems shifted by 500)
    const b2: u32 = 131072;
    i = 0;
    while (i < 4500) {
        * = try a.add(b2 + i);
        * = try b.add(b2 + i + 500);
        i += 1;
    }

    let u = try RoaringBitmap.unionWith(alloc, a, b);
    defer u.deinit();
    // Bucket 0: 150, Bucket 1: 5000, Bucket 2: 5000 => 10150
    expect_eq(u.cardinality(), 10150);
    expect_eq(u.contains(0), true);
    expect_eq(u.contains(149), true);
    expect_eq(u.contains(b2 + 4999), true);

    let inter = try RoaringBitmap.intersectWith(alloc, a, b);
    defer inter.deinit();
    // Bucket 0: 50, Bucket 1: 100, Bucket 2: 4000 => 4150
    expect_eq(inter.cardinality(), 4150);
    expect_eq(inter.contains(49), false);
    expect_eq(inter.contains(50), true);
    expect_eq(inter.contains(b2 + 500), true);
    expect_eq(inter.contains(b2 + 499), false);

    let diff = try RoaringBitmap.differenceWith(alloc, a, b);
    defer diff.deinit();
    // Bucket 0: 50, Bucket 1: 4900, Bucket 2: 500 => 5450
    expect_eq(diff.cardinality(), 5450);
    expect_eq(diff.contains(0), true);
    expect_eq(diff.contains(50), false);
    expect_eq(diff.contains(b1 + 200), true);
    expect_eq(diff.contains(b1 + 0), false);
    expect_eq(diff.contains(b2 + 0), true);
    expect_eq(diff.contains(b2 + 500), false);
}

test "roaring: cardinality, rank, minimum, maximum, and sorted toArray" {
    const alloc = std.testing.allocator;
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();

    // Empty checks
    expect_eq(bm.cardinality(), 0);
    expect_eq(bm.minimum(), null);
    expect_eq(bm.maximum(), null);
    expect_eq(bm.rank(100), 0);

    // Insert across multiple buckets out of order
    * = try bm.add(100000);
    * = try bm.add(5);
    * = try bm.add(70000);
    * = try bm.add(10);
    * = try bm.add(0xFFFF_FFFF);

    expect_eq(bm.cardinality(), 5);
    expect_eq(bm.minimum().?, 5);
    expect_eq(bm.maximum().?, 0xFFFF_FFFF);
    expect_eq(bm.rank(4), 0);
    expect_eq(bm.rank(5), 1);
    expect_eq(bm.rank(10), 2);
    expect_eq(bm.rank(69999), 2);
    expect_eq(bm.rank(70000), 3);
    expect_eq(bm.rank(0xFFFF_FFFF), 5);

    const arr = try bm.toArray(alloc);
    defer alloc.free(arr);
    expect_eq(arr.len, 5);
    expect_eq(arr[0], 5);
    expect_eq(arr[1], 10);
    expect_eq(arr[2], 70000);
    expect_eq(arr[3], 100000);
    expect_eq(arr[4], 0xFFFF_FFFF);
}

test "roaring: runOptimize and toOwnedSlice / fromSlice serialization roundtrip" {
    const alloc = std.testing.allocator;
    let bm = RoaringBitmap.init(alloc);
    defer bm.deinit();

    // Sparse bucket + dense bucket
    * = try bm.add(42);
    * = try bm.add(999);
    let i: u32 = 0;
    while (i < 4500) {
        * = try bm.add(65536 + i);
        i += 1;
    }

    * = try bm.runOptimize();
    const encoded = try bm.toOwnedSlice(alloc);
    defer alloc.free(encoded);

    let decoded = try RoaringBitmap.fromSlice(alloc, encoded);
    defer decoded.deinit();

    expect_eq(decoded.cardinality(), bm.cardinality());
    expect_eq(decoded.minimum().?, 42);
    expect_eq(decoded.maximum().?, 65536 + 4499);
    expect_eq(decoded.contains(999), true);
    expect_eq(decoded.contains(65536 + 2000), true);
    expect_eq(decoded.contains(65536 + 4500), false);

    const arr_orig = try bm.toArray(alloc);
    defer alloc.free(arr_orig);
    const arr_dec = try decoded.toArray(alloc);
    defer alloc.free(arr_dec);
    expect_eq(arr_orig.len, arr_dec.len);
    expect_eq(arr_dec[0], 42);
    expect_eq(arr_dec[1], 999);
}

test "roaring: cap.roaring_bitmap dynamic mounting and zero-leak cleanup" {
    const alloc = std.testing.allocator;
    expect_eq(ARRAY_MAX_SIZE, 4096);

    let bm = cap.db.RoaringBitmap.init(alloc);
    defer bm.deinit();

    * = try bm.add(1);
    * = try bm.add(200000);
    expect_eq(bm.contains(1), true);
    expect_eq(bm.contains(200000), true);
    expect_eq(bm.cardinality(), 2);

    expect_eq(try bm.remove(1), true);
    expect_eq(bm.cardinality(), 1);
    expect_eq(bm.minimum().?, 200000);
    expect_eq(try bm.remove(200000), true);
    expect_eq(bm.cardinality(), 0);
    expect_eq(bm.buckets.items.len, 0);
}
