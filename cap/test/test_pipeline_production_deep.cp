// Source: cp/cap/test/test_pipeline_production_deep.cp
// Deep Production Verification Suite for cap:pipeline & cap.pipeline
// Covers:
//   1. Classic onion model execution order (A.before -> B.before -> C.before -> C.after -> B.after -> A.after) & PipelineContext state
//   2. Short-circuit interception via ctx.halt() with reverse after-cleanup & ShortCircuit trace
//   3. Error propagation & onError fallback hook with leak-free reverse cleanup
//   4. Conditional stages (useConditional), dynamic setStageEnabled, DuplicateStage & StageNotFound
//   5. `import cap from "cap"; cap.pipeline` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PipelineError,
    StageOutcome,
    StageTrace,
    PipelineContext,
    Pipeline,
} from "cap:sys";

fn appendOrder(ctx: *PipelineContext, tag: string): !PipelineError {
    const cur = ctx.get("order") ?? "";
    if (cur.len == 0) {
        ctx.set("order", tag)?;
    } else {
        const next = std.fmt.allocPrint(ctx.alloc, "{s}->{s}", .{ cur, tag }) catch return PipelineError.OutOfMemory;
        defer ctx.alloc.free(next);
        ctx.set("order", next)?;
    }
}

fn stageABefore(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "A.before")?;
    ctx.set("trace_id", "req-9001")?;
}

fn stageAAfter(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "A.after")?;
    ctx.set("a_cleaned", "true")?;
}

fn stageBBefore(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "B.before")?;
    const tid = ctx.get("trace_id") ?? "";
    if (!std.mem.eql(u8, tid, "req-9001")) {
        return PipelineError.StageFailed;
    }
    ctx.setPayload("enriched-payload")?;
}

fn stageBAfter(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "B.after")?;
    ctx.set("b_cleaned", "true")?;
}

fn stageCBefore(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "C.before")?;
    ctx.set("c_ran", "true")?;
}

fn stageCAfter(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "C.after")?;
}

fn stageBHaltBefore(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "B.before")?;
    ctx.halt("auth_denied");
}

fn stageBErrorBefore(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "B.before")?;
    return PipelineError.StageFailed;
}

fn globalErrorHook(ctx: *PipelineContext, e: anyerror) {
    * = ctx.set("err_hook", @errorName(e)) catch {};
}

fn onlyWhenAdmin(ctx: *PipelineContext): bool {
    if (role := ctx.get("role")) {
        return std.mem.eql(u8, role, "admin");
    }
    return false;
}

fn adminBefore(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "Admin.before")?;
}

fn adminAfter(ctx: *PipelineContext): !PipelineError {
    appendOrder(ctx, "Admin.after")?;
}

test("cap:pipeline - classic onion order and PipelineContext state passing") {
    const alloc = std.heap.page_allocator;
    let pipe = Pipeline.init(alloc, "onion-core");
    defer pipe.deinit();

    pipe.useOnion("A", stageABefore, stageAAfter) catch unreachable;
    pipe.useOnion("B", stageBBefore, stageBAfter) catch unreachable;
    pipe.useOnion("C", stageCBefore, stageCAfter) catch unreachable;

    expect_eq(pipe.stageCount(), 3);

    let ctx = PipelineContext.init(alloc, "raw-payload");
    defer ctx.deinit();

    pipe.execute(&ctx) catch unreachable;

    expect_eq(ctx.isHalted(), false);
    expect_eq(ctx.payload(), "enriched-payload");
    expect_eq(ctx.get("c_ran") ?? "", "true");
    expect_eq(ctx.get("a_cleaned") ?? "", "true");
    expect_eq(ctx.get("b_cleaned") ?? "", "true");
    expect_eq(
        ctx.get("order") ?? "",
        "A.before->B.before->C.before->C.after->B.after->A.after",
    );

    const traces = ctx.traces();
    expect_eq(traces.len, 3);
    expect_eq(traces[0].name, "A");
    expect_eq(traces[0].outcome == StageOutcome.Continue, true);
    expect_eq(traces[1].name, "B");
    expect_eq(traces[1].outcome == StageOutcome.Continue, true);
    expect_eq(traces[2].name, "C");
    expect_eq(traces[2].outcome == StageOutcome.Continue, true);
}

test("cap:pipeline - short-circuit halt with reverse after-cleanup") {
    const alloc = std.heap.page_allocator;
    let pipe = Pipeline.init(alloc, "auth-pipe");
    defer pipe.deinit();

    pipe.useOnion("A", stageABefore, stageAAfter) catch unreachable;
    pipe.useOnion("B", stageBHaltBefore, stageBAfter) catch unreachable;
    pipe.useOnion("C", stageCBefore, stageCAfter) catch unreachable;

    let ctx = PipelineContext.init(alloc, "req");
    defer ctx.deinit();

    pipe.execute(&ctx) catch unreachable;

    expect_eq(ctx.isHalted(), true);
    expect_eq(ctx.haltReason(), "auth_denied");
    expect_eq(ctx.get("c_ran") ?? "none", "none");
    expect_eq(ctx.get("b_cleaned") ?? "", "true");
    expect_eq(ctx.get("a_cleaned") ?? "", "true");
    expect_eq(
        ctx.get("order") ?? "",
        "A.before->B.before->B.after->A.after",
    );

    const traces = ctx.traces();
    expect_eq(traces.len, 2);
    expect_eq(traces[0].outcome == StageOutcome.Continue, true);
    expect_eq(traces[1].name, "B");
    expect_eq(traces[1].outcome == StageOutcome.ShortCircuit, true);
    expect_eq(traces[1].error_msg, "auth_denied");
}

