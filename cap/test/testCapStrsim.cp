import { test, expect_eq } from "cap:test";
import {
    hamming, levenshtein, osaDistance, normalizedLevenshteinBps,
    jaroBps, jaroWinklerBps, sorensenDiceBps
} from "../../packages/@cap/strsim/src/index.cp";

fn main() {
    test("1. @cap/strsim — hamming equal & unequal length handling", () => {
        expect_eq(hamming("karolin", "kathrin") ?? 99, 3);
        expect_eq(hamming("abc", "abc") ?? 99, 0);
        expect_eq(hamming("ab", "abc") == null, true);
    });

    test("2. @cap/strsim — levenshtein classic edit distances", () => {
        expect_eq(levenshtein("kitten", "sitting"), 3);
        expect_eq(levenshtein("saturday", "sunday"), 3);
        expect_eq(levenshtein("", "hello"), 5);
        expect_eq(levenshtein("same", "same"), 0);
    });

    test("3. @cap/strsim — osaDistance counts adjacent transposition as 1", () => {
        expect_eq(levenshtein("ca", "ac"), 2);
        expect_eq(osaDistance("ca", "ac"), 1);
        expect_eq(osaDistance("abcdef", "abcfed"), 2);
        expect_eq(osaDistance("hello", "ehllo"), 1);
    });

    test("4. @cap/strsim — normalizedLevenshteinBps basis points (0..10000)", () => {
        expect_eq(normalizedLevenshteinBps("", ""), 10000);
        expect_eq(normalizedLevenshteinBps("kitten", "kitten"), 10000);
        expect_eq(normalizedLevenshteinBps("abcd", "abef"), 5000);
        expect_eq(normalizedLevenshteinBps("abc", "xyz"), 0);
    });

    test("5. @cap/strsim — jaroBps & jaroWinklerBps prefix boost", () => {
        expect_eq(jaroBps("martha", "marhta") > 9400, true);
        expect_eq(jaroWinklerBps("martha", "marhta") > jaroBps("martha", "marhta"), true);
        expect_eq(jaroWinklerBps("identical", "identical"), 10000);
        expect_eq(jaroBps("abc", "xyz"), 0);
    });

    test("6. @cap/strsim — sorensenDiceBps bigram similarity", () => {
        expect_eq(sorensenDiceBps("night", "nacht"), 2500);
        expect_eq(sorensenDiceBps("same", "same"), 10000);
        expect_eq(sorensenDiceBps("", ""), 10000);
        expect_eq(sorensenDiceBps("abc", "xyz"), 0);
    });
}
