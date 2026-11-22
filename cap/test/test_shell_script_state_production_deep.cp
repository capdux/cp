// cap/test/test_shell_script_state_production_deep.cp
// 深度生产级回归测试：cap:shell 顶层脚本与语句序列状态机 (lib/shell/script_state.cp)

import { test } from "cap:test";
import {
    SHELL_SCRIPT_MAX_EXPRS,
    ScriptHostKind,
    ShellExprKind,
    ScriptStepKind,
    scriptPropagatesEnvToParent,
    ShellScriptStateMachine,
} from "cap:shell";

test("shell_script_state: empty script completes immediately with exit code 0") {
    let sc = ShellScriptStateMachine.init(ScriptHostKind.TopLevel, false);
    const res = sc.nextStep()?;
    if (res.kind != ScriptStepKind.Completed || res.exit_code != 0 || !sc.isDone() || sc.executedCount() != 0) {
        return err.EmptyScriptMustCompleteZero;
    }
}

test("shell_script_state: nothrow sequential execution returns last statement exit code") {
    let sc = ShellScriptStateMachine.init(ScriptHostKind.TopLevel, false);
    * = sc.pushExpr(ShellExprKind.Assign, 10, false)?;
    * = sc.pushExpr(ShellExprKind.Cmd, 20, false)?;
    * = sc.pushExpr(ShellExprKind.Pipeline, 30, false)?;

    const s0 = sc.nextStep()?;
    if (s0.kind != ScriptStepKind.RunExpr || s0.expr_kind != ShellExprKind.Assign || s0.expr_id != 10) {
        return err.ExpectedExpr10;
    }
    if (!(sc.onExprDone(0)?)) return err.ShouldContinue0;

    // 非 errexit 模式：即使语句 20 返回非零退出码 1，也继续执行语句 30
    const s1 = sc.nextStep()?;
    if (s1.expr_id != 20) return err.ExpectedExpr20;
    if (!(sc.onExprDone(1)?)) return err.NothrowMustContinue;

    const s2 = sc.nextStep()?;
    if (s2.expr_kind != ShellExprKind.Pipeline || s2.expr_id != 30) return err.ExpectedExpr30;
    if (!(sc.onExprDone(5)?)) return err.ShouldContinue2;

    const done = sc.nextStep()?;
    if (done.kind != ScriptStepKind.Completed ||
        done.exit_code != 5 ||
        sc.finalExitCode() != 5 ||
        sc.executedCount() != 3 ||
        sc.isAborted()) {
        return err.SequentialDoneMismatch;
    }
}

test("shell_script_state: errexit (.throws) aborts on non-zero exit & skips remaining stmts") {
    let sc = ShellScriptStateMachine.init(ScriptHostKind.TopLevel, true);
    * = sc.pushExpr(ShellExprKind.Cmd, 1, false)?;
    * = sc.pushExpr(ShellExprKind.Cmd, 2, false)?;
    * = sc.pushExpr(ShellExprKind.Cmd, 3, false)?;

    * = sc.nextStep()?;
    if (!(sc.onExprDone(0)?)) return err.ZeroMustContinue;

    * = sc.nextStep()?;
    const should_continue = sc.onExprDone(127)?;
    if (should_continue || !sc.isAborted() || !sc.isDone() || sc.finalExitCode() != 127) {
        return err.ErrexitMustAbortOn127;
    }

    const after = sc.nextStep()?;
    if (after.kind != ScriptStepKind.AbortedOnError || after.exit_code != 127 || sc.executedCount() != 2) {
        return err.Stmt3MustBeSkipped;
    }
}

test("shell_script_state: errexit exemption allows guarded conditional expressions to fail safely") {
    let sc = ShellScriptStateMachine.init(ScriptHostKind.CommandSubstitution, true);
    // exempt_from_errexit = true 的语句即使返回非 0 也不触发提前中止
    * = sc.pushExpr(ShellExprKind.If, 100, true)?;
    * = sc.pushExpr(ShellExprKind.Cmd, 200, false)?;

    * = sc.nextStep()?;
    if (!(sc.onExprDone(1)?)) return err.ExemptStmtMustNotAbort;
    if (sc.isAborted()) return err.MustNotBeAborted;

    const s2 = sc.nextStep()?;
    if (s2.expr_id != 200) return err.ExpectedStmt200;
    * = sc.onExprDone(0)?;

    const done = sc.nextStep()?;
    if (done.kind != ScriptStepKind.Completed || done.exit_code != 0) {
        return err.ExemptFlowCompletionMismatch;
    }
}

test("shell_script_state: host environment propagation rules (TopLevel vs Subshell/CmdSubst)") {
    if (!scriptPropagatesEnvToParent(ScriptHostKind.TopLevel)) return err.TopLevelMustPropagate;
    if (scriptPropagatesEnvToParent(ScriptHostKind.Subshell)) return err.SubshellMustIsolate;
    if (scriptPropagatesEnvToParent(ScriptHostKind.CommandSubstitution)) return err.CmdSubstMustIsolate;

    const sub = ShellScriptStateMachine.init(ScriptHostKind.Subshell, true);
    if (sub.hostKind() != ScriptHostKind.Subshell || !sub.isErrexitEnabled()) {
        return err.HostGettersMismatch;
    }
}

test("shell_script_state: protocol guards & 64-expression capacity bound") {
    let sc = ShellScriptStateMachine.init(ScriptHostKind.TopLevel, false);

    // 未执行语句时调用 onExprDone 必须拦截
    let not_await = false;
    * = sc.onExprDone(0) catch {
        not_await = true;
        false;
    };
    if (!not_await) return err.ExpectedNotAwaitingExpr;

    // 填满 64 条语句，第 65 条拦截
    let i: usize = 0;
    while (i < SHELL_SCRIPT_MAX_EXPRS) {
        * = sc.pushExpr(ShellExprKind.Cmd, i as u32, false)?;
        i += 1;
    }
    let overflow = false;
    * = sc.pushExpr(ShellExprKind.Cmd, 999, false) catch {
        overflow = true;
        0;
    };
    if (!overflow) return err.ExpectedTooManyExprs;

    // 启动后禁止 pushExpr，等待回调期间禁止重复 nextStep
    * = sc.nextStep()?;
    let pending = false;
    * = sc.nextStep() catch { pending = true; };
    if (!pending) return err.ExpectedExprStillPending;

    let started = false;
    * = sc.pushExpr(ShellExprKind.Cmd, 888, false) catch {
        started = true;
        0;
    };
    if (!started) return err.ExpectedAlreadyStarted;
}
