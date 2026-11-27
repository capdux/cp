import { test, expect_eq } from "cap:test";
import {
    damerauLevenshteinSteps,
    damerauLevenshteinStepsBytes,
    osaDistance,
    trueDamerauLevenshteinSteps,
    trueDamerauLevenshteinBytes,
    trueDamerauLevenshtein,
    damerauLevenshteinBounded,
    damerauLevenshteinBoundedBytes,
    damerauLevenshtein,
    similarity,
    trueSimilarity,
} from "../../packages/damerau-levenshtein/src/index.cp";

fn approx(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return d >= -eps && d <= eps;
}

fn main() {
    test("1. damerau-levenshtein — counts adjacent transposition as 1 step", () => {
        expect_eq(damerauLevenshteinSteps("ab", "ba"), 1 as usize);
        expect_eq(damerauLevenshteinSteps("teh", "the"), 1 as usize);
        expect_eq(damerauLevenshteinSteps("recieve", "receive"), 1 as usize);
        expect_eq(osaDistance("ab", "ba"), 1 as usize);
    });

    test("2. damerau-levenshtein — identical strings have 0 steps and similarity 1.0", () => {
        const res = damerauLevenshtein("capdux", "capdux");
        expect_eq(res.steps, 0 as usize);
        expect_eq(approx(res.relative, 0.0, 1.0e-9), true);
        expect_eq(approx(res.similarity, 1.0, 1.0e-9), true);
        expect_eq(approx(similarity("capdux", "capdux"), 1.0, 1.0e-9), true);
    });

    test("3. damerau-levenshtein — computes classic kitten -> sitting = 3", () => {
        expect_eq(damerauLevenshteinSteps("kitten", "sitting"), 3 as usize);
        expect_eq(trueDamerauLevenshteinSteps("kitten", "sitting"), 3 as usize);
    });

    test("4. damerau-levenshtein — handles empty strings accurately", () => {
        expect_eq(damerauLevenshteinSteps("", ""), 0 as usize);
        expect_eq(damerauLevenshteinSteps("abc", ""), 3 as usize);
        expect_eq(damerauLevenshteinSteps("", "hello"), 5 as usize);
        expect_eq(approx(damerauLevenshtein("", "").similarity, 1.0, 1.0e-9), true);
        expect_eq(trueDamerauLevenshteinSteps("", "xyz"), 3 as usize);
        expect_eq(trueDamerauLevenshteinSteps("xyz", ""), 3 as usize);
    });

    test("5. damerau-levenshtein — computes relative distance and similarity accurately", () => {
        const res = damerauLevenshtein("abcd", "abdc"); // 1 transposition / 4 = 0.25
        expect_eq(res.steps, 1 as usize);
        expect_eq(approx(res.relative, 0.25, 1.0e-9), true);
        expect_eq(approx(res.similarity, 0.75, 1.0e-9), true);
    });

    test("6. damerau-levenshtein — completely disjoint strings have similarity 0.0", () => {
        const res = damerauLevenshtein("aaaa", "bbbb");
        expect_eq(res.steps, 4 as usize);
        expect_eq(approx(res.similarity, 0.0, 1.0e-9), true);
    });

    test("7. damerau-levenshtein — True Damerau-Levenshtein vs OSA on unrestricted transposition", () => {
        // Classic distinguishing example: "ca" -> "abc"
        // OSA (restricted) = 3; True Damerau-Levenshtein = 2 ("ca" -> "ac" -> "abc")
        expect_eq(osaDistance("ca", "abc"), 3 as usize);
        expect_eq(trueDamerauLevenshteinSteps("ca", "abc"), 2 as usize);
        expect_eq(trueDamerauLevenshteinSteps("abc", "ca"), 2 as usize);
        const res = trueDamerauLevenshtein("ca", "abc");
        expect_eq(res.steps, 2 as usize);
        expect_eq(approx(trueSimilarity("ca", "abc"), 1.0 / 3.0, 1.0e-9), true);
    });

    test("8. damerau-levenshtein — byte slice API supports arbitrary binary bytes", () => {
        const a: [4]u8 = [0, 1, 2, 3];
        const b: [4]u8 = [0, 2, 1, 3];
        expect_eq(damerauLevenshteinStepsBytes(a[0..4], b[0..4]), 1 as usize);
        expect_eq(trueDamerauLevenshteinBytes(a[0..4], b[0..4]), 1 as usize);
    });

    test("9. damerau-levenshtein — maxDistance early pruning returns distance or null", () => {
        expect_eq(damerauLevenshteinBounded("kitten", "sitting", 3) ?? 99, 3 as usize);
        expect_eq(damerauLevenshteinBounded("kitten", "sitting", 2) == null, true);
        expect_eq(damerauLevenshteinBounded("abc", "abcdefghi", 2) == null, true);
        expect_eq(damerauLevenshteinBounded("ab", "ba", 1) ?? 99, 1 as usize);
        expect_eq(damerauLevenshteinBounded("ab", "ba", 0) == null, true);

        const x: [3]u8 = [10, 20, 30];
        const y: [3]u8 = [10, 30, 20];
        expect_eq(damerauLevenshteinBoundedBytes(x[0..3], y[0..3], 1) ?? 99, 1 as usize);
    });
}
