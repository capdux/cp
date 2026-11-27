// cap/test/testCapLexicalParseFloat.cp
// Comprehensive unit tests for @cap/lexical-parse-float

import { test, expect_eq } from "cap:test";
import {
    ParseFloatResult,
    ParseIntResult,
    parseFloat,
    parseIntRadix,
} from "../../packages/lexical-parse-float/src/index.cp";

fn lexpApproxEq(a: f64, b: f64, eps: f64): bool {
    const diff: f64 = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

fn main() {
    test("1. @cap/lexical-parse-float — Basic signed integers and decimal floats", () => {
        const r1 = parseFloat("0");
        expect_eq(r1.ok, true);
        expect_eq(r1.value == 0.0, true);
        expect_eq(r1.consumed, 1 as usize);

        const r2 = parseFloat("3.141592653589793");
        expect_eq(r2.ok, true);
        expect_eq(lexpApproxEq(r2.value, 3.141592653589793, 1.0e-15), true);
        expect_eq(r2.consumed, 17 as usize);

        const r3 = parseFloat("-42.5");
        expect_eq(r3.ok, true);
        expect_eq(r3.value == -42.5, true);

        const r4 = parseFloat("+0.25");
        expect_eq(r4.ok, true);
        expect_eq(r4.value == 0.25, true);

        const r5 = parseFloat(".5");
        expect_eq(r5.ok, true);
        expect_eq(r5.value == 0.5, true);
    });

    test("2. @cap/lexical-parse-float — Scientific notation (e/E, positive & negative exponents)", () => {
        const r1 = parseFloat("1.5e3");
        expect_eq(r1.ok, true);
        expect_eq(r1.value == 1500.0, true);

        const r2 = parseFloat("-2.5E-4");
        expect_eq(r2.ok, true);
        expect_eq(lexpApproxEq(r2.value, -0.00025, 1.0e-15), true);

        const r3 = parseFloat("6.02214076e+23");
        expect_eq(r3.ok, true);
        expect_eq(lexpApproxEq(r3.value, 6.02214076e23, 1.0e10), true);

        const r4 = parseFloat("1e-30");
        expect_eq(r4.ok, true);
        expect_eq(lexpApproxEq(r4.value, 1.0e-30, 1.0e-44), true);
    });

    test("3. @cap/lexical-parse-float — Special IEEE-754 values (inf, -inf, infinity, nan)", () => {
        const rInf = parseFloat("inf");
        expect_eq(rInf.ok, true);
        expect_eq(std.math.isPositiveInf(rInf.value), true);

        const rNegInf = parseFloat("-Infinity");
        expect_eq(rNegInf.ok, true);
        expect_eq(std.math.isNegativeInf(rNegInf.value), true);

        const rPosInf = parseFloat("+INF");
        expect_eq(rPosInf.ok, true);
        expect_eq(std.math.isPositiveInf(rPosInf.value), true);

        const rNan = parseFloat("NaN");
        expect_eq(rNan.ok, true);
        expect_eq(std.math.isNan(rNan.value), true);
    });

    test("4. @cap/lexical-parse-float — Reject empty, malformed, and trailing-garbage floats", () => {
        expect_eq(parseFloat("").ok, false);
        expect_eq(parseFloat("+").ok, false);
        expect_eq(parseFloat("-").ok, false);
        expect_eq(parseFloat(".").ok, false);
        expect_eq(parseFloat("1.2.3").ok, false);
        expect_eq(parseFloat("1e").ok, false);
        expect_eq(parseFloat("1e+").ok, false);
        expect_eq(parseFloat("1.5abc").ok, false);
        expect_eq(parseFloat(" 1.5").ok, false);
        expect_eq(parseFloat("1.5 ").ok, false);
        expect_eq(parseFloat("infx").ok, false);
    });

    test("5. @cap/lexical-parse-float — parseIntRadix across bases 2, 8, 10, 16, 36", () => {
        const b2 = parseIntRadix("101101", 2);
        expect_eq(b2.ok, true);
        expect_eq(b2.value, 45 as i64);
        expect_eq(b2.consumed, 6 as usize);

        const b8 = parseIntRadix("-755", 8);
        expect_eq(b8.ok, true);
        expect_eq(b8.value, -493 as i64);

        const b10 = parseIntRadix("+123456789", 10);
        expect_eq(b10.ok, true);
        expect_eq(b10.value, 123456789 as i64);

        const b16 = parseIntRadix("deadBEEF", 16);
        expect_eq(b16.ok, true);
        expect_eq(b16.value, 3735928559 as i64);

        const b36 = parseIntRadix("z", 36);
        expect_eq(b36.ok, true);
        expect_eq(b36.value, 35 as i64);
    });

    test("6. @cap/lexical-parse-float — parseIntRadix i64 min/max boundaries, overflow & invalid radix", () => {
        const maxI64 = parseIntRadix("9223372036854775807", 10);
        expect_eq(maxI64.ok, true);
        expect_eq(maxI64.value, 9223372036854775807 as i64);

        const minI64 = parseIntRadix("-9223372036854775808", 10);
        expect_eq(minI64.ok, true);
        expect_eq(minI64.value, (-9223372036854775807 as i64) - (1 as i64));

        // Overflow by 1 rejected
        expect_eq(parseIntRadix("9223372036854775808", 10).ok, false);
        expect_eq(parseIntRadix("-9223372036854775809", 10).ok, false);

        // Invalid digits / radix / empty
        expect_eq(parseIntRadix("", 10).ok, false);
        expect_eq(parseIntRadix("10", 1).ok, false);
        expect_eq(parseIntRadix("10", 37).ok, false);
        expect_eq(parseIntRadix("2", 2).ok, false);
        expect_eq(parseIntRadix("12g", 16).ok, false);
    });
}
