// Source: Absorbed from /data/dev/bun/src/shell/states/Async.cp | Synced: 2026-11-22T13:47:00Z
// Pure-CP zero-allocation POSIX / Bun Shell Background Async (`&`) Job State Machine & Tracker.
// Faithfully models Bun `src/shell/states/Async.cp`:
// - Immediate non-blocking parent completion (`start()` enqueues background execution and returns exit code 0)
// - Three-phase lifecycle (`Idle` -> `Exec` -> `Done`) over valid child kinds (`Pipeline`/`Cmd`/`If`/`CondExpr`)
// - Interpreter-level in-flight counter (`inFlightCount` / `canFinishShell`) so the shell waits for background jobs
// - Fixed-capacity `ShellAsyncJobTracker` supporting up to `SHELL_ASYNC_MAX_JOBS` concurrent `&` jobs

/// 单个 Shell 会话同时追踪的最大后台异步 (`&`) 作业数
pub const SHELL_ASYNC_MAX_JOBS: usize = 32;

/// Shell 后台异步状态机错误集
pub err ShellAsyncError {
    AlreadyStarted,
    NotRunning,
    AlreadyDone,
    InvalidStateTransition,
    JobNotFound,
    CapacityExceeded,
}

/// 后台异步节点的父状态类型（对齐 Bun `Async.ParentPtr`）
pub enum AsyncParentKind: u8 {
    Binary = 0,
    Stmt = 1,
}

/// 后台异步节点允许包装的子表达式类型（对齐 Bun `Async.ChildPtr`）
pub enum AsyncChildKind: u8 {
    Pipeline = 0,
    Cmd = 1,
    If = 2,
    CondExpr = 3,
}

/// 后台异步节点运行阶段（对齐 Bun `AsyncState`）
pub enum AsyncRunPhase: u8 {
    Idle = 0,
    Scheduled = 1,
    Executing = 2,
    Done = 3,
}

/// 单步调度动作种类
pub enum AsyncStepKind: u8 {
    /// 已入队事件循环，立即向父状态汇报退出码 0（非阻塞）
    EnqueueAndReturnParentZero = 0,
    /// 事件循环回调触发，启动实际子表达式执行
    StartChild = 1,
    /// 等待后台子表达式异步完成（挂起）
    Suspended = 2,
    /// 后台子表达式已结束并释放资源
    Finished = 3,
}

/// 单步调度动作载荷
pub struct AsyncStepAction {
    kind: AsyncStepKind = .Suspended,
    job_id: u32 = 0,
    child_kind: AsyncChildKind = .Cmd,
    child_expr_id: u32 = 0,
    parent_exit_code: u8 = 0,
    child_exit_code: u8 = 0,
}

/// 单个后台异步 (`cmd &`) 作业状态机（对齐 Bun `Async`）
pub struct ShellAsyncStateMachine {
    job_id: u32 = 0,
    parent: AsyncParentKind = .Stmt,
    child_kind: AsyncChildKind = .Cmd,
    child_expr_id: u32 = 0,
    phase: AsyncRunPhase = .Idle,
    parent_notified: bool = false,
    exit_code: u8 = 0,

    static fn init(
        job_id: u32,
        parent: AsyncParentKind,
        child_kind: AsyncChildKind,
        child_expr_id: u32,
    ): ShellAsyncStateMachine {
        return ShellAsyncStateMachine{
            job_id: job_id,
            parent: parent,
            child_kind: child_kind,
            child_expr_id: child_expr_id,
            phase: .Idle,
            parent_notified: false,
            exit_code: 0,
        };
    }

    /// 前台启动（对齐 Bun `Async.start()`）：将自身入队事件循环并立即向父状态返回退出码 0
    fn start(*_): AsyncStepAction! {
        if (_.phase != .Idle) {
            return err.AlreadyStarted;
        }
        _.phase = .Scheduled;
        _.parent_notified = true;
        return AsyncStepAction{
            kind: .EnqueueAndReturnParentZero,
            job_id: _.job_id,
            child_kind: _.child_kind,
            child_expr_id: _.child_expr_id,
            parent_exit_code: 0,
            child_exit_code: 0,
        };
    }

