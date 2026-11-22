// Source: cp/lib/pipeline/pipeline.cp (Composable Onion Middleware & Multi-Stage Pipeline)
// Pure CP zero-C-dependency composable onion middleware and multi-stage data processing pipeline.
// Supports before/after/onError hooks, short-circuit halting with reverse after-cleanup,
// PipelineContext key-value bag, conditional stages, dynamic enable/disable, and per-stage trace timing.

const std = @import("std");

pub err PipelineError {
    StageFailed,
    EmptyPipeline,
    DuplicateStage,
    StageNotFound,
    OutOfMemory,
}

pub enum StageOutcome {
    Continue,
    ShortCircuit,
    Skipped,
    Failed,

    fn toString(): string {
        switch (_) {
            .Continue => return "Continue",
            .ShortCircuit => return "ShortCircuit",
            .Skipped => return "Skipped",
            .Failed => return "Failed",
        }
    }
}

pub struct StageTrace {
    name: string;
    outcome: StageOutcome;
    duration_ns: u64;
    error_msg: string;
}

pub struct PipelineContext {
    alloc: std.mem.Allocator;
    kv: std.StringHashMapUnmanaged([]u8) = .empty;
    payload_str: string;
    payload_owned: ([]u8)? = null;
    halted: bool = false;
    halt_reason_str: string = "";
    halt_reason_owned: ([]u8)? = null;
    trace_list: std.ArrayListUnmanaged(StageTrace) = .empty;

    static fn init(alloc: std.mem.Allocator, initial_payload: string): PipelineContext {
        return PipelineContext{
            alloc: alloc,
            kv: .empty,
            payload_str: initial_payload,
            payload_owned: null,
            halted: false,
            halt_reason_str: "",
            halt_reason_owned: null,
            trace_list: .empty,
        };
    }

    fn deinit(*_) {
        let it = _.kv.iterator();
        while (entry := it.next()) {
            _.alloc.free(entry.key_ptr.*);
            _.alloc.free(entry.value_ptr.*);
        }
        _.kv.deinit(_.alloc);

        if (owned_p := _.payload_owned) {
            _.alloc.free(owned_p);
            _.payload_owned = null;
        }
        if (owned_r := _.halt_reason_owned) {
            _.alloc.free(owned_r);
            _.halt_reason_owned = null;
        }
        _.trace_list.deinit(_.alloc);
    }

    fn set(*_, key: string, val: string): !PipelineError {
        const val_copy = _.alloc.dupe(u8, val) catch return PipelineError.OutOfMemory;
        if (existing := _.kv.getPtr(key)) {
            _.alloc.free(existing.*);
            existing.* = val_copy;
            return;
        }
        const key_copy = _.alloc.dupe(u8, key) catch {
            _.alloc.free(val_copy);
            return PipelineError.OutOfMemory;
        };
        _.kv.put(_.alloc, key_copy, val_copy) catch {
            _.alloc.free(key_copy);
            _.alloc.free(val_copy);
            return PipelineError.OutOfMemory;
        };
    }

    fn get(key: string): string? {
        if (val := _.kv.get(key)) {
            return val;
        }
        return null;
    }

    fn setPayload(*_, s: string): !PipelineError {
        const copy = _.alloc.dupe(u8, s) catch return PipelineError.OutOfMemory;
        if (old := _.payload_owned) {
            _.alloc.free(old);
        }
        _.payload_owned = copy;
        _.payload_str = copy;
    }

    fn payload(): string {
        return _.payload_str;
    }

    fn halt(*_, reason: string) {
        _.halted = true;
        if (old := _.halt_reason_owned) {
            _.alloc.free(old);
            _.halt_reason_owned = null;
        }
        const copy = _.alloc.dupe(u8, reason) catch {
            _.halt_reason_str = reason;
            return;
        };
        _.halt_reason_owned = copy;
        _.halt_reason_str = copy;
    }

    fn isHalted(): bool {
        return _.halted;
    }

    fn haltReason(): string {
        return _.halt_reason_str;
    }

    fn traces(): []const StageTrace {
        return _.trace_list.items;
    }

    fn recordTrace(*_, trace: StageTrace): !PipelineError {
        _.trace_list.append(_.alloc, trace) catch return PipelineError.OutOfMemory;
    }

    fn clearTraces(*_) {
        _.trace_list.clearRetainingCapacity();
    }
}

pub type StageFn = fn (*PipelineContext) !PipelineError;
pub type ErrorHookFn = fn (*PipelineContext, anyerror) void;
pub type PredicateFn = fn (*PipelineContext) bool;

pub struct StageSpec {
    name: string;
    before: StageFn? = null;
    after: StageFn? = null;
    on_error: ErrorHookFn? = null;
    when: PredicateFn? = null;
    enabled: bool = true;
}

fn nowNs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    zig {
        const builtin = @import("builtin");
        if (builtin.os.tag == .linux) {
            _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        }
    }
    return ((ts.sec as u64) * 1000000000) + (ts.nsec as u64);
}

pub struct Pipeline {
    alloc: std.mem.Allocator;
    name: string;
    stages: std.ArrayListUnmanaged(StageSpec) = .empty;
    global_on_error: ErrorHookFn? = null;

