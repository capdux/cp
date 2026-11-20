// Source: cp/cap/test/test_collections_production_deep.cp
const std = @import("std");
const testing = std.testing;
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BitSet,
    BoundedDeque,
    RingDeque,
    SmallVec,
    BinaryHeap,
    CollectionError,
} from "cap:collections";

fn cmpMin(a: i32, b: i32): std.math.Order {
    return std.math.order(a, b);
}

fn cmpMax(a: i32, b: i32): std.math.Order {
    return std.math.order(b, a);
}

test("BitSet bitwise ops, set algebra, and nextSetBit") {
    const alloc = testing.allocator;
    let a = try BitSet.init(alloc, 130);
    defer a.deinit();
    let b = try BitSet.init(alloc, 130);
    defer b.deinit();

    a.set(0);
    a.set(63);
    a.set(64);
    a.set(129);
    try expect_eq(a.isSet(64), true);
    try expect_eq(a.countSetBits(), 4);

    a.toggle(64);
    try expect_eq(a.isSet(64), false);
    a.unset(0);
    try expect_eq(a.isSet(0), false);
    try expect_eq(a.nextSetBit(0).?, 63);
    try expect_eq(a.nextSetBit(64).?, 129);
    try expect_eq(a.nextSetBit(130), null);

    b.set(63);
    b.set(100);

    let u = try BitSet.init(alloc, 130);
    defer u.deinit();
    u.unionWith(&a);
    u.unionWith(&b);
    try expect_eq(u.countSetBits(), 3);
    try expect_eq(u.isSet(100), true);

    u.intersectWith(&b);
    try expect_eq(u.countSetBits(), 2);
    try expect_eq(u.isSet(129), false);

    u.differenceWith(&a);
    try expect_eq(u.countSetBits(), 1);
    try expect_eq(u.isSet(100), true);
}

test("BoundedDeque and RingDeque wraparound and Full error") {
    const alloc = testing.allocator;

    let bd = BoundedDeque(i32, 3).init();
    try bd.pushBack(10);
    try bd.pushFront(5);
    try bd.pushBack(20);
    try expect_eq(bd.isFull(), true);
    try testing.expectError(CollectionError.Full, bd.pushBack(99));
    try testing.expectError(CollectionError.Full, bd.pushFront(99));

    try expect_eq(bd.peekFront().?, 5);
    try expect_eq(bd.peekBack().?, 20);
    try expect_eq(bd.popFront().?, 5);
    try expect_eq(bd.popBack().?, 20);
    try expect_eq(bd.popFront().?, 10);
    try expect_eq(bd.popFront(), null);

    let rd = RingDeque(i32).init(alloc);
    defer rd.deinit();
    for (i of 0..20) {
        if ((i & 1) == 0) {
            try rd.pushBack(@intCast(i));
        } else {
            try rd.pushFront(@intCast(i));
        }
    }
    try expect_eq(rd.len(), 20);
    try expect_eq(rd.peekFront().?, 19);
    try expect_eq(rd.peekBack().?, 18);
    try expect_eq(rd.popFront().?, 19);
    try expect_eq(rd.popBack().?, 18);
    rd.clear();
    try expect_eq(rd.isEmpty(), true);
}

test("SmallVec inline zero-heap and seamless heap spill") {
    const alloc = testing.allocator;
    let sv = SmallVec(i32, 4).init();
    defer sv.deinit(alloc);

    try sv.push(alloc, 1);
    try sv.push(alloc, 2);
    try sv.push(alloc, 3);
    try sv.push(alloc, 4);
    try expect_eq(sv.isHeap(), false);
    try expect_eq(sv.len(), 4);
    try expect_eq(sv.slice()[2], 3);

    try sv.push(alloc, 5);
    try sv.push(alloc, 6);
    try expect_eq(sv.isHeap(), true);
    try expect_eq(sv.len(), 6);
    try expect_eq(sv.slice()[0], 1);
    try expect_eq(sv.slice()[5], 6);

    try expect_eq(sv.pop().?, 6);
    try expect_eq(sv.len(), 5);
}

test("BinaryHeap ordered pop and peek") {
    const alloc = testing.allocator;
    let min_heap = BinaryHeap(i32, cmpMin).init(alloc);
    defer min_heap.deinit();

    const vals: [6]i32 = [42, 7, 19, 3, 25, 11];
    for (v of vals) {
        try min_heap.push(v);
    }
    try expect_eq(min_heap.len(), 6);
    try expect_eq(min_heap.peek().?, 3);
    try expect_eq(min_heap.pop().?, 3);
    try expect_eq(min_heap.pop().?, 7);
    try expect_eq(min_heap.pop().?, 11);
    try expect_eq(min_heap.pop().?, 19);
    try expect_eq(min_heap.pop().?, 25);
    try expect_eq(min_heap.pop().?, 42);
    try expect_eq(min_heap.pop(), null);

    let max_heap = BinaryHeap(i32, cmpMax).init(alloc);
    defer max_heap.deinit();
    for (v of vals) {
        try max_heap.push(v);
    }
    try expect_eq(max_heap.pop().?, 42);
    try expect_eq(max_heap.pop().?, 25);
}

test("cap.collections dynamic mount and zero-leak verification") {
    const alloc = testing.allocator;
    let bs = try cap.collections.BitSet.init(alloc, 64);
    defer bs.deinit();
    bs.set(17);
    try expect_eq(bs.isSet(17), true);

    let sv = cap.collections.SmallVec(u32, 2).init();
    defer sv.deinit(alloc);
    try sv.push(alloc, 100);
    try sv.push(alloc, 200);
    try sv.push(alloc, 300);
    try expect_eq(sv.len(), 3);
    try expect_eq(sv.slice()[2], 300);
}
