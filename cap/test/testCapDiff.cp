// cap/test/testCapDiff.cp
// Comprehensive unit tests for @cap/diff:
// - Identical texts (including empty texts)
// - Pure insertion & pure deletion
// - Middle modifications, line reordering, and multi-hunk scenarios
// - Character/byte-level Myers diff
// - Standard GNU Unified Diff formatting (single hunk, multi-hunk, context merging, no-newline)
// - Lossless applyLineDiff reconstruction & mismatch/invalid-patch rejection
// - Resource limit protection (TooManyLines, BufferTooSmall)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MAX_LINES,
    MAX_EDITS,
    DiffError,
    DiffOp,
    DiffEdit,
    DiffStats,
    LineDiffResult,
    diffLines,
    diffChars,
    formatUnifiedDiff,
    applyLineDiff,
} from "../../packages/diff/src/index.cp";

fn expectDiffLinesError(old_text: []const u8, new_text: []const u8, expected: DiffError): bool {
    * = diffLines(old_text, new_text) catch |e| {
        return e == expected;
    };
    return false;
}

fn expectDiffCharsError(old_text: []const u8, new_text: []const u8, expected: DiffError): bool {
    * = diffChars(old_text, new_text) catch |e| {
        return e == expected;
    };
    return false;
}

fn expectUnifiedError(
    old_text: []const u8,
    new_text: []const u8,
    buf_len: usize,
    expected: DiffError,
): bool {
    let buf: [256]u8 = undefined;
    * = formatUnifiedDiff("a", "b", old_text, new_text, 3, buf[0..buf_len]) catch |e| {
        return e == expected;
    };
    return false;
}

fn expectApplyError(
    old_text: []const u8,
    res: LineDiffResult,
    buf_len: usize,
    expected: DiffError,
): bool {
    let buf: [256]u8 = undefined;
    * = applyLineDiff(old_text, res, buf[0..buf_len]) catch |e| {
        return e == expected;
    };
    return false;
}

