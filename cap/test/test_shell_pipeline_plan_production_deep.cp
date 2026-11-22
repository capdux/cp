// cap/test/test_shell_pipeline_plan_production_deep.cp
// 深度生产级回归测试：cap:shell 管道拓扑规划器与执行状态机 (lib/shell/pipeline_plan.cp)

import { test } from "cap:test";
import {
    SHELL_PIPELINE_MAX_STAGES,
    SHELL_PIPELINE_NO_PIPE,
    PipeStageKind,
    PipeIoSource,
    PipeIoSink,
    PipeMachineState,
    PipeExitPolicy,
    ShellPipelinePlanner,
} from "cap:shell";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

test("pipeline_plan: 3-stage pipeline wires 2 pipes and ignores Assigns items") {
    let pp = ShellPipelinePlanner.init(false, PipeExitPolicy.LastStage);
    // Assigns 在管道中不产生可运行阶段
    if (pp.addItem(PipeStageKind.Assigns, "FOO=1")?) { return err.AssignsShouldBeIgnored; }
    if (!pp.addItem(PipeStageKind.Cmd, "echo")?) { return err.Stage0AddFailed; }
    if (!pp.addItem(PipeStageKind.If, "if-block")?) { return err.Stage1AddFailed; }
    if (!pp.addItem(PipeStageKind.Subshell, "subshell")?) { return err.Stage2AddFailed; }
    pp.finalizeWiring();

    if (pp.stage_count != 3 || pp.pipe_count != 2 || pp.ignored_assign_count != 1) {
        return err.TopologyCountMismatch;
    }

    const stages = pp.stagesSlice();
    const pipes = pp.pipesSlice();

    // Stage 0: stdin=inherit, stdout=pipe[0]
    if (stages[0].stdin_source != PipeIoSource.InheritStdin || stages[0].stdin_pipe_index != SHELL_PIPELINE_NO_PIPE) {
        return err.Stage0StdinMismatch;
    }
    if (stages[0].stdout_sink != PipeIoSink.PipeWrite || stages[0].stdout_pipe_index != 0) {
        return err.Stage0StdoutMismatch;
    }

    // Stage 1: stdin=pipe[0], stdout=pipe[1]
    if (stages[1].stdin_source != PipeIoSource.PipeRead || stages[1].stdin_pipe_index != 0) {
        return err.Stage1StdinMismatch;
    }
    if (stages[1].stdout_sink != PipeIoSink.PipeWrite || stages[1].stdout_pipe_index != 1) {
        return err.Stage1StdoutMismatch;
    }

    // Stage 2: stdin=pipe[1], stdout=inherit
    if (stages[2].stdin_source != PipeIoSource.PipeRead || stages[2].stdin_pipe_index != 1) {
        return err.Stage2StdinMismatch;
    }
    if (stages[2].stdout_sink != PipeIoSink.InheritStdout || stages[2].stdout_pipe_index != SHELL_PIPELINE_NO_PIPE) {
        return err.Stage2StdoutMismatch;
    }

    // Pipe channel endpoints
    if (pipes[0].writer_stage != 0 || pipes[0].reader_stage != 1 ||
        pipes[1].writer_stage != 1 || pipes[1].reader_stage != 2)
    {
        return err.PipeChannelEndpointsMismatch;
    }

    // Subshell deinit 语义对齐 Bun Pipeline.childDone
    if (!PipeStageKind.Cmd.needsSubshellDeinit() || !PipeStageKind.If.needsSubshellDeinit() ||
        !PipeStageKind.CondExpr.needsSubshellDeinit() || PipeStageKind.Subshell.needsSubshellDeinit() ||
        PipeStageKind.Assigns.needsSubshellDeinit())
    {
        return err.SubshellDeinitContractMismatch;
    }
}

