// cap/test/testCapLexicalWriteFloat.cp
// Comprehensive unit tests for @cap/lexical-write-float

import { test, expect_eq } from "cap:test";
import {
    writeUintRadix,
    writeIntRadix,
    writeFloatFixed,
    writeFloatSci,
} from "../../packages/lexical-write-float/src/index.cp";

fn lexwTestEq(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/lexical-write-float — writeUintRadix across bases 2, 10, 16, 36", () => {
        let buf: [64]u8 = undefined;

        const n0 = writeUintRadix(0, 10, false, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n0], "0"), true);

        const n2 = writeUintRadix(45, 2, false, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n2], "101101"), true);

        const n16l = writeUintRadix(0xdeadbeef, 16, false, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n16l], "deadbeef"), true);

        const n16u = writeUintRadix(0xdeadbeef, 16, true, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n16u], "DEADBEEF"), true);

        const n36 = writeUintRadix(35, 36, true, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n36], "Z"), true);
    });

    test("2. @cap/lexical-write-float — writeIntRadix handles negative and i64::MIN safely", () => {
        let buf: [64]u8 = undefined;

        const n1 = writeIntRadix(-255, 16, true, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n1], "-FF"), true);

        const n2 = writeIntRadix(9223372036854775807, 10, false, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n2], "9223372036854775807"), true);

        const minI64: i64 = (-9223372036854775807 as i64) - (1 as i64);
        const n3 = writeIntRadix(minI64, 10, false, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n3], "-9223372036854775808"), true);
    });

    test("3. @cap/lexical-write-float — writeFloatFixed rounding, carry, and negative zero", () => {
        let buf: [64]u8 = undefined;

        const n1 = writeFloatFixed(3.14159, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n1], "3.14"), true);

        const n2 = writeFloatFixed(3.145, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n2], "3.15"), true);

        // Rounding carry into integer part
        const n3 = writeFloatFixed(9.999, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n3], "10.00"), true);

        // Zero decimals
        const n4 = writeFloatFixed(-42.7, 0, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n4], "-43"), true);

        // Negative zero preserves minus sign
        const negZero: f64 = -0.0;
        const n5 = writeFloatFixed(negZero, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n5], "-0.00"), true);
    });

    test("4. @cap/lexical-write-float — Special float values (NaN, inf, -inf)", () => {
        let buf: [64]u8 = undefined;

        const nNan = writeFloatFixed(std.math.nan(f64), 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..nNan], "NaN"), true);

        const infVal: f64 = std.math.inf(f64);
        const nInf = writeFloatFixed(infVal, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..nInf], "inf"), true);

        const nNegInf = writeFloatSci(-infVal, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..nNegInf], "-inf"), true);
    });

    test("5. @cap/lexical-write-float — writeFloatSci scientific notation formatting", () => {
        let buf: [64]u8 = undefined;

        const n1 = writeFloatSci(12345.0, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n1], "1.23e+4"), true);

        const n2 = writeFloatSci(-0.005, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n2], "-5.00e-3"), true);

        const n3 = writeFloatSci(0.0, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n3], "0.00e+0"), true);

        // Carry into exponent (9.999 -> 1.00e+1)
        const n4 = writeFloatSci(9.999, 2, buf[0..64]);
        expect_eq(lexwTestEq(buf[0..n4], "1.00e+1"), true);
    });

    test("6. @cap/lexical-write-float — Invalid radix and insufficient buffer boundaries", () => {
        let small: [2]u8 = undefined;
        let buf: [64]u8 = undefined;

        expect_eq(writeUintRadix(10, 1, false, buf[0..64]), 0 as usize);
        expect_eq(writeUintRadix(10, 37, false, buf[0..64]), 0 as usize);
        expect_eq(writeIntRadix(-100, 10, false, small[0..2]), 0 as usize);
        expect_eq(writeFloatFixed(123.45, 2, small[0..2]), 0 as usize);
        expect_eq(writeFloatSci(123.45, 2, small[0..2]), 0 as usize);
    });
}
