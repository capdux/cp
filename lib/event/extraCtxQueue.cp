// Source: Absorbed from /data/dev/bun/src/event_loop/AnyTaskWithExtraContext.cp | Synced: 2026-11-22T10:19:16Z
// Pure-CP zero-allocation external-slice-backed ring queue for dual-context tasks
// (per-task context + dispatch-time extra context, O(1) FIFO/LIFO, selective cancel & bounded drain).

/// 双上下文任务队列错误集
pub err ExtraCtxQueueError {
    QueueFull,
    QueueEmpty,
}

/// 双上下文任务回调签名：`(task_ctx, extra_ctx) -> result_code`（`0` 表示成功，非零计入错误统计）
pub type ExtraCtxTaskFn = fn(u64, u64): i32;

fn noopExtraCtxTask(_: u64, _: u64): i32 {
    return 0;
}

/// 队列中单条双上下文任务记录
pub struct ExtraCtxTaskItem {
    task_ctx: u64 = 0,
    callback: ExtraCtxTaskFn = noopExtraCtxTask,
    cancelled: bool = false,
}

/// 批量排空执行统计报告
pub struct ExtraCtxDrainStats {
    ran: usize = 0,
    skipped_cancelled: usize = 0,
    error_count: usize = 0,
    last_result: i32 = 0,
}

/// 外部切片驱动的双上下文环形任务队列（对齐 Bun `event_loop/AnyTaskWithExtraContext.cp`）
///
/// - 由调用方提供任意长度的 `[]ExtraCtxTaskItem` 后备切片，无硬编码容量上限，零动态堆分配；
/// - 入队仅记录单任务上下文 `task_ctx` 与回调，派发时由事件循环实时注入第二个上下文 `extra_ctx`；
/// - 支持 `enqueue` (O(1) 队尾入队)、`pushFront` (O(1) 队首高优插队)、`cancelByContext` 与有界 `drain`。
pub struct ExtraCtxTaskQueue {
    buf: []ExtraCtxTaskItem,
    head: usize = 0,
    tail: usize = 0,
    count: usize = 0,

    /// 绑定调用方提供的后备切片
    static fn init(backing: []ExtraCtxTaskItem): ExtraCtxTaskQueue {
        return ExtraCtxTaskQueue{
            buf: backing,
            head: 0,
            tail: 0,
            count: 0,
        };
    }

    fn capacity(): usize {
        return _.buf.len;
    }

    fn len(): usize {
        return _.count;
    }

    fn remaining(): usize {
        return _.buf.len - _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn isFull(): bool {
        return _.count == _.buf.len;
    }

    /// 尾部 O(1) FIFO 入队
    fn enqueue(*_, task_ctx: u64, cb: ExtraCtxTaskFn): bool! {
        if (_.buf.len == 0 || _.count >= _.buf.len) {
            return err.QueueFull;
        }
        _.buf[_.tail] = ExtraCtxTaskItem{
            task_ctx: task_ctx,
            callback: cb,
            cancelled: false,
        };
        _.tail = (_.tail + 1) % _.buf.len;
        _.count += 1;
        return true;
    }

    /// 头部 O(1) 高优插队
    fn pushFront(*_, task_ctx: u64, cb: ExtraCtxTaskFn): bool! {
        if (_.buf.len == 0 || _.count >= _.buf.len) {
            return err.QueueFull;
        }
        if (_.head == 0) {
            _.head = _.buf.len - 1;
        } else {
            _.head -= 1;
        }
        _.buf[_.head] = ExtraCtxTaskItem{
            task_ctx: task_ctx,
            callback: cb,
            cancelled: false,
        };
        _.count += 1;
        return true;
    }

    /// 按 `task_ctx` 标记取消队列中所有匹配未取消任务，返回本次取消条数
    fn cancelByContext(*_, task_ctx: u64): usize {
        let cancelled: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            const idx = (_.head + i) % _.buf.len;
            if (!_.buf[idx].cancelled && _.buf[idx].task_ctx == task_ctx) {
                _.buf[idx].cancelled = true;
                _.buf[idx].callback = noopExtraCtxTask;
                cancelled += 1;
            }
            i += 1;
        }
        return cancelled;
    }

    /// 弹出并执行下一条未取消任务（自动跳过已取消项），若队空返回 `QueueEmpty`
    fn runNext(*_, extra_ctx: u64): i32! {
        while (_.count > 0) {
            const item = _.buf[_.head];
            _.head = (_.head + 1) % _.buf.len;
            _.count -= 1;
            if (!item.cancelled) {
                return item.callback(item.task_ctx, extra_ctx);
            }
        }
        return err.QueueEmpty;
    }

    /// 批量排空至多 `max_items` 条队列项（`max_items == 0` 表示排空全部），返回执行统计
    fn drain(*_, extra_ctx: u64, max_items: usize): ExtraCtxDrainStats {
        let stats = ExtraCtxDrainStats{};
        let popped: usize = 0;
        while (_.count > 0) {
            if (max_items > 0 && popped >= max_items) {
                break;
            }
            const item = _.buf[_.head];
            _.head = (_.head + 1) % _.buf.len;
            _.count -= 1;
            popped += 1;
            if (item.cancelled) {
                stats.skipped_cancelled += 1;
                continue;
            }
            const rc = item.callback(item.task_ctx, extra_ctx);
            stats.ran += 1;
            stats.last_result = rc;
            if (rc != 0) {
                stats.error_count += 1;
            }
        }
        return stats;
    }

    /// O(1) 清空队列
    fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }
}