fn main() {
    test("1. @cap/diff — Identical Texts & Empty Texts", () => {
        const empty_res = diffLines("", "") catch unreachable;
        expect_eq(empty_res.len, 0 as usize);
        expect_eq(empty_res.isIdentical(), true);
        const empty_stats = empty_res.stats();
        expect_eq(empty_stats.equal, 0 as usize);
        expect_eq(empty_stats.inserted, 0 as usize);
        expect_eq(empty_stats.deleted, 0 as usize);
        expect_eq(empty_stats.similarity(), 1.0);

        const doc = "line1\nline2\nline3\n";
        const same_res = diffLines(doc, doc) catch unreachable;
        expect_eq(same_res.len, 3 as usize);
        expect_eq(same_res.isIdentical(), true);
        const same_stats = same_res.stats();
        expect_eq(same_stats.equal, 3 as usize);
        expect_eq(same_stats.inserted, 0 as usize);
        expect_eq(same_stats.deleted, 0 as usize);
        expect_eq(same_stats.similarity(), 1.0);

        // Unified diff of identical texts is empty
        let ubuf: [256]u8 = undefined;
        const udiff = formatUnifiedDiff("a.txt", "b.txt", doc, doc, 3, ubuf[0..]) catch unreachable;
        expect_eq(udiff.len, 0 as usize);

        // Lossless apply
        let abuf: [256]u8 = undefined;
        const applied = applyLineDiff(doc, same_res, abuf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, applied, doc), true);
    });

    test("2. @cap/diff — Pure Insertion & Pure Deletion", () => {
        const text = "alpha\nbeta\ngamma\n";

        // Pure insertion: "" -> text
        const ins_res = diffLines("", text) catch unreachable;
        expect_eq(ins_res.isIdentical(), false);
        expect_eq(ins_res.len, 3 as usize);
        const ins_stats = ins_res.stats();
        expect_eq(ins_stats.equal, 0 as usize);
        expect_eq(ins_stats.inserted, 3 as usize);
        expect_eq(ins_stats.deleted, 0 as usize);
        expect_eq(ins_stats.similarity(), 0.0);

        let buf1: [256]u8 = undefined;
        const rebuilt_ins = applyLineDiff("", ins_res, buf1[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, rebuilt_ins, text), true);

        // Pure deletion: text -> ""
        const del_res = diffLines(text, "") catch unreachable;
        expect_eq(del_res.isIdentical(), false);
        expect_eq(del_res.len, 3 as usize);
        const del_stats = del_res.stats();
        expect_eq(del_stats.equal, 0 as usize);
        expect_eq(del_stats.inserted, 0 as usize);
        expect_eq(del_stats.deleted, 3 as usize);
        expect_eq(del_stats.similarity(), 0.0);

        let buf2: [256]u8 = undefined;
        const rebuilt_del = applyLineDiff(text, del_res, buf2[0..]) catch unreachable;
        expect_eq(rebuilt_del.len, 0 as usize);
    });

    test("3. @cap/diff — Middle Modifications, Myers SES & Lossless Apply", () => {
        const old_doc = "a\nb\nc\nd\ne\nf\ng\n";
        const new_doc = "a\nb\nX\nd\nY\nZ\ng\n";

        const res = diffLines(old_doc, new_doc) catch unreachable;
        expect_eq(res.isIdentical(), false);

        // Common lines: a, b, d, g (4 equal); deleted: c, e, f (3); inserted: X, Y, Z (3)
        const st = res.stats();
        expect_eq(st.equal, 4 as usize);
        expect_eq(st.deleted, 3 as usize);
        expect_eq(st.inserted, 3 as usize);
        // similarity = 2*4 / (7 + 7) = 8/14
        const sim = st.similarity();
        expect_eq(sim > 0.57 && sim < 0.58, true);

        let out: [512]u8 = undefined;
        const rebuilt = applyLineDiff(old_doc, res, out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, rebuilt, new_doc), true);

        // Missing trailing newline transition: "a\nb" -> "a\nb\n"
        const no_nl = "a\nb";
        const with_nl = "a\nb\n";
        const nl_res = diffLines(no_nl, with_nl) catch unreachable;
        expect_eq(nl_res.isIdentical(), false);
        let nl_out: [128]u8 = undefined;
        const nl_rebuilt = applyLineDiff(no_nl, nl_res, nl_out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, nl_rebuilt, with_nl), true);
    });

    test("4. @cap/diff — Multi-Hunk Unified Diff Formatting & Context Merging", () => {
        const old_src =
            "L1\nL2\nL3\nL4\nL5\nL6\nL7\nL8\nL9\nL10\n";
        const new_src =
            "L1\nL2_MOD\nL3\nL4\nL5\nL6\nL7\nL8\nL9_MOD\nL10\n";

        // With context=1, gap between L2 and L9 is 6 equal lines (> 2*1), so 2 separate hunks
        let buf_split: [1024]u8 = undefined;
        const u_split = formatUnifiedDiff(
            "a/file.cp",
            "b/file.cp",
            old_src,
            new_src,
            1,
            buf_split[0..],
        ) catch unreachable;

        const expected_split =
            "--- a/file.cp\n" ++
            "+++ b/file.cp\n" ++
            "@@ -1,3 +1,3 @@\n" ++
            " L1\n" ++
            "-L2\n" ++
            "+L2_MOD\n" ++
            " L3\n" ++
            "@@ -8,3 +8,3 @@\n" ++
            " L8\n" ++
            "-L9\n" ++
            "+L9_MOD\n" ++
            " L10\n";
        expect_eq(std.mem.eql(u8, u_split, expected_split), true);

        // With context=3, gap is 6 <= 2*3, so both changes merge into a single hunk!
        let buf_merged: [1024]u8 = undefined;
        const u_merged = formatUnifiedDiff(
            "a/file.cp",
            "b/file.cp",
            old_src,
            new_src,
            3,
            buf_merged[0..],
        ) catch unreachable;

        const expected_merged =
            "--- a/file.cp\n" ++
            "+++ b/file.cp\n" ++
            "@@ -1,10 +1,10 @@\n" ++
            " L1\n" ++
            "-L2\n" ++
            "+L2_MOD\n" ++
            " L3\n" ++
            " L4\n" ++
            " L5\n" ++
            " L6\n" ++
            " L7\n" ++
            " L8\n" ++
            "-L9\n" ++
            "+L9_MOD\n" ++
            " L10\n";
        expect_eq(std.mem.eql(u8, u_merged, expected_merged), true);

        // No-newline-at-EOF marker
        let buf_eof: [512]u8 = undefined;
        const u_eof = formatUnifiedDiff("old", "new", "a\nb", "a\nb\n", 1, buf_eof[0..]) catch unreachable;
        const expected_eof =
            "--- old\n" ++
            "+++ new\n" ++
            "@@ -1,2 +1,2 @@\n" ++
            " a\n" ++
            "-b\n" ++
            "\\ No newline at end of file\n" ++
            "+b\n";
        expect_eq(std.mem.eql(u8, u_eof, expected_eof), true);
    });

    test("5. @cap/diff — Character-Level Myers Diff", () => {
        const res = diffChars("kitten", "sitting") catch unreachable;
        expect_eq(res.isIdentical(), false);
        const st = res.stats();
        // LCS of "kitten" and "sitting" is "ittn" (len 4)
        // deleted: 'k', 'e' (2); inserted: 's', 'i', 'g' (3)
        expect_eq(st.equal, 4 as usize);
        expect_eq(st.deleted, 2 as usize);
        expect_eq(st.inserted, 3 as usize);

        // Reconstruct "sitting" from Equal + Insert edits
        let rebuilt: [32]u8 = undefined;
        let rlen: usize = 0;
        let i: usize = 0;
        while (i < res.len) : (i += 1) {
            const e = res.edits[i];
            if (e.op != DiffOp.Delete) {
                rebuilt[rlen] = e.text[0];
                rlen += 1;
            }
        }
        expect_eq(std.mem.eql(u8, rebuilt[0..rlen], "sitting"), true);

        // Identical char diff
        const same_c = diffChars("hello", "hello") catch unreachable;
        expect_eq(same_c.isIdentical(), true);
        expect_eq(same_c.stats().equal, 5 as usize);
    });

    test("6. @cap/diff — Bounds, Patch Failure & Buffer-Too-Small Protection", () => {
        // Build a 257-line input to trigger TooManyLines (> MAX_LINES = 256)
        let big_lines: [514]u8 = undefined;
        let idx: usize = 0;
        while (idx < 257) : (idx += 1) {
            big_lines[idx * 2] = 'a';
            big_lines[idx * 2 + 1] = '\n';
        }
        expect_eq(expectDiffLinesError(big_lines[0..], "a\n", DiffError.TooManyLines), true);
        expect_eq(expectDiffLinesError("a\n", big_lines[0..], DiffError.TooManyLines), true);

        // 257-byte input to diffChars triggers TooManyLines
        let big_chars: [257]u8 = undefined;
        let c: usize = 0;
        while (c < 257) : (c += 1) {
            big_chars[c] = 'x';
        }
        expect_eq(expectDiffCharsError(big_chars[0..], "x", DiffError.TooManyLines), true);

        // BufferTooSmall in formatUnifiedDiff
        expect_eq(expectUnifiedError("a\n", "b\n", 8, DiffError.BufferTooSmall), true);

        // BufferTooSmall in applyLineDiff
        const res = diffLines("hello\n", "hello world\n") catch unreachable;
        expect_eq(expectApplyError("hello\n", res, 4, DiffError.BufferTooSmall), true);

        // PatchFailed when applying diff to mismatched old_text
        expect_eq(expectApplyError("wrong\n", res, 128, DiffError.PatchFailed), true);

        // InvalidPatch when diff_res.len > MAX_EDITS
        let bad_res = LineDiffResult{};
        bad_res.len = MAX_EDITS + 1;
        expect_eq(expectApplyError("hello\n", bad_res, 128, DiffError.InvalidPatch), true);
    });
}
