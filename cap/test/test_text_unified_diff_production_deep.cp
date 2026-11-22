// Origin: Pure-CP Stdlib Test Suite (AI Coding Agent Unified Diff Hunk Parser & Zero-Alloc Patch Applier) | Created: 2026-11-22
// Deep Production Verification Suite for lib/text/unified_diff.cp
// Covers:
//   1. Single-hunk add/remove/replace with file headers and "\ No newline at end of file" handling
//   2. Multi-hunk sequential application with cumulative line-count shifts & exact output verification
//   3. Abbreviated hunk header parsing (omitted ",count" defaulting to 1), section suffixes & malformed rejection
//   4. Context/remove mismatch (ContextMismatch), out-of-range (LineOutOfRange), InvalidHunk & zero-corruption atomicity
//   5. BufferTooSmall protection (sentinel untouched), empty patch passthrough & empty-file insertion

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DiffHunkHeader,
    DiffLineKind,
    DiffOp,
    PatchApplyStatus,
    PatchApplyResult,
    parseHunkHeader,
    parseDiffOp,
    applyUnifiedPatch,
} from "../../lib/text/index.cp";

fn testSingleHunkPatchAndNoNewline()! {
    const orig =
        "line1\n" ++
        "line2\n" ++
        "line3\n" ++
        "line4\n";

    const patch =
        "diff --git a/demo.txt b/demo.txt\n" ++
        "index 1111111..2222222 100644\n" ++
        "--- a/demo.txt\n" ++
        "+++ b/demo.txt\n" ++
        "@@ -1,4 +1,5 @@\n" ++
        " line1\n" ++
        "-line2\n" ++
        "+line2_modified\n" ++
        "+line2_extra\n" ++
        " line3\n" ++
        " line4\n";

    const expected =
        "line1\n" ++
        "line2_modified\n" ++
        "line2_extra\n" ++
        "line3\n" ++
        "line4\n";

    let out_buf: [256]u8 = [0; 256];
    let out: []u8 = out_buf[0..];
    const res = applyUnifiedPatch(orig, patch, out);

    expect_eq(res.status, PatchApplyStatus.Ok);
    expect_eq(res.hunks_applied, 1);
    expect_eq(res.lines_added, 2);
    expect_eq(res.lines_removed, 1);
    expect_eq(res.written_bytes, expected.len);
    expect_eq(std.mem.eql(u8, out[0..res.written_bytes], expected), true);

    // "\ No newline at end of file" strips trailing LF on final added line
    const orig_no_nl = "alpha\nbeta";
    const patch_no_nl =
        "--- a/f.txt\n" ++
        "+++ b/f.txt\n" ++
        "@@ -1,2 +1,2 @@\n" ++
        " alpha\n" ++
        "-beta\n" ++
        "\\ No newline at end of file\n" ++
        "+gamma\n" ++
        "\\ No newline at end of file\n";

    const res2 = applyUnifiedPatch(orig_no_nl, patch_no_nl, out);
    expect_eq(res2.status, PatchApplyStatus.Ok);
    expect_eq(std.mem.eql(u8, out[0..res2.written_bytes], "alpha\ngamma"), true);
}

fn testMultiHunkSequentialWithLineOffsets()! {
    const orig =
        "L1\n" ++
        "L2\n" ++
        "L3\n" ++
        "L4\n" ++
        "L5\n" ++
        "L6\n" ++
        "L7\n" ++
        "L8\n";

    // Hunk 1: at line 2, replaces L2 with L2a + L2b + L2c (+2 net lines)
    // Hunk 2: at original line 5..7, removes L6 & L7 (-2 net lines)
    // Hunk 3: pure insertion after original line 8 (+1 net line)
    const patch =
        "--- a/multi.txt\n" ++
        "+++ b/multi.txt\n" ++
        "@@ -1,3 +1,5 @@\n" ++
        " L1\n" ++
        "-L2\n" ++
        "+L2a\n" ++
        "+L2b\n" ++
        "+L2c\n" ++
        " L3\n" ++
        "@@ -5,3 +7,1 @@\n" ++
        " L5\n" ++
        "-L6\n" ++
        "-L7\n" ++
        "@@ -8,0 +9,1 @@\n" ++
        "+L9_tail\n";

    const expected =
        "L1\n" ++
        "L2a\n" ++
        "L2b\n" ++
        "L2c\n" ++
        "L3\n" ++
        "L4\n" ++
        "L5\n" ++
        "L8\n" ++
        "L9_tail\n";

    let out_buf: [256]u8 = [0; 256];
    let out: []u8 = out_buf[0..];
    const res = applyUnifiedPatch(orig, patch, out);

    expect_eq(res.status, PatchApplyStatus.Ok);
    expect_eq(res.hunks_applied, 3);
    expect_eq(res.lines_added, 4);
    expect_eq(res.lines_removed, 3);
    expect_eq(std.mem.eql(u8, out[0..res.written_bytes], expected), true);
}

