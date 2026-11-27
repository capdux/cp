import { test, expect_eq } from "cap:test";
import { EvalResult, evalCExpr } from "../../packages/cexpr/src/index.cp";

fn main() {
    test("1. @cap/cexpr — arithmetic & parenthesis precedence", () => {
        let r1: EvalResult = evalCExpr("2 + 3 * (4 - 1)");
        expect_eq(r1.ok, true);
        expect_eq(r1.value, 11 as i64);

        let r2: EvalResult = evalCExpr("(20 - 5) / 3 + 17 % 5");
        expect_eq(r2.ok, true);
        expect_eq(r2.value, 7 as i64);

        let r3: EvalResult = evalCExpr("-17 / 5");
        expect_eq(r3.ok, true);
        expect_eq(r3.value, -3 as i64);
    });

    test("2. @cap/cexpr — hex, binary, octal & char literals", () => {
        // 0x10 (16) + 0b101 (5) + 010 (8) + 'A' (65) = 94
        let r1: EvalResult = evalCExpr("0x10 + 0b101 + 010 + 'A'");
        expect_eq(r1.ok, true);
        expect_eq(r1.value, 94 as i64);

        let r2: EvalResult = evalCExpr("0755");
        expect_eq(r2.ok, true);
        expect_eq(r2.value, 493 as i64);

        let r3: EvalResult = evalCExpr("'\\n' + '\\t' + '\\0'");
        expect_eq(r3.ok, true);
        expect_eq(r3.value, 19 as i64);
    });

    test("3. @cap/cexpr — bitwise ops, shifts & ~0 == -1", () => {
        let r1: EvalResult = evalCExpr("(1 << 8) | 0x0f");
        expect_eq(r1.ok, true);
        expect_eq(r1.value, 271 as i64);

        let r2: EvalResult = evalCExpr("~0");
        expect_eq(r2.ok, true);
        expect_eq(r2.value, -1 as i64);

        let r3: EvalResult = evalCExpr("(0xFF & 0x0F) ^ 0x05");
        expect_eq(r3.ok, true);
        expect_eq(r3.value, 10 as i64);

        let r4: EvalResult = evalCExpr("64 >> 2");
        expect_eq(r4.ok, true);
        expect_eq(r4.value, 16 as i64);
    });

    test("4. @cap/cexpr — comparisons, logical ops & ternary expression", () => {
        let r1: EvalResult = evalCExpr("(3 < 5 && 2 != 0) ? 100 : 200");
        expect_eq(r1.ok, true);
        expect_eq(r1.value, 100 as i64);

        let r2: EvalResult = evalCExpr("(3 > 5 || 0) ? 100 : 200");
        expect_eq(r2.ok, true);
        expect_eq(r2.value, 200 as i64);

        let r3: EvalResult = evalCExpr("!0 == 1 && !5 == 0");
        expect_eq(r3.ok, true);
        expect_eq(r3.value, 1 as i64);
    });

    test("5. @cap/cexpr — unary operators & integer suffixes (UL, u, ull)", () => {
        let r1: EvalResult = evalCExpr("100UL + 23u");
        expect_eq(r1.ok, true);
        expect_eq(r1.value, 123 as i64);

        let r2: EvalResult = evalCExpr("-(-42L) + +8ULL");
        expect_eq(r2.ok, true);
        expect_eq(r2.value, 50 as i64);
    });

    test("6. @cap/cexpr — div-by-zero, unclosed paren & trailing garbage rejected", () => {
        expect_eq(evalCExpr("10 / 0").ok, false);
        expect_eq(evalCExpr("10 % 0").ok, false);
        expect_eq(evalCExpr("(1 + 2").ok, false);
        expect_eq(evalCExpr("1 + 2 foo").ok, false);
        expect_eq(evalCExpr("089").ok, false);
        expect_eq(evalCExpr("").ok, false);
    });
}
