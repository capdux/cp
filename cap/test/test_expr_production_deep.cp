// Source: cp/cap/test/test_expr_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:expr & cap.expr

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CompiledExpr,
    ExprContext,
    ExprValue,
    ExprError,
    compile,
    compileWithMaxDepth,
    eval,
} from "cap:compiler";

fn expectNumClose(actual: f64, expected: f64) {
    const diff = @abs(actual - expected);
    expect_eq(diff < 0.000000001, true);
}

fn testArithmeticUnaryTernaryAndFloat(): !void {
    const alloc = std.heap.page_allocator;
    let ctx = ExprContext.init(alloc);
    defer ctx.deinit();

    // Precedence and parentheses
    const v1 = eval("1 + 2 * 3", ctx)?;
    expectNumClose(v1.asNumber().?, 7.0);

    const v2 = eval("(1 + 2) * 3", ctx)?;
    expectNumClose(v2.asNumber().?, 9.0);

    const v3 = eval("20 - 12 / 3 + 7 % 4", ctx)?;
    expectNumClose(v3.asNumber().?, 19.0);

    // Unary minus and logical not
    const v4 = eval("--5 + -(2 + 3)", ctx)?;
    expectNumClose(v4.asNumber().?, 0.0);

    const v5 = eval("!false && !(10 < 5)", ctx)?;
    expect_eq(v5.asBool().?, true);

    // Scientific notation and floating-point precision
    const vf = eval("1.5e2 * 2.0 + 2.5e-1 - 0.25", ctx)?;
    expectNumClose(vf.asNumber().?, 300.0);

    // Ternary conditional with lazy branch evaluation
    const vt1 = eval("10 > 5 ? 42 : (1 / 0)", ctx)?;
    expectNumClose(vt1.asNumber().?, 42.0);

    const vt2 = eval("false ? (1 / 0) : 99", ctx)?;
    expectNumClose(vt2.asNumber().?, 99.0);
}

fn testContextVarsStringsAndBuiltins(): !void {
    const alloc = std.heap.page_allocator;
    let mem_arena = std.heap.ArenaAllocator.init(alloc);
    defer mem_arena.deinit();
    const ta = mem_arena.allocator();

    let ctx = ExprContext.init(ta);
    defer ctx.deinit();

    ctx.setNumber("user.score", 85.5)?;
    ctx.setBool("user.verified", true)?;
    ctx.setString("user.tier", "enterprise_v2")?;
    ctx.setNull("user.deleted_at")?;

    expect_eq(ctx.get("user.verified").?.asBool().?, true);
    expect_eq(ctx.get("user.deleted_at").?.isNull(), true);

    // String comparisons and concatenation
    expect_eq((eval("'alpha' < 'beta' && \"cp\" == 'cp'", ctx)?).asBool().?, true);
    const joined = eval("\"tier:\" + user.tier + '\\nok'", ctx)?;
    expect_eq(joined.asString().?, "tier:enterprise_v2\nok");

    // Built-in math & string functions
    expectNumClose((eval("min(10, 3.5, 7)", ctx)?).asNumber().?, 3.5);
    expectNumClose((eval("max(10, 3.5, 22)", ctx)?).asNumber().?, 22.0);
    expectNumClose((eval("abs(-42.5)", ctx)?).asNumber().?, 42.5);
    expectNumClose((eval("floor(3.9)", ctx)?).asNumber().?, 3.0);
    expectNumClose((eval("ceil(3.1)", ctx)?).asNumber().?, 4.0);
    expectNumClose((eval("round(3.6)", ctx)?).asNumber().?, 4.0);
    expectNumClose((eval("clamp(user.score, 0, 80)", ctx)?).asNumber().?, 80.0);
    expectNumClose((eval("clamp(-5, 0, 80)", ctx)?).asNumber().?, 0.0);
    expectNumClose((eval("len(user.tier)", ctx)?).asNumber().?, 13.0);
    expect_eq((eval("contains(user.tier, 'prise')", ctx)?).asBool().?, true);
    expect_eq((eval("startsWith(user.tier, 'ent')", ctx)?).asBool().?, true);
    expect_eq((eval("startsWith(user.tier, 'pro')", ctx)?).asBool().?, false);

    // CompiledExpr reuse
    let compiled = compile(ta, "user.verified && clamp(user.score, 0, 100) >= 85 && startsWith(user.tier, 'ent')")?;
    defer compiled.deinit();
    expect_eq((compiled.eval(ctx)?).asBool().?, true);
}

fn testShortCircuitEvaluation(): !void {
    const alloc = std.heap.page_allocator;
    let ctx = ExprContext.init(alloc);
    defer ctx.deinit();

    // Right-hand side division-by-zero must NOT trigger when short-circuited
    expect_eq((eval("1 == 1 || (1 / 0) > 0", ctx)?).asBool().?, true);
    expect_eq((eval("1 == 2 && (1 / 0) > 0", ctx)?).asBool().?, false);
    expect_eq((eval("false && unknown_var && evil_fn(1)", ctx)?).asBool().?, false);
    expect_eq((eval("true || unknown_var || (10 % 0 == 0)", ctx)?).asBool().?, true);

    // Non-short-circuited branch MUST trigger DivisionByZero
    const res = eval("true && (1 / 0) > 0", ctx);
    const tag = switch (res) {
        ExprError.DivisionByZero => "div_zero",
        else => "other",
        ok => "ok",
    };
    expect_eq(tag, "div_zero");
}