test("pipeline_plan: out-of-order completion under POSIX LastStage policy") {
    let pp = ShellPipelinePlanner.init(false, PipeExitPolicy.LastStage);
    * = pp.addItem(PipeStageKind.Cmd, "false")?;
    * = pp.addItem(PipeStageKind.Cmd, "cat")?;
    * = pp.addItem(PipeStageKind.Cmd, "true")?;
    pp.finalizeWiring();

    // 顺序启动所有 3 个阶段
    if (pp.state != PipeMachineState.StartingCmds) { return err.InitialStateMismatch; }
    let started: usize = 0;
    while (sid := pp.nextStageToStart()) {
        if (sid != @as(u32, @intCast(started))) { return err.StartOrderMismatch; }
        started += 1;
    }
    if (started != 3 || pp.state != PipeMachineState.Pending) { return err.PendingTransitionFailed; }

    // 乱序退出：stage 2 (0) -> stage 0 (1) -> stage 1 (2)
    if (pp.onStageDone(2, 0)?) { return err.ShouldNotBeDoneAfter1; }
    if (pp.onStageDone(0, 1)?) { return err.ShouldNotBeDoneAfter2; }
    if (!(pp.onStageDone(1, 2)?)) { return err.ShouldBeDoneAfter3; }

    if (pp.state != PipeMachineState.Done || pp.final_exit_code != 0) {
        return err.LastStageExitCodeMismatch;
    }
}

test("pipeline_plan: Pipefail policy selects rightmost non-zero exit code") {
    let pp = ShellPipelinePlanner.init(false, PipeExitPolicy.Pipefail);
    * = pp.addItem(PipeStageKind.Cmd, "cmd0")?;
    * = pp.addItem(PipeStageKind.Cmd, "cmd1")?;
    * = pp.addItem(PipeStageKind.Cmd, "cmd2")?;
    pp.finalizeWiring();

    while (sid := pp.nextStageToStart()) { * = sid; }
    * = pp.onStageDone(0, 1)?;
    * = pp.onStageDone(1, 42)?;
    const done = pp.onStageDone(2, 0)?;

    if (!done || pp.final_exit_code != 42) {
        return err.PipefailRightmostNonZeroMismatch;
    }
}

test("pipeline_plan: negated pipeline flips zero and non-zero exit codes") {
    let pp = ShellPipelinePlanner.init(true, PipeExitPolicy.LastStage);
    * = pp.addItem(PipeStageKind.Cmd, "echo")?;
    * = pp.addItem(PipeStageKind.Cmd, "grep")?;
    pp.finalizeWiring();

    while (sid := pp.nextStageToStart()) { * = sid; }
    * = pp.onStageDone(0, 0)?;
    const done = pp.onStageDone(1, 1)?;
    if (!done || pp.final_exit_code != 0) {
        return err.NegatedNonZeroShouldBeZero;
    }

    let pp2 = ShellPipelinePlanner.init(true, PipeExitPolicy.LastStage);
    * = pp2.addItem(PipeStageKind.Cmd, "true")?;
    pp2.finalizeWiring();
    * = pp2.nextStageToStart();
    const done2 = pp2.onStageDone(0, 0)?;
    if (!done2 || pp2.final_exit_code != 1) {
        return err.NegatedZeroShouldBeOne;
    }
}

test("pipeline_plan: empty and single-stage pipelines allocate zero pipes") {
    let empty_pp = ShellPipelinePlanner.init(false, PipeExitPolicy.LastStage);
    * = empty_pp.addItem(PipeStageKind.Assigns, "A=1")?;
    empty_pp.finalizeWiring();
    if (empty_pp.stage_count != 0 || empty_pp.pipe_count != 0 ||
        empty_pp.state != PipeMachineState.Done || empty_pp.final_exit_code != 0)
    {
        return err.EmptyPipelineStateMismatch;
    }

    let single = ShellPipelinePlanner.init(false, PipeExitPolicy.LastStage);
    * = single.addItem(PipeStageKind.Cmd, "ls")?;
    single.finalizeWiring();
    if (single.stage_count != 1 || single.pipe_count != 0) {
        return err.SingleStageShouldHaveZeroPipes;
    }
    const s0 = single.stagesSlice()[0];
    if (s0.stdin_source != PipeIoSource.InheritStdin || s0.stdout_sink != PipeIoSink.InheritStdout) {
        return err.SingleStageShouldInheritStdio;
    }
}

