// Deep production regression tests for cap:css Math Functions Level 4 (lib/css/mathFunc.cp)

const std = @import("std");
import { test, expect, expectError } from "cap:test";
import {
    CSS_MATH_PI,
    CSS_MATH_E,
    CssMathFuncError,
    CssMathRoundingStrategy,
    CssMathConstant,
    CssMathFuncKind,
    cssMathRound,
    cssMathMod,
    cssMathRem,
    cssMathSign,
    cssMathClamp,
    cssMathHypot,
    cssMathLog,
    cssMathAngleToRad,
    evalCssMathExpr,
} from "cap:css";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const d = if (a >= b) a - b else b - a;
    return d <= eps;
}

test("cssMathFunc: function kind, rounding strategy and constant parsing", () => {
    const k_round = CssMathFuncKind.parse("ROUND") ?? return;
    expect(k_round == .Round).toBe(true);
    expect(k_round.isStepped()).toBe(true);
    expect(k_round.isTrig()).toBe(false);
    expect(k_round.asStr()).toEqual("round");

    const k_atan2 = CssMathFuncKind.parse("atan2") ?? return;
    expect(k_atan2.isTrig()).toBe(true);
    expect(k_atan2.asStr()).toEqual("atan2");

    const k_hypot = CssMathFuncKind.parse("Hypot") ?? return;
    expect(k_hypot.isExponential()).toBe(true);

    expect(CssMathFuncKind.parse("unknownFn") == null).toBe(true);

    const st = CssMathRoundingStrategy.parse("TO-ZERO") ?? return;
    expect(st == .ToZero).toBe(true);
    expect(st.asStr()).toEqual("to-zero");

    const c_pi = CssMathConstant.parse("PI") ?? return;
    expect(approxEq(c_pi.toF64(), CSS_MATH_PI, 1e-12)).toBe(true);
    const c_ninf = CssMathConstant.parse("-infinity") ?? return;
    expect(std.math.isInf(c_ninf.toF64()) && c_ninf.toF64() < 0.0).toBe(true);
    const c_nan = CssMathConstant.parse("NaN") ?? return;
    expect(std.math.isNan(c_nan.toF64())).toBe(true);
});

test("cssMathFunc: stepped functions round, mod and rem sign semantics", () => {
    expect(approxEq(cssMathRound(.Nearest, 14.0, 5.0)?, 15.0, 1e-9)).toBe(true);
    expect(approxEq(cssMathRound(.Up, 11.0, 5.0)?, 15.0, 1e-9)).toBe(true);
    expect(approxEq(cssMathRound(.Down, 14.0, 5.0)?, 10.0, 1e-9)).toBe(true);
    expect(approxEq(cssMathRound(.ToZero, -14.0, 5.0)?, -10.0, 1e-9)).toBe(true);

    // W3C mod() 与 divisor 同号；rem() 与 dividend 同号
    expect(approxEq(cssMathMod(-18.0, 5.0)?, 2.0, 1e-9)).toBe(true);
    expect(approxEq(cssMathRem(-18.0, 5.0)?, -3.0, 1e-9)).toBe(true);
    expect(approxEq(cssMathMod(18.0, -5.0)?, -2.0, 1e-9)).toBe(true);
    expect(approxEq(cssMathRem(18.0, -5.0)?, 3.0, 1e-9)).toBe(true);

    // 表达式求值（含常量作为 round 首参，不得误判为非法策略）
    expect(approxEq(evalCssMathExpr("round(pi, 1)")?, 3.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("round(up, 12, 5)")?, 15.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("mod(-18, 5) + rem(-18, 5)")?, -1.0, 1e-9)).toBe(true);
});

test("cssMathFunc: comparison min/max/clamp and sign/abs semantics", () => {
    expect(approxEq(evalCssMathExpr("min(10, 3, 7, -2)")?, -2.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("max(10, 3, 7, -2)")?, 10.0, 1e-9)).toBe(true);
    // W3C 规范：当 min > max 时，clamp(min, val, max) 恒返回 min
    expect(approxEq(cssMathClamp(10.0, 5.0, 2.0), 10.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("clamp(10, 5, 2)")?, 10.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("clamp(0, 25, 100)")?, 25.0, 1e-9)).toBe(true);

    expect(approxEq(evalCssMathExpr("abs(-42.5)")?, 42.5, 1e-9)).toBe(true);
    expect(approxEq(cssMathSign(-7.0), -1.0, 1e-9)).toBe(true);
    expect(approxEq(cssMathSign(0.0), 0.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("sign(99)")?, 1.0, 1e-9)).toBe(true);
});

