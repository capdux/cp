import { test, expect_eq } from "cap:test";
import {
    damerauLevenshteinSteps,
    damerauLevenshtein,
} from "../../packages/@cap/damerau-levenshtein/src/index.cp";

fn approx(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return d >= -eps && d <= eps;
}

fn main() {
    test("1. @cap/damerau-levenshtein — counts adjacent transposition as 1 step", () => {
        expect_eq(damerauLevenshteinSteps("ab", "ba"), 1 as usize);
        expect_eq(damerauLevenshteinSteps("teh", "the"), 1 as usize);
        expect_eq(damerauLevenshteinSteps("recieve", "receive"), 1 as usize);
    });

    test("2. @cap/damerau-levenshtein — identical strings have 0 steps and similarity 1.0", () => {
        const res = damerauLevenshtein("capdux", "capdux");
        expect_eq(res.steps, 0 as usize);
        expect_eq(approx(res.relative, 0.0, 1.0e-9), true);
        expect_eq(approx(res.similarity, 1.0, 1.0e-9), true);
    });

    test("3. @cap/damerau-levenshtein — computes classic kitten -> sitting = 3", () => {
        expect_eq(damerauLevenshteinSteps("kitten", "sitting"), 3 as usize);
    });

    test("4. @cap/damerau-levenshtein — handles empty strings accurately", () => {
        expect_eq(damerauLevenshteinSteps("", ""), 0 as usize);
        expect_eq(damerauLevenshteinSteps("abc", ""), 3 as usize);
        expect_eq(damerauLevenshteinSteps("", "hello"), 5 as usize);
        expect_eq(approx(damerauLevenshtein("", "").similarity, 1.0, 1.0e-9), true);
    });

    test("5. @cap/damerau-levenshtein — computes relative distance and similarity accurately", () => {
        const res = damerauLevenshtein("abcd", "abdc"); // 1 transposition / 4 = 0.25
        expect_eq(res.steps, 1 as usize);
        expect_eq(approx(res.relative, 0.25, 1.0e-9), true);
        expect_eq(approx(res.similarity, 0.75, 1.0e-9), true);
    });

    test("6. @cap/damerau-levenshtein — completely disjoint strings have similarity 0.0", () => {
        const res = damerauLevenshtein("aaaa", "bbbb");
        expect_eq(res.steps, 4 as usize);
        expect_eq(approx(res.similarity, 0.0, 1.0e-9), true);
    });
}
