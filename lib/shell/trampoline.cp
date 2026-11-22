// Source: Absorbed from /data/dev/bun/src/shell/Yield.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP stack-safe Shell Trampoline & Continuation Scheduler supporting
// non-recursive state-machine stepping, nested pipeline stack draining
// (starting_cmds / running / done), I/O chunk continuations, reentry depth
// guard, and step-budget protection.

/// 默认最大重入深度（与 Bun `Yield.MAX_DEPTH = 2` 对齐）
pub const MAX_TRAMPOLINE_REENTRY: u32 = 2;

/// 静态有界嵌套管道栈最大深度（零堆分配）
pub const MAX_PIPELINE_STACK_DEPTH: usize = 64;

/// 默认单次 trampoline 驱动的最大步数上限（防状态机死循环）
pub const DEFAULT_MAX_TRAMPOLINE_STEPS: u64 = 100000;

/// Shell Trampoline 调度错误集
pub err TrampolineError {
    ReentryDepthExceeded,
    PipelineStackOverflow,
    DuplicatePipeline,
    StepLimitExceeded,
}

/// Shell 状态机 Continuation 节点类型（对齐 Bun `Yield` 变体）
pub enum YieldKind: u8 {
    script = 0,
    stmt = 1,
    pipeline = 2,
    cmd = 3,
    assigns = 4,
    expansion = 5,
    if_branch = 6,
    subshell = 7,
    cond_expr = 8,
    io_writer_chunk = 9,
    suspended = 10,
    failed = 11,
    done = 12,
}

/// 管道状态机阶段（对齐 Bun `Pipeline.state` 在 `Yield` 中的调度判定）
pub enum PipelinePhase: u8 {
    /// 管道仍在并发启动其子命令，挂起或子分支结束时需优先续跑
    starting_cmds = 0,
    /// 所有子命令均已启动，等待 I/O 或子进程完成
    running = 1,
    /// 管道已执行完毕，需从栈中移除并调用 `.next()` 清理并推进父节点
    done = 2,
}

/// 单步 Continuation 值对象（替代深层递归调用）
pub struct ShellYield {
    kind: YieldKind,
    node_id: u32 = 0,
    io_written: usize = 0,
    io_err_code: i32 = 0,

    static fn forNode(kind: YieldKind, node_id: u32): ShellYield {
        return ShellYield{
            kind: kind,
            node_id: node_id,
            io_written: 0,
            io_err_code: 0,
        };
    }

    static fn ioChunk(node_id: u32, written: usize, err_code: i32): ShellYield {
        return ShellYield{
            kind: .io_writer_chunk,
            node_id: node_id,
            io_written: written,
            io_err_code: err_code,
        };
    }

    static fn suspended(): ShellYield {
        return ShellYield{
            kind: .suspended,
            node_id: 0,
            io_written: 0,
            io_err_code: 0,
        };
    }

    static fn failed(err_code: i32): ShellYield {
        return ShellYield{
            kind: .failed,
            node_id: 0,
            io_written: 0,
            io_err_code: err_code,
        };
    }

    static fn done(): ShellYield {
        return ShellYield{
            kind: .done,
            node_id: 0,
            io_written: 0,
            io_err_code: 0,
        };
    }

    fn isDone(): bool {
        return _.kind == .done;
    }

    fn isTerminal(): bool {
        return _.kind == .done || _.kind == .suspended || _.kind == .failed;
    }

    fn isPipeline(): bool {
        return _.kind == .pipeline;
    }
}

/// 静态有界嵌套管道栈（保持外层到内层的嵌套顺序，零堆分配）
pub struct PipelineStack {
    items: [MAX_PIPELINE_STACK_DEPTH]u32 = [0; MAX_PIPELINE_STACK_DEPTH],
    len: usize = 0,

