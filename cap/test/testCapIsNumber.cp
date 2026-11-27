import { test, expect_eq } from "cap:test";
import { isNumber, isIntegerStr, isFiniteF64 } from "../../packages/is-number/src/index.cp";

fn main() {
    test("integers, signs & whitespace trimming", () => {
        expect_eq(isNumber("0"), true);
        expect_eq(isNumber("42"), true);
        expect_eq(isNumber("-123"), true);
        expect_eq(isNumber("+99"), true);
        expect_eq(isNumber("  3.14  "), true);
    });

    test("decimals & scientific notation", () => {
        expect_eq(isNumber(".5"), true);
        expect_eq(isNumber("5."), true);
        expect_eq(isNumber("1e10"), true);
        expect_eq(isNumber("-2.5E-3"), true);
        expect_eq(isNumber("1e"), false);
        expect_eq(isNumber("."), false);
    });

    test("hex, binary & octal literals", () => {
        expect_eq(isNumber("0xff"), true);
        expect_eq(isNumber("0X1A2b"), true);
        expect_eq(isNumber("0b1010"), true);
        expect_eq(isNumber("0o755"), true);
        expect_eq(isNumber("0x"), false);
        expect_eq(isNumber("0b102"), false);
        expect_eq(isNumber("-0xff"), false);
    });

    test("rejects empty, whitespace, NaN, Infinity & alpha", () => {
        expect_eq(isNumber(""), false);
        expect_eq(isNumber("   \t\n "), false);
        expect_eq(isNumber("NaN"), false);
        expect_eq(isNumber("Infinity"), false);
        expect_eq(isNumber("12abc"), false);
        expect_eq(isNumber("--5"), false);
    });

    test("isIntegerStr strict integer check", () => {
        expect_eq(isIntegerStr("0"), true);
        expect_eq(isIntegerStr("-4096"), true);
        expect_eq(isIntegerStr(" +7 "), true);
        expect_eq(isIntegerStr("3.14"), false);
        expect_eq(isIntegerStr("1e5"), false);
        expect_eq(isIntegerStr(""), false);
    });

    test("isFiniteF64 checks", () => {
        expect_eq(isFiniteF64(0.0), true);
        expect_eq(isFiniteF64(-12345.678), true);
        let z: f64 = 0.0;
        let inf: f64 = 1.0 / z;
        let nan_v: f64 = z / z;
        expect_eq(isFiniteF64(inf), false);
        expect_eq(isFiniteF64(-inf), false);
        expect_eq(isFiniteF64(nan_v), false);
    });
}
