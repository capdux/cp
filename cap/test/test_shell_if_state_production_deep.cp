// cap/test/test_shell_if_state_production_deep.cp
// 深度生产级回归测试：cap:shell POSIX `if / elif / else` 控制流状态机 (lib/shell/if_state.cp)

import { test } from "cap:test";
import {
    SHELL_IF_MAX_ELSE_PARTS,
    IfBranchPhase,
    IfStepKind,
    ShellIfStateMachine,
} from "cap:shell";

test("shell_if_state: multi-stmt cond uses last exit code & returns then exit code") {
    let sm = ShellIfStateMachine.init();
    // if cmd10; cmd11; then cmd20; cmd21; fi
    * = sm.addCondStmt(10)?;
    * = sm.addCondStmt(11)?;
    * = sm.addThenStmt(20)?;
    * = sm.addThenStmt(21)?;

    // cmd10 返回非 0，但 cmd11 返回 0 -> 条件整体为真
    const s1 = sm.nextStep()?;
    if (s1.kind != IfStepKind.RunStmt || s1.phase != IfBranchPhase.Cond || s1.stmt_id != 10) {
        return err.ExpectedCond10;
    }
    * = sm.onStmtDone(1)?;

    const s2 = sm.nextStep()?;
    if (s2.kind != IfStepKind.RunStmt || s2.phase != IfBranchPhase.Cond || s2.stmt_id != 11) {
        return err.ExpectedCond11;
    }
    * = sm.onStmtDone(0)?;

    // 进入 then 分支，最终退出码等于最后一条语句 cmd21 的退出码 7
    const s3 = sm.nextStep()?;
    if (s3.kind != IfStepKind.RunStmt || s3.phase != IfBranchPhase.Then || s3.stmt_id != 20) {
        return err.ExpectedThen20;
    }
    * = sm.onStmtDone(0)?;

    const s4 = sm.nextStep()?;
    if (s4.kind != IfStepKind.RunStmt || s4.phase != IfBranchPhase.Then || s4.stmt_id != 21) {
        return err.ExpectedThen21;
    }
    * = sm.onStmtDone(7)?;

    const done = sm.nextStep()?;
    if (done.kind != IfStepKind.Completed || done.exit_code != 7 || !sm.isDone() || sm.executedStmtCount() != 4) {
        return err.ExpectedCompletedExit7;
    }
}

test("shell_if_state: false condition with no else returns POSIX exit code 0") {
    let sm = ShellIfStateMachine.init();
    * = sm.addCondStmt(100)?;
    * = sm.addThenStmt(200)?;

    const s1 = sm.nextStep()?;
    if (s1.stmt_id != 100) return err.ExpectedCond100;
    * = sm.onStmtDone(2)?; // 条件失败

    // 无 else 时 POSIX 规定整体退出码必须为 0
    const done = sm.nextStep()?;
    if (done.kind != IfStepKind.Completed || done.exit_code != 0 || sm.finalExitCode() != 0) {
        return err.FalseCondNoElseMustExitZero;
    }
    if (sm.executedStmtCount() != 1) return err.ThenMustNotRun;
}

test("shell_if_state: simple if-else branch dispatch (else_parts len == 1)") {
    let sm = ShellIfStateMachine.init();
    * = sm.addCondStmt(1)?;
    * = sm.addThenStmt(2)?;
    const p_else = sm.beginElsePart()?;
    * = sm.addElsePartStmt(p_else, 3)?;
    * = sm.addElsePartStmt(p_else, 4)?;

    * = sm.nextStep()?; // cond 1
    * = sm.onStmtDone(1)?;

    const e1 = sm.nextStep()?;
    if (e1.phase != IfBranchPhase.ElseBranch || e1.stmt_id != 3) return err.ExpectedElse3;
    * = sm.onStmtDone(0)?;

    const e2 = sm.nextStep()?;
    if (e2.phase != IfBranchPhase.ElseBranch || e2.stmt_id != 4) return err.ExpectedElse4;
    * = sm.onStmtDone(19)?;

    const done = sm.nextStep()?;
    if (done.kind != IfStepKind.Completed || done.exit_code != 19) return err.ElseExitCodeMismatch;
}

