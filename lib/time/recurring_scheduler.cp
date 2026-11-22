// Origin: Pure-CP Stdlib (Zero-Heap Deterministic One-Shot & Recurring Task Scheduler) | Created: 2026-11-22
// Pure CP zero-heap-allocation deterministic scheduler for server timers & AI Agent periodic jobs:
//   - OneShot, FixedInterval (drift-free aligned), and FixedDelay scheduling modes
//   - Deterministic SplitMix64 jitter (`jitter_window_ms`) preventing thundering-herd spikes
//   - Exponential backoff retry (`max_retries`, `retry_backoff_ms`) with terminal `Failed` / `Completed` states
//   - Exact `nextDueMs()` event-loop wake-up calculation, ordered `pollDue`, `pause`/`resume`/`cancel` & slot reuse

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) return 0xFFFFFFFF;
    return a + b;
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) return 0xFFFFFFFFFFFFFFFF;
    return a + b;
}

fn satAddI64(a: i64, b: i64): i64 {
    const i64_max: i64 = 9223372036854775807;
    if (b > 0 && a > i64_max - b) return i64_max;
    return a + b;
}

fn satMulPosI64(a: i64, mult: i64): i64 {
    const i64_max: i64 = 9223372036854775807;
    if (a <= 0 || mult <= 0) return 0;
    if (a > i64_max / mult) return i64_max;
    return a * mult;
}

fn deterministicJitterMs(task_id: u64, seq: u64, jitter_window_ms: i64): i64 {
    if (jitter_window_ms <= 0) return 0;
    let z: u64 = task_id ^ (seq *% 0x9E3779B97F4A7C15);
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    z = z ^ (z >> 31);
    const span: u64 = (jitter_window_ms as u64) + 1;
    return (z % span) as i64;
}

/// Scheduling cadence mode for a task in `RecurringScheduler`.
pub enum ScheduleKind {
    OneShot,
    FixedInterval,
    FixedDelay,
}

/// Lifecycle state of a scheduled task.
pub enum ScheduleState {
    Active,
    Paused,
    Completed,
    Failed,
    Cancelled,
}

fn isTerminalScheduleState(st: ScheduleState): bool {
    return st == ScheduleState.Completed ||
        st == ScheduleState.Failed ||
        st == ScheduleState.Cancelled;
}

/// Caller-owned fixed-layout slot representing one scheduled task.
pub struct ScheduledTaskSlot {
    task_id: u64 = 0;
    name: string = "";
    kind: ScheduleKind = ScheduleKind.OneShot;
    state: ScheduleState = ScheduleState.Active;
    next_run_ms: i64 = 0;
    interval_ms: i64 = 0;
    max_runs: u32 = 0;
    run_count: u32 = 0;
    max_retries: u32 = 0;
    retry_count: u32 = 0;
    retry_backoff_ms: i64 = 0;
    jitter_window_ms: i64 = 0;
    in_flight: bool = false;
    last_run_ms: i64 = 0;
    occupied: bool = false;
}

/// Aggregate operational statistics returned by `RecurringScheduler.stats`.
pub struct SchedulerStats {
    capacity: usize = 0;
    total: usize = 0;
    active: usize = 0;
    in_flight: usize = 0;
    paused: usize = 0;
    completed: usize = 0;
    failed: usize = 0;
    cancelled: usize = 0;
    total_dispatches: u64 = 0;
    total_successes: u64 = 0;
    total_failures: u64 = 0;
    total_retries: u64 = 0;
}

/// Zero-heap-allocation deterministic one-shot and recurring task scheduler.
pub struct RecurringScheduler {
    slots: []ScheduledTaskSlot;
    total_dispatches: u64 = 0;
    total_successes: u64 = 0;
    total_failures: u64 = 0;
    total_retries: u64 = 0;