test("cap:pipeline - error propagation, onError hook, and reverse cleanup") {
    const alloc = std.heap.page_allocator;
    let pipe = Pipeline.init(alloc, "err-pipe");
    defer pipe.deinit();

    pipe.onError(globalErrorHook);
    pipe.useOnion("A", stageABefore, stageAAfter) catch unreachable;
    pipe.useOnion("B", stageBErrorBefore, stageBAfter) catch unreachable;
    pipe.useOnion("C", stageCBefore, stageCAfter) catch unreachable;

    let ctx = PipelineContext.init(alloc, "bad-req");
    defer ctx.deinit();

    let got_stage_failed = false;
    pipe.execute(&ctx) catch e => {
        if (e == PipelineError.StageFailed) got_stage_failed = true;
    };

    expect_eq(got_stage_failed, true);
    expect_eq(ctx.get("err_hook") ?? "", "StageFailed");
    expect_eq(ctx.get("a_cleaned") ?? "", "true");
    expect_eq(ctx.get("b_cleaned") ?? "none", "none");
    expect_eq(ctx.get("c_ran") ?? "none", "none");
    expect_eq(ctx.get("order") ?? "", "A.before->B.before->A.after");

    const traces = ctx.traces();
    expect_eq(traces.len, 2);
    expect_eq(traces[0].outcome == StageOutcome.Continue, true);
    expect_eq(traces[1].outcome == StageOutcome.Failed, true);
}

test("cap:pipeline - conditional stages, dynamic toggle, and error guards") {
    const alloc = std.heap.page_allocator;
    let pipe = Pipeline.init(alloc, "cond-pipe");
    defer pipe.deinit();

    // EmptyPipeline check
    let empty_ctx = PipelineContext.init(alloc, "x");
    defer empty_ctx.deinit();
    let got_empty = false;
    pipe.execute(&empty_ctx) catch e => {
        if (e == PipelineError.EmptyPipeline) got_empty = true;
    };
    expect_eq(got_empty, true);

    pipe.useOnion("A", stageABefore, stageAAfter) catch unreachable;
    pipe.useConditional("admin_only", onlyWhenAdmin, adminBefore, adminAfter) catch unreachable;
    pipe.use("C", stageCBefore) catch unreachable;

    // DuplicateStage check
    let got_dup = false;
    pipe.use("A", stageABefore) catch e => {
        if (e == PipelineError.DuplicateStage) got_dup = true;
    };
    expect_eq(got_dup, true);

    // StageNotFound check
    let got_not_found = false;
    pipe.setStageEnabled("missing", false) catch e => {
        if (e == PipelineError.StageNotFound) got_not_found = true;
    };
    expect_eq(got_not_found, true);

    // Run 1: role=guest -> admin_only skipped
    let ctx1 = PipelineContext.init(alloc, "guest-req");
    defer ctx1.deinit();
    ctx1.set("role", "guest") catch unreachable;
    pipe.execute(&ctx1) catch unreachable;
    expect_eq(ctx1.get("order") ?? "", "A.before->C.before->A.after");
    expect_eq(ctx1.traces()[1].outcome == StageOutcome.Skipped, true);

    // Run 2: role=admin -> admin_only executes
    let ctx2 = PipelineContext.init(alloc, "admin-req");
    defer ctx2.deinit();
    ctx2.set("role", "admin") catch unreachable;
    pipe.execute(&ctx2) catch unreachable;
    expect_eq(
        ctx2.get("order") ?? "",
        "A.before->Admin.before->C.before->Admin.after->A.after",
    );

    // Run 3: disable C dynamically
    pipe.setStageEnabled("C", false) catch unreachable;
    let ctx3 = PipelineContext.init(alloc, "admin-no-c");
    defer ctx3.deinit();
    ctx3.set("role", "admin") catch unreachable;
    pipe.execute(&ctx3) catch unreachable;
    expect_eq(
        ctx3.get("order") ?? "",
        "A.before->Admin.before->Admin.after->A.after",
    );
    expect_eq(ctx3.traces()[2].outcome == StageOutcome.Skipped, true);
}

test("cap:pipeline - dynamic cap.pipeline mounting") {
    const alloc = std.heap.page_allocator;
    let pipe = cap.sys.Pipeline.init(alloc, "dynamic-hub");
    defer pipe.deinit();

    pipe.useOnion("A", stageABefore, stageAAfter) catch unreachable;
    pipe.use("C", stageCBefore) catch unreachable;

    let ctx = cap.sys.PipelineContext.init(alloc, "hub-payload");
    defer ctx.deinit();

    pipe.execute(&ctx) catch unreachable;
    expect_eq(ctx.get("order") ?? "", "A.before->C.before->A.after");
    expect_eq(ctx.traces().len, 2);
}
