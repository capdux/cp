// cap/test/testCapLevenshtein.cp
// Comprehensive unit tests for @cap/levenshtein (Levenshtein & Damerau-Levenshtein)

import { test, expect_eq } from "cap:test";
import {
    distance,
    damerauDistance,
    similarity,
    withinDistance,
    closestIndex,
} from "../../packages/@cap/levenshtein/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff: f64 = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/levenshtein — classic 'kitten'/'sitting' = 3 & 'saturday'/'sunday' = 3", () => {
        expect_eq(distance("kitten", "sitting"), 3 as usize);
        expect_eq(distance("sitting", "kitten"), 3 as usize);
        expect_eq(distance("saturday", "sunday"), 3 as usize);
        expect_eq(distance("flaw", "lawn"), 2 as usize);
    });

    test("2. @cap/levenshtein — empty strings & identical strings", () => {
        expect_eq(distance("", ""), 0 as usize);
        expect_eq(distance("abc", ""), 3 as usize);
        expect_eq(distance("", "abcd"), 4 as usize);
        expect_eq(distance("same", "same"), 0 as usize);
        expect_eq(damerauDistance("", ""), 0 as usize);
        expect_eq(damerauDistance("same", "same"), 0 as usize);
    });

    test("3. @cap/levenshtein — damerauDistance adjacent transpositions", () => {
        expect_eq(distance("ab", "ba"), 2 as usize);
        expect_eq(damerauDistance("ab", "ba"), 1 as usize);
        expect_eq(damerauDistance("ca", "ac"), 1 as usize);
        expect_eq(damerauDistance("teh", "the"), 1 as usize);
        expect_eq(damerauDistance("recieve", "receive"), 1 as usize);
    });

    test("4. @cap/levenshtein — normalized similarity 0.0..1.0", () => {
        expect_eq(approxEq(similarity("", ""), 1.0, 0.000000001), true);
        expect_eq(approxEq(similarity("cap", "cap"), 1.0, 0.000000001), true);
        expect_eq(approxEq(similarity("abc", "xyz"), 0.0, 0.000000001), true);
        // distance("abcd", "abce") = 1, max_len = 4 => 0.75
        expect_eq(approxEq(similarity("abcd", "abce"), 0.75, 0.000000001), true);
    });

    test("5. @cap/levenshtein — withinDistance fast length-diff pruning", () => {
        expect_eq(withinDistance("kitten", "sitting", 3), true);
        expect_eq(withinDistance("kitten", "sitting", 2), false);
        expect_eq(withinDistance("a", "abcdef", 2), false);
    });

    test("6. @cap/levenshtein — closestIndex CLI typo suggestion", () => {
        const cmds: [4]string = ["build", "install", "publish", "test"];
        expect_eq(closestIndex("buld", cmds[0..4], 2), 0 as i64);
        expect_eq(closestIndex("instal", cmds[0..4], 2), 1 as i64);
        expect_eq(closestIndex("test", cmds[0..4], 2), 3 as i64);
        expect_eq(closestIndex("zzzzzz", cmds[0..4], 2), -1 as i64);
    });
}