test("shell_if_state: chained elif-elif-else hitting second elif") {
    let sm = ShellIfStateMachine.init();
    * = sm.addCondStmt(1)?;
    * = sm.addThenStmt(2)?;
    // else_parts: [0]=elif1_cond(10), [1]=elif1_then(11), [2]=elif2_cond(20), [3]=elif2_then(21), [4]=else(30)
    const p0 = sm.beginElsePart()?; * = sm.addElsePartStmt(p0, 10)?;
    const p1 = sm.beginElsePart()?; * = sm.addElsePartStmt(p1, 11)?;
    const p2 = sm.beginElsePart()?; * = sm.addElsePartStmt(p2, 20)?;
    const p3 = sm.beginElsePart()?; * = sm.addElsePartStmt(p3, 21)?;
    const p4 = sm.beginElsePart()?; * = sm.addElsePartStmt(p4, 30)?;

    // if cond(1) -> fail
    * = sm.nextStep()?;
    * = sm.onStmtDone(1)?;

    // elif1 cond(10) -> fail
    const c1 = sm.nextStep()?;
    if (c1.phase != IfBranchPhase.ElifCond || c1.stmt_id != 10) return err.ExpectedElif1Cond;
    * = sm.onStmtDone(1)?;

    // elif2 cond(20) -> succeed (0)
    const c2 = sm.nextStep()?;
    if (c2.phase != IfBranchPhase.ElifCond || c2.stmt_id != 20) return err.ExpectedElif2Cond;
    * = sm.onStmtDone(0)?;

    // elif2 then(21) -> runs and exits 5
    const t2 = sm.nextStep()?;
    if (t2.phase != IfBranchPhase.Then || t2.stmt_id != 21) return err.ExpectedElif2Then;
    * = sm.onStmtDone(5)?;

    const done = sm.nextStep()?;
    if (done.kind != IfStepKind.Completed || done.exit_code != 5 || sm.executedStmtCount() != 4) {
        return err.Elif2DoneMismatch;
    }
}

test("shell_if_state: chained elif falling through to trailing else vs no-else exit 0") {
    // 场景 A: if + elif + else，所有条件失败 -> 执行 else(30)
    let sm_a = ShellIfStateMachine.init();
    * = sm_a.addCondStmt(1)?;
    * = sm_a.addThenStmt(2)?;
    const a0 = sm_a.beginElsePart()?; * = sm_a.addElsePartStmt(a0, 10)?;
    const a1 = sm_a.beginElsePart()?; * = sm_a.addElsePartStmt(a1, 11)?;
    const a2 = sm_a.beginElsePart()?; * = sm_a.addElsePartStmt(a2, 30)?;

    * = sm_a.nextStep()?; * = sm_a.onStmtDone(1)?; // cond(1) false
    * = sm_a.nextStep()?; * = sm_a.onStmtDone(1)?; // elif(10) false
    const el = sm_a.nextStep()?;
    if (el.phase != IfBranchPhase.ElseBranch || el.stmt_id != 30) return err.ExpectedTrailingElse;
    * = sm_a.onStmtDone(3)?;
    const done_a = sm_a.nextStep()?;
    if (done_a.exit_code != 3) return err.TrailingElseExitMismatch;

    // 场景 B: if + elif（无 trailing else），所有条件失败 -> POSIX 退出码 0
    let sm_b = ShellIfStateMachine.init();
    * = sm_b.addCondStmt(1)?;
    * = sm_b.addThenStmt(2)?;
    const b0 = sm_b.beginElsePart()?; * = sm_b.addElsePartStmt(b0, 10)?;
    const b1 = sm_b.beginElsePart()?; * = sm_b.addElsePartStmt(b1, 11)?;

    * = sm_b.nextStep()?; * = sm_b.onStmtDone(1)?; // cond(1) false
    * = sm_b.nextStep()?; * = sm_b.onStmtDone(1)?; // elif(10) false
    const done_b = sm_b.nextStep()?;
    if (done_b.kind != IfStepKind.Completed || done_b.exit_code != 0) {
        return err.ElifNoElseFallthroughMustExitZero;
    }
}

test("shell_if_state: protocol guards (StmtStillPending, NotAwaitingStmt, AlreadyStarted, bounds)") {
    let sm = ShellIfStateMachine.init();
    * = sm.addCondStmt(1)?;

    // 未启动时调用 onStmtDone 必须拦截
    let not_awaiting = false;
    * = sm.onStmtDone(0) catch {
        not_awaiting = true;
        false;
    };
    if (!not_awaiting) return err.ExpectedNotAwaitingStmt;

    * = sm.nextStep()?;

    // 等待子语句完成期间再次 nextStep 必须拦截
    let pending_err = false;
    * = sm.nextStep() catch {
        pending_err = true;
    };
    if (!pending_err) return err.ExpectedStmtStillPending;

    // 已启动后不能再修改语句结构
    let started_err = false;
    * = sm.addThenStmt(99) catch {
        started_err = true;
        0;
    };
    if (!started_err) return err.ExpectedAlreadyStarted;

    // 容量上限校验
    let sm2 = ShellIfStateMachine.init();
    let i: usize = 0;
    while (i < SHELL_IF_MAX_ELSE_PARTS) {
        * = sm2.beginElsePart()?;
        i += 1;
    }
    let parts_overflow = false;
    * = sm2.beginElsePart() catch {
        parts_overflow = true;
        0;
    };
    if (!parts_overflow) return err.ExpectedTooManyElseParts;
}