fn testAbbreviatedHunkHeaderAndDiffOpParsing()! {
    // Omitted old_count defaults to 1
    const h1 = parseHunkHeader("@@ -10 +10,2 @@") ?? DiffHunkHeader{};
    expect_eq(h1.old_start, 10);
    expect_eq(h1.old_count, 1);
    expect_eq(h1.new_start, 10);
    expect_eq(h1.new_count, 2);

    // Omitted new_count defaults to 1, with trailing section context & CR
    const h2 = parseHunkHeader("@@ -4,3 +4 @@ fn helper()\r") ?? DiffHunkHeader{};
    expect_eq(h2.old_start, 4);
    expect_eq(h2.old_count, 3);
    expect_eq(h2.new_start, 4);
    expect_eq(h2.new_count, 1);

    // Both counts omitted
    const h3 = parseHunkHeader("@@ -1 +1 @@") ?? DiffHunkHeader{};
    expect_eq(h3.old_start, 1);
    expect_eq(h3.old_count, 1);
    expect_eq(h3.new_start, 1);
    expect_eq(h3.new_count, 1);

    // Empty-file insertion header
    const h4 = parseHunkHeader("@@ -0,0 +1,2 @@") ?? DiffHunkHeader{};
    expect_eq(h4.old_start, 0);
    expect_eq(h4.old_count, 0);
    expect_eq(h4.new_start, 1);
    expect_eq(h4.new_count, 2);

    // Malformed headers rejected
    expect_eq(parseHunkHeader("@@ -a,1 +1,1 @@") == null, true);
    expect_eq(parseHunkHeader("@@ -1,1 +1,1 @") == null, true);
    expect_eq(parseHunkHeader("@@ -1,1 +1,1 @@extra") == null, true);
    expect_eq(parseHunkHeader("not a hunk") == null, true);

    // parseDiffOp zero-copy line views
    const op_ctx = parseDiffOp(" hello") ?? DiffOp{};
    expect_eq(op_ctx.kind, DiffLineKind.Context);
    expect_eq(std.mem.eql(u8, op_ctx.text, "hello"), true);

    const op_rem = parseDiffOp("-old") ?? DiffOp{};
    expect_eq(op_rem.kind, DiffLineKind.Remove);
    expect_eq(std.mem.eql(u8, op_rem.text, "old"), true);

    const op_add = parseDiffOp("+new") ?? DiffOp{};
    expect_eq(op_add.kind, DiffLineKind.Add);
    expect_eq(std.mem.eql(u8, op_add.text, "new"), true);

    expect_eq(parseDiffOp("?invalid") == null, true);

    // End-to-end apply using abbreviated @@ -2 +2,2 @@
    const orig = "a\nb\nc\n";
    const patch =
        "@@ -2 +2,2 @@\n" ++
        "-b\n" ++
        "+b1\n" ++
        "+b2\n";
    let out_buf: [64]u8 = [0; 64];
    let out: []u8 = out_buf[0..];
    const res = applyUnifiedPatch(orig, patch, out);
    expect_eq(res.status, PatchApplyStatus.Ok);
    expect_eq(std.mem.eql(u8, out[0..res.written_bytes], "a\nb1\nb2\nc\n"), true);
}

