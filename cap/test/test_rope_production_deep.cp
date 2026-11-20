// Source: cp/cap/test/test_rope_production_deep.cp (Deep production tests for cap:rope & cap.rope)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { RopeText, RopeError, LineCol } from "cap:rope";
import cap from "cap";

fn naiveInsert(alloc: std.mem.Allocator, list: *std.ArrayList(u8), offset: usize, text: []const u8): !void {
    const old_len = list.items.len;
    try list.resize(alloc, old_len + text.len);
    let i: usize = old_len;
    while (i > offset) {
        list.items[i + text.len - 1] = list.items[i - 1];
        i -= 1;
    }
    @memcpy(list.items[offset .. offset + text.len], text);
}

fn naiveDelete(list: *std.ArrayList(u8), start: usize, del_len: usize) {
    if (del_len == 0) {
        return;
    }
    let i: usize = start;
    while (i + del_len < list.items.len) {
        list.items[i] = list.items[i + del_len];
        i += 1;
    }
    list.items.len -= del_len;
}

fn pickSnippet(idx: usize): []const u8 {
    if (idx == 0) return "X";
    if (idx == 1) return "hello\n";
    if (idx == 2) return "12345";
    if (idx == 3) return "\r\n";
    if (idx == 4) return "piece_table";
    return "Z";
}

test("1. Randomized insert/deleteRange/replaceRange Equivalence vs ArrayList(u8)") {
    const alloc = std.testing.allocator;
    const seed_text = "alpha\nbeta\ngamma\ndelta\n";
    let rope = try RopeText.initFromText(alloc, seed_text);
    defer rope.deinit();

    let ref_buf = std.ArrayList(u8).empty;
    defer ref_buf.deinit(alloc);
    try ref_buf.appendSlice(alloc, seed_text);

    let prng = std.Random.DefaultPrng.init(0x5EED_CAFE_1234);
    const rng = prng.random();

    let step: usize = 0;
    while (step < 250) {
        const op = rng.uintLessThan(u8, 3);
        const cur_len = ref_buf.items.len;
        if (op == 0 or cur_len == 0) {
            const off = rng.uintLessThan(usize, cur_len + 1);
            const snip = pickSnippet(rng.uintLessThan(usize, 6));
            try rope.insert(off, snip);
            try naiveInsert(alloc, &ref_buf, off, snip);
        } else if (op == 1) {
            const start = rng.uintLessThan(usize, cur_len);
            const max_del = @min(cur_len - start, 8);
            const del_len = rng.uintLessThan(usize, max_del + 1);
            try rope.deleteRange(start, del_len);
            naiveDelete(&ref_buf, start, del_len);
        } else {
            const start = rng.uintLessThan(usize, cur_len);
            const max_del = @min(cur_len - start, 6);
            const del_len = rng.uintLessThan(usize, max_del + 1);
            const snip = pickSnippet(rng.uintLessThan(usize, 6));
            try rope.replaceRange(start, del_len, snip);
            naiveDelete(&ref_buf, start, del_len);
            try naiveInsert(alloc, &ref_buf, start, snip);
        }

        expect_eq(rope.len(), ref_buf.items.len);
        if (step % 25 == 0) {
            const actual = try rope.toString(alloc);
            defer alloc.free(actual);
            try std.testing.expectEqualStrings(ref_buf.items, actual);
        }
        step += 1;
    }

    const final_str = try rope.toString(alloc);
    defer alloc.free(final_str);
    try std.testing.expectEqualStrings(ref_buf.items, final_str);
}

test("2. Multi-line lineCount, getLine, offsetToLineCol & lineColToOffset (LF & CRLF)") {
    const alloc = std.testing.allocator;
    let rope = try RopeText.initFromText(alloc, "first\r\n\nthird line");
    defer rope.deinit();

    // Split across pieces to ensure line mapping works across multiple fragments.
    try rope.insert(5, "!");
    // Text is now: "first!\r\n\nthird line"
    expect_eq(rope.lineCount(), 3);

    const l0 = try rope.getLine(alloc, 0);
    defer alloc.free(l0);
    const l1 = try rope.getLine(alloc, 1);
    defer alloc.free(l1);
    const l2 = try rope.getLine(alloc, 2);
    defer alloc.free(l2);

    try std.testing.expectEqualStrings("first!", l0);
    try std.testing.expectEqualStrings("", l1);
    try std.testing.expectEqualStrings("third line", l2);

    const lc0 = try rope.offsetToLineCol(0);
    expect_eq(lc0.line, 0);
    expect_eq(lc0.col, 0);

    const lc1 = try rope.offsetToLineCol(8);
    expect_eq(lc1.line, 1);
    expect_eq(lc1.col, 0);

    const lc2 = try rope.offsetToLineCol(14);
    expect_eq(lc2.line, 2);
    expect_eq(lc2.col, 5);

    const off2 = try rope.lineColToOffset(2, 5);
    expect_eq(off2, 14);

    const end_off = try rope.lineColToOffset(2, 10);
    expect_eq(end_off, rope.len());
}

