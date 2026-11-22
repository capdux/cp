// Source: native-cp-stdlib (Rope Balanced Text Buffer)

import std from "std";
import { test, expect_eq } from "cap:test";
import { Rope, RopeError } from "cap:ds";
import cap from "cap";

test "rope_text: large fromSlice and random insert/delete with zero leaks" {
    const alloc = std.testing.allocator;
    let buf = std.ArrayList(u8).empty;
    defer buf.deinit(alloc);

    let i: usize = 0;
    while (i < 300) {
        try buf.append(alloc, @intCast((i % 26) + 'a'));
        i = i + 1;
    }

    let r = try Rope.fromSlice(alloc, buf.items);
    defer r.deinit();

    expect_eq(r.len(), 300);

    try r.insert(0, "HEAD:");
    try r.insert(150, "[MID]");
    try r.insert(r.len(), ":TAIL");
    expect_eq(r.len(), 315);

    try r.delete(150, 155);
    expect_eq(r.len(), 310);

    try r.delete(0, 5);
    expect_eq(r.len(), 305);

    try r.delete(300, 305);
    expect_eq(r.len(), 300);

    const out = try r.toString(alloc);
    defer alloc.free(out);
    try std.testing.expectEqualSlices(u8, buf.items, out);
}

test "rope_text: multi-line lineCount and O(log N) lineAt" {
    const alloc = std.testing.allocator;
    const text = "alpha line 0\nbeta line 1 with more text to span multiple leaves\ngamma line 2\n\nepsilon line 4";
    let r = try Rope.fromSlice(alloc, text);
    defer r.deinit();

    expect_eq(r.lineCount(), 5);

    const l0 = try r.lineAt(alloc, 0);
    defer alloc.free(l0);
    try std.testing.expectEqualSlices(u8, "alpha line 0", l0);

    const l1 = try r.lineAt(alloc, 1);
    defer alloc.free(l1);
    try std.testing.expectEqualSlices(u8, "beta line 1 with more text to span multiple leaves", l1);

    const l2 = try r.lineAt(alloc, 2);
    defer alloc.free(l2);
    try std.testing.expectEqualSlices(u8, "gamma line 2", l2);

    const l3 = try r.lineAt(alloc, 3);
    defer alloc.free(l3);
    try std.testing.expectEqualSlices(u8, "", l3);

    const l4 = try r.lineAt(alloc, 4);
    defer alloc.free(l4);
    try std.testing.expectEqualSlices(u8, "epsilon line 4", l4);

    // Insert newline in the middle and re-verify lineCount & lineAt
    try r.insert(6, "\ninserted");
    expect_eq(r.lineCount(), 6);
    const nl0 = try r.lineAt(alloc, 0);
    defer alloc.free(nl0);
    try std.testing.expectEqualSlices(u8, "alpha ", nl0);

    const nl1 = try r.lineAt(alloc, 1);
    defer alloc.free(nl1);
    try std.testing.expectEqualSlices(u8, "insertedline 0", nl1);
}

test "rope_text: sliceRange, charAt, and toString reconstruction" {
    const alloc = std.testing.allocator;
    let r = Rope.init(alloc);
    defer r.deinit();

    try r.insert(0, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    expect_eq(r.len(), 72);

    const c0 = try r.charAt(0);
    expect_eq(c0, '0');
    const c10 = try r.charAt(10);
    expect_eq(c10, 'a');
    const c71 = try r.charAt(71);
    expect_eq(c71, '9');

    const sub = try r.sliceRange(alloc, 10, 36);
    defer alloc.free(sub);
    try std.testing.expectEqualSlices(u8, "abcdefghijklmnopqrstuvwxyz", sub);

    const empty_sub = try r.sliceRange(alloc, 5, 5);
    defer alloc.free(empty_sub);
    expect_eq(empty_sub.len, 0);

    const full = try r.toString(alloc);
    defer alloc.free(full);
    try std.testing.expectEqualSlices(u8, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", full);
}

test "rope_text: bounds and invalid range error interception" {
    const alloc = std.testing.allocator;
    let r = try Rope.fromSlice(alloc, "hello world");
    defer r.deinit();

    try std.testing.expectError(RopeError.OutOfBounds, r.charAt(11));
    try std.testing.expectError(RopeError.OutOfBounds, r.insert(12, "x"));
    try std.testing.expectError(RopeError.InvalidRange, r.delete(6, 3));
    try std.testing.expectError(RopeError.OutOfBounds, r.delete(5, 12));
    try std.testing.expectError(RopeError.InvalidRange, r.sliceRange(alloc, 8, 2));
    try std.testing.expectError(RopeError.OutOfBounds, r.sliceRange(alloc, 0, 12));
    try std.testing.expectError(RopeError.OutOfBounds, r.lineAt(alloc, 1));
}

test "rope_text: dynamic cap.rope_text mount and full delete to empty" {
    const alloc = std.testing.allocator;
    let r = try cap.ds.Rope.fromSlice(alloc, "dynamic\nmount\ncheck");
    defer r.deinit();

    expect_eq(r.lineCount(), 3);
    const l1 = try r.lineAt(alloc, 1);
    defer alloc.free(l1);
    try std.testing.expectEqualSlices(u8, "mount", l1);

    try r.delete(0, r.len());
    expect_eq(r.len(), 0);
    expect_eq(r.lineCount(), 1);
    const empty_str = try r.toString(alloc);
    defer alloc.free(empty_str);
    expect_eq(empty_str.len, 0);
}
