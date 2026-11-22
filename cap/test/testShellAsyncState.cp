// Deep production regression tests for cap:shell Async (`&`) Job State Machine (lib/shell/asyncState.cp)

import { test, expect, expectError } from "cap:test";
import {
    SHELL_ASYNC_MAX_JOBS,
    ShellAsyncError,
    AsyncParentKind,
    AsyncChildKind,
    AsyncRunPhase,
    AsyncStepKind,
    ShellAsyncStateMachine,
    ShellAsyncJobTracker,
} from "cap:shell";

test("shellAsyncState: start enqueues background job and returns parent exit code 0 immediately", () => {
    let sm = ShellAsyncStateMachine.init(1, .Stmt, .Pipeline, 42);
    expect(sm.phase == .Idle).toBe(true);
    expect(sm.isInFlight()).toBe(true);

    const a0 = sm.start()?;
    expect(a0.kind == .EnqueueAndReturnParentZero).toBe(true);
    expect(a0.parent_exit_code).toBe(0 as u8);
    expect(a0.job_id).toBe(1 as u32);
    expect(sm.phase == .Scheduled).toBe(true);
    expect(sm.parent_notified).toBe(true);
});

test("shellAsyncState: full lifecycle Scheduled -> Executing -> Suspended -> Done", () => {
    let sm = ShellAsyncStateMachine.init(7, .Binary, .Cmd, 100);
    * = sm.start()?;

    const a1 = sm.onEventLoopTick()?;
    expect(a1.kind == .StartChild).toBe(true);
    expect(a1.child_kind == .Cmd).toBe(true);
    expect(a1.child_expr_id).toBe(100 as u32);
    expect(sm.phase == .Executing).toBe(true);

    // 仍在执行中再次 tick 返回 Suspended
    const a2 = sm.onEventLoopTick()?;
    expect(a2.kind == .Suspended).toBe(true);

    // 后台子表达式以非零码结束
    const a3 = sm.onChildDone(2)?;
    expect(a3.kind == .Finished).toBe(true);
    expect(a3.child_exit_code).toBe(2 as u8);
    expect(sm.phase == .Done).toBe(true);
    expect(sm.isInFlight()).toBe(false);

    // Done 后再次 tick 保持 Finished
    const a4 = sm.onEventLoopTick()?;
    expect(a4.kind == .Finished).toBe(true);
    expect(a4.child_exit_code).toBe(2 as u8);
});

test("shellAsyncState: job tracker coordinates multiple concurrent background jobs and shell drain", () => {
    let tracker = ShellAsyncJobTracker.init();
    expect(tracker.canFinishShell()).toBe(true);
    expect(tracker.inFlightCount()).toBe(0 as usize);

    const j1 = tracker.spawn(.Stmt, .Cmd, 10)?;
    const j2 = tracker.spawn(.Stmt, .Pipeline, 20)?;
    expect(j1.job_id).toBe(1 as u32);
    expect(j2.job_id).toBe(2 as u32);
    expect(tracker.inFlightCount()).toBe(2 as usize);
    expect(tracker.canFinishShell()).toBe(false);

    // 启动并完成 j2（乱序完成）
    const t2 = tracker.tickJob(j2.job_id)?;
    expect(t2.kind == .StartChild).toBe(true);
    const f2 = tracker.finishJob(j2.job_id, 0)?;
    expect(f2.kind == .Finished).toBe(true);
    expect(tracker.inFlightCount()).toBe(1 as usize);
    expect(tracker.canFinishShell()).toBe(false);

    // 启动并完成 j1（非零退出码计入 failed_count）
    * = tracker.tickJob(j1.job_id)?;
    const f1 = tracker.finishJob(j1.job_id, 127)?;
    expect(f1.child_exit_code).toBe(127 as u8);
    expect(tracker.inFlightCount()).toBe(0 as usize);
    expect(tracker.canFinishShell()).toBe(true);
    expect(tracker.completed_count).toBe(2 as usize);
    expect(tracker.failed_count).toBe(1 as usize);
});

test("shellAsyncState: tracker reuses finished slots up to and beyond capacity over time", () => {
    let tracker = ShellAsyncJobTracker.init();
    for (i of 0..(SHELL_ASYNC_MAX_JOBS + 4)) {
        const j = tracker.spawn(.Stmt, .If, i as u32)?;
        * = tracker.tickJob(j.job_id)?;
        * = tracker.finishJob(j.job_id, 0)?;
    }
    expect(tracker.inFlightCount()).toBe(0 as usize);
    expect(tracker.completed_count).toBe(SHELL_ASYNC_MAX_JOBS + 4);
    expect(tracker.canFinishShell()).toBe(true);
});

test("shellAsyncState: state machine invalid transition guards", () => {
    let sm = ShellAsyncStateMachine.init(1, .Stmt, .CondExpr, 5);
    // Idle 状态下不能直接 tick 或 childDone
    expectError(sm.onEventLoopTick(), ShellAsyncError.InvalidStateTransition);
    expectError(sm.onChildDone(0), ShellAsyncError.NotRunning);

    * = sm.start()?;
    // 不可重复 start，Scheduled 状态下尚未进入 Executing 不可 childDone
    expectError(sm.start(), ShellAsyncError.AlreadyStarted);
    expectError(sm.onChildDone(0), ShellAsyncError.NotRunning);

    * = sm.onEventLoopTick()?;
    * = sm.onChildDone(0)?;
    // Done 后不可再次 childDone
    expectError(sm.onChildDone(1), ShellAsyncError.AlreadyDone);
});

test("shellAsyncState: tracker capacity limit and unknown job id guards", () => {
    let tracker = ShellAsyncJobTracker.init();
    expectError(tracker.tickJob(999), ShellAsyncError.JobNotFound);
    expectError(tracker.finishJob(999, 0), ShellAsyncError.JobNotFound);

    for (i of 0..SHELL_ASYNC_MAX_JOBS) {
        * = tracker.spawn(.Stmt, .Cmd, i as u32)?;
    }
    expect(tracker.inFlightCount()).toBe(SHELL_ASYNC_MAX_JOBS);
    expectError(tracker.spawn(.Stmt, .Cmd, 999), ShellAsyncError.CapacityExceeded);
});