test("cssMathFunc: trigonometric functions with deg/rad/grad/turn units", () => {
    expect(approxEq(cssMathAngleToRad(180.0, "deg")?, CSS_MATH_PI, 1e-9)).toBe(true);
    expect(approxEq(cssMathAngleToRad(200.0, "grad")?, CSS_MATH_PI, 1e-9)).toBe(true);
    expect(approxEq(cssMathAngleToRad(0.5, "turn")?, CSS_MATH_PI, 1e-9)).toBe(true);

    expect(approxEq(evalCssMathExpr("sin(90deg)")?, 1.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("cos(0.5turn)")?, -1.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("tan(50grad)")?, 1.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("sin(pi / 2)")?, 1.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("asin(1)")?, CSS_MATH_PI / 2.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("acos(0)")?, CSS_MATH_PI / 2.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("atan2(1, 1)")?, CSS_MATH_PI / 4.0, 1e-9)).toBe(true);
});

test("cssMathFunc: exponential functions and deeply nested calc expressions", () => {
    expect(approxEq(evalCssMathExpr("pow(2, 10)")?, 1024.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("sqrt(81)")?, 9.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("hypot(3, 4)")?, 5.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("hypot(2, 3, 6)")?, 7.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("log(e)")?, 1.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("log(1000, 10)")?, 3.0, 1e-9)).toBe(true);
    expect(approxEq(evalCssMathExpr("exp(1)")?, CSS_MATH_E, 1e-9)).toBe(true);

    const arr: [3]f64 = [2.0, 3.0, 6.0];
    expect(approxEq(cssMathHypot(arr[0..])?, 7.0, 1e-9)).toBe(true);
    expect(approxEq(cssMathLog(64.0, 2.0)?, 6.0, 1e-9)).toBe(true);

    // 复合嵌套：round(up, hypot(3, 4) * 2 + 1, 5) = round(up, 11, 5) = 15; + abs(-3) = 18
    expect(approxEq(evalCssMathExpr("calc(round(up, hypot(3, 4) * 2 + 1, 5) + abs(-3))")?, 18.0, 1e-9)).toBe(true);
});

test("cssMathFunc: domain, arity, division-by-zero and malformed expression guards", () => {
    expectError(evalCssMathExpr("   "), CssMathFuncError.EmptyExpression);
    expectError(evalCssMathExpr("foo(1)"), CssMathFuncError.UnknownFunction);
    expectError(evalCssMathExpr("tau"), CssMathFuncError.UnknownConstant);
    expectError(evalCssMathExpr("clamp(1, 2)"), CssMathFuncError.InvalidArity);
    expectError(evalCssMathExpr("round(bogus, 1, 2)"), CssMathFuncError.InvalidRoundingStrategy);
    expectError(evalCssMathExpr("10 / 0"), CssMathFuncError.DivisionByZero);
    expectError(evalCssMathExpr("mod(5, 0)"), CssMathFuncError.DivisionByZero);
    expectError(evalCssMathExpr("round(up, 5, 0)"), CssMathFuncError.DivisionByZero);
    expectError(evalCssMathExpr("sqrt(-4)"), CssMathFuncError.DomainError);
    expectError(evalCssMathExpr("asin(1.5)"), CssMathFuncError.DomainError);
    expectError(evalCssMathExpr("log(0)"), CssMathFuncError.DomainError);
    expectError(evalCssMathExpr("log(10, 1)"), CssMathFuncError.DomainError);
    expectError(evalCssMathExpr("calc(1 + 2"), CssMathFuncError.UnclosedParen);
    expectError(evalCssMathExpr("1 + 2 3"), CssMathFuncError.TrailingTokens);
    expectError(
        evalCssMathExpr("((((((((((((((((((((((((((((((((((1))))))))))))))))))))))))))))))))))"),
        CssMathFuncError.MaxDepthExceeded,
    );
});
