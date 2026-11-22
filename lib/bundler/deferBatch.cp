// Source: Absorbed from /data/dev/bun/src/bundler/DeferredBatchTask.cp | Synced: 2026-11-22T12:19:00Z
// Pure-CP zero-allocation Bundler `DeferredBatchTask` coordinator for `onLoad` plugin
// `.defer()` promises: enforces single-defer-per-module invariant, non-reentrant
// Idle -> Scheduled -> Running -> Idle lifecycle, multi-wave cascade draining, and
// build-error fast-fail rejection (`drainDeferred(is_err)`).

/// 单批次最多挂起的 `.defer()` 插件任务数
pub const DEFERRED_BATCH_MAX_TASKS: usize = 64;

/// 延迟批次任务协调器错误集
pub err DeferredBatchError {
    AlreadyScheduled,
    NotScheduled,
    NotRunning,
    DuplicateSourceIndex,
    QueueFull,
    BufferTooSmall,
}

/// `DeferredBatchTask` 状态机阶段（防重入守卫）
pub enum DeferredBatchState: u8 {
    Idle = 0,
    Scheduled = 1,
    Running = 2,
}

/// 单条延迟任务的唤醒结果
pub enum DeferredTaskOutcome: u8 {
    Pending = 0,
    Resumed = 1,
    Rejected = 2,
}

/// 挂起的 `onLoad` `.defer()` 插件任务条目
pub struct DeferredPluginTask {
    source_index: u32 = 0,
    plugin_id: u16 = 0,
    wave: u16 = 0,
    outcome: DeferredTaskOutcome = .Pending,
}

/// 单次 `drainDeferred(is_err)` 批量消费摘要
pub struct DeferredDrainSummary {
    wave: u16 = 0,
    drained_count: usize = 0,
    resumed_count: usize = 0,
    rejected_count: usize = 0,
    had_build_error: bool = false,
}

/// 零堆分配 Bundler 延迟插件批次协调器（对齐 Bun `DeferredBatchTask`）
pub struct DeferredBatchCoordinator {
    state: DeferredBatchState = .Idle,
    current_wave: u16 = 0,
    total_drained: usize = 0,
    tasks: [DEFERRED_BATCH_MAX_TASKS]DeferredPluginTask = [DeferredPluginTask{}; DEFERRED_BATCH_MAX_TASKS],
    pending_len: usize = 0,
    seen_sources: [DEFERRED_BATCH_MAX_TASKS]u32 = [0; DEFERRED_BATCH_MAX_TASKS],
    seen_len: usize = 0,

    static fn init(): DeferredBatchCoordinator {
        return DeferredBatchCoordinator{};
    }

    fn isIdle(): bool {
        return _.state == .Idle;
    }

    fn isScheduled(): bool {
        return _.state == .Scheduled;
    }

    fn isRunning(): bool {
        return _.state == .Running;
    }

    fn pendingCount(): usize {
        return _.pending_len;
    }

    fn hasPending(): bool {
        return _.pending_len > 0;
    }

    /// 检查给定 `source_index` 是否已经调用过 `.defer()`
    fn hasDeferredSource(source_index: u32): bool {
        let i: usize = 0;
        while (i < _.seen_len) {
            if (_.seen_sources[i] == source_index) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 注册一条 `onLoad` `.defer()` 挂起任务（强制单模块仅允许 `.defer()` 一次）
    fn enqueueDeferred(*_, source_index: u32, plugin_id: u16): usize! {
        if (_.hasDeferredSource(source_index)) {
            return err.DuplicateSourceIndex;
        }
        if (_.pending_len >= DEFERRED_BATCH_MAX_TASKS || _.seen_len >= DEFERRED_BATCH_MAX_TASKS) {
            return err.QueueFull;
        }
        _.tasks[_.pending_len] = DeferredPluginTask{
            source_index: source_index,
            plugin_id: plugin_id,
            wave: _.current_wave,
            outcome: .Pending,
        };
        _.pending_len += 1;
        _.seen_sources[_.seen_len] = source_index;
        _.seen_len += 1;
        return _.pending_len;
    }

    /// 调度批次任务到 JS 线程（对齐 `DeferredBatchTask.schedule()`）：
    /// - 无挂起任务返回 `false`；
    /// - 已在 `Scheduled`/`Running` 返回 `AlreadyScheduled`；
    /// - 否则转移至 `Scheduled` 并返回 `true`。
    fn schedule(*_): bool! {
        if (_.state != .Idle) {
            return err.AlreadyScheduled;
        }
        if (_.pending_len == 0) {
            return false;
        }
        _.state = .Scheduled;
        return true;
    }

    /// JS 线程接管调度任务进入 `Running` 状态
    fn beginRun(*_): usize! {
        if (_.state != .Scheduled) {
            return err.NotScheduled;
        }
        _.state = .Running;
        return _.pending_len;
    }

    /// 批量消费当前所有挂起的 `.defer()` 任务并自动复位状态为 `Idle`（对齐 `DeferredBatchTask.runOnJSThread()` -> `drainDeferred(is_err)`）
    fn drainDeferred(*_, is_err: bool, out_tasks: []DeferredPluginTask): DeferredDrainSummary! {
        if (_.state != .Running) {
            return err.NotRunning;
        }
        if (out_tasks.len < _.pending_len) {
            return err.BufferTooSmall;
        }
        const wave = _.current_wave;
        const count = _.pending_len;
        const outcome: DeferredTaskOutcome = if (is_err) .Rejected else .Resumed;

        let i: usize = 0;
        while (i < count) {
            let item = _.tasks[i];
            item.outcome = outcome;
            out_tasks[i] = item;
            i += 1;
        }

        _.pending_len = 0;
        _.current_wave += 1;
        _.total_drained += count;
        _.state = .Idle;

        return DeferredDrainSummary{
            wave: wave,
            drained_count: count,
            resumed_count: if (is_err) 0 else count,
            rejected_count: if (is_err) count else 0,
            had_build_error: is_err,
        };
    }

    /// 便捷一步执行 `schedule -> beginRun -> drainDeferred`（适用于同步测试与单线程编排）
    fn runBatchNow(*_, is_err: bool, out_tasks: []DeferredPluginTask): DeferredDrainSummary! {
        const scheduled = _.schedule()?;
        if (!scheduled) {
            return DeferredDrainSummary{
                wave: _.current_wave,
                drained_count: 0,
                resumed_count: 0,
                rejected_count: 0,
                had_build_error: is_err,
            };
        }
        * = _.beginRun()?;
        return _.drainDeferred(is_err, out_tasks);
    }

    /// 清空全部波次与历史记录以复用于新一轮构建
    fn reset(*_) {
        _.state = .Idle;
        _.current_wave = 0;
        _.total_drained = 0;
        _.pending_len = 0;
        _.seen_len = 0;
    }
}