test("pipeline_plan: abort cleanup computes exact unowned pipe FD ranges") {
    // 4 阶段 -> 3 条管道: pipe[0], pipe[1], pipe[2]
    let pp = ShellPipelinePlanner.init(false, PipeExitPolicy.LastStage);
    * = pp.addItem(PipeStageKind.Cmd, "c0")?;
    * = pp.addItem(PipeStageKind.Cmd, "c1")?;
    * = pp.addItem(PipeStageKind.Cmd, "c2")?;
    * = pp.addItem(PipeStageKind.Cmd, "c3")?;
    pp.finalizeWiring();

    // 在 stage 1 创建失败时：read 端需关闭 [1..3)，write 端需关闭 [2..3)
    const c1 = pp.computeAbortCleanup(1)?;
    if (c1.close_read_from != 1 || c1.close_read_to != 3 || c1.readCount() != 2) {
        return err.AbortReadRangeMismatch;
    }
    if (c1.close_write_from != 2 || c1.close_write_to != 3 || c1.writeCount() != 1) {
        return err.AbortWriteRangeMismatch;
    }

    // 在末阶段 stage 3 失败时，全部管道均已交由前序阶段处理，清理区间为空
    const c3 = pp.computeAbortCleanup(3)?;
    if (c3.readCount() != 0 || c3.writeCount() != 0) {
        return err.AbortLastStageShouldBeEmpty;
    }
}

test("pipeline_plan: topology formatting and write-error transition") {
    let pp = ShellPipelinePlanner.init(true, PipeExitPolicy.Pipefail);
    * = pp.addItem(PipeStageKind.Cmd, "cat")?;
    * = pp.addItem(PipeStageKind.Cmd, "grep")?;
    * = pp.addItem(PipeStageKind.Cmd, "wc")?;
    pp.finalizeWiring();

    let buf: [64]u8 = [0; 64];
    const n = pp.formatTopology(buf[0..])?;
    if (!bytesEq(buf[0..n], "! cat | grep | wc")) {
        return err.TopologyFormatMismatch;
    }

    pp.enterWriteError();
    if (pp.state != PipeMachineState.WaitingWriteErr || pp.final_exit_code != 1) {
        return err.WriteErrorStateMismatch;
    }
}

test("pipeline_plan: malformed transitions, capacity overflow and tiny buffer guards") {
    let pp = ShellPipelinePlanner.init(false, PipeExitPolicy.LastStage);
    * = pp.addItem(PipeStageKind.Cmd, "a")?;
    * = pp.addItem(PipeStageKind.Cmd, "b")?;
    pp.finalizeWiring();

    // 未启动阶段不能标记完成
    let not_started_err = false;
    pp.onStageDone(0, 0) catch {
        not_started_err = true;
    };
    if (!not_started_err) { return err.NotStartedNotRejected; }

    // 越界阶段索引拒绝
    let oob_err = false;
    pp.onStageDone(99, 0) catch {
        oob_err = true;
    };
    if (!oob_err) { return err.OutOfBoundsNotRejected; }

    // 重复完成同一阶段拒绝
    * = pp.nextStageToStart();
    * = pp.nextStageToStart();
    * = pp.onStageDone(0, 0)?;
    let dup_done_err = false;
    pp.onStageDone(0, 0) catch {
        dup_done_err = true;
    };
    if (!dup_done_err) { return err.DupCompletionNotRejected; }

    // 完成后再次调用 onStageDone 拒绝
    * = pp.onStageDone(1, 0)?;
    let after_done_err = false;
    pp.onStageDone(1, 0) catch {
        after_done_err = true;
    };
    if (!after_done_err) { return err.AfterDoneNotRejected; }

    // 超出最大阶段数拦截
    let full = ShellPipelinePlanner.init(false, PipeExitPolicy.LastStage);
    let i: usize = 0;
    while (i < SHELL_PIPELINE_MAX_STAGES) {
        * = full.addItem(PipeStageKind.Cmd, "x")?;
        i += 1;
    }
    let stage_overflow = false;
    full.addItem(PipeStageKind.Cmd, "overflow") catch {
        stage_overflow = true;
    };
    if (!stage_overflow) { return err.StageOverflowNotRejected; }

    // 拓扑格式化缓冲区过小拦截
    let tiny: [3]u8 = [0; 3];
    let buf_small = false;
    pp.formatTopology(tiny[0..]) catch {
        buf_small = true;
    };
    if (!buf_small) { return err.BufferTooSmallNotRejected; }
}
