// cap/test/testCollectionsSliceList.cp
// Production-grade deep tests for cap:collections sliceList.cp (Bun collections/array_list.cp)

import { test, assert, assertEq } from "cap:test";
import {
    SliceListError,
    SliceList,
} from "cap:collections";

struct Pair {
    k: i32 = 0,
    v: i32 = 0,
}

test("SliceList: append / appendSlice / appendNTimes / pop / first / last", () => {
    let storage: [16]i32 = [0; 16];
    let list = SliceList<i32>.init(storage[0..]);
    assert(list.isEmpty());
    assert(!list.isFull());
    assertEq(list.capacity(), @as(usize, 16));
    assertEq(list.remaining(), @as(usize, 16));
    assert(list.first() == null);
    assert(list.last() == null);
    assert(list.pop() == null);

    assert(list.append(10) catch false);
    const batch: [3]i32 = [20, 30, 40];
    assert(list.appendSlice(batch[0..]) catch false);
    assert(list.appendNTimes(99, 2) catch false);

    assertEq(list.len(), @as(usize, 6));
    assertEq(list.first() ?? -1, @as(i32, 10));
    assertEq(list.last() ?? -1, @as(i32, 99));
    assertEq(list.get(2) catch -1, @as(i32, 30));

    assert(list.set(2, 300) catch false);
    assertEq(list.get(2) catch -1, @as(i32, 300));

    assertEq(list.pop() ?? -1, @as(i32, 99));
    assertEq(list.len(), @as(usize, 5));
});

test("SliceList: insert & insertSlice preserve element order", () => {
    let storage: [12]i32 = [0; 12];
    let list = SliceList<i32>.init(storage[0..]);
    const init_vals: [3]i32 = [1, 5, 6];
    assert(list.appendSlice(init_vals[0..]) catch false);

    // Insert at front, middle, and end
    assert(list.insert(0, 0) catch false);          // [0, 1, 5, 6]
    const mid: [3]i32 = [2, 3, 4];
    assert(list.insertSlice(2, mid[0..]) catch false); // [0, 1, 2, 3, 4, 5, 6]
    assert(list.insert(7, 7) catch false);          // [0, 1, 2, 3, 4, 5, 6, 7]

    assertEq(list.len(), @as(usize, 8));
    let i: usize = 0;
    while (i < 8) {
        assertEq(list.get(i) catch -1, @as(i32, @intCast(i)));
        i += 1;
    }
});

test("SliceList: orderedRemove & swapRemove", () => {
    let storage: [8]i32 = [10, 20, 30, 40, 50, 0, 0, 0];
    let list = SliceList<i32>.fromInitial(storage[0..], 5) catch SliceList<i32>.init(storage[0..]);

    // orderedRemove index 1 (20) -> [10, 30, 40, 50]
    assertEq(list.orderedRemove(1) catch -1, @as(i32, 20));
    assertEq(list.len(), @as(usize, 4));
    assertEq(list.get(1) catch -1, @as(i32, 30));
    assertEq(list.get(3) catch -1, @as(i32, 50));

    // swapRemove index 0 (10) -> [50, 30, 40]
    assertEq(list.swapRemove(0) catch -1, @as(i32, 10));
    assertEq(list.len(), @as(usize, 3));
    assertEq(list.get(0) catch -1, @as(i32, 50));
    assertEq(list.get(1) catch -1, @as(i32, 30));
    assertEq(list.get(2) catch -1, @as(i32, 40));

    // swapRemove last element (40) -> [50, 30]
    assertEq(list.swapRemove(2) catch -1, @as(i32, 40));
    assertEq(list.len(), @as(usize, 2));
});

