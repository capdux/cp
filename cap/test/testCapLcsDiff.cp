// cap/test/testCapLcsDiff.cp
// Comprehensive unit tests for @cap/lcs-diff (LCS & Longest Common Substring)

import { test, expect_eq } from "cap:test";
import {
    SubstringMatch,
    DiffStats,
    lcsLength,
    diffStats,
    lcsSimilarity,
    longestCommonSubstring,
} from "../../packages/@cap/lcs-diff/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff: f64 = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/lcs-diff — classic 'ABCDGH'/'AEDFHR' = 3 & 'AGGTAB'/'GXTXAYB' = 4", () => {
        expect_eq(lcsLength("ABCDGH", "AEDFHR"), 3 as usize);
        expect_eq(lcsLength("AGGTAB", "GXTXAYB"), 4 as usize);
    });

    test("2. @cap/lcs-diff — diffStats exact common, insertions & deletions", () => {
        const st: DiffStats = diffStats("abcdef", "abXYefZ");
        // LCS is "abef" (4); deletions = 6-4 = 2 ("cd"), insertions = 7-4 = 3 ("XY","Z")
        expect_eq(st.common, 4 as usize);
        expect_eq(st.deletions, 2 as usize);
        expect_eq(st.insertions, 3 as usize);
    });

    test("3. @cap/lcs-diff — lcsSimilarity boundaries", () => {
        expect_eq(approxEq(lcsSimilarity("", ""), 1.0, 0.000000001), true);
        expect_eq(approxEq(lcsSimilarity("cap", "cap"), 1.0, 0.000000001), true);
        expect_eq(approxEq(lcsSimilarity("abc", "xyz"), 0.0, 0.000000001), true);
        // "ABCDGH" (6) vs "AEDFHR" (6) -> common=3 -> 2*3/12 = 0.5
        expect_eq(approxEq(lcsSimilarity("ABCDGH", "AEDFHR"), 0.5, 0.000000001), true);
    });

    test("4. @cap/lcs-diff — longestCommonSubstring locates 'BABC' (len 4)", () => {
        const a = "ABABC";
        const b = "BABCA";
        const m: SubstringMatch = longestCommonSubstring(a, b);
        expect_eq(m.len, 4 as usize);
        expect_eq(m.a_start, 1 as usize);
        expect_eq(m.b_start, 0 as usize);
        expect_eq(sliceEq(a[m.a_start..(m.a_start + m.len)], "BABC"), true);
        expect_eq(sliceEq(b[m.b_start..(m.b_start + m.len)], "BABC"), true);
    });

    test("5. @cap/lcs-diff — empty, identical & disjoint strings", () => {
        expect_eq(lcsLength("", "abc"), 0 as usize);
        expect_eq(lcsLength("abc", ""), 0 as usize);
        expect_eq(lcsLength("hello", "hello"), 5 as usize);
        expect_eq(lcsLength("abc", "xyz"), 0 as usize);

        const no_sub: SubstringMatch = longestCommonSubstring("abc", "xyz");
        expect_eq(no_sub.len, 0 as usize);
    });

    test("6. @cap/lcs-diff — 220-byte long string LCS & substring verification", () => {
        let s1: [220]u8 = undefined;
        let s2: [220]u8 = undefined;
        let i: usize = 0;
        while (i < 220) {
            s1[i] = ('a' as u8) + ((i % 26) as u8);
            s2[i] = s1[i];
            i += 1;
        }
        // Mutate 10 positions in s2
        let k: usize = 0;
        while (k < 10) {
            s2[k * 20] = 'Z' as u8;
            k += 1;
        }
        expect_eq(lcsLength(s1[0..220], s2[0..220]), 210 as usize);
        const sub: SubstringMatch = longestCommonSubstring(s1[0..220], s2[0..220]);
        // Tail s1[181..220] has 39 unchanged chars
        expect_eq(sub.len, 39 as usize);
    });
}