    static fn init(): PipelineStack {
        return PipelineStack{
            items: [0; MAX_PIPELINE_STACK_DEPTH],
            len: 0,
        };
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    fn indexOf(pipeline_id: u32): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (_.items[i] == pipeline_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn contains(pipeline_id: u32): bool {
        return _.indexOf(pipeline_id) != null;
    }

    fn top(): u32? {
        if (_.len == 0) {
            return null;
        }
        return _.items[_.len - 1];
    }

    fn push(*_, pipeline_id: u32): ! {
        if (_.contains(pipeline_id)) {
            return err.DuplicatePipeline;
        }
        if (_.len >= MAX_PIPELINE_STACK_DEPTH) {
            return err.PipelineStackOverflow;
        }
        _.items[_.len] = pipeline_id;
        _.len += 1;
    }

    fn pop(*_): u32? {
        if (_.len == 0) {
            return null;
        }
        _.len -= 1;
        return _.items[_.len];
    }

    fn orderedRemove(*_, idx: usize): u32? {
        if (idx >= _.len) {
            return null;
        }
        const removed = _.items[idx];
        let i = idx;
        while (i + 1 < _.len) {
            _.items[i] = _.items[i + 1];
            i += 1;
        }
        _.len -= 1;
        return removed;
    }

    fn clear(*_) {
        _.len = 0;
    }
}

/// Trampoline 单次执行汇总结果
pub struct TrampolineResult {
    final_yield: ShellYield,
    steps_executed: u64,
    pipelines_drained: u32,
    max_pipeline_depth: usize,
}

/// 内部 RAII 重入深度守卫（替代 defer，确保任何提前返回/错误均正确恢复深度）
struct ReentryGuard {
    depth_ptr: *u32!,

    ~init() {
        unsafe {
            if (_.depth_ptr.* > 0) {
                _.depth_ptr.* -= 1;
            }
        }
    }
}

/// 栈安全 Shell Trampoline 驱动引擎（对应 Bun `Yield.run` + `Yield.drainPipelines`）
pub struct ShellTrampoline {
    reentry_depth: u32 = 0,
    max_reentry: u32 = MAX_TRAMPOLINE_REENTRY,
    max_steps: u64 = DEFAULT_MAX_TRAMPOLINE_STEPS,

    static fn init(): ShellTrampoline {
        return ShellTrampoline{
            reentry_depth: 0,
            max_reentry: MAX_TRAMPOLINE_REENTRY,
            max_steps: DEFAULT_MAX_TRAMPOLINE_STEPS,
        };
    }

    static fn initWithLimits(max_reentry: u32, max_steps: u64): ShellTrampoline {
        return ShellTrampoline{
            reentry_depth: 0,
            max_reentry: max_reentry,
            max_steps: max_steps,
        };
    }

    /// 自顶向下排空嵌套管道栈（对齐 Bun `Yield.drainPipelines`）：
    /// - 若栈顶管道仍处于 `starting_cmds`，保留在栈中并立即续跑该管道；
    /// - 否则将其弹出；若该管道处于 `done`，则回调 `step_fn` 完成清理并推进父状态；
    /// - 若该管道处于 `running`，继续向下扫描更外层管道。
    static fn drainPipelines(
        comptime Ctx: type,
        stack: *PipelineStack,
        ctx: *Ctx,
        phase_fn: fn(ctx: *Ctx, pipeline_id: u32): PipelinePhase,
        step_fn: fn(ctx: *Ctx, y: ShellYield): ShellYield,
        drained_count: *u32,
    ): ShellYield? {
        while (stack.len > 0) {
            const pid = stack.items[stack.len - 1];
            const phase = phase_fn(ctx, pid);
            if (phase == .starting_cmds) {
                return step_fn(ctx, ShellYield.forNode(.pipeline, pid));
            }
            * = stack.pop();
            if (phase == .done) {
                drained_count.* += 1;
                return step_fn(ctx, ShellYield.forNode(.pipeline, pid));
            }
        }
        return null;
    }

    /// 驱动 Continuation 循环直至到达终结态且嵌套管道栈排空
    fn run(
        *_,
        comptime Ctx: type,
        ctx: *Ctx,
        initial: ShellYield,
        step_fn: fn(ctx: *Ctx, y: ShellYield): ShellYield,
        phase_fn: fn(ctx: *Ctx, pipeline_id: u32): PipelinePhase,
    ): TrampolineResult! {
        if (_.reentry_depth > _.max_reentry) {
            return err.ReentryDepthExceeded;
        }
        _.reentry_depth += 1;
        const _reentry_guard = ReentryGuard{ depth_ptr: &_.reentry_depth };
        * = _reentry_guard;

        let stack = PipelineStack.init();
        let cur = initial;
        let steps: u64 = 0;
        let drained: u32 = 0;
        let max_depth: usize = 0;

        while (true) {
            if (cur.isTerminal()) {
                if (next_y := ShellTrampoline.drainPipelines(Ctx, &stack, ctx, phase_fn, step_fn, &drained)) {
                    cur = next_y;
                    steps += 1;
                    if (steps > _.max_steps) {
                        return err.StepLimitExceeded;
                    }
                    continue;
                }
                return TrampolineResult{
                    final_yield: cur,
                    steps_executed: steps,
                    pipelines_drained: drained,
                    max_pipeline_depth: max_depth,
                };
            }

            if (steps >= _.max_steps) {
                return err.StepLimitExceeded;
            }
            steps += 1;

            if (cur.kind == .pipeline) {
                const phase = phase_fn(ctx, cur.node_id);
                if (phase == .done) {
                    if (idx := stack.indexOf(cur.node_id)) {
                        * = stack.orderedRemove(idx);
                        drained += 1;
                    }
                    cur = step_fn(ctx, cur);
                } else {
                    stack.push(cur.node_id)?;
                    if (stack.len > max_depth) {
                        max_depth = stack.len;
                    }
                    cur = step_fn(ctx, cur);
                }
            } else {
                cur = step_fn(ctx, cur);
            }
        }
    }
}
