// cap/test/testCapDidyoumean.cp
// Comprehensive unit tests for @cap/didyoumean

import { test, expect_eq } from "cap:test";
import {
    levenshtein,
    damerauLevenshtein,
    similarity,
    didYouMean,
    didYouMeanWith,
} from "../../packages/@cap/didyoumean/src/index.cp";

fn main() {
    test("1. @cap/didyoumean — Levenshtein & Damerau-Levenshtein transposition", () => {
        expect_eq(levenshtein("kitten", "sitting"), 3 as usize);
        expect_eq(levenshtein("", "abc"), 3 as usize);
        expect_eq(levenshtein("same", "same"), 0 as usize);

        // Adjacent transposition: "depoly" -> "deploy" costs 1 in Damerau, 2 in plain Levenshtein
        expect_eq(levenshtein("depoly", "deploy"), 2 as usize);
        expect_eq(damerauLevenshtein("depoly", "deploy", false), 1 as usize);
        expect_eq(damerauLevenshtein("Teh", "the", true), 1 as usize);
    });

    test("2. @cap/didyoumean — CLI command typo suggestion", () => {
        const cmds: [5]string = ["deploy", "publish", "install", "remove", "status"];
        expect_eq(didYouMean("depoly", cmds[0..5]) ?? "", "deploy");
        expect_eq(didYouMean("pubish", cmds[0..5]) ?? "", "publish");
        expect_eq(didYouMean("instlal", cmds[0..5]) ?? "", "install");
        expect_eq(didYouMean("STATUS", cmds[0..5]) ?? "", "status");
        expect_eq(didYouMean("zzzzzzzzz", cmds[0..5]) == null, true);
    });

    test("3. @cap/didyoumean — Structured Suggestion & custom threshold", () => {
        const opts: [3]string = ["--sandbox", "--production", "--verbose"];
        const s = didYouMeanWith("--sandox", opts[0..3], 2 as usize);
        expect_eq(s.found, true);
        expect_eq(s.match_str, "--sandbox");
        expect_eq(s.distance, 1 as usize);
        expect_eq(s.index, 0 as usize);

        const miss = didYouMeanWith("--xyz", opts[0..3], 1 as usize);
        expect_eq(miss.found, false);
    });

    test("4. @cap/didyoumean — Normalized similarity score", () => {
        expect_eq(similarity("capdux", "capdux") > 0.99, true);
        expect_eq(similarity("capdux", "CAPDUX") > 0.99, true);
        expect_eq(similarity("abc", "xyz") < 0.01, true);
        expect_eq(similarity("deploy", "depoly") > 0.8, true);
    });
}
