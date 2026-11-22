// Production Deep Tests for cap:shell Trampoline & Continuation Scheduler (lib/shell/trampoline.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_TRAMPOLINE_REENTRY,
    MAX_PIPELINE_STACK_DEPTH,
    DEFAULT_MAX_TRAMPOLINE_STEPS,
    TrampolineError,
    YieldKind,
    PipelinePhase,
    ShellYield,
    PipelineStack,
    TrampolineResult,
    ShellTrampoline,
} from "cap:shell";

struct LinearCtx {
    trace_len: usize = 0,
    trace: [16]u8 = [0; 16],
    io_total_written: usize = 0,
}

fn linearStep(ctx: *LinearCtx, y: ShellYield): ShellYield {
    if (ctx.trace_len < ctx.trace.len) {
        ctx.trace[ctx.trace_len] = @intFromEnum(y.kind);
        ctx.trace_len += 1;
    }
    if (y.kind == .script) { return ShellYield.forNode(.stmt, 1); }
    if (y.kind == .stmt) { return ShellYield.forNode(.assigns, 2); }
    if (y.kind == .assigns) { return ShellYield.forNode(.expansion, 3); }
    if (y.kind == .expansion) { return ShellYield.forNode(.cmd, 4); }
    if (y.kind == .cmd) { return ShellYield.ioChunk(4, 128, 0); }
    if (y.kind == .io_writer_chunk) {
        ctx.io_total_written += y.io_written;
        return ShellYield.done();
    }
    return ShellYield.done();
}

fn dummyPhase(_ctx: *LinearCtx, _pid: u32): PipelinePhase {
    return .running;
}

test("shell trampoline: linear continuation chain without recursion", fn() {
    let tramp = ShellTrampoline.init();
    let ctx = LinearCtx{};

    const res = tramp.run(LinearCtx, &ctx, ShellYield.forNode(.script, 0), linearStep, dummyPhase) catch unreachable;
    expect_eq(res.final_yield.isDone(), true);
    expect_eq(res.steps_executed, @as(u64, 6));
    expect_eq(res.pipelines_drained, @as(u32, 0));
    expect_eq(res.max_pipeline_depth, @as(usize, 0));
    expect_eq(ctx.trace_len, @as(usize, 6));
    expect_eq(ctx.io_total_written, @as(usize, 128));
    expect_eq(tramp.reentry_depth, @as(u32, 0));
});

struct PipelineCtx {
    // 0: starting_cmds (cmd1), 1: starting_cmds (cmd2), 2: done
    p1_stage: u8 = 0,
    p1_cmds_started: u32 = 0,
    p1_finished: bool = false,
}

fn singlePipePhase(ctx: *PipelineCtx, pid: u32): PipelinePhase {
    if (pid == 1) {
        if (ctx.p1_stage < 2) { return .starting_cmds; }
        return .done;
    }
    return .done;
}

fn singlePipeStep(ctx: *PipelineCtx, y: ShellYield): ShellYield {
    if (y.kind == .pipeline && y.node_id == 1) {
        if (ctx.p1_stage == 0) {
            ctx.p1_stage = 1;
            return ShellYield.forNode(.cmd, 10);
        }
        if (ctx.p1_stage == 1) {
            ctx.p1_stage = 2;
            return ShellYield.forNode(.cmd, 11);
        }
        ctx.p1_finished = true;
        return ShellYield.done();
    }
    if (y.kind == .cmd) {
        ctx.p1_cmds_started += 1;
        // First cmd suspends waiting for pipe I/O; drainPipelines resumes pipeline 1 to launch cmd2
        if (y.node_id == 10) { return ShellYield.suspended(); }
        return ShellYield.done();
    }
    return ShellYield.done();
}

test("shell trampoline: pipeline starting_cmds suspension and drainPipelines resume", fn() {
    let tramp = ShellTrampoline.init();
    let ctx = PipelineCtx{};

    const res = tramp.run(PipelineCtx, &ctx, ShellYield.forNode(.pipeline, 1), singlePipeStep, singlePipePhase) catch unreachable;
    expect_eq(res.final_yield.isDone(), true);
    expect_eq(ctx.p1_cmds_started, @as(u32, 2));
    expect_eq(ctx.p1_finished, true);
    expect_eq(res.pipelines_drained, @as(u32, 1));
    expect_eq(res.max_pipeline_depth, @as(usize, 1));
});

struct NestedPipeCtx {
    p1_phase: PipelinePhase = .starting_cmds,
    p2_phase: PipelinePhase = .starting_cmds,
    p1_step_calls: u32 = 0,
    p2_step_calls: u32 = 0,
}

fn nestedPipePhase(ctx: *NestedPipeCtx, pid: u32): PipelinePhase {
    if (pid == 1) { return ctx.p1_phase; }
    if (pid == 2) { return ctx.p2_phase; }
    return .done;
}

fn nestedPipeStep(ctx: *NestedPipeCtx, y: ShellYield): ShellYield {
    if (y.kind == .pipeline && y.node_id == 1) {
        ctx.p1_step_calls += 1;
        if (ctx.p1_step_calls == 1) {
            // Outer pipeline launches inner pipeline 2
            ctx.p1_phase = .done;
            return ShellYield.forNode(.pipeline, 2);
        }
        return ShellYield.done();
    }
    if (y.kind == .pipeline && y.node_id == 2) {
        ctx.p2_step_calls += 1;
        if (ctx.p2_step_calls == 1) {
            ctx.p2_phase = .done;
            return ShellYield.forNode(.subshell, 20);
        }
        return ShellYield.done();
    }
    if (y.kind == .subshell) {
        // Return directly to inner pipeline 2 (now .done) -> orderedRemove from stack
        return ShellYield.forNode(.pipeline, 2);
    }
    return ShellYield.done();
}