fn testMismatchOutOfRangeAndAtomicNoCorruption()! {
    const orig = "one\ntwo\nthree\n";
    let out_buf: [64]u8 = [0xAB; 64];
    let out: []u8 = out_buf[0..];

    // 1. Context line mismatch -> ContextMismatch, out_buf untouched
    const bad_ctx =
        "@@ -1,2 +1,2 @@\n" ++
        " WRONG_CONTEXT\n" ++
        "-two\n" ++
        "+TWO\n";
    const r1 = applyUnifiedPatch(orig, bad_ctx, out);
    expect_eq(r1.status, PatchApplyStatus.ContextMismatch);
    expect_eq(r1.written_bytes, 0);
    expect_eq(out_buf[0], 0xAB);

    // 2. Remove line mismatch -> ContextMismatch, out_buf untouched
    const bad_rem =
        "@@ -1,2 +1,2 @@\n" ++
        " one\n" ++
        "-NOT_TWO\n" ++
        "+TWO\n";
    const r2 = applyUnifiedPatch(orig, bad_rem, out);
    expect_eq(r2.status, PatchApplyStatus.ContextMismatch);
    expect_eq(r2.written_bytes, 0);
    expect_eq(out_buf[0], 0xAB);

    // 3. Hunk start past EOF -> LineOutOfRange
    const oob_patch =
        "@@ -10,1 +10,1 @@\n" ++
        "-ghost\n" ++
        "+new\n";
    const r3 = applyUnifiedPatch(orig, oob_patch, out);
    expect_eq(r3.status, PatchApplyStatus.LineOutOfRange);
    expect_eq(r3.written_bytes, 0);
    expect_eq(out_buf[0], 0xAB);

    // 4. Truncated hunk body -> InvalidHunk
    const trunc_patch =
        "@@ -1,3 +1,3 @@\n" ++
        " one\n";
    const r4 = applyUnifiedPatch(orig, trunc_patch, out);
    expect_eq(r4.status, PatchApplyStatus.InvalidHunk);
    expect_eq(r4.written_bytes, 0);
    expect_eq(out_buf[0], 0xAB);

    // 5. Malformed hunk header -> InvalidHunk
    const bad_hdr = "@@ broken header @@\n+x\n";
    const r5 = applyUnifiedPatch(orig, bad_hdr, out);
    expect_eq(r5.status, PatchApplyStatus.InvalidHunk);
    expect_eq(r5.written_bytes, 0);
}

fn testBufferTooSmallEmptyPatchAndEmptyFileInsert()! {
    const orig = "hello\nworld\n";
    const patch =
        "@@ -1,2 +1,2 @@\n" ++
        " hello\n" ++
        "-world\n" ++
        "+cp_lang\n";
    // Result is "hello\ncp_lang\n" (14 bytes)

    // 1. Buffer 1 byte too small (13 bytes) -> BufferTooSmall, sentinel untouched
    let small_buf: [13]u8 = [0xCD; 13];
    let small: []u8 = small_buf[0..];
    const r_small = applyUnifiedPatch(orig, patch, small);
    expect_eq(r_small.status, PatchApplyStatus.BufferTooSmall);
    expect_eq(r_small.written_bytes, 0);
    expect_eq(small_buf[0], 0xCD);
    expect_eq(small_buf[12], 0xCD);

    // 2. Exact-size buffer (14 bytes) -> Ok
    let exact_buf: [14]u8 = [0; 14];
    let exact: []u8 = exact_buf[0..];
    const r_exact = applyUnifiedPatch(orig, patch, exact);
    expect_eq(r_exact.status, PatchApplyStatus.Ok);
    expect_eq(r_exact.written_bytes, 14);
    expect_eq(std.mem.eql(u8, exact[0..14], "hello\ncp_lang\n"), true);

    // 3. Empty patch copies original untouched
    let copy_buf: [32]u8 = [0; 32];
    let copy: []u8 = copy_buf[0..];
    const r_empty = applyUnifiedPatch(orig, "", copy);
    expect_eq(r_empty.status, PatchApplyStatus.Ok);
    expect_eq(r_empty.hunks_applied, 0);
    expect_eq(r_empty.written_bytes, orig.len);
    expect_eq(std.mem.eql(u8, copy[0..r_empty.written_bytes], orig), true);

    // 4. Pure insertion into empty original file (@@ -0,0 +1,2 @@)
    const create_patch =
        "--- /dev/null\n" ++
        "+++ b/new.cp\n" ++
        "@@ -0,0 +1,2 @@\n" ++
        "+pub fn ok() {}\n" ++
        "+// done\n";
    const r_create = applyUnifiedPatch("", create_patch, copy);
    expect_eq(r_create.status, PatchApplyStatus.Ok);
    expect_eq(r_create.hunks_applied, 1);
    expect_eq(r_create.lines_added, 2);
    expect_eq(r_create.lines_removed, 0);
    expect_eq(std.mem.eql(u8, copy[0..r_create.written_bytes], "pub fn ok() {}\n// done\n"), true);
}

test("text unified_diff: single-hunk add/remove/replace and no-newline handling", testSingleHunkPatchAndNoNewline);
test("text unified_diff: multi-hunk sequential application with cumulative line offsets", testMultiHunkSequentialWithLineOffsets);
test("text unified_diff: abbreviated hunk header parsing and zero-copy DiffOp", testAbbreviatedHunkHeaderAndDiffOpParsing);
test("text unified_diff: ContextMismatch, LineOutOfRange, InvalidHunk and atomic zero-corruption", testMismatchOutOfRangeAndAtomicNoCorruption);
test("text unified_diff: BufferTooSmall, empty patch passthrough and empty-file creation", testBufferTooSmallEmptyPatchAndEmptyFileInsert);