    /// 事件循环驱动推进（对齐 Bun `Async.next()`）
    fn onEventLoopTick(*_): AsyncStepAction! {
        return switch (_.phase) {
            .Idle => err.InvalidStateTransition,
            .Scheduled => {
                _.phase = .Executing;
                return AsyncStepAction{
                    kind: .StartChild,
                    job_id: _.job_id,
                    child_kind: _.child_kind,
                    child_expr_id: _.child_expr_id,
                };
            },
            .Executing => AsyncStepAction{
                kind: .Suspended,
                job_id: _.job_id,
                child_kind: _.child_kind,
                child_expr_id: _.child_expr_id,
            },
            .Done => AsyncStepAction{
                kind: .Finished,
                job_id: _.job_id,
                child_kind: _.child_kind,
                child_expr_id: _.child_expr_id,
                child_exit_code: _.exit_code,
            },
        };
    }

    /// 后台子表达式完成回调（对齐 Bun `Async.childDone()`）
    fn onChildDone(*_, code: u8): AsyncStepAction! {
        if (_.phase == .Done) return err.AlreadyDone;
        if (_.phase != .Executing) return err.NotRunning;
        _.exit_code = code;
        _.phase = .Done;
        return AsyncStepAction{
            kind: .Finished,
            job_id: _.job_id,
            child_kind: _.child_kind,
            child_expr_id: _.child_expr_id,
            child_exit_code: code,
        };
    }

    /// 当前作业是否仍在途（未进入 `Done`）
    fn isInFlight(): bool {
        return _.phase != .Done;
    }
}

/// 零堆分配 Shell 后台异步作业追踪器（管理解释器级 `async_commands_executing` 计数与生命周期）
pub struct ShellAsyncJobTracker {
    jobs: [SHELL_ASYNC_MAX_JOBS]ShellAsyncStateMachine = [ShellAsyncStateMachine{}; SHELL_ASYNC_MAX_JOBS],
    active: [SHELL_ASYNC_MAX_JOBS]bool = [false; SHELL_ASYNC_MAX_JOBS],
    next_job_id: u32 = 1,
    in_flight_count: usize = 0,
    completed_count: usize = 0,
    failed_count: usize = 0,

    static fn init(): ShellAsyncJobTracker {
        return ShellAsyncJobTracker{};
    }

    /// 派生一个新后台作业并立即执行 `start()`，返回 `(job_id)` 并使 `in_flight_count += 1`
    fn spawn(
        *_,
        parent: AsyncParentKind,
        child_kind: AsyncChildKind,
        child_expr_id: u32,
    ): AsyncStepAction! {
        let free_slot: usize? = null;
        for (i of 0..SHELL_ASYNC_MAX_JOBS) {
            if (!_.active[i]) {
                free_slot = i;
                break;
            }
        }
        const slot = free_slot ?? return err.CapacityExceeded;
        const jid = _.next_job_id;
        _.next_job_id += 1;

        let sm = ShellAsyncStateMachine.init(jid, parent, child_kind, child_expr_id);
        const action = sm.start()?;
        _.jobs[slot] = sm;
        _.active[slot] = true;
        _.in_flight_count += 1;
        return action;
    }

    fn findSlot(job_id: u32): usize? {
        for (i of 0..SHELL_ASYNC_MAX_JOBS) {
            if (_.active[i] && _.jobs[i].job_id == job_id) {
                return i;
            }
        }
        return null;
    }

    /// 驱动指定作业执行事件循环 tick
    fn tickJob(*_, job_id: u32): AsyncStepAction! {
        const slot = _.findSlot(job_id) ?? return err.JobNotFound;
        return _.jobs[slot].onEventLoopTick();
    }

    /// 标记指定后台作业完成，递减 `in_flight_count` 并回收槽位
    fn finishJob(*_, job_id: u32, exit_code: u8): AsyncStepAction! {
        const slot = _.findSlot(job_id) ?? return err.JobNotFound;
        const action = _.jobs[slot].onChildDone(exit_code)?;
        _.active[slot] = false;
        if (_.in_flight_count > 0) {
            _.in_flight_count -= 1;
        }
        _.completed_count += 1;
        if (exit_code != 0) {
            _.failed_count += 1;
        }
        return action;
    }

    /// 返回当前在途后台作业数量（对齐 Bun `interpreter.async_commands_executing`）
    fn inFlightCount(): usize {
        return _.in_flight_count;
    }

    /// 判定 Shell 解释器是否可以安全退出（所有后台 `&` 作业均已排空）
    fn canFinishShell(): bool {
        return _.in_flight_count == 0;
    }
}
