// cap/test/testCapMathjsExpr.cp
// Comprehensive unit tests for @cap/mathjs-expr (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    MJ_OK,
    MJ_ERR_SYNTAX,
    MJ_ERR_UNCLOSED_PAREN,
    MJ_ERR_DIV_ZERO,
    MJ_ERR_UNKNOWN_IDENT,
    MJ_ERR_ARITY,
    MathScope,
    EvalResult,
    evalExpr,
    evalExprWithScope,
} from "../../packages/mathjs/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = a - b;
    const ad = if (diff < 0.0) -diff else diff;
    return ad <= eps;
}

fn main() {
    test("1. @cap/mathjs-expr — operator precedence, parentheses, unary minus & right-associative power", () => {
        const r1: EvalResult = evalExpr("1 + 2 * 3");
        expect_eq(r1.ok, true);
        expect_eq(r1.err_code, MJ_OK);
        expect_eq(approxEq(r1.value, 7.0, 0.000000001), true);

        const r2 = evalExpr("(1 + 2) * (10 - 4) / 3");
        expect_eq(r2.ok, true);
        expect_eq(approxEq(r2.value, 6.0, 0.000000001), true);

        // Right-associative power: 2^3^2 == 2^(3^2) == 512
        const r3 = evalExpr("2^3^2");
        expect_eq(r3.ok, true);
        expect_eq(approxEq(r3.value, 512.0, 0.000000001), true);

        // Unary minus & modulo
        const r4 = evalExpr("-2^2 + (-3)^2 + 17 % 5");
        expect_eq(r4.ok, true);
        expect_eq(approxEq(r4.value, 7.0, 0.000000001), true); // -4 + 9 + 2
    });

    test("2. @cap/mathjs-expr — built-in constants (pi, e, tau, phi) & @cap/libm unary functions", () => {
        const rSin = evalExpr("sin(pi / 2)");
        expect_eq(rSin.ok, true);
        expect_eq(approxEq(rSin.value, 1.0, 0.000000001), true);

        const rCos = evalExpr("cos(0)");
        expect_eq(rCos.ok, true);
        expect_eq(approxEq(rCos.value, 1.0, 0.000000001), true);

        const rSqrt = evalExpr("sqrt(16)");
        expect_eq(rSqrt.ok, true);
        expect_eq(approxEq(rSqrt.value, 4.0, 0.000000001), true);

        const rLog = evalExpr("log(e)");
        expect_eq(rLog.ok, true);
        expect_eq(approxEq(rLog.value, 1.0, 0.000000001), true);

        const rExp = evalExpr("exp(0) + floor(3.9) + ceil(2.1) + round(2.6)");
        expect_eq(rExp.ok, true);
        expect_eq(approxEq(rExp.value, 10.0, 0.000000001), true); // 1 + 3 + 3 + 3

        const rTau = evalExpr("tau / pi");
        expect_eq(rTau.ok, true);
        expect_eq(approxEq(rTau.value, 2.0, 0.000000001), true);
    });

    test("3. @cap/mathjs-expr — 2-arg and 3-arg functions (pow, min, max, clamp)", () => {
        const rPow = evalExpr("pow(3, 4)");
        expect_eq(rPow.ok, true);
        expect_eq(approxEq(rPow.value, 81.0, 0.000000001), true);

        const rMin = evalExpr("min(12.5, -4.25)");
        expect_eq(rMin.ok, true);
        expect_eq(approxEq(rMin.value, -4.25, 0.000000001), true);

        const rMax = evalExpr("max(12.5, -4.25)");
        expect_eq(rMax.ok, true);
        expect_eq(approxEq(rMax.value, 12.5, 0.000000001), true);

        expect_eq(approxEq(evalExpr("clamp(15, 0, 10)").value, 10.0, 0.000000001), true);
        expect_eq(approxEq(evalExpr("clamp(-5, 0, 10)").value, 0.0, 0.000000001), true);
        expect_eq(approxEq(evalExpr("clamp(4.5, 0, 10)").value, 4.5, 0.000000001), true);
    });

    test("4. @cap/mathjs-expr — MathScope custom variable bindings & repeated evaluation", () => {
        let scope = MathScope.new();
        expect_eq(scope.set("x", 3.0), true);
        expect_eq(scope.set("y", 4.0), true);
        expect_eq(scope.set("scale", 2.5), true);

        const r1 = evalExprWithScope("scale * sqrt(x^2 + y^2)", scope);
        expect_eq(r1.ok, true);
        expect_eq(approxEq(r1.value, 12.5, 0.000000001), true);

        // Update variable in scope and re-evaluate
        expect_eq(scope.set("x", 6.0), true);
        expect_eq(scope.set("y", 8.0), true);
        const r2 = evalExprWithScope("sqrt(x^2 + y^2)", scope);
        expect_eq(r2.ok, true);
        expect_eq(approxEq(r2.value, 10.0, 0.000000001), true);
    });

    test("5. @cap/mathjs-expr — complex nested expression evaluation", () => {
        const res = evalExpr("2 * (3 + sqrt(4^2 + 3^2)) - abs(-5)");
        expect_eq(res.ok, true);
        expect_eq(approxEq(res.value, 11.0, 0.000000001), true); // 2 * (3 + 5) - 5 == 11

        const res2 = evalExpr("clamp(pow(2, 4) - max(3, 7), 0, 10) + tan(0)");
        expect_eq(res2.ok, true);
        expect_eq(approxEq(res2.value, 9.0, 0.000000001), true); // 16 - 7 == 9
    });

    test("6. @cap/mathjs-expr — strict error detection (div-by-zero, unclosed paren, unknown ident, arity, empty)", () => {
        const eDiv = evalExpr("10 / (5 - 5)");
        expect_eq(eDiv.ok, false);
        expect_eq(eDiv.err_code, MJ_ERR_DIV_ZERO);

        const eParen = evalExpr("(1 + 2 * (3 + 4)");
        expect_eq(eParen.ok, false);
        expect_eq(eParen.err_code, MJ_ERR_UNCLOSED_PAREN);

        const eIdent = evalExpr("unknown_var + 1");
        expect_eq(eIdent.ok, false);
        expect_eq(eIdent.err_code, MJ_ERR_UNKNOWN_IDENT);

        const eFn = evalExpr("no_such_fn(1, 2)");
        expect_eq(eFn.ok, false);
        expect_eq(eFn.err_code, MJ_ERR_UNKNOWN_IDENT);

        const eArity = evalExpr("pow(2)");
        expect_eq(eArity.ok, false);
        expect_eq(eArity.err_code, MJ_ERR_ARITY);

        const eEmpty = evalExpr("   ");
        expect_eq(eEmpty.ok, false);
        expect_eq(eEmpty.err_code, MJ_ERR_SYNTAX);
    });
}
