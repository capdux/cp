// Source: cp/cap/test/test_diff3_production_deep.cp
// Deep Production Test Suite for cap:diff3
// Covers:
//   1. Non-overlapping independent changes (ours edits top, theirs edits bottom) clean auto-merge
//   2. Identical changes on both sides (false conflict) auto-resolved cleanly
//   3. Real conflicts rendered in both Standard and Diff3 (with ||||||| base) styles & conflict_count
//   4. Edge cases: empty text, single-line insert/delete, and CRLF (\r\n) preservation
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.diff3` & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Diff3Error,
    ConflictStyle,
    ConflictBlock,
    MergeHunk,
    MergeOptions,
    MergeResult,
    mergeLines,
    mergeTexts,
    hasConflicts,
} from "cap:text";
import cap from "cap";

test("diff3 non-overlapping independent edits merge cleanly") {
    const alloc = std.testing.allocator;

    const base =
        "header\n" ++
        "alpha\n" ++
        "beta\n" ++
        "gamma\n" ++
        "footer\n";

    const ours =
        "header (ours)\n" ++
        "alpha\n" ++
        "beta\n" ++
        "gamma\n" ++
        "footer\n";

    const theirs =
        "header\n" ++
        "alpha\n" ++
        "beta\n" ++
        "gamma\n" ++
        "footer (theirs)\n";

    let res = mergeTexts(alloc, base, ours, theirs, MergeOptions{})?;
    defer res.deinit(alloc);

    expect_eq(res.has_conflicts, false);
    expect_eq(res.conflict_count, usize(0));
    expect_eq(res.hunks.len, usize(1));

    const expected =
        "header (ours)\n" ++
        "alpha\n" ++
        "beta\n" ++
        "gamma\n" ++
        "footer (theirs)\n";
    expect_eq(std.mem.eql(u8, res.merged_text, expected), true);
}

test("diff3 identical changes on both sides resolve without false conflict") {
    const alloc = std.testing.allocator;

    const base = "let x = 1;\nlet y = 2;\nlet z = 3;\n";
    const ours = "let x = 1;\nlet y = 42;\nlet z = 3;\n";
    const theirs = "let x = 1;\nlet y = 42;\nlet z = 3;\n";

    // Default resolve_identical_changes = true
    let res = mergeTexts(alloc, base, ours, theirs, MergeOptions{})?;
    defer res.deinit(alloc);

    expect_eq(res.has_conflicts, false);
    expect_eq(res.conflict_count, usize(0));
    expect_eq(std.mem.eql(u8, res.merged_text, "let x = 1;\nlet y = 42;\nlet z = 3;\n"), true);

    // When resolve_identical_changes = false, same edit is reported as conflict
    let strict_res = mergeTexts(alloc, base, ours, theirs, MergeOptions{
        resolve_identical_changes: false,
    })?;
    defer strict_res.deinit(alloc);

    expect_eq(strict_res.has_conflicts, true);
    expect_eq(strict_res.conflict_count, usize(1));
}

test("diff3 real conflict in Standard and Diff3 styles with accurate conflict_count") {
    const alloc = std.testing.allocator;

    const base = "top\nmid_a\nmid_b\nbot\n";
    const ours = "top\nours_1\nmid_b\nbot_ours\n";
    const theirs = "top\ntheirs_1\nmid_b\nbot_theirs\n";

    // 1. Standard style
    let std_res = mergeTexts(alloc, base, ours, theirs, MergeOptions{
        ours_label: "HEAD",
        theirs_label: "feature",
        style: ConflictStyle.Standard,
    })?;
    defer std_res.deinit(alloc);

    expect_eq(std_res.has_conflicts, true);
    expect_eq(std_res.conflict_count, usize(2));

    const expected_std =
        "top\n" ++
        "<<<<<<< HEAD\n" ++
        "ours_1\n" ++
        "=======\n" ++
        "theirs_1\n" ++
        ">>>>>>> feature\n" ++
        "mid_b\n" ++
        "<<<<<<< HEAD\n" ++
        "bot_ours\n" ++
        "=======\n" ++
        "bot_theirs\n" ++
        ">>>>>>> feature\n";
    expect_eq(std.mem.eql(u8, std_res.merged_text, expected_std), true);

    // 2. Diff3 style (includes ||||||| base_label and base lines)
    let d3_res = mergeTexts(alloc, base, ours, theirs, MergeOptions{
        ours_label: "HEAD",
        base_label: "ancestor",
        theirs_label: "feature",
        style: ConflictStyle.Diff3,
    })?;
    defer d3_res.deinit(alloc);

    expect_eq(d3_res.has_conflicts, true);
    expect_eq(d3_res.conflict_count, usize(2));

    const expected_d3 =
        "top\n" ++
        "<<<<<<< HEAD\n" ++
        "ours_1\n" ++
        "||||||| ancestor\n" ++
        "mid_a\n" ++
        "=======\n" ++
        "theirs_1\n" ++
        ">>>>>>> feature\n" ++
        "mid_b\n" ++
        "<<<<<<< HEAD\n" ++
        "bot_ours\n" ++
        "||||||| ancestor\n" ++
        "bot\n" ++
        "=======\n" ++
        "bot_theirs\n" ++
        ">>>>>>> feature\n";
    expect_eq(std.mem.eql(u8, d3_res.merged_text, expected_d3), true);
}

test("diff3 empty text, single-line insert/delete, and CRLF preservation") {
    const alloc = std.testing.allocator;

    // 1. All empty
    let empty_res = mergeTexts(alloc, "", "", "", MergeOptions{})?;
    defer empty_res.deinit(alloc);
    expect_eq(empty_res.has_conflicts, false);
    expect_eq(empty_res.merged_text.len, usize(0));

    // 2. Empty base, ours inserts, theirs empty -> clean insert
    let ins_res = mergeTexts(alloc, "", "hello\n", "", MergeOptions{})?;
    defer ins_res.deinit(alloc);
    expect_eq(ins_res.has_conflicts, false);
    expect_eq(std.mem.eql(u8, ins_res.merged_text, "hello\n"), true);

    // 3. Single-line delete on ours, unchanged on theirs -> clean delete
    let del_res = mergeTexts(alloc, "only_line\n", "", "only_line\n", MergeOptions{})?;
    defer del_res.deinit(alloc);
    expect_eq(del_res.has_conflicts, false);
    expect_eq(del_res.merged_text.len, usize(0));

    // 4. CRLF preservation
    const base_crlf = "a\r\nb\r\nc\r\n";
    const ours_crlf = "a_ours\r\nb\r\nc\r\n";
    const theirs_crlf = "a\r\nb\r\nc_theirs\r\n";
    let crlf_res = mergeTexts(alloc, base_crlf, ours_crlf, theirs_crlf, MergeOptions{})?;
    defer crlf_res.deinit(alloc);

    expect_eq(crlf_res.has_conflicts, false);
    expect_eq(std.mem.eql(u8, crlf_res.merged_text, "a_ours\r\nb\r\nc_theirs\r\n"), true);
}

test("diff3 cap.diff3 lazy mounting, mergeLines, hasConflicts, and zero leaks") {
    const alloc = std.testing.allocator;

    const base_lines: [3][]const u8 = ["one", "two", "three"];
    const ours_lines: [3][]const u8 = ["one_ours", "two", "three"];
    const theirs_lines: [3][]const u8 = ["one", "two", "three_theirs"];

    let res = cap.text.mergeLines(alloc, &base_lines, &ours_lines, &theirs_lines, cap.text.MergeOptions{})?;
    defer res.deinit(alloc);

    expect_eq(res.has_conflicts, false);
    expect_eq(res.conflict_count, usize(0));
    expect_eq(std.mem.eql(u8, res.merged_text, "one_ours\ntwo\nthree_theirs\n"), true);

    expect_eq(hasConflicts("a\nb\n", "a1\nb\n", "a2\nb\n"), true);
    expect_eq(cap.text.hasConflicts("a\nb\n", "a1\nb\n", "a\nb1\n"), false);
}
