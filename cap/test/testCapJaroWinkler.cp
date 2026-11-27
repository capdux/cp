import { test, expect_eq } from "cap:test";
import {
    jaro,
    jaroWinkler,
    jaroWinklerWithScale,
    bestMatchIndex,
} from "../../packages/jaro-winkler/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/jaro-winkler — Classic paper vector MARTHA vs MARHTA", () => {
        expect_eq(approxEq(jaro("MARTHA", "MARHTA"), 0.944444, 0.0005), true);
        expect_eq(approxEq(jaroWinkler("MARTHA", "MARHTA"), 0.961111, 0.0005), true);
    });

    test("2. @cap/jaro-winkler — Classic paper vector DWAYNE vs DUANE", () => {
        expect_eq(approxEq(jaro("DWAYNE", "DUANE"), 0.822222, 0.0005), true);
        expect_eq(approxEq(jaroWinkler("DWAYNE", "DUANE"), 0.840000, 0.0005), true);
    });

    test("3. @cap/jaro-winkler — Classic paper vector DIXON vs DICKSONX", () => {
        expect_eq(approxEq(jaro("DIXON", "DICKSONX"), 0.766667, 0.0005), true);
        expect_eq(approxEq(jaroWinkler("DIXON", "DICKSONX"), 0.813333, 0.0005), true);
    });

    test("4. @cap/jaro-winkler — Identical 1.0 & completely disjoint 0.0", () => {
        expect_eq(approxEq(jaro("cp-lang", "cp-lang"), 1.0, 0.0001), true);
        expect_eq(approxEq(jaroWinkler("cp-lang", "cp-lang"), 1.0, 0.0001), true);
        expect_eq(approxEq(jaro("abcdef", "xyz123"), 0.0, 0.0001), true);
        expect_eq(approxEq(jaroWinkler("abcdef", "xyz123"), 0.0, 0.0001), true);
    });

    test("5. @cap/jaro-winkler — bestMatchIndex entity matching & threshold filtering", () => {
        const candidates: [4]string = ["kubernetes", "postgres", "redis", "rabbitmq"];
        expect_eq(bestMatchIndex("postgre", candidates[0..4], 0.8), 1 as i64);
        expect_eq(bestMatchIndex("kubernetis", candidates[0..4], 0.8), 0 as i64);
        expect_eq(bestMatchIndex("zzzzzz", candidates[0..4], 0.7), -1 as i64);
    });

    test("6. @cap/jaro-winkler — Empty strings & custom prefix_scale clamping", () => {
        expect_eq(approxEq(jaro("", ""), 1.0, 0.0001), true);
        expect_eq(approxEq(jaro("", "abc"), 0.0, 0.0001), true);
        expect_eq(approxEq(jaro("abc", ""), 0.0, 0.0001), true);

        // Scale clamped at 0.25 max and 0.0 min
        const s_zero = jaroWinklerWithScale("MARTHA", "MARHTA", -1.0);
        expect_eq(approxEq(s_zero, jaro("MARTHA", "MARHTA"), 0.0001), true);

        const s_max = jaroWinklerWithScale("MARTHA", "MARHTA", 0.99);
        const s_cap = jaroWinklerWithScale("MARTHA", "MARHTA", 0.25);
        expect_eq(approxEq(s_max, s_cap, 0.0001), true);
        expect_eq(s_cap <= 1.0, true);
    });
}
