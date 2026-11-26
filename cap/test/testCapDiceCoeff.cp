// cap/test/testCapDiceCoeff.cp
// Comprehensive unit tests for @cap/dice-coeff

import { test, expect_eq } from "cap:test";
import {
    diceCoefficient,
    diceCaseSensitive,
    bestMatchIndex,
} from "../../packages/@cap/dice-coeff/src/index.cp";

fn nearF64(a: f64, b: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d < (0.000001 as f64);
}

fn main() {
    test("1. @cap/dice-coeff — Classic paper vectors ('night'/'nacht' = 0.25, 'healed'/'sealed' = 0.8)", () => {
        expect_eq(nearF64(diceCoefficient("night", "nacht"), 0.25 as f64), true);
        expect_eq(nearF64(diceCoefficient("healed", "sealed"), 0.8 as f64), true);
    });

    test("2. @cap/dice-coeff — Identical (1.0), disjoint (0.0) & short/empty boundaries", () => {
        expect_eq(nearF64(diceCoefficient("Hello World", "helloworld"), 1.0 as f64), true);
        expect_eq(nearF64(diceCoefficient("", ""), 1.0 as f64), true);
        expect_eq(nearF64(diceCoefficient("a", "A"), 1.0 as f64), true);
        expect_eq(nearF64(diceCoefficient("a", "b"), 0.0 as f64), true);
        expect_eq(nearF64(diceCoefficient("abc", "xyz"), 0.0 as f64), true);
    });

    test("3. @cap/dice-coeff — Duplicate bigram multiset intersection ('aaaa' vs 'aa' = 0.5)", () => {
        // bigrams("aaaa") = {aa, aa, aa} (3), bigrams("aa") = {aa} (1), intersection = 1 => 2*1/(3+1) = 0.5
        expect_eq(nearF64(diceCoefficient("aaaa", "aa"), 0.5 as f64), true);
        // bigrams("abab") = {ab, ba, ab} (3), bigrams("ab") = {ab} (1), intersection = 1 => 0.5
        expect_eq(nearF64(diceCoefficient("abab", "ab"), 0.5 as f64), true);
    });

    test("4. @cap/dice-coeff — diceCaseSensitive distinguishes case & whitespace", () => {
        expect_eq(nearF64(diceCaseSensitive("Night", "night"), 0.75 as f64), true);
        expect_eq(nearF64(diceCaseSensitive("ABC", "abc"), 0.0 as f64), true);
        expect_eq(nearF64(diceCaseSensitive("a b", "ab"), 0.0 as f64), true);
        expect_eq(nearF64(diceCaseSensitive("healed", "sealed"), 0.8 as f64), true);
    });

    test("5. @cap/dice-coeff — bestMatchIndex fuzzy lookup & threshold filtering", () => {
        let cands: [4]string = undefined;
        cands[0] = "olive";
        cands[1] = "sealed";
        cands[2] = "healed";
        cands[3] = "banana";

        expect_eq(bestMatchIndex("healed", cands[0..4], 0.5 as f64), 2 as i64);
        expect_eq(bestMatchIndex("fealed", cands[0..4], 0.5 as f64), 1 as i64);
        expect_eq(bestMatchIndex("zzzzzz", cands[0..4], 0.2 as f64), -1 as i64);
    });

    test("6. @cap/dice-coeff — Empty candidates & high min_rating returns -1", () => {
        let cands: [2]string = undefined;
        cands[0] = "night";
        cands[1] = "light";
        expect_eq(bestMatchIndex("nacht", cands[0..0], 0.0 as f64), -1 as i64);
        expect_eq(bestMatchIndex("nacht", cands[0..2], 0.9 as f64), -1 as i64);
        expect_eq(bestMatchIndex("nacht", cands[0..2], 0.25 as f64), 0 as i64);
    });
}