    static fn init(alloc: std.mem.Allocator, name: string): Pipeline {
        return Pipeline{
            alloc: alloc,
            name: name,
            stages: .empty,
            global_on_error: null,
        };
    }

    fn deinit(*_) {
        _.stages.deinit(_.alloc);
    }

    fn hasStage(name: string): bool {
        for (st of _.stages.items) {
            if (std.mem.eql(u8, st.name, name)) {
                return true;
            }
        }
        return false;
    }

    fn useStage(*_, spec: StageSpec): !PipelineError {
        if (_.hasStage(spec.name)) {
            return PipelineError.DuplicateStage;
        }
        _.stages.append(_.alloc, spec) catch return PipelineError.OutOfMemory;
    }

    fn use(*_, name: string, before_fn: StageFn): !PipelineError {
        return _.useStage(StageSpec{
            name: name,
            before: before_fn,
            after: null,
            on_error: null,
            when: null,
            enabled: true,
        });
    }

    fn useOnion(*_, name: string, before_fn: StageFn?, after_fn: StageFn?): !PipelineError {
        return _.useStage(StageSpec{
            name: name,
            before: before_fn,
            after: after_fn,
            on_error: null,
            when: null,
            enabled: true,
        });
    }

    fn useConditional(
        *_,
        name: string,
        when_fn: PredicateFn,
        before_fn: StageFn?,
        after_fn: StageFn?,
    ): !PipelineError {
        return _.useStage(StageSpec{
            name: name,
            before: before_fn,
            after: after_fn,
            on_error: null,
            when: when_fn,
            enabled: true,
        });
    }

    fn onError(*_, hook_fn: ErrorHookFn) {
        _.global_on_error = hook_fn;
    }

    fn setStageEnabled(*_, name: string, enabled: bool): !PipelineError {
        let i: usize = 0;
        while (i < _.stages.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.stages.items[i].name, name)) {
                _.stages.items[i].enabled = enabled;
                return;
            }
        }
        return PipelineError.StageNotFound;
    }

    fn stageCount(): usize {
        return _.stages.items.len;
    }

    fn execute(ctx: *PipelineContext): !PipelineError {
        if (_.stages.items.len == 0) {
            return PipelineError.EmptyPipeline;
        }

        let entered: std.ArrayListUnmanaged(usize) = .empty;
        defer entered.deinit(_.alloc);

        let i: usize = 0;
        while (i < _.stages.items.len) : (i += 1) {
            const stage = _.stages.items[i];
            let should_run = stage.enabled;
            if (should_run) {
                if (pred := stage.when) {
                    should_run = pred(ctx);
                }
            }

            if (!should_run) {
                ctx.recordTrace(StageTrace{
                    name: stage.name,
                    outcome: StageOutcome.Skipped,
                    duration_ns: 0,
                    error_msg: "",
                })?;
                continue;
            }

            const t0 = nowNs();
            if (before_fn := stage.before) {
                before_fn(ctx) catch e => {
                    const t1 = nowNs();
                    const dt = if (t1 >= t0) (t1 - t0) else 0;
                    * = ctx.recordTrace(StageTrace{
                        name: stage.name,
                        outcome: StageOutcome.Failed,
                        duration_ns: dt,
                        error_msg: @errorName(e),
                    }) catch {};

                    if (stage_hook := stage.on_error) {
                        stage_hook(ctx, e);
                    }
                    if (global_hook := _.global_on_error) {
                        global_hook(ctx, e);
                    }

                    // Reverse-order after() cleanup for all previously entered stages
                    let k: usize = entered.items.len;
                    while (k > 0) {
                        k -= 1;
                        const prev_stage = _.stages.items[entered.items[k]];
                        if (after_fn := prev_stage.after) {
                            * = after_fn(ctx) catch {};
                        }
                    }
                    return e;
                };
            }
            const t1 = nowNs();
            const dt = if (t1 >= t0) (t1 - t0) else 0;

            entered.append(_.alloc, i) catch return PipelineError.OutOfMemory;

            if (ctx.isHalted()) {
                ctx.recordTrace(StageTrace{
                    name: stage.name,
                    outcome: StageOutcome.ShortCircuit,
                    duration_ns: dt,
                    error_msg: ctx.haltReason(),
                })?;
                break;
            } else {
                ctx.recordTrace(StageTrace{
                    name: stage.name,
                    outcome: StageOutcome.Continue,
                    duration_ns: dt,
                    error_msg: "",
                })?;
            }
        }

        // Reverse-order onion after() cleanup for all entered stages
        let first_after_err: anyerror? = null;
        let k: usize = entered.items.len;
        while (k > 0) {
            k -= 1;
            const st = _.stages.items[entered.items[k]];
            if (after_fn := st.after) {
                after_fn(ctx) catch e => {
                    if (stage_hook := st.on_error) {
                        stage_hook(ctx, e);
                    }
                    if (global_hook := _.global_on_error) {
                        global_hook(ctx, e);
                    }
                    if (first_after_err == null) {
                        first_after_err = e;
                    }
                };
            }
        }

        if (err_val := first_after_err) {
            return err_val as PipelineError;
        }
    }
}
