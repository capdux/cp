// cap/test/test_shell_assigns_state_production_deep.cp
// 深度生产级回归测试：cap:shell POSIX 环境变量赋值展开与作用域提交状态机 (lib/shell/assigns_state.cp)

import { test } from "cap:test";
import {
    SHELL_ASSIGNS_MAX_ITEMS,
    AssignScopeCtx,
    AssignStepKind,
    isValidAssignLabel,
    canSkipPipelineAssigns,
    joinExpandedWords,
    ShellAssignsStateMachine,
} from "cap:shell";

test("shell_assigns_state: label validation & multi-word space join") {
    if (!isValidAssignLabel("FOO")) return err.FooMustBeValid;
    if (!isValidAssignLabel("_BAR_2")) return err.UnderscoreMustBeValid;
    if (isValidAssignLabel("")) return err.EmptyMustBeInvalid;
    if (isValidAssignLabel("1ABC")) return err.LeadingDigitMustBeInvalid;
    if (isValidAssignLabel("FOO-BAR")) return err.HyphenMustBeInvalid;

    let out: [64]u8 = [0; 64];
    const empty: [0]([]const u8) = [];
    if ((joinExpandedWords(empty[0..], out[0..])?) != 0) return err.EmptyWordsMustBe0;

    const single: [1]([]const u8) = ["hello"];
    const n1 = joinExpandedWords(single[0..], out[0..])?;
    if (n1 != 5 || out[0] != 'h' || out[4] != 'o') return err.SingleWordMismatch;

    const multi: [3]([]const u8) = ["one", "two", "three"];
    const n3 = joinExpandedWords(multi[0..], out[0..])?;
    // "one two three" == 13 bytes
    if (n3 != 13 || out[3] != ' ' || out[7] != ' ' || out[12] != 'e') return err.MultiWordMismatch;

    let tiny: [4]u8 = [0; 4];
    let too_small = false;
    * = joinExpandedWords(multi[0..], tiny[0..]) catch {
        too_small = true;
        0;
    };
    if (!too_small) return err.TinyBufMustFail;
}

test("shell_assigns_state: left-to-right expansion, multi-word merge & last-write-wins") {
    let am = ShellAssignsStateMachine.init(AssignScopeCtx.Exported, false);
    * = am.addAssignLabel("A")?;
    * = am.addAssignLabel("B")?;
    * = am.addAssignLabel("A")?; // 同名覆盖

    const s0 = am.nextStep()?;
    if (s0.kind != AssignStepKind.ExpandValue || s0.assign_index != 0) return err.ExpectedExpand0;
    const w0: [1]([]const u8) = ["first"];
    * = am.onExpansionDone(w0[0..])?;

    const s1 = am.nextStep()?;
    if (s1.kind != AssignStepKind.ExpandValue || s1.assign_index != 1) return err.ExpectedExpand1;
    const w1: [2]([]const u8) = ["hello", "world"];
    * = am.onExpansionDone(w1[0..])?;

    const s2 = am.nextStep()?;
    if (s2.kind != AssignStepKind.ExpandValue || s2.assign_index != 2) return err.ExpectedExpand2;
    const w2: [1]([]const u8) = ["second"];
    * = am.onExpansionDone(w2[0..])?;

    const done = am.nextStep()?;
    if (done.kind != AssignStepKind.Completed || done.exit_code != 0 || !am.isDone()) {
        return err.ExpectedCompleted;
    }
    // 去重后仅保留 A、B 两条绑定
    if (am.committedCount() != 2) return err.Expected2Committed;

    if (ca := am.lookupCommitted("A")) {
        if (ca.value().len != 6 || ca.value()[0] != 's' || ca.ctx != AssignScopeCtx.Exported) {
            return err.LastWriteWinsMismatch;
        }
    } else {
        return err.MissingCommittedA;
    }
    if (cb := am.lookupCommitted("B")) {
        if (cb.value().len != 11 || cb.value()[5] != ' ') return err.MergedValueBMismatch;
    } else {
        return err.MissingCommittedB;
    }
}

