// Source: cp/cap/test/test_diff_production_deep.cp
// Deep Production Test Suite for cap:diff
// Covers:
//   1. DiffEngine.diffLines & diffWords: shortest edit script, empty/insert-all/delete-all, stats
//   2. UnifiedDiff.createPatch multi-hunk grouping & applyPatch exact round-trip verification
//   3. Malformed patch interception (InvalidPatch) & tampered context/line mismatch (HunkMismatch)
//   4. UnifiedDiff.merge3: non-overlapping clean merge & overlapping conflict markers
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.diff`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DiffError,
    DiffOpKind,
    DiffEdit,
    DiffStats,
    DiffEngine,
    MergeResult,
    UnifiedDiff,
} from "cap:diff";
import cap from "cap";

test("DiffEngine diffLines and diffWords shortest edit script and edge cases") {
    const alloc = std.heap.page_allocator;

    // 1. Empty -> Empty
    const empty_edits = DiffEngine.diffLines(alloc, "", "")?;
    defer alloc.free(empty_edits);
    expect_eq(empty_edits.len, usize(0));

    // 2. Empty -> Insert all
    const ins_edits = DiffEngine.diffLines(alloc, "", "alpha\nbeta\n")?;
    defer alloc.free(ins_edits);
    const ins_stats = DiffEngine.computeStats(ins_edits);
    expect_eq(ins_stats.equal_count, usize(0));
    expect_eq(ins_stats.insert_count, usize(2));
    expect_eq(ins_stats.delete_count, usize(0));

    // 3. Delete all -> Empty
    const del_edits = DiffEngine.diffLines(alloc, "one\ntwo\nthree\n", "")?;
    defer alloc.free(del_edits);
    const del_stats = DiffEngine.computeStats(del_edits);
    expect_eq(del_stats.equal_count, usize(0));
    expect_eq(del_stats.insert_count, usize(0));
    expect_eq(del_stats.delete_count, usize(3));

    // 4. Shortest edit script verification on classic Myers sequence
    const old_seq = "A\nB\nC\nA\nB\nB\nA\n";
    const new_seq = "C\nB\nA\nB\nA\nC\n";
    const ses_edits = DiffEngine.diffLines(alloc, old_seq, new_seq)?;
    defer alloc.free(ses_edits);
    const ses_stats = DiffEngine.computeStats(ses_edits);
    // Shortest edit distance between ABCABBA and CBABAC is 5 (e.g. 3 deletes + 2 inserts, 4 equals)
    expect_eq(ses_stats.delete_count + ses_stats.insert_count, usize(5));
    expect_eq(ses_stats.equal_count, usize(4));

    // 5. Word-level fine-grained diff
    const w_edits = DiffEngine.diffWords(alloc, "let count = 10;", "let total = 42;")?;
    defer alloc.free(w_edits);
    const w_stats = DiffEngine.computeStats(w_edits);
    // Tokens: "let", " ", "count"/"total", " ", "=", " ", "10"/"42", ";"
    expect_eq(w_stats.equal_count, usize(6));
    expect_eq(w_stats.delete_count, usize(2));
    expect_eq(w_stats.insert_count, usize(2));
}

test("UnifiedDiff createPatch multi-hunk grouping and applyPatch round-trip") {
    const alloc = std.heap.page_allocator;

    const old_doc =
        "line 01\n" ++
        "line 02\n" ++
        "line 03\n" ++
        "line 04\n" ++
        "line 05\n" ++
        "line 06\n" ++
        "line 07\n" ++
        "line 08\n" ++
        "line 09\n" ++
        "line 10\n" ++
        "line 11\n" ++
        "line 12\n";

    const new_doc =
        "line 01\n" ++
        "line 02 modified\n" ++
        "line 03\n" ++
        "line 04\n" ++
        "line 05\n" ++
        "line 06\n" ++
        "line 07\n" ++
        "line 08\n" ++
        "line 09\n" ++
        "line 10\n" ++
        "line 11 modified\n" ++
        "line 11.5 inserted\n" ++
        "line 12\n";

    // With context_lines = 1, gap between line 2 and line 11 is 8 equal lines (> 2*1), so 2 hunks must be emitted
    const patch = UnifiedDiff.createPatch(alloc, "src/app.cp", "src/app.cp", old_doc, new_doc, 1)?;
    defer alloc.free(patch);

    expect_eq(std.mem.indexOf(u8, patch, "--- a/src/app.cp") != null, true);
    expect_eq(std.mem.indexOf(u8, patch, "+++ b/src/app.cp") != null, true);

    // Verify at least 2 hunk headers exist
    const first_hunk = std.mem.indexOf(u8, patch, "@@ ").?;
    const second_hunk = std.mem.indexOfPos(u8, patch, first_hunk + 3, "@@ ");
    expect_eq(second_hunk != null, true);

    const applied = UnifiedDiff.applyPatch(alloc, old_doc, patch)?;
    defer alloc.free(applied);
    expect_eq(std.mem.eql(u8, applied, new_doc), true);

    // Trailing newline difference round-trip
    const no_nl_old = "alpha\nbeta";
    const nl_new = "alpha\nbeta\n";
    const nl_patch = UnifiedDiff.createPatch(alloc, "nl.txt", "nl.txt", no_nl_old, nl_new, 1)?;
    defer alloc.free(nl_patch);
    const nl_applied = UnifiedDiff.applyPatch(alloc, no_nl_old, nl_patch)?;
    defer alloc.free(nl_applied);
    expect_eq(std.mem.eql(u8, nl_applied, nl_new), true);
}

test("UnifiedDiff applyPatch strict InvalidPatch and HunkMismatch interception") {
    const alloc = std.heap.page_allocator;

    const base_text = "a\nb\nc\nd\ne\n";

    // 1. Garbage patch without @@ hunk header -> InvalidPatch
    let got_invalid = false;
    const res_inv = UnifiedDiff.applyPatch(alloc, base_text, "not a unified diff\njust noise\n") catch e => {
        got_invalid = (e == DiffError.InvalidPatch);
        ""
    };
    * = res_inv;
    expect_eq(got_invalid, true);

    // 2. Patch with wrong line count in header -> InvalidPatch
    const bad_count_patch =
        "--- a/f.txt\n" ++
        "+++ b/f.txt\n" ++
        "@@ -1,5 +1,3 @@\n" ++
        " a\n" ++
        "-b\n" ++
        "+B\n" ++
        " c\n";
    let caught_bad_count = false;
    const res_bad = UnifiedDiff.applyPatch(alloc, base_text, bad_count_patch) catch e => {
        caught_bad_count = (e == DiffError.InvalidPatch);
        ""
    };
    * = res_bad;
    expect_eq(caught_bad_count, true);

    // 3. Valid patch whose context was tampered -> HunkMismatch
    const valid_patch = UnifiedDiff.createPatch(alloc, "f.txt", "f.txt", base_text, "a\nB\nc\nd\ne\n", 1)?;
    defer alloc.free(valid_patch);

    const tampered_base = "a\nWRONG\nc\nd\ne\n";
    let caught_mismatch = false;
    const res_mis = UnifiedDiff.applyPatch(alloc, tampered_base, valid_patch) catch e => {
        caught_mismatch = (e == DiffError.HunkMismatch);
        ""
    };
    * = res_mis;
    expect_eq(caught_mismatch, true);
}

test("UnifiedDiff merge3 clean non-overlapping merge and conflict detection") {
    const alloc = std.heap.page_allocator;

    const base =
        "fn init() {\n" ++
        "    let port = 8080;\n" ++
        "    let host = \"127.0.0.1\";\n" ++
        "    let workers = 4;\n" ++
        "    let timeout = 30;\n" ++
        "}\n";

    // Ours changes port; Theirs changes timeout -> non-overlapping clean merge
    const ours_clean =
        "fn init() {\n" ++
        "    let port = 9090;\n" ++
        "    let host = \"127.0.0.1\";\n" ++
        "    let workers = 4;\n" ++
        "    let timeout = 30;\n" ++
        "}\n";

    const theirs_clean =
        "fn init() {\n" ++
        "    let port = 8080;\n" ++
        "    let host = \"127.0.0.1\";\n" ++
        "    let workers = 4;\n" ++
        "    let timeout = 60;\n" ++
        "}\n";

    const clean_res = UnifiedDiff.merge3(alloc, base, ours_clean, theirs_clean)?;
    defer alloc.free(clean_res.merged_text);

    expect_eq(clean_res.clean, true);
    expect_eq(clean_res.conflict_count, usize(0));
    expect_eq(std.mem.indexOf(u8, clean_res.merged_text, "let port = 9090;") != null, true);
    expect_eq(std.mem.indexOf(u8, clean_res.merged_text, "let timeout = 60;") != null, true);

    // Overlapping divergent changes on `host` -> conflict markers
    const ours_conflict =
        "fn init() {\n" ++
        "    let port = 8080;\n" ++
        "    let host = \"0.0.0.0\";\n" ++
        "    let workers = 4;\n" ++
        "    let timeout = 30;\n" ++
        "}\n";

    const theirs_conflict =
        "fn init() {\n" ++
        "    let port = 8080;\n" ++
        "    let host = \"10.0.0.1\";\n" ++
        "    let workers = 4;\n" ++
        "    let timeout = 30;\n" ++
        "}\n";

    const conf_res = UnifiedDiff.merge3(alloc, base, ours_conflict, theirs_conflict)?;
    defer alloc.free(conf_res.merged_text);

    expect_eq(conf_res.clean, false);
    expect_eq(conf_res.conflict_count >= 1, true);
    expect_eq(std.mem.indexOf(u8, conf_res.merged_text, "<<<<<<< ours") != null, true);
    expect_eq(std.mem.indexOf(u8, conf_res.merged_text, "=======") != null, true);
    expect_eq(std.mem.indexOf(u8, conf_res.merged_text, ">>>>>>> theirs") != null, true);
}

test("cap.diff dynamic lazy namespace mounting") {
    const alloc = std.heap.page_allocator;

    const old_s = "hello world\nline two\n";
    const new_s = "hello cp\nline two\n";

    const edits = cap.diff.DiffEngine.diffLines(alloc, old_s, new_s)?;
    defer alloc.free(edits);
    const stats = cap.diff.DiffEngine.computeStats(edits);
    expect_eq(stats.equal_count, usize(1));
    expect_eq(stats.delete_count, usize(1));
    expect_eq(stats.insert_count, usize(1));

    const patch = cap.diff.UnifiedDiff.createPatch(alloc, "demo.txt", "demo.txt", old_s, new_s, 1)?;
    defer alloc.free(patch);
    const patched = cap.diff.UnifiedDiff.applyPatch(alloc, old_s, patch)?;
    defer alloc.free(patched);
    expect_eq(std.mem.eql(u8, patched, new_s), true);
}
