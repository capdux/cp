// cap/test/testCapKmpSearch.cp
// Comprehensive unit tests for @cap/kmp-search

import { test, expect_eq } from "cap:test";
import {
    computeLps,
    findFirst,
    findAll,
    countMatches,
    contains,
} from "../../packages/@cap/kmp-search/src/index.cp";

fn main() {
    test("1. @cap/kmp-search — computeLps classic vector 'ABABCABAB' -> [0,0,1,2,0,1,2,3,4]", () => {
        let lps: [16]usize = undefined;
        const n = computeLps("ABABCABAB", lps[0..16]);
        expect_eq(n, 9 as usize);
        expect_eq(lps[0], 0 as usize);
        expect_eq(lps[1], 0 as usize);
        expect_eq(lps[2], 1 as usize);
        expect_eq(lps[3], 2 as usize);
        expect_eq(lps[4], 0 as usize);
        expect_eq(lps[5], 1 as usize);
        expect_eq(lps[6], 2 as usize);
        expect_eq(lps[7], 3 as usize);
        expect_eq(lps[8], 4 as usize);
    });

    test("2. @cap/kmp-search — findFirst hit, miss & empty needle", () => {
        expect_eq(findFirst("ABABDABACDABABCABAB", "ABABCABAB"), 10 as i64);
        expect_eq(findFirst("hello world", "world"), 6 as i64);
        expect_eq(findFirst("hello world", "xyz"), -1 as i64);
        expect_eq(findFirst("hello", ""), 0 as i64);
        expect_eq(findFirst("", ""), 0 as i64);
    });

    test("3. @cap/kmp-search — findAll overlapping vs non-overlapping ('aaaaa' / 'aa')", () => {
        let offsOver: [8]usize = undefined;
        const nOver = findAll("aaaaa", "aa", true, offsOver[0..8]);
        expect_eq(nOver, 4 as usize);
        expect_eq(offsOver[0], 0 as usize);
        expect_eq(offsOver[1], 1 as usize);
        expect_eq(offsOver[2], 2 as usize);
        expect_eq(offsOver[3], 3 as usize);

        let offsNon: [8]usize = undefined;
        const nNon = findAll("aaaaa", "aa", false, offsNon[0..8]);
        expect_eq(nNon, 2 as usize);
        expect_eq(offsNon[0], 0 as usize);
        expect_eq(offsNon[1], 2 as usize);
    });

    test("4. @cap/kmp-search — countMatches overlapping & non-overlapping", () => {
        expect_eq(countMatches("aaaaa", "aa", true), 4 as usize);
        expect_eq(countMatches("aaaaa", "aa", false), 2 as usize);
        expect_eq(countMatches("abababa", "aba", true), 3 as usize);
        expect_eq(countMatches("abababa", "aba", false), 2 as usize);
        expect_eq(countMatches("abcdef", "xyz", true), 0 as usize);
    });

    test("5. @cap/kmp-search — Pathological repetitive haystack/needle O(N+M) test", () => {
        let hay: [512]u8 = undefined;
        let i: usize = 0;
        while (i < 511) {
            hay[i] = 'a' as u8;
            i += 1;
        }
        hay[511] = 'b' as u8;

        let pat: [64]u8 = undefined;
        let j: usize = 0;
        while (j < 63) {
            pat[j] = 'a' as u8;
            j += 1;
        }
        pat[63] = 'b' as u8;

        expect_eq(findFirst(hay[0..512], pat[0..64]), 448 as i64);
        pat[63] = 'c' as u8;
        expect_eq(findFirst(hay[0..512], pat[0..64]), -1 as i64);
    });

    test("6. @cap/kmp-search — contains, empty haystack & small output buffer boundaries", () => {
        expect_eq(contains("cap-lang/cp", "lang"), true);
        expect_eq(contains("cap-lang/cp", "rust"), false);
        expect_eq(contains("", "a"), false);
        expect_eq(contains("short", "muchlonger"), false);

        let tinyLps: [2]usize = undefined;
        expect_eq(computeLps("abcd", tinyLps[0..2]), 0 as usize);

        let tinyOff: [2]usize = undefined;
        expect_eq(findAll("aaaaa", "aa", true, tinyOff[0..2]), 2 as usize);
    });
}
