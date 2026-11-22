// Source: cp/cap/test/test_circ_buf_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { RingError, RingBuffer } from "cap:ds";

test "circ_buf: FIFO push/popFront and wrap-around indexing" {
    const alloc = std.testing.allocator;
    let rb = try RingBuffer(u32).init(alloc, 4);
    defer rb.deinit();

    try expect_eq(rb.isEmpty(), true);
    try rb.push(1);
    try rb.push(2);
    try rb.push(3);
    try rb.push(4);
    try expect_eq(rb.isFull(), true);
    try expect_eq(rb.push(5) == RingError.BufferFull, true);

    try expect_eq(rb.popFront().?, 1);
    try expect_eq(rb.popFront().?, 2);
    try rb.push(5);
    try rb.push(6);

    const slice = try rb.toOwnedSlice(alloc);
    defer alloc.free(slice);
    const expected: [4]u32 = [ 3, 4, 5, 6 ];
    try expect_eq(std.mem.eql(u32, slice, &expected), true);
}

test "circ_buf: pushOverwrite evicts oldest and preserves order" {
    const alloc = std.testing.allocator;
    let rb = try RingBuffer(i32).init(alloc, 3);
    defer rb.deinit();

    try expect_eq(rb.pushOverwrite(10) == null, true);
    try expect_eq(rb.pushOverwrite(20) == null, true);
    try expect_eq(rb.pushOverwrite(30) == null, true);
    try expect_eq(rb.pushOverwrite(40).?, 10);
    try expect_eq(rb.pushOverwrite(50).?, 20);

    try expect_eq(try rb.get(0), 30);
    try expect_eq(try rb.get(1), 40);
    try expect_eq(try rb.get(2), 50);
}

test "circ_buf: popBack, peekFront, peekBack, and clear" {
    const alloc = std.testing.allocator;
    let rb = try RingBuffer(i32).init(alloc, 4);
    defer rb.deinit();

    try rb.push(100);
    try rb.push(200);
    try rb.push(300);
    try expect_eq(rb.peekFront().?, 100);
    try expect_eq(rb.peekBack().?, 300);
    try expect_eq(rb.popBack().?, 300);
    try expect_eq(rb.len(), 2);

    rb.clear();
    try expect_eq(rb.isEmpty(), true);
    try expect_eq(rb.popFront() == null, true);
}

test "circ_buf: error guards for zero capacity and OOB get" {
    const alloc = std.testing.allocator;
    try expect_eq(RingBuffer(u8).init(alloc, 0) == RingError.InvalidCapacity, true);
    let rb = try RingBuffer(u8).init(alloc, 2);
    defer rb.deinit();
    try rb.push(7);
    try expect_eq(rb.get(1) == RingError.IndexOutOfBounds, true);
}

test "circ_buf: cap.circ_buf dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let rb = try cap.ds.RingBuffer(u64).init(alloc, 2);
    defer rb.deinit();
    * = rb.pushOverwrite(99);
    try expect_eq(rb.popFront().?, 99);
}
