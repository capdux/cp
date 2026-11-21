// Source: cp/cap/test/test_diff_unified_production_deep.cp (Production deep tests for cap:diff_unified)

import { test, expect_eq } from "cap:test";
import {
    DiffOpKind,
    DiffLine,
    DiffSummary,
    diffLines,
    summarize,
    formatUnified,
} from "cap:diff_unified";

test("diff_unified: identical texts produce empty unified diff and isIdentical summary") {
    const txt = "alpha\nbeta\ngamma\n";
    let ops: [32]DiffLine = [DiffLine{}; 32];
    const n = diffLines(txt, txt, ops[0..]);
    expect_eq(n, 3);

    const s = summarize(ops[0..n]);
    expect_eq(s.isIdentical(), true);
    expect_eq(s.added, 0);
    expect_eq(s.removed, 0);
    expect_eq(s.unchanged, 3);

    let out: [256]u8 = [0; 256];
    const diff_str = formatUnified("a.txt", "b.txt", ops[0..n], 3, out[0..]).?;
    expect_eq(diff_str.len, 0);
}

test("diff_unified: pure insertion into empty text and pure deletion to empty text") {
    let ops: [32]DiffLine = [DiffLine{}; 32];
    let out: [256]u8 = [0; 256];

    // 纯新增
    const n_add = diffLines("", "line1\nline2\n", ops[0..]);
    expect_eq(n_add, 2);
    const s_add = summarize(ops[0..n_add]);
    expect_eq(s_add.added, 2);
    expect_eq(s_add.removed, 0);
    expect_eq(s_add.isIdentical(), false);

    const u_add = formatUnified("empty", "added", ops[0..n_add], 1, out[0..]).?;
    expect_eq(
        std.mem.eql(u8, u_add, "--- empty\n+++ added\n@@ -0,0 +1,2 @@\n+line1\n+line2\n"),
        true
    );

    // 纯删除
    const n_del = diffLines("old1\nold2\n", "", ops[0..]);
    expect_eq(n_del, 2);
    const s_del = summarize(ops[0..n_del]);
    expect_eq(s_del.added, 0);
    expect_eq(s_del.removed, 2);

    const u_del = formatUnified("removed", "empty", ops[0..n_del], 1, out[0..]).?;
    expect_eq(
        std.mem.eql(u8, u_del, "--- removed\n+++ empty\n@@ -1,2 +0,0 @@\n-old1\n-old2\n"),
        true
    );
}

test("diff_unified: middle edit with exact hunk header @@ -start,count +start,count @@") {
    const old_s = "1\n2\n3\n4\n5\n";
    const new_s = "1\n2\nTHREE\n4\n5\n";

    let ops: [32]DiffLine = [DiffLine{}; 32];
    const n = diffLines(old_s, new_s, ops[0..]);
    const st = summarize(ops[0..n]);
    expect_eq(st.added, 1);
    expect_eq(st.removed, 1);
    expect_eq(st.unchanged, 4);

    let out: [256]u8 = [0; 256];
    const u = formatUnified("old.cp", "new.cp", ops[0..n], 1, out[0..]).?;
    const expected =
        "--- old.cp\n" ++
        "+++ new.cp\n" ++
        "@@ -2,3 +2,3 @@\n" ++
        " 2\n" ++
        "-3\n" ++
        "+THREE\n" ++
        " 4\n";
    expect_eq(std.mem.eql(u8, u, expected), true);
}

test("diff_unified: nearby changes merge into single hunk while distant changes split into two hunks") {
    // 10 行文本：第 2 行与第 4 行修改（间隔 1 行，context=1 时应合并为单 Hunk），第 9 行修改（间隔 4 行，应拆为第 2 Hunk）
    const old_s = "L1\nL2\nL3\nL4\nL5\nL6\nL7\nL8\nL9\nL10\n";
    const new_s = "L1\nL2_mod\nL3\nL4_mod\nL5\nL6\nL7\nL8\nL9_mod\nL10\n";

    let ops: [64]DiffLine = [DiffLine{}; 64];
    const n = diffLines(old_s, new_s, ops[0..]);
    const st = summarize(ops[0..n]);
    expect_eq(st.added, 3);
    expect_eq(st.removed, 3);
    expect_eq(st.unchanged, 7);

    let out: [512]u8 = [0; 512];
    const u = formatUnified("a", "b", ops[0..n], 1, out[0..]).?;
    const expected =
        "--- a\n" ++
        "+++ b\n" ++
        "@@ -1,5 +1,5 @@\n" ++
        " L1\n" ++
        "-L2\n" ++
        "+L2_mod\n" ++
        " L3\n" ++
        "-L4\n" ++
        "+L4_mod\n" ++
        " L5\n" ++
        "@@ -8,3 +8,3 @@\n" ++
        " L8\n" ++
        "-L9\n" ++
        "+L9_mod\n" ++
        " L10\n";
    expect_eq(std.mem.eql(u8, u, expected), true);
}

test("diff_unified: CRLF normalization and small output buffer overflow guard") {
    let ops: [32]DiffLine = [DiffLine{}; 32];
    const n = diffLines("foo\r\nbar\r\n", "foo\nbaz\n", ops[0..]);
    const st = summarize(ops[0..n]);
    expect_eq(st.unchanged, 1);
    expect_eq(st.removed, 1);
    expect_eq(st.added, 1);

    // 过小输出缓冲区应安全返回 null 而非越界
    let tiny: [12]u8 = [0; 12];
    expect_eq(formatUnified("a", "b", ops[0..n], 1, tiny[0..]) == null, true);

    // ops 容量受限时同样安全截断不越界
    let small_ops: [2]DiffLine = [DiffLine{}; 2];
    const n2 = diffLines("a\nb\nc\n", "x\ny\nz\n", small_ops[0..]);
    expect_eq(n2, 2);
}
