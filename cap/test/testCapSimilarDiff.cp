// cap/test/testCapSimilarDiff.cp
// Unit tests for @cap/similar-diff (Zero-heap LCS diff, unified diff & similarity ratio)

import { test, expect_eq } from "cap:test";
import {
    DIFF_EQUAL,
    DIFF_DELETE,
    DIFF_INSERT,
    DiffOp,
    DiffStats,
    diffLines,
    similarityRatio,
    formatUnifiedDiff,
    hasChanges,
} from "../../packages/similar/src/index.cp";

fn bufEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/similar-diff — identical texts & empty-to-empty", () => {
        expect_eq(hasChanges("", ""), false);
        expect_eq(similarityRatio("", ""), 10000 as u32);

        const text = "line1\nline2\nline3\n";
        expect_eq(hasChanges(text, text), false);
        expect_eq(similarityRatio(text, text), 10000 as u32);

        let ops: [16]DiffOp = undefined;
        const st = diffLines(text, text, ops[0..16]);
        expect_eq(st.equal_lines, 3 as usize);
        expect_eq(st.deleted_lines, 0 as usize);
        expect_eq(st.inserted_lines, 0 as usize);
        expect_eq(st.total_ops, 3 as usize);

        let out: [128]u8 = undefined;
        expect_eq(formatUnifiedDiff("a", "b", text, text, out[0..128]), 0 as usize);
    });

    test("2. @cap/similar-diff — LCS minimal edit script (replace + insert)", () => {
        const old_s = "a\nb\nc\n";
        const new_s = "a\nB\nc\nd\n";

        let ops: [16]DiffOp = undefined;
        const st = diffLines(old_s, new_s, ops[0..16]);
        expect_eq(st.equal_lines, 2 as usize);
        expect_eq(st.deleted_lines, 1 as usize);
        expect_eq(st.inserted_lines, 2 as usize);
        expect_eq(st.total_ops, 5 as usize);

        expect_eq(ops[0].tag, DIFF_EQUAL);
        expect_eq(ops[1].tag, DIFF_DELETE);
        expect_eq(ops[1].old_idx, 1 as usize);
        expect_eq(ops[2].tag, DIFF_INSERT);
        expect_eq(ops[2].new_idx, 1 as usize);
        expect_eq(ops[3].tag, DIFF_EQUAL);
        expect_eq(ops[4].tag, DIFF_INSERT);
        expect_eq(ops[4].new_idx, 3 as usize);
    });

    test("3. @cap/similar-diff — pure insertions and pure deletions", () => {
        let ops: [16]DiffOp = undefined;

        const st_ins = diffLines("", "x\ny\n", ops[0..16]);
        expect_eq(st_ins.equal_lines, 0 as usize);
        expect_eq(st_ins.deleted_lines, 0 as usize);
        expect_eq(st_ins.inserted_lines, 2 as usize);
        expect_eq(similarityRatio("", "x\ny\n"), 0 as u32);

        const st_del = diffLines("x\ny\nz\n", "", ops[0..16]);
        expect_eq(st_del.equal_lines, 0 as usize);
        expect_eq(st_del.deleted_lines, 3 as usize);
        expect_eq(st_del.inserted_lines, 0 as usize);
        expect_eq(similarityRatio("x\ny\nz\n", ""), 0 as u32);
    });

    test("4. @cap/similar-diff — similarityRatio exact basis-point calculation", () => {
        // 2 equal out of (4 + 4) = 8 total => 2 * 2 * 10000 / 8 = 5000
        const a = "one\ntwo\nthree\nfour\n";
        const b = "one\nTWO\nthree\nFOUR\n";
        expect_eq(hasChanges(a, b), true);
        expect_eq(similarityRatio(a, b), 5000 as u32);

        // 3 equal out of (3 + 5) = 8 total => 2 * 3 * 10000 / 8 = 7500
        const c = "a\nb\nc\n";
        const d = "a\nb\nc\nd\ne\n";
        expect_eq(similarityRatio(c, d), 7500 as u32);
    });

    test("5. @cap/similar-diff — formatUnifiedDiff standard output", () => {
        const old_s = "hello\nworld\n";
        const new_s = "hello\ncap\n";
        let out: [256]u8 = undefined;
        const n = formatUnifiedDiff("old.txt", "new.txt", old_s, new_s, out[0..256]);
        const expected = "--- old.txt\n+++ new.txt\n@@ -1,2 +1,2 @@\n hello\n-world\n+cap\n";
        expect_eq(bufEq(out[0..n], expected), true);
    });

    test("6. @cap/similar-diff — CRLF normalization & small buffer safety", () => {
        expect_eq(hasChanges("a\r\nb\r\n", "a\nb\n"), false);
        expect_eq(similarityRatio("a\r\nb\r\n", "a\nb\n"), 10000 as u32);

        // Small output buffer truncates safely without overflow
        let tiny: [16]u8 = undefined;
        const n = formatUnifiedDiff("old.txt", "new.txt", "a\n", "b\n", tiny[0..16]);
        expect_eq(n, 16 as usize);
        expect_eq(bufEq(tiny[0..n], "--- old.txt\n+++ "), true);
    });
}
