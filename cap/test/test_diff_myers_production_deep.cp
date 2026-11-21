// Source: native-cp-stdlib (Myers SES Line Diff & Unified Diff)
import std from "std";
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    MyersOpKind,
    diffLines,
    reconstructNew,
    formatUnifiedDiff,
} from "cap:diff";

const DiffOpKind = MyersOpKind;

test "diffLines produces exact SES operations for insert, delete, and replace" {
    const alloc = std.testing.allocator;
    const old_text = "alpha\nbeta\ngamma\ndelta";
    const new_text = "alpha\nbeta_mod\ngamma\nepsilon\ndelta";

    let res = try diffLines(alloc, old_text, new_text);
    defer res.deinit(alloc);

    expect_eq(res.deleted, @as(usize, 1));
    expect_eq(res.added, @as(usize, 2));
    expect_eq(res.ops.len, @as(usize, 6));

    expect_eq(res.ops[0].kind == DiffOpKind.Equal, true);
    expect_eq(std.mem.eql(u8, res.ops[0].line, "alpha"), true);
    expect_eq(res.ops[0].old_line_no orelse 0, @as(usize, 1));
    expect_eq(res.ops[0].new_line_no orelse 0, @as(usize, 1));

    expect_eq(res.ops[1].kind == DiffOpKind.Delete, true);
    expect_eq(std.mem.eql(u8, res.ops[1].line, "beta"), true);
    expect_eq(res.ops[1].old_line_no orelse 0, @as(usize, 2));
    expect_eq(res.ops[1].new_line_no == null, true);

    expect_eq(res.ops[2].kind == DiffOpKind.Insert, true);
    expect_eq(std.mem.eql(u8, res.ops[2].line, "beta_mod"), true);
    expect_eq(res.ops[2].old_line_no == null, true);
    expect_eq(res.ops[2].new_line_no orelse 0, @as(usize, 2));

    expect_eq(res.ops[3].kind == DiffOpKind.Equal, true);
    expect_eq(std.mem.eql(u8, res.ops[3].line, "gamma"), true);

    expect_eq(res.ops[4].kind == DiffOpKind.Insert, true);
    expect_eq(std.mem.eql(u8, res.ops[4].line, "epsilon"), true);

    expect_eq(res.ops[5].kind == DiffOpKind.Equal, true);
    expect_eq(std.mem.eql(u8, res.ops[5].line, "delta"), true);
}

test "reconstructNew restores new text 100 percent from ops" {
    const alloc = std.testing.allocator;
    const old_text = "line1\nline2\nline3\nline4\n";
    const new_text = "line1\nchanged\nline3\nadded1\nadded2\n";

    let res = try diffLines(alloc, old_text, new_text);
    defer res.deinit(alloc);

    const rebuilt = try reconstructNew(alloc, res.ops);
    defer alloc.free(rebuilt);

    expect_eq(std.mem.eql(u8, rebuilt, new_text), true);
}

test "formatUnifiedDiff generates standard unified diff with context" {
    const alloc = std.testing.allocator;
    const old_text = "1\n2\n3\n4\n5\n6\n7";
    const new_text = "1\n2\n3_x\n4\n5\n6\n7";

    const patch = try formatUnifiedDiff(alloc, "old.txt", "new.txt", old_text, new_text, 1);
    defer alloc.free(patch);

    const expected =
        "--- old.txt\n" ++
        "+++ new.txt\n" ++
        "@@ -2,3 +2,3 @@\n" ++
        " 2\n" ++
        "-3\n" ++
        "+3_x\n" ++
        " 4\n";

    expect_eq(std.mem.eql(u8, patch, expected), true);
}

test "boundary cases: empty, identical, and full replacement" {
    const alloc = std.testing.allocator;

    // 1. Both empty
    let r_empty = try diffLines(alloc, "", "");
    defer r_empty.deinit(alloc);
    expect_eq(r_empty.ops.len, @as(usize, 0));
    expect_eq(r_empty.added, @as(usize, 0));
    expect_eq(r_empty.deleted, @as(usize, 0));

    // 2. Empty to non-empty and reconstruct
    let r_ins = try diffLines(alloc, "", "a\nb");
    defer r_ins.deinit(alloc);
    expect_eq(r_ins.added, @as(usize, 2));
    const rec_ins = try reconstructNew(alloc, r_ins.ops);
    defer alloc.free(rec_ins);
    expect_eq(std.mem.eql(u8, rec_ins, "a\nb"), true);

    // 3. Identical texts produce empty unified diff
    const same_patch = try formatUnifiedDiff(alloc, "a", "b", "x\ny", "x\ny", 2);
    defer alloc.free(same_patch);
    expect_eq(same_patch.len, @as(usize, 0));

    // 4. Full replacement
    let r_rep = try diffLines(alloc, "old1\nold2", "new1\nnew2");
    defer r_rep.deinit(alloc);
    expect_eq(r_rep.deleted, @as(usize, 2));
    expect_eq(r_rep.added, @as(usize, 2));
    const rec_rep = try reconstructNew(alloc, r_rep.ops);
    defer alloc.free(rec_rep);
    expect_eq(std.mem.eql(u8, rec_rep, "new1\nnew2"), true);
}

test "cap.diff_myers dynamic mount and zero-leak check" {
    const alloc = std.testing.allocator;
    let res = try cap.diff_myers.diffLines(alloc, "a\nb", "a\nc");
    defer res.deinit(alloc);

    expect_eq(res.added, @as(usize, 1));
    expect_eq(res.deleted, @as(usize, 1));

    const rebuilt = try cap.diff_myers.reconstructNew(alloc, res.ops);
    defer alloc.free(rebuilt);
    expect_eq(std.mem.eql(u8, rebuilt, "a\nc"), true);
}