test("3. Multi-step Undo/Redo Rollback & Branching Redo-Stack Clear") {
    const alloc = std.testing.allocator;
    let rope = try RopeText.initFromText(alloc, "hello world");
    defer rope.deinit();

    try std.testing.expect(!rope.canUndo());
    try std.testing.expect(!rope.canRedo());

    try rope.insert(5, " CP");
    try rope.replaceRange(0, 5, "hi");
    try rope.deleteRange(2, 3);
    // "hi world"
    {
        const s = try rope.toString(alloc);
        defer alloc.free(s);
        try std.testing.expectEqualStrings("hi world", s);
    }

    try std.testing.expect(rope.canUndo());
    try std.testing.expect(try rope.undo());
    try std.testing.expect(try rope.undo());
    {
        const s = try rope.toString(alloc);
        defer alloc.free(s);
        try std.testing.expectEqualStrings("hello CP world", s);
    }
    try std.testing.expect(rope.canRedo());
    try std.testing.expect(try rope.redo());
    {
        const s = try rope.toString(alloc);
        defer alloc.free(s);
        try std.testing.expectEqualStrings("hi CP world", s);
    }

    // New edit must clear redo stack.
    try rope.insert(rope.len(), "!");
    try std.testing.expect(!rope.canRedo());
    try std.testing.expect(!(try rope.redo()));

    while (rope.canUndo()) {
        * = try rope.undo();
    }
    const back = try rope.toString(alloc);
    defer alloc.free(back);
    try std.testing.expectEqualStrings("hello world", back);
}

test("4. OutOfBounds Guards, Empty Boundaries & Tail-Append Coalescing") {
    const alloc = std.testing.allocator;
    let rope = try RopeText.initFromText(alloc, "");
    defer rope.deinit();

    expect_eq(rope.len(), 0);
    expect_eq(rope.lineCount(), 1);
    const empty_line = try rope.getLine(alloc, 0);
    defer alloc.free(empty_line);
    try std.testing.expectEqualStrings("", empty_line);

    try std.testing.expectError(RopeError.OutOfBounds, rope.byteAt(0));
    try std.testing.expectError(RopeError.OutOfBounds, rope.insert(1, "x"));
    try std.testing.expectError(RopeError.OutOfBounds, rope.deleteRange(0, 1));
    try std.testing.expectError(RopeError.OutOfBounds, rope.getLine(alloc, 1));
    try std.testing.expectError(RopeError.OutOfBounds, rope.offsetToLineCol(1));
    try std.testing.expectError(RopeError.OutOfBounds, rope.lineColToOffset(0, 1));

    // Sequential tail appends should coalesce into a single piece.
    let k: usize = 0;
    while (k < 50) {
        try rope.insert(rope.len(), "a");
        k += 1;
    }
    expect_eq(rope.len(), 50);
    expect_eq(rope.pieceCount(), 1);
    expect_eq(try rope.byteAt(49), 'a');
}

test("5. Dynamic cap.rope Namespace Mounting & Zero-Leak Verification") {
    const alloc = std.testing.allocator;
    let rope = try cap.rope.RopeText.initFromText(alloc, "module:\n");
    defer rope.deinit();

    try rope.insert(rope.len(), "cap.rope");
    const sub = try rope.slice(alloc, 8, 16);
    defer alloc.free(sub);
    try std.testing.expectEqualStrings("cap.rope", sub);

    const lc: LineCol = try rope.offsetToLineCol(12);
    expect_eq(lc.line, 1);
    expect_eq(lc.col, 4);
}
