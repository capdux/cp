// cap/test/testCapExpr.cp
// Comprehensive unit tests for @cap/expr:
// 1. Arithmetic precedence & parentheses
// 2. Right-associative power & unary operators (-, +, !)
// 3. Comparison, logical short-circuiting & ternary conditionals
// 4. ExprEnv variable binding & formula evaluation
// 5. Built-in math & conditional functions + PI / E constants
// 6. Structured error codes (DivisionByZero, UnknownVariable, UnknownFunction, InvalidArgs, UnexpectedToken, MaxDepthExceeded)

import { test, expect_eq } from "cap:test";
import {
    ExprEnv,
    ExprError,
    EvalResult,
    MATH_PI,
    MATH_E,
    evalExpr,
    evalExprWith,
    evalBool,
} from "../../packages/expr-eval/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn main() {
    test("1. @cap/expr — Arithmetic Precedence, Modulo & Parentheses", () => {
        const r1 = evalExpr("1 + 2 * 3");
        expect_eq(r1.ok, true);
        expect_eq(approxEq(r1.value, 7.0, 1.0e-9), true);

        const r2 = evalExpr("(1 + 2) * 3");
        expect_eq(r2.ok, true);
        expect_eq(approxEq(r2.value, 9.0, 1.0e-9), true);

        const r3 = evalExpr("20 - 12 / 4 + 7 % 4");
        expect_eq(r3.ok, true);
        expect_eq(approxEq(r3.value, 20.0, 1.0e-9), true);

        const r4 = evalExpr("((2.5 + 1.5) * (10 - 2.5)) / 3");
        expect_eq(r4.ok, true);
        expect_eq(approxEq(r4.value, 10.0, 1.0e-9), true);

        const r5 = evalExpr("1.5e2 + .5 * 10");
        expect_eq(r5.ok, true);
        expect_eq(approxEq(r5.value, 155.0, 1.0e-9), true);
    });

    test("2. @cap/expr — Right-Associative Power & Unary Operators", () => {
        // Right-associative: 2 ^ (3 ^ 2) = 2 ^ 9 = 512
        const r1 = evalExpr("2 ^ 3 ^ 2");
        expect_eq(r1.ok, true);
        expect_eq(approxEq(r1.value, 512.0, 1.0e-9), true);

        // Unary minus has higher precedence than ^: (-2) ^ 2 = 4, -(2 ^ 2) = -4
        const r2 = evalExpr("-2 ^ 2");
        expect_eq(r2.ok, true);
        expect_eq(approxEq(r2.value, 4.0, 1.0e-9), true);

        const r3 = evalExpr("-(2 ^ 2)");
        expect_eq(r3.ok, true);
        expect_eq(approxEq(r3.value, -4.0, 1.0e-9), true);

        // Negative exponent: 2 ^ -3 = 0.125
        const r4 = evalExpr("2 ^ -3");
        expect_eq(r4.ok, true);
        expect_eq(approxEq(r4.value, 0.125, 1.0e-9), true);

        // Double unary & logical NOT
        const r5 = evalExpr("--5 + +3");
        expect_eq(r5.ok, true);
        expect_eq(approxEq(r5.value, 8.0, 1.0e-9), true);

        const r6 = evalExpr("!0 + !5 + !!2");
        expect_eq(r6.ok, true);
        expect_eq(approxEq(r6.value, 2.0, 1.0e-9), true);
    });

    test("3. @cap/expr — Comparisons, Logical Short-Circuit & Ternary", () => {
        const empty = ExprEnv.init();
        expect_eq(evalBool("3 < 5 && 5 <= 5 && 6 > 4 && 6 >= 6", empty), true);
        expect_eq(evalBool("4 == 4 && 4 != 5", empty), true);
        expect_eq(evalBool("false || true", empty), true);
        expect_eq(evalBool("true && false", empty), false);

        // Short-circuit prevents division by zero on inactive branch
        expect_eq(evalBool("false && (1 / 0 == 1)", empty), false);
        expect_eq(evalBool("true || (1 / 0 == 1)", empty), true);

        const t1 = evalExpr("10 > 5 ? 100 : 200");
        expect_eq(t1.ok, true);
        expect_eq(approxEq(t1.value, 100.0, 1.0e-9), true);

        // Right-associative nested ternary + short-circuit on untaken branch
        const t2 = evalExpr("false ? (1 / 0) : (2 == 3 ? 10 : 42)");
        expect_eq(t2.ok, true);
        expect_eq(approxEq(t2.value, 42.0, 1.0e-9), true);
    });

    test("4. @cap/expr — ExprEnv Variable Binding & Formula Evaluation", () => {
        let env = ExprEnv.init();
        expect_eq(env.has("price"), false);
        expect_eq(env.get("price") == null, true);

        env.set("price", 80.0);
        env.set("qty", 4.0);
        env.set("tax_rate", 0.1);
        expect_eq(env.has("price"), true);
        expect_eq(env.len(), 3 as usize);

        // Overwrite existing variable
        env.set("price", 100.0);
        expect_eq(env.len(), 3 as usize);
        if (p := env.get("price")) {
            expect_eq(approxEq(p, 100.0, 1.0e-9), true);
        } else {
            expect_eq(false, true);
        }

        const total = evalExprWith("price * qty * (1 + tax_rate)", env);
        expect_eq(total.ok, true);
        expect_eq(approxEq(total.value, 440.0, 1.0e-9), true);

        expect_eq(evalBool("price * qty >= 400 && tax_rate < 0.2", env), true);
        expect_eq(evalBool("price * qty > 500", env), false);
    });

    test("5. @cap/expr — Built-in Functions & Mathematical Constants", () => {
        const r_abs = evalExpr("abs(-12.5)");
        expect_eq(r_abs.ok && approxEq(r_abs.value, 12.5, 1.0e-9), true);

        const r_mm = evalExpr("min(9, 4) + max(-3, 7)");
        expect_eq(r_mm.ok && approxEq(r_mm.value, 11.0, 1.0e-9), true);

        const r_fcr = evalExpr("floor(3.9) + ceil(3.1) + round(2.6)");
        expect_eq(r_fcr.ok && approxEq(r_fcr.value, 10.0, 1.0e-9), true);

        const r_sp = evalExpr("sqrt(81) + pow(2, 5)");
        expect_eq(r_sp.ok && approxEq(r_sp.value, 41.0, 1.0e-9), true);

        const r_clamp = evalExpr("clamp(15, 0, 10) + clamp(-5, 0, 10) + clamp(6, 0, 10)");
        expect_eq(r_clamp.ok && approxEq(r_clamp.value, 16.0, 1.0e-9), true);

        const r_sign = evalExpr("sign(-42) + sign(0) + sign(99)");
        expect_eq(r_sign.ok && approxEq(r_sign.value, 0.0, 1.0e-9), true);

        const r_avg_if = evalExpr("avg(10, 20) + if(3 > 1, 5, 100) + if(0, 5, 2)");
        expect_eq(r_avg_if.ok && approxEq(r_avg_if.value, 22.0, 1.0e-9), true);

        const r_const = evalExpr("PI + E");
        expect_eq(r_const.ok && approxEq(r_const.value, MATH_PI + MATH_E, 1.0e-9), true);
    });

    test("6. @cap/expr — Error Codes & Recursion Depth Guard", () => {
        // Division by zero (/ and %)
        const e_div = evalExpr("10 / (3 - 3)");
        expect_eq(e_div.ok, false);
        expect_eq(e_div.err == ExprError.DivisionByZero, true);

        const e_mod = evalExpr("10 % 0");
        expect_eq(e_mod.ok, false);
        expect_eq(e_mod.err == ExprError.DivisionByZero, true);

        // Unknown variable
        const e_var = evalExpr("x + 1");
        expect_eq(e_var.ok, false);
        expect_eq(e_var.err == ExprError.UnknownVariable, true);

        // Unknown function
        const e_fn = evalExpr("foobar(1, 2)");
        expect_eq(e_fn.ok, false);
        expect_eq(e_fn.err == ExprError.UnknownFunction, true);

        // Invalid args (wrong arity or negative sqrt)
        const e_args1 = evalExpr("min(1)");
        expect_eq(e_args1.ok, false);
        expect_eq(e_args1.err == ExprError.InvalidArgs, true);

        const e_args2 = evalExpr("sqrt(-4)");
        expect_eq(e_args2.ok, false);
        expect_eq(e_args2.err == ExprError.InvalidArgs, true);

        // Unexpected trailing token
        const e_trail = evalExpr("1 + 2 3");
        expect_eq(e_trail.ok, false);
        expect_eq(e_trail.err == ExprError.UnexpectedToken, true);

        // Syntax error (empty or unclosed parenthesis)
        const e_syn1 = evalExpr("   ");
        expect_eq(e_syn1.ok, false);
        expect_eq(e_syn1.err == ExprError.SyntaxError, true);

        const e_syn2 = evalExpr("(1 + 2");
        expect_eq(e_syn2.ok, false);
        expect_eq(e_syn2.err == ExprError.SyntaxError, true);

        // Depth guard: 35 nested parentheses exceeds MAX_DEPTH = 32
        const deep = "(((((((((((((((((((((((((((((((((((1)))))))))))))))))))))))))))))))))))";
        const e_depth = evalExpr(deep);
        expect_eq(e_depth.ok, false);
        expect_eq(e_depth.err == ExprError.MaxDepthExceeded, true);
    });
}