test("shell_assigns_state: pipeline no-op skip for Shell/Exported vs active Cmd prefix") {
    if (!canSkipPipelineAssigns(true, AssignScopeCtx.Shell)) return err.PipelineShellShouldSkip;
    if (!canSkipPipelineAssigns(true, AssignScopeCtx.Exported)) return err.PipelineExportShouldSkip;
    if (canSkipPipelineAssigns(true, AssignScopeCtx.Cmd)) return err.PipelineCmdPrefixMustNotSkip;
    if (canSkipPipelineAssigns(false, AssignScopeCtx.Shell)) return err.NonPipelineMustNotSkip;

    // 管道中的独立 Shell 赋值直接跳过且零提交
    let am_pipe = ShellAssignsStateMachine.init(AssignScopeCtx.Shell, true);
    * = am_pipe.addAssignLabel("IGNORED")?;
    const r_pipe = am_pipe.nextStep()?;
    if (r_pipe.kind != AssignStepKind.Completed || !r_pipe.skipped_in_pipeline || am_pipe.committedCount() != 0) {
        return err.PipelineSkipFailed;
    }

    // 管道中的 Cmd 前缀赋值（如 `FOO=1 cmd | cat`）必须正常展开提交
    let am_cmd = ShellAssignsStateMachine.init(AssignScopeCtx.Cmd, true);
    * = am_cmd.addAssignLabel("FOO")?;
    const r_cmd = am_cmd.nextStep()?;
    if (r_cmd.kind != AssignStepKind.ExpandValue || r_cmd.skipped_in_pipeline) {
        return err.PipelineCmdPrefixMustRun;
    }
}

test("shell_assigns_state: expansion failure aborts remaining assigns with exit code 1") {
    let am = ShellAssignsStateMachine.init(AssignScopeCtx.Shell, false);
    * = am.addAssignLabel("FIRST")?;
    * = am.addAssignLabel("BROKEN")?;
    * = am.addAssignLabel("NEVER")?;

    * = am.nextStep()?;
    const ok_w: [1]([]const u8) = ["1"];
    * = am.onExpansionDone(ok_w[0..])?;

    * = am.nextStep()?;
    const ec = am.onExpansionFailed()?;
    if (ec != 1 || !am.isFailed() || !am.isDone() || am.exitCode() != 1) {
        return err.ExpectedFailedExit1;
    }

    // 后续 nextStep 保持 Failed，且 NEVER 未被提交
    const after = am.nextStep()?;
    if (after.kind != AssignStepKind.Failed || after.exit_code != 1) return err.AfterFailMustStayFailed;
    if (am.committedCount() != 1 || am.lookupCommitted("NEVER") != null) {
        return err.NeverMustNotCommit;
    }
}

test("shell_assigns_state: empty expansion words commit empty string value") {
    let am = ShellAssignsStateMachine.init(AssignScopeCtx.Shell, false);
    * = am.addAssignLabel("EMPTY_VAR")?;
    * = am.nextStep()?;
    const empty_w: [0]([]const u8) = [];
    const len = am.onExpansionDone(empty_w[0..])?;
    if (len != 0) return err.ExpectedEmptyValueLen0;
    * = am.nextStep()?;
    if (cv := am.lookupCommitted("EMPTY_VAR")) {
        if (cv.value().len != 0) return err.ExpectedEmptySlice;
    } else {
        return err.MissingEmptyVar;
    }
}

test("shell_assigns_state: protocol guards & 16-item capacity limit") {
    let am = ShellAssignsStateMachine.init(AssignScopeCtx.Shell, false);

    // 非法变量名拦截
    let bad_lbl = false;
    * = am.addAssignLabel("9BAD") catch {
        bad_lbl = true;
        0;
    };
    if (!bad_lbl) return err.BadLabelMustFail;

    // 未等待展开时不能调用 onExpansionDone / onExpansionFailed
    let not_await = false;
    const dummy: [0]([]const u8) = [];
    * = am.onExpansionDone(dummy[0..]) catch {
        not_await = true;
        0;
    };
    if (!not_await) return err.NotAwaitingMustFail;

    // 填满 16 项，第 17 项拦截
    let lbl: [4]u8 = ['V', '_', '0', '0'];
    let i: usize = 0;
    while (i < SHELL_ASSIGNS_MAX_ITEMS) {
        lbl[2] = ('0' as u8) + ((i / 10) as u8);
        lbl[3] = ('0' as u8) + ((i % 10) as u8);
        * = am.addAssignLabel(lbl[0..])?;
        i += 1;
    }
    let overflow = false;
    * = am.addAssignLabel("EXTRA") catch {
        overflow = true;
        0;
    };
    if (!overflow) return err.TooManyAssignsMustFail;

    // 启动后不允许再添加、等待展开时不允许重复 nextStep
    * = am.nextStep()?;
    let pending = false;
    * = am.nextStep() catch { pending = true; };
    if (!pending) return err.PendingExpansionMustFail;
}