    /// Initializes a scheduler backed by caller-owned `slots`. Returns `null` if `slots.len == 0`.
    static fn init(slots: *[]ScheduledTaskSlot): RecurringScheduler? {
        if (slots.len == 0) return null;
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = ScheduledTaskSlot{};
        }
        return RecurringScheduler{
            slots: slots,
            total_dispatches: 0,
            total_successes: 0,
            total_failures: 0,
            total_retries: 0,
        };
    }

    fn findSlot(task_id: u64): usize? {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].task_id == task_id) {
                return i;
            }
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) return i;
        }
        return null;
    }

    /// Schedules a one-shot task to execute at `run_at_ms`.
    /// Returns `false` if `task_id == 0`, duplicate `task_id`, or scheduler is full.
    fn scheduleOneShot(*_, task_id: u64, name: string, run_at_ms: i64): bool {
        if (task_id == 0) return false;
        if (_.findSlot(task_id) != null) return false;
        const idx = _.findFreeSlot() ?? return false;

        _.slots[idx] = ScheduledTaskSlot{
            task_id: task_id,
            name: name,
            kind: ScheduleKind.OneShot,
            state: ScheduleState.Active,
            next_run_ms: run_at_ms,
            interval_ms: 0,
            max_runs: 1,
            occupied: true,
        };
        return true;
    }

    /// Schedules a fixed-interval recurring task starting at `first_run_ms` (plus deterministic jitter)
    /// repeating every `interval_ms`. `max_runs == 0` runs indefinitely.
    fn scheduleInterval(
        *_,
        task_id: u64,
        name: string,
        first_run_ms: i64,
        interval_ms: i64,
        max_runs: u32,
        jitter_window_ms: i64,
    ): bool {
        if (task_id == 0 || interval_ms <= 0 || jitter_window_ms < 0) return false;
        if (_.findSlot(task_id) != null) return false;
        const idx = _.findFreeSlot() ?? return false;

        const j = deterministicJitterMs(task_id, 0, jitter_window_ms);
        _.slots[idx] = ScheduledTaskSlot{
            task_id: task_id,
            name: name,
            kind: ScheduleKind.FixedInterval,
            state: ScheduleState.Active,
            next_run_ms: satAddI64(first_run_ms, j),
            interval_ms: interval_ms,
            max_runs: max_runs,
            jitter_window_ms: jitter_window_ms,
            occupied: true,
        };
        return true;
    }

    /// Schedules a fixed-delay recurring task where the next run is scheduled `delay_ms`
    /// after each successful completion.
    fn scheduleFixedDelay(
        *_,
        task_id: u64,
        name: string,
        first_run_ms: i64,
        delay_ms: i64,
        max_runs: u32,
        jitter_window_ms: i64,
    ): bool {
        if (task_id == 0 || delay_ms <= 0 || jitter_window_ms < 0) return false;
        if (_.findSlot(task_id) != null) return false;
        const idx = _.findFreeSlot() ?? return false;

        const j = deterministicJitterMs(task_id, 0, jitter_window_ms);
        _.slots[idx] = ScheduledTaskSlot{
            task_id: task_id,
            name: name,
            kind: ScheduleKind.FixedDelay,
            state: ScheduleState.Active,
            next_run_ms: satAddI64(first_run_ms, j),
            interval_ms: delay_ms,
            max_runs: max_runs,
            jitter_window_ms: jitter_window_ms,
            occupied: true,
        };
        return true;
    }

    /// Configures exponential retry backoff (`retry_backoff_ms * 2^(retry-1)`) on `task_id`.
    fn setRetryPolicy(*_, task_id: u64, max_retries: u32, retry_backoff_ms: i64): bool {
        if (retry_backoff_ms < 0) return false;
        const idx = _.findSlot(task_id) ?? return false;
        if (isTerminalScheduleState(_.slots[idx].state)) return false;
        _.slots[idx].max_retries = max_retries;
        _.slots[idx].retry_backoff_ms = retry_backoff_ms;
        return true;
    }

    /// Returns the earliest `next_run_ms` among all active, non-in-flight tasks (O(N)),
    /// suitable for passing directly into an event loop sleep/poll timeout.
    fn nextDueMs(): i64? {
        let best: i64? = null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == ScheduleState.Active && !_.slots[i].in_flight) {
                if (cur_best := best) {
                    if (_.slots[i].next_run_ms < cur_best) {
                        best = _.slots[i].next_run_ms;
                    }
                } else {
                    best = _.slots[i].next_run_ms;
                }
            }
        }
        return best;
    }

    /// Extracts up to `out_task_ids.len` due tasks (`next_run_ms <= now_ms`) in deterministic
    /// `(next_run_ms asc, task_id asc)` order, marking each `in_flight = true`.
    fn pollDue(*_, now_ms: i64, out_task_ids: *[]u64): usize {
        if (out_task_ids.len == 0) return 0;

        let count: usize = 0;
        while (count < out_task_ids.len) {
            let best_idx: usize? = null;
            let i: usize = 0;
            while (i < _.slots.len) : (i += 1) {
                if (!_.slots[i].occupied || _.slots[i].state != ScheduleState.Active || _.slots[i].in_flight) {
                    continue;
                }
                if (_.slots[i].next_run_ms > now_ms) continue;

                if (b := best_idx) {
                    const better = (_.slots[i].next_run_ms < _.slots[b].next_run_ms) ||
                        (_.slots[i].next_run_ms == _.slots[b].next_run_ms && _.slots[i].task_id < _.slots[b].task_id);
                    if (better) best_idx = i;
                } else {
                    best_idx = i;
                }
            }

            if (chosen := best_idx) {
                _.slots[chosen].in_flight = true;
                _.slots[chosen].last_run_ms = now_ms;
                out_task_ids[count] = _.slots[chosen].task_id;
                count += 1;
                _.total_dispatches = satAddU64(_.total_dispatches, 1);
            } else {
                break;
            }
        }
        return count;
    }

    /// Reports execution outcome for an `in_flight` task at `now_ms`.
    /// On success: advances `run_count` and schedules the next cycle (or marks `Completed`).
    /// On failure: reschedules with exponential backoff up to `max_retries` (or marks `Failed`).
    fn reportResult(*_, task_id: u64, succeeded: bool, now_ms: i64): bool {
        const idx = _.findSlot(task_id) ?? return false;
        if (_.slots[idx].state != ScheduleState.Active || !_.slots[idx].in_flight) return false;

        _.slots[idx].in_flight = false;

        if (succeeded) {
            _.total_successes = satAddU64(_.total_successes, 1);
            _.slots[idx].run_count = satAddU32(_.slots[idx].run_count, 1);
            _.slots[idx].retry_count = 0;

            const reached_cap = _.slots[idx].kind == ScheduleKind.OneShot ||
                (_.slots[idx].max_runs > 0 && _.slots[idx].run_count >= _.slots[idx].max_runs);
            if (reached_cap) {
                _.slots[idx].state = ScheduleState.Completed;
                return true;
            }

            const j = deterministicJitterMs(_.slots[idx].task_id, _.slots[idx].run_count as u64, _.slots[idx].jitter_window_ms);
            if (_.slots[idx].kind == ScheduleKind.FixedDelay) {
                _.slots[idx].next_run_ms = satAddI64(satAddI64(now_ms, _.slots[idx].interval_ms), j);
            } else {
                // FixedInterval: advance by interval_ms, skipping missed slots if now_ms jumped past.
                let next_base = satAddI64(_.slots[idx].next_run_ms, _.slots[idx].interval_ms);
                if (next_base <= now_ms) {
                    const behind = now_ms - next_base;
                    const skips = (behind / _.slots[idx].interval_ms) + 1;
                    next_base = satAddI64(next_base, satMulPosI64(skips, _.slots[idx].interval_ms));
                }
                _.slots[idx].next_run_ms = satAddI64(next_base, j);
            }
            return true;
        }

        // Execution failed
        _.total_failures = satAddU64(_.total_failures, 1);
        if (_.slots[idx].retry_count < _.slots[idx].max_retries) {
            _.slots[idx].retry_count = satAddU32(_.slots[idx].retry_count, 1);
            _.total_retries = satAddU64(_.total_retries, 1);

            const shift_u32: u32 = if (_.slots[idx].retry_count > 31) 30 else (_.slots[idx].retry_count - 1);
            const factor: i64 = (1 as i64) << (shift_u32 as u6);
            const backoff = satMulPosI64(_.slots[idx].retry_backoff_ms, factor);
            const seq: u64 = ((_.slots[idx].run_count as u64) * 1024) + (_.slots[idx].retry_count as u64);
            const j = deterministicJitterMs(_.slots[idx].task_id, seq, _.slots[idx].jitter_window_ms);
            _.slots[idx].next_run_ms = satAddI64(satAddI64(now_ms, backoff), j);
        } else {
            _.slots[idx].state = ScheduleState.Failed;
        }
        return true;
    }

    /// Pauses an `Active` task so it will not be returned by `nextDueMs` or `pollDue`.
    fn pause(*_, task_id: u64): bool {
        const idx = _.findSlot(task_id) ?? return false;
        if (_.slots[idx].state != ScheduleState.Active) return false;
        _.slots[idx].state = ScheduleState.Paused;
        return true;
    }

    /// Resumes a `Paused` task, clamping `next_run_ms` up to `now_ms` if it fell behind while paused.
    fn resumeTask(*_, task_id: u64, now_ms: i64): bool {
        const idx = _.findSlot(task_id) ?? return false;
        if (_.slots[idx].state != ScheduleState.Paused) return false;
        _.slots[idx].state = ScheduleState.Active;
        if (_.slots[idx].next_run_ms < now_ms) {
            _.slots[idx].next_run_ms = now_ms;
        }
        return true;
    }

    /// Cancels an `Active` or `Paused` task permanently.
    fn cancel(*_, task_id: u64): bool {
        const idx = _.findSlot(task_id) ?? return false;
        if (isTerminalScheduleState(_.slots[idx].state)) return false;
        _.slots[idx].state = ScheduleState.Cancelled;
        _.slots[idx].in_flight = false;
        return true;
    }

    /// Frees a slot in a terminal state (`Completed | Failed | Cancelled`) for zero-alloc reuse.
    fn releaseTerminal(*_, task_id: u64): bool {
        const idx = _.findSlot(task_id) ?? return false;
        if (!isTerminalScheduleState(_.slots[idx].state)) return false;
        _.slots[idx] = ScheduledTaskSlot{};
        return true;
    }

    /// Returns a read-only copy of `task_id`'s slot, or `null` if not found.
    fn get(task_id: u64): ScheduledTaskSlot? {
        const idx = _.findSlot(task_id) ?? return null;
        return _.slots[idx];
    }

    /// Computes live aggregate statistics across all occupied slots.
    fn stats(): SchedulerStats {
        let st = SchedulerStats{
            capacity: _.slots.len,
            total_dispatches: _.total_dispatches,
            total_successes: _.total_successes,
            total_failures: _.total_failures,
            total_retries: _.total_retries,
        };
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            st.total += 1;
            if (_.slots[i].in_flight) st.in_flight += 1;
            match (_.slots[i].state) {
                ScheduleState.Active => st.active += 1,
                ScheduleState.Paused => st.paused += 1,
                ScheduleState.Completed => st.completed += 1,
                ScheduleState.Failed => st.failed += 1,
                ScheduleState.Cancelled => st.cancelled += 1,
            }
        }
        return st;
    }
}