test("shell trampoline: nested pipeline stack depth tracking and ordered removal", fn() {
    let tramp = ShellTrampoline.init();
    let ctx = NestedPipeCtx{};

    const res = tramp.run(NestedPipeCtx, &ctx, ShellYield.forNode(.pipeline, 1), nestedPipeStep, nestedPipePhase) catch unreachable;
    expect_eq(res.final_yield.isDone(), true);
    expect_eq(res.max_pipeline_depth, @as(usize, 2));
    expect_eq(res.pipelines_drained, @as(u32, 2));
    expect_eq(ctx.p1_step_calls, @as(u32, 2));
    expect_eq(ctx.p2_step_calls, @as(u32, 2));
});

struct SuspendCtx {
    phase: PipelinePhase = .running,
}

fn suspendPhase(ctx: *SuspendCtx, _pid: u32): PipelinePhase {
    return ctx.phase;
}

fn suspendStep(_ctx: *SuspendCtx, y: ShellYield): ShellYield {
    if (y.kind == .pipeline) { return ShellYield.forNode(.cmd, 99); }
    if (y.kind == .cmd) { return ShellYield.suspended(); }
    return ShellYield.done();
}

test("shell trampoline: running pipeline pops cleanly on suspended yield", fn() {
    let tramp = ShellTrampoline.init();
    let ctx = SuspendCtx{};

    const res = tramp.run(SuspendCtx, &ctx, ShellYield.forNode(.pipeline, 7), suspendStep, suspendPhase) catch unreachable;
    expect_eq(res.final_yield.kind == .suspended, true);
    expect_eq(res.final_yield.isTerminal(), true);
    expect_eq(res.final_yield.isDone(), false);
    expect_eq(res.pipelines_drained, @as(u32, 0));
    expect_eq(res.max_pipeline_depth, @as(usize, 1));
});

test("shell trampoline: PipelineStack duplicate, overflow, and orderedRemove guards", fn() {
    let st = PipelineStack.init();
    expect_eq(st.isEmpty(), true);
    expect_eq(st.top() == null, true);
    expect_eq(st.orderedRemove(0) == null, true);

    st.push(10) catch unreachable;
    st.push(20) catch unreachable;
    st.push(30) catch unreachable;
    expect_eq(st.len, @as(usize, 3));
    expect_eq(st.contains(20), true);
    expect_eq(st.top() ?? 0, @as(u32, 30));

    // Duplicate push must fail with DuplicatePipeline
    let dup_err = false;
    st.push(20) catch |e| {
        dup_err = (e == TrampolineError.DuplicatePipeline);
    };
    expect_eq(dup_err, true);

    // orderedRemove middle preserves order [10, 30]
    expect_eq(st.orderedRemove(1) ?? 0, @as(u32, 20));
    expect_eq(st.len, @as(usize, 2));
    expect_eq(st.items[0], @as(u32, 10));
    expect_eq(st.items[1], @as(u32, 30));

    // Fill to MAX_PIPELINE_STACK_DEPTH and verify overflow error
    st.clear();
    let i: u32 = 0;
    while (i < MAX_PIPELINE_STACK_DEPTH) {
        st.push(i + 1) catch unreachable;
        i += 1;
    }
    let ovf_err = false;
    st.push(9999) catch |e| {
        ovf_err = (e == TrampolineError.PipelineStackOverflow);
    };
    expect_eq(ovf_err, true);
});

struct LoopCtx {
    ticks: u64 = 0,
}

fn infiniteStep(ctx: *LoopCtx, y: ShellYield): ShellYield {
    ctx.ticks += 1;
    return ShellYield.forNode(.cmd, y.node_id + 1);
}

fn loopPhase(_ctx: *LoopCtx, _pid: u32): PipelinePhase {
    return .running;
}

test("shell trampoline: step budget guard and reentry depth guard with RAII recovery", fn() {
    let tramp = ShellTrampoline.initWithLimits(MAX_TRAMPOLINE_REENTRY, 50);
    let ctx = LoopCtx{};

    let step_err = false;
    * = tramp.run(LoopCtx, &ctx, ShellYield.forNode(.cmd, 0), infiniteStep, loopPhase) catch |e| {
        step_err = (e == TrampolineError.StepLimitExceeded);
        TrampolineResult{
            final_yield: ShellYield.failed(-1),
            steps_executed: 0,
            pipelines_drained: 0,
            max_pipeline_depth: 0,
        }
    };
    expect_eq(step_err, true);
    expect_eq(ctx.ticks, @as(u64, 50));
    // RAII ReentryGuard must have restored reentry_depth to 0 on error return
    expect_eq(tramp.reentry_depth, @as(u32, 0));

    // Exceeding max_reentry must be rejected immediately
    tramp.reentry_depth = MAX_TRAMPOLINE_REENTRY + 1;
    let reentry_err = false;
    * = tramp.run(LoopCtx, &ctx, ShellYield.done(), infiniteStep, loopPhase) catch |e| {
        reentry_err = (e == TrampolineError.ReentryDepthExceeded);
        TrampolineResult{
            final_yield: ShellYield.failed(-2),
            steps_executed: 0,
            pipelines_drained: 0,
            max_pipeline_depth: 0,
        }
    };
    expect_eq(reentry_err, true);
    expect_eq(DEFAULT_MAX_TRAMPOLINE_STEPS, @as(u64, 100000));
});