test("SliceList: replaceRange equal, shrink & grow paths", () => {
    let storage: [32]u8 = [0; 32];
    let list = SliceList<u8>.init(storage[0..]);
    assert(list.appendSlice("hello world") catch false);

    // 1. Grow: replace "world" (len 5) with "cp stdlib!" (len 10)
    assert(list.replaceRange(6, 5, "cp stdlib!") catch false);
    assertEq(list.items(), "hello cp stdlib!");

    // 2. Shrink: replace "cp stdlib" (len 9) with "CP" (len 2)
    assert(list.replaceRange(6, 9, "CP") catch false);
    assertEq(list.items(), "hello CP!");

    // 3. Equal length: replace "hello" with "HELLO"
    assert(list.replaceRange(0, 5, "HELLO") catch false);
    assertEq(list.items(), "HELLO CP!");
});

test("SliceList: resize / shrinkRetainingCapacity / reverse / unusedCapacitySlice", () => {
    let storage: [10]i32 = [0; 10];
    let list = SliceList<i32>.init(storage[0..]);

    assert(list.resize(4, 7) catch false);
    assertEq(list.len(), @as(usize, 4));
    assertEq(list.unusedCapacitySlice().len, @as(usize, 6));
    assertEq(list.get(3) catch -1, @as(i32, 7));

    assert(list.set(0, 1) catch false);
    assert(list.set(1, 2) catch false);
    assert(list.set(2, 3) catch false);
    assert(list.set(3, 4) catch false);
    list.reverse();
    assertEq(list.get(0) catch -1, @as(i32, 4));
    assertEq(list.get(1) catch -1, @as(i32, 3));
    assertEq(list.get(2) catch -1, @as(i32, 2));
    assertEq(list.get(3) catch -1, @as(i32, 1));

    assert(list.shrinkRetainingCapacity(2) catch false);
    assertEq(list.len(), @as(usize, 2));
    assertEq(list.last() ?? -1, @as(i32, 3));

    list.clearRetainingCapacity();
    assert(list.isEmpty());
    assertEq(list.capacity(), @as(usize, 10));
});

test("SliceList: generic struct instantiation & large external slice (no small cap)", () => {
    let big: [512]Pair = [Pair{}; 512];
    let list = SliceList<Pair>.init(big[0..]);
    let i: usize = 0;
    while (i < 512) {
        assert(list.append(Pair{ k: @as(i32, @intCast(i)), v: @as(i32, @intCast(i * 2)) }) catch false);
        i += 1;
    }
    assert(list.isFull());
    assertEq(list.len(), @as(usize, 512));
    const last_p = list.last() ?? Pair{};
    assertEq(last_p.k, @as(i32, 511));
    assertEq(last_p.v, @as(i32, 1022));
});

test("SliceList: Overflow / OutOfBounds / InvalidRange atomic guards", () => {
    let storage: [4]i32 = [0; 4];
    let list = SliceList<i32>.init(storage[0..]);
    const two: [2]i32 = [1, 2];
    assert(list.appendSlice(two[0..]) catch false);

    // Overflow on appendSlice must not partially write
    const three: [3]i32 = [3, 4, 5];
    let got_overflow = false;
    * = list.appendSlice(three[0..]) catch |e| {
        if (e == SliceListError.Overflow) {
            got_overflow = true;
        }
        false
    };
    assert(got_overflow);
    assertEq(list.len(), @as(usize, 2));

    // OutOfBounds on get & insert
    let got_oob = false;
    * = list.get(5) catch |e| {
        if (e == SliceListError.OutOfBounds) {
            got_oob = true;
        }
        0
    };
    assert(got_oob);

    // InvalidRange on replaceRange & shrinkRetainingCapacity
    let got_invalid_range = false;
    * = list.replaceRange(1, 5, two[0..]) catch |e| {
        if (e == SliceListError.InvalidRange) {
            got_invalid_range = true;
        }
        false
    };
    assert(got_invalid_range);
    assertEq(list.len(), @as(usize, 2));

    let got_invalid_shrink = false;
    * = list.shrinkRetainingCapacity(3) catch |e| {
        if (e == SliceListError.InvalidRange) {
            got_invalid_shrink = true;
        }
        false
    };
    assert(got_invalid_shrink);
});