fn testErrorGuardsAndMaxDepth(): !void {
    const alloc = std.heap.page_allocator;
    let ctx = ExprContext.init(alloc);
    defer ctx.deinit();

    // DivisionByZero
    const e_div = switch (eval("10 / (5 - 5)", ctx)) {
        ExprError.DivisionByZero => "ok",
        else => "fail",
        ok => "fail",
    };
    expect_eq(e_div, "ok");

    // UndefinedVariable
    const e_undef = switch (eval("user.missing_field", ctx)) {
        ExprError.UndefinedVariable => "ok",
        else => "fail",
        ok => "fail",
    };
    expect_eq(e_undef, "ok");

    // TypeMismatch
    const e_tm1 = switch (eval("\"abc\" + 1", ctx)) {
        ExprError.TypeMismatch => "ok",
        else => "fail",
        ok => "fail",
    };
    expect_eq(e_tm1, "ok");

    const e_tm2 = switch (eval("true < false", ctx)) {
        ExprError.TypeMismatch => "ok",
        else => "fail",
        ok => "fail",
    };
    expect_eq(e_tm2, "ok");

    // UnknownFunction & InvalidArity
    const e_fn = switch (eval("exec_shell('ls')", ctx)) {
        ExprError.UnknownFunction => "ok",
        else => "fail",
        ok => "fail",
    };
    expect_eq(e_fn, "ok");

    const e_arity = switch (eval("len('a', 'b')", ctx)) {
        ExprError.InvalidArity => "ok",
        else => "fail",
        ok => "fail",
    };
    expect_eq(e_arity, "ok");

    // MaxDepthExceeded on 65-level nested parentheses (default max_depth = 64)
    let buf: [256]u8 = undefined;
    let idx: usize = 0;
    let k: usize = 0;
    while (k < 65) {
        buf[idx] = '(';
        idx += 1;
        k += 1;
    }
    buf[idx] = '1';
    idx += 1;
    k = 0;
    while (k < 65) {
        buf[idx] = ')';
        idx += 1;
        k += 1;
    }
    const e_depth = switch (eval(buf[0..idx], ctx)) {
        ExprError.MaxDepthExceeded => "ok",
        else => "fail",
        ok => "fail",
    };
    expect_eq(e_depth, "ok");

    // Custom max_depth via compileWithMaxDepth
    const e_custom = switch (compileWithMaxDepth(alloc, "((1 + 2))", 1)) {
        ExprError.MaxDepthExceeded => "ok",
        else => "fail",
        ok => "fail",
    };
    expect_eq(e_custom, "ok");
}

fn testCapNamespaceAndZeroLeak(): !void {
    const alloc = std.testing.allocator;
    let ctx = cap.compiler.ExprContext.init(alloc);
    defer ctx.deinit();

    ctx.setNumber("agent.score", 92.0)?;
    ctx.setBool("agent.enabled", true)?;
    ctx.setString("agent.mode", "production")?;

    let compiled = cap.compiler.compile(
        alloc,
        "agent.enabled && cap_check_ok ? 1 : 0",
    )?;
    defer compiled.deinit();
    const err_tag = switch (compiled.eval(ctx)) {
        ExprError.UndefinedVariable => "undef",
        else => "other",
        ok => "ok",
    };
    expect_eq(err_tag, "undef");

    let valid = cap.compiler.compile(
        alloc,
        "agent.enabled && clamp(agent.score, 0, 100) >= 90 && contains(agent.mode, 'prod') ? max(10, 20) : min(1, 2)",
    )?;
    defer valid.deinit();

    const r1 = valid.eval(ctx)?;
    expectNumClose(r1.asNumber().?, 20.0);

    const r2 = cap.compiler.eval("startsWith(agent.mode, 'pro') && len(agent.mode) == 10", ctx)?;
    expect_eq(r2.asBool().?, true);
}

test("1. Arithmetic Precedence, Unary, Ternary & Scientific Float", () => {
    testArithmeticUnaryTernaryAndFloat()?;
});

test("2. ExprContext Dotted Variables, Strings & Built-in Pure Functions", () => {
    testContextVarsStringsAndBuiltins()?;
});

test("3. Short-Circuit && / || Evaluation & DivisionByZero Guard", () => {
    testShortCircuitEvaluation()?;
});

test("4. Error Guards (DivisionByZero, UndefinedVariable, TypeMismatch, MaxDepthExceeded)", () => {
    testErrorGuardsAndMaxDepth()?;
});

test("5. Dynamic `cap.expr` Mounting & `std.testing.allocator` Zero-Leak Verification", () => {
    testCapNamespaceAndZeroLeak()?;
});
