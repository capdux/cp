import { test, expect_eq } from "cap:test";
import {
    StrNumOptions,
    StrNumResult,
    parseStrNum,
} from "../../packages/strnum/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

fn main() {
    test("1. @cap/strnum — Basic integers & floats with default options", () => {
        const opts = StrNumOptions{ hex: true, leading_zeros: true, e_notation: true };

        const r1 = parseStrNum("42", opts);
        expect_eq(r1.is_number, true);
        expect_eq(r1.is_int, true);
        expect_eq(r1.int_value, 42);
        expect_eq(approxEq(r1.value, 42.0, 1.0e-12), true);

        const r2 = parseStrNum("-123456", opts);
        expect_eq(r2.is_number, true);
        expect_eq(r2.is_int, true);
        expect_eq(r2.int_value, -123456);

        const r3 = parseStrNum("3.14159", opts);
        expect_eq(r3.is_number, true);
        expect_eq(r3.is_int, false);
        expect_eq(approxEq(r3.value, 3.14159, 1.0e-9), true);

        const r4 = parseStrNum("-.5", opts);
        expect_eq(r4.is_number, true);
        expect_eq(approxEq(r4.value, -0.5, 1.0e-12), true);
    });

    test("2. @cap/strnum — Leading zeros policy (007 vs 0.5 vs -0.5)", () => {
        const strict = StrNumOptions{ hex: true, leading_zeros: false, e_notation: true };
        const loose = StrNumOptions{ hex: true, leading_zeros: true, e_notation: true };

        // Disallowed when leading_zeros == false
        expect_eq(parseStrNum("007", strict).is_number, false);
        expect_eq(parseStrNum("00", strict).is_number, false);
        expect_eq(parseStrNum("-01", strict).is_number, false);
        expect_eq(parseStrNum("00.5", strict).is_number, false);

        // Single 0 and 0.x are still valid under strict leading_zeros == false
        const z0 = parseStrNum("0", strict);
        expect_eq(z0.is_number, true);
        expect_eq(z0.int_value, 0);

        const z1 = parseStrNum("0.5", strict);
        expect_eq(z1.is_number, true);
        expect_eq(approxEq(z1.value, 0.5, 1.0e-12), true);

        const z2 = parseStrNum("-0.5", strict);
        expect_eq(z2.is_number, true);
        expect_eq(approxEq(z2.value, -0.5, 1.0e-12), true);

        // Allowed when leading_zeros == true
        const l1 = parseStrNum("007", loose);
        expect_eq(l1.is_number, true);
        expect_eq(l1.is_int, true);
        expect_eq(l1.int_value, 7);
    });

    test("3. @cap/strnum — Hexadecimal parsing & toggle", () => {
        const hex_on = StrNumOptions{ hex: true, leading_zeros: false, e_notation: true };
        const hex_off = StrNumOptions{ hex: false, leading_zeros: false, e_notation: true };

        const h1 = parseStrNum("0xFF", hex_on);
        expect_eq(h1.is_number, true);
        expect_eq(h1.is_int, true);
        expect_eq(h1.int_value, 255);

        const h2 = parseStrNum("-0X10a", hex_on);
        expect_eq(h2.is_number, true);
        expect_eq(h2.int_value, -266);

        expect_eq(parseStrNum("0xFF", hex_off).is_number, false);
        expect_eq(parseStrNum("0x", hex_on).is_number, false);
        expect_eq(parseStrNum("0xGG", hex_on).is_number, false);
    });

    test("4. @cap/strnum — Scientific notation (e_notation) & toggle", () => {
        const exp_on = StrNumOptions{ hex: true, leading_zeros: false, e_notation: true };
        const exp_off = StrNumOptions{ hex: true, leading_zeros: false, e_notation: false };

        const e1 = parseStrNum("1.5e-3", exp_on);
        expect_eq(e1.is_number, true);
        expect_eq(e1.is_int, false);
        expect_eq(approxEq(e1.value, 0.0015, 1.0e-12), true);

        const e2 = parseStrNum("2.5E+4", exp_on);
        expect_eq(e2.is_number, true);
        expect_eq(approxEq(e2.value, 25000.0, 1.0e-9), true);

        expect_eq(parseStrNum("1.5e-3", exp_off).is_number, false);
        expect_eq(parseStrNum("1e", exp_on).is_number, false);
        expect_eq(parseStrNum("1e+", exp_on).is_number, false);
    });

    test("5. @cap/strnum — Whitespace trimming & signed zero", () => {
        const opts = StrNumOptions{ hex: true, leading_zeros: false, e_notation: true };

        const w1 = parseStrNum("  \t 1024 \n ", opts);
        expect_eq(w1.is_number, true);
        expect_eq(w1.is_int, true);
        expect_eq(w1.int_value, 1024);

        const w2 = parseStrNum("+99.25", opts);
        expect_eq(w2.is_number, true);
        expect_eq(approxEq(w2.value, 99.25, 1.0e-12), true);

        expect_eq(parseStrNum("", opts).is_number, false);
        expect_eq(parseStrNum("   \t\r\n  ", opts).is_number, false);
    });

    test("6. @cap/strnum — Malformed & non-numeric strings safely rejected", () => {
        const opts = StrNumOptions{ hex: true, leading_zeros: true, e_notation: true };

        expect_eq(parseStrNum("abc", opts).is_number, false);
        expect_eq(parseStrNum("123abc", opts).is_number, false);
        expect_eq(parseStrNum("12.34.56", opts).is_number, false);
        expect_eq(parseStrNum("1.", opts).is_number, false);
        expect_eq(parseStrNum(".", opts).is_number, false);
        expect_eq(parseStrNum("+", opts).is_number, false);
        expect_eq(parseStrNum("-", opts).is_number, false);
        expect_eq(parseStrNum("1 2", opts).is_number, false);
    });
}
