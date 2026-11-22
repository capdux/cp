// Source: Absorbed from /data/dev/bun/src/event_loop/ManagedTask.cp | Synced: 2026-11-22T02:34:44Z
// Pure-CP zero-allocation external-slice-backed one-shot managed task pool
// (no hardcoded slot limit; generation-guarded cancel->noop & run-and-auto-reclaim lifecycle).

/// 一次性托管任务错误集
pub err OneShotTaskError {
    PoolFull,
    InvalidHandle,
    StaleHandle,
}

/// 一次性托管任务槽位生命周期状态
pub enum OneShotTaskState: u8 {
    Empty = 0,
    Pending = 1,
    Cancelled = 2,
}

/// 一次性托管任务回调签名：`(ctx_id, payload) -> result_code`
pub type OneShotTaskFn = fn(u64, u64): i32;

fn noopOneShotCallback(_: u64, _: u64): i32 {
    return 0;
}

/// 8 字节代次保护的一次性任务句柄（防消费后悬垂访问与 ABA 重用冲突）
pub struct OneShotTaskHandle {
    index: u32 = 0,
    generation: u32 = 0,

    fn eql(other: OneShotTaskHandle): bool {
        return _.index == other.index && _.generation == other.generation;
    }
}

/// 单个外部切片承载的一次性任务槽位
pub struct OneShotTaskSlot {
    ctx_id: u64 = 0,
    payload: u64 = 0,
    callback: OneShotTaskFn = noopOneShotCallback,
    state: OneShotTaskState = .Empty,
    generation: u32 = 1,
}

/// 单次任务执行结果摘要
pub struct OneShotRunOutcome {
    executed: bool = false,
    was_cancelled: bool = false,
    result_code: i32 = 0,
    ctx_id: u64 = 0,
}

/// 外部切片驱动的一次性可取消托管任务池（对齐 Bun `event_loop/ManagedTask.cp`）
///
/// - 由调用方提供任意大小的 `[]OneShotTaskSlot` 后备切片，无硬编码槽位上限；
/// - `cancel(h)` 复刻 Bun `ManagedTask.cancel()`，将回调原子替换为 `noopOneShotCallback`；
/// - `run(h)` 复刻 Bun `ManagedTask.run()` + `DestroyDefaultGuard`，执行后无条件回收槽位并递增 `generation`；
/// - 支持 `runAllPending(max_count)` 按上限批量排空，防止事件循环单轮饿死。
pub struct OneShotTaskPool {
    slots: []OneShotTaskSlot,
    active_count: usize = 0,
    pending_count: usize = 0,
    completed_total: usize = 0,
    cancelled_total: usize = 0,
    next_hint: usize = 0,

    /// 绑定并初始化调用方提供的槽位切片
    static fn init(backing: []OneShotTaskSlot): OneShotTaskPool {
        let i: usize = 0;
        while (i < backing.len) {
            backing[i] = OneShotTaskSlot{
                ctx_id: 0,
                payload: 0,
                callback: noopOneShotCallback,
                state: .Empty,
                generation: 1,
            };
            i += 1;
        }
        return OneShotTaskPool{
            slots: backing,
            active_count: 0,
            pending_count: 0,
            completed_total: 0,
            cancelled_total: 0,
            next_hint: 0,
        };
    }

    fn capacity(): usize {
        return _.slots.len;
    }

    fn activeCount(): usize {
        return _.active_count;
    }

    fn pendingCount(): usize {
        return _.pending_count;
    }

    fn completedTotal(): usize {
        return _.completed_total;
    }

    fn cancelledTotal(): usize {
        return _.cancelled_total;
    }

    fn validateIndex(h: OneShotTaskHandle): usize! {
        const idx = @as(usize, h.index);
        if (idx >= _.slots.len) {
            return err.InvalidHandle;
        }
        if (_.slots[idx].state == .Empty || _.slots[idx].generation != h.generation) {
            return err.StaleHandle;
        }
        return idx;
    }

