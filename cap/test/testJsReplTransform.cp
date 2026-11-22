// cap/test/testJsReplTransform.cp
// Production-grade deep tests for cap:js replTransform.cp
// (Zero-alloc Interactive REPL AST Transform Planner & Wrapper Formatter)

import { test, assert, assertEq } from "cap:test";
import {
    REPL_MAX_STMTS,
    REPL_MAX_HOISTED_VARS,
    ReplTransformError,
    ReplStmtKind,
    ReplWrapMode,
    ReplStmtDesc,
    ReplProgramInput,
    planReplTransform,
} from "cap:js";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("planReplTransform: skips empty programs and top-level return modules", () => {
    const empty_plan = planReplTransform(ReplProgramInput.init()) catch unreachable;
    assert(empty_plan.mode == ReplWrapMode.skip);
    assertEq(empty_plan.hoisted_count, @as(usize, 0));
    assert(!empty_plan.captures_last_expr);

    let mod_prog = ReplProgramInput.init();
    mod_prog.has_top_level_return = true;
    * = mod_prog.addStmt(ReplStmtDesc.expr(false)) catch unreachable;
    const mod_plan = planReplTransform(mod_prog) catch unreachable;
    assert(mod_plan.mode == ReplWrapMode.skip);

    let buf: [32]u8 = [0; 32];
    assertEq(mod_plan.formatWrapperPrefix(buf[0..]) catch unreachable, @as(usize, 0));
    assertEq(mod_plan.formatWrapperSuffix(buf[0..]) catch unreachable, @as(usize, 0));
});

test("planReplTransform: sync IIFE, deduplicated var hoisting & last-expression capture", () => {
    let prog = ReplProgramInput.init();
    * = prog.addStmt(ReplStmtDesc.decl(ReplStmtKind.const_decl, "x", true)) catch unreachable;
    * = prog.addStmt(ReplStmtDesc.decl(ReplStmtKind.let_decl, "y", false)) catch unreachable; // no init
    * = prog.addStmt(ReplStmtDesc.decl(ReplStmtKind.var_decl, "x", true)) catch unreachable; // duplicate name
    * = prog.addStmt(ReplStmtDesc.decl(ReplStmtKind.class_decl, "Counter", true)) catch unreachable;
    * = prog.addStmt(ReplStmtDesc.expr(false)) catch unreachable;

    const plan = planReplTransform(prog) catch unreachable;
    assert(plan.mode == ReplWrapMode.sync_iife);
    assertEq(plan.hoisted_count, @as(usize, 3)); // x, y, Counter (deduplicated)
    assert(plan.hasHoistedVar("x"));
    assert(plan.hasHoistedVar("y"));
    assert(plan.hasHoistedVar("Counter"));
    assert(!plan.hasHoistedVar("z"));
    assertEq(plan.inner_stmt_count, @as(usize, 4)); // let y without init has no inner stmt
    assert(plan.captures_last_expr);
    if (idx := plan.last_expr_stmt_index) {
        assertEq(idx, @as(usize, 4));
    } else {
        assert(false);
    }

    let buf: [128]u8 = [0; 128];
    const n_pre = plan.formatWrapperPrefix(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_pre], "var x, y, Counter;\n(() => {\n"));
    const n_suf = plan.formatWrapperSuffix(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n_suf], "})()"));
});

test("planReplTransform: upgrades to async IIFE on top-level await or import statements", () => {
    // Triggered by import statement (becomes dynamic awaited import)
    let p_imp = ReplProgramInput.init();
    * = p_imp.addStmt(ReplStmtDesc.importDecl("fs")) catch unreachable;
    const plan_imp = planReplTransform(p_imp) catch unreachable;
    assert(plan_imp.mode == ReplWrapMode.async_iife);
    assert(plan_imp.hasHoistedVar("fs"));
    assert(!plan_imp.captures_last_expr);

    let buf: [128]u8 = [0; 128];
    const n1 = plan_imp.formatWrapperPrefix(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], "var fs;\n(async () => {\n"));

    // Triggered by top-level await flag on expression
    let p_await = ReplProgramInput.init();
    p_await.has_top_level_await = true;
    * = p_await.addStmt(ReplStmtDesc.expr(true)) catch unreachable;
    const plan_await = planReplTransform(p_await) catch unreachable;
    assert(plan_await.mode == ReplWrapMode.async_iife);
    assert(plan_await.captures_last_expr);
    const n2 = plan_await.formatWrapperPrefix(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n2], "(async () => {\n"));
});

test("ReplStmtKind: hoistsBinding & requiresAsync predicates", () => {
    assert(ReplStmtKind.var_decl.hoistsBinding());
    assert(ReplStmtKind.let_decl.hoistsBinding());
    assert(ReplStmtKind.const_decl.hoistsBinding());
    assert(ReplStmtKind.fn_decl.hoistsBinding());
    assert(ReplStmtKind.class_decl.hoistsBinding());
    assert(ReplStmtKind.import_stmt.hoistsBinding());
    assert(!ReplStmtKind.expr_stmt.hoistsBinding());
    assert(!ReplStmtKind.other_stmt.hoistsBinding());

    assert(ReplStmtKind.import_stmt.requiresAsync());
    assert(!ReplStmtKind.fn_decl.requiresAsync());
});

test("ReplTransformError: TooManyStmts & BufferTooSmall guards", () => {
    assertEq(REPL_MAX_STMTS, @as(usize, 32));
    assertEq(REPL_MAX_HOISTED_VARS, @as(usize, 64));

    let prog = ReplProgramInput.init();
    let k: usize = 0;
    while (k < REPL_MAX_STMTS) {
        * = prog.addStmt(ReplStmtDesc.expr(false)) catch unreachable;
        k += 1;
    }

    let too_many_err = false;
    const r1 = prog.addStmt(ReplStmtDesc.expr(false)) catch |e| {
        if (e == ReplTransformError.TooManyStmts) too_many_err = true;
        @as(usize, 0)
    };
    * = r1;
    assert(too_many_err);

    const plan = planReplTransform(prog) catch unreachable;
    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    const r2 = plan.formatWrapperPrefix(tiny[0..]) catch |e| {
        if (e == ReplTransformError.BufferTooSmall) buf_err = true;
        @as(usize, 0)
    };
    * = r2;
    assert(buf_err);
});
