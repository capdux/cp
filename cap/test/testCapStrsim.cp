// cap/test/testCapStrsim.cp
// Comprehensive unit tests for @cap/strsim (Rust strsim parity)

import { test, expect_eq } from "cap:test";
import {
    hamming,
    levenshtein,
    normalizedLevenshtein,
    osaDistance,
    jaro,
    jaroWinkler,
    sorensenDice,
} from "../../packages/@cap/strsim/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    const diff = if (a > b) { a - b } else { b - a };
    return diff <= eps;
}

fn main() {
    test("1. @cap/strsim — hamming distance & unequal length rejection", () => {
        expect_eq(hamming("karolin", "kathrin").?, 3 as usize);
        expect_eq(hamming("1011101", "1001001").?, 2 as usize);
        expect_eq(hamming("same", "same").?, 0 as usize);
        expect_eq(hamming("", "").?, 0 as usize);
        expect_eq(hamming("short", "longer") == null, true);
    });

    test("2. @cap/strsim — levenshtein & normalizedLevenshtein", () => {
        expect_eq(levenshtein("kitten", "sitting"), 3 as usize);
        expect_eq(levenshtein("book", "back"), 2 as usize);
        expect_eq(levenshtein("", "abc"), 3 as usize);
        expect_eq(levenshtein("abc", ""), 3 as usize);
        expect_eq(levenshtein("", ""), 0 as usize);

        expect_eq(approxEq(normalizedLevenshtein("", ""), 1.0, 0.000001), true);
        expect_eq(approxEq(normalizedLevenshtein("kitten", "kitten"), 1.0, 0.000001), true);
        expect_eq(approxEq(normalizedLevenshtein("kitten", "sitting"), 4.0 / 7.0, 0.000001), true);
        expect_eq(approxEq(normalizedLevenshtein("abc", ""), 0.0, 0.000001), true);
    });

    test("3. @cap/strsim — osaDistance with adjacent transpositions", () => {
        expect_eq(osaDistance("ca", "ac"), 1 as usize);
        expect_eq(osaDistance("ca", "abc"), 3 as usize);
        expect_eq(osaDistance("abcd", "acbd"), 1 as usize);
        expect_eq(osaDistance("kitten", "sitting"), 3 as usize);
        expect_eq(osaDistance("", "xyz"), 3 as usize);
    });

    test("4. @cap/strsim — jaro similarity classic benchmarks", () => {
        expect_eq(approxEq(jaro("", ""), 1.0, 0.000001), true);
        expect_eq(approxEq(jaro("abc", ""), 0.0, 0.000001), true);
        expect_eq(approxEq(jaro("abc", "xyz"), 0.0, 0.000001), true);
        expect_eq(approxEq(jaro("MARTHA", "MARHTA"), 17.0 / 18.0, 0.000001), true);
        expect_eq(approxEq(jaro("DWAYNE", "DUANE"), 0.822222, 0.0001), true);
        expect_eq(approxEq(jaro("DIXON", "DICKSONX"), 0.766667, 0.0001), true);
    });

    test("5. @cap/strsim — jaroWinkler prefix boost & 0.7 threshold", () => {
        expect_eq(approxEq(jaroWinkler("MARTHA", "MARHTA"), 0.961111, 0.0001), true);
        expect_eq(approxEq(jaroWinkler("DWAYNE", "DUANE"), 0.84, 0.0001), true);
        expect_eq(approxEq(jaroWinkler("DIXON", "DICKSONX"), 0.813333, 0.0001), true);
        expect_eq(approxEq(jaroWinkler("identical", "identical"), 1.0, 0.000001), true);
        // Below 0.7 threshold: no prefix boost applied
        const j_low = jaro("ab", "axxxx");
        expect_eq(approxEq(jaroWinkler("ab", "axxxx"), j_low, 0.000001), true);
    });

    test("6. @cap/strsim — sorensenDice bigram similarity & edge cases", () => {
        expect_eq(approxEq(sorensenDice("night", "nacht"), 0.25, 0.000001), true);
        expect_eq(approxEq(sorensenDice("ferris", "ferris"), 1.0, 0.000001), true);
        expect_eq(approxEq(sorensenDice("", ""), 1.0, 0.000001), true);
        expect_eq(approxEq(sorensenDice("a", "a"), 1.0, 0.000001), true);
        expect_eq(approxEq(sorensenDice("a", "b"), 0.0, 0.000001), true);
        expect_eq(approxEq(sorensenDice("abc", "xyz"), 0.0, 0.000001), true);
    });
}