    fn isAlive(h: OneShotTaskHandle): bool {
        const idx = @as(usize, h.index);
        if (idx >= _.slots.len) {
            return false;
        }
        return _.slots[idx].state != .Empty && _.slots[idx].generation == h.generation;
    }

    fn stateOf(h: OneShotTaskHandle): OneShotTaskState! {
        const idx = _.validateIndex(h)?;
        return _.slots[idx].state;
    }

    /// 投递一个一次性托管任务并返回代次保护句柄
    fn schedule(*_, ctx_id: u64, payload: u64, cb: OneShotTaskFn): OneShotTaskHandle! {
        const cap = _.slots.len;
        if (cap == 0 || _.active_count >= cap) {
            return err.PoolFull;
        }
        let scanned: usize = 0;
        let idx = _.next_hint % cap;
        while (scanned < cap) {
            if (_.slots[idx].state == .Empty) {
                const gen = _.slots[idx].generation;
                _.slots[idx].ctx_id = ctx_id;
                _.slots[idx].payload = payload;
                _.slots[idx].callback = cb;
                _.slots[idx].state = .Pending;
                _.active_count += 1;
                _.pending_count += 1;
                _.next_hint = (idx + 1) % cap;
                return OneShotTaskHandle{
                    index: @as(u32, @intCast(idx)),
                    generation: gen,
                };
            }
            idx = (idx + 1) % cap;
            scanned += 1;
        }
        return err.PoolFull;
    }

    /// 取消任务（对齐 Bun `ManagedTask.cancel()`：将回调替换为 `noop`，幂等返回是否首次取消）
    fn cancel(*_, h: OneShotTaskHandle): bool! {
        const idx = _.validateIndex(h)?;
        if (_.slots[idx].state == .Cancelled) {
            return false;
        }
        _.slots[idx].callback = noopOneShotCallback;
        _.slots[idx].state = .Cancelled;
        _.pending_count -= 1;
        _.cancelled_total += 1;
        return true;
    }

    fn reclaimSlot(*_, idx: usize) {
        _.slots[idx].ctx_id = 0;
        _.slots[idx].payload = 0;
        _.slots[idx].callback = noopOneShotCallback;
        _.slots[idx].state = .Empty;
        _.slots[idx].generation = _.slots[idx].generation +% 1;
        if (_.slots[idx].generation == 0) {
            _.slots[idx].generation = 1;
        }
        _.active_count -= 1;
    }

    /// 消费并执行一次性任务（对齐 Bun `ManagedTask.run()`：先保存回调/上下文并原子回收槽位，再执行）
    fn run(*_, h: OneShotTaskHandle): OneShotRunOutcome! {
        const idx = _.validateIndex(h)?;
        const was_cancelled = _.slots[idx].state == .Cancelled;
        const cb = _.slots[idx].callback;
        const ctx_id = _.slots[idx].ctx_id;
        const payload = _.slots[idx].payload;

        if (!was_cancelled) {
            _.pending_count -= 1;
        }
        // 在调用用户回调前先原子回收槽位，支持回调重入 schedule 同一槽位且绝不泄漏
        _.reclaimSlot(idx);

        if (was_cancelled) {
            return OneShotRunOutcome{
                executed: false,
                was_cancelled: true,
                result_code: 0,
                ctx_id: ctx_id,
            };
        }
        const rc = cb(ctx_id, payload);
        _.completed_total += 1;
        return OneShotRunOutcome{
            executed: true,
            was_cancelled: false,
            result_code: rc,
            ctx_id: ctx_id,
        };
    }

    /// 批量消费至多 `max_count` 个活跃槽位（`max_count == 0` 表示排空全部活跃槽位）
    fn runAllPending(*_, max_count: usize): usize {
        let processed: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) {
            if (max_count > 0 && processed >= max_count) {
                break;
            }
            if (_.slots[i].state != .Empty) {
                const h = OneShotTaskHandle{
                    index: @as(u32, @intCast(i)),
                    generation: _.slots[i].generation,
                };
                * = _.run(h) catch OneShotRunOutcome{};
                processed += 1;
            }
            i += 1;
        }
        return processed;
    }
}
