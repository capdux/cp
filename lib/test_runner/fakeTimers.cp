// Source: Absorbed from /data/dev/bun/src/test_runner/timers/FakeTimers.cp | Synced: 2026-11-22T17:00:28Z
// Pure-CP zero-allocation deterministic fake timer & mock clock engine:
// mirrors Bun's FakeTimers & CurrentTime semantics (useFakeTimers/useRealTimers,
// setSystemTime/now/monotonicNow, setTimeout/setInterval/clearTimer/clearAllTimers,
// advanceTimersToNextTimer, advanceTimersByTime with HTML 1ms immediate clamp,
// runOnlyPendingTimers, runAllTimers with infinite-loop guard).

pub const FAKE_TIMERS_MAX_SLOTS: usize = 256;
pub const FAKE_TIMERS_MAX_ADVANCE_MS: i64 = 4294967295;
pub const FAKE_TIMERS_DEFAULT_RUN_LIMIT: usize = 10000;

pub error FakeTimerError {
    NotActive,
    AdvanceOutOfRange,
    CapacityExceeded,
    RunLimitExceeded,
}

pub enum FakeTimerKind: u8 {
    Timeout = 0,
    Interval = 1,
}

pub struct FakeTimerSlot {
    active: bool = false,
    id: u64 = 0,
    kind: FakeTimerKind = .Timeout,
    due_ms: i64 = 0,
    interval_ms: i64 = 0,
    tag: u64 = 0,
    seq: u64 = 0,
}

pub struct FakeTimerFired {
    id: u64 = 0,
    kind: FakeTimerKind = .Timeout,
    tag: u64 = 0,
    fired_at_ms: i64 = 0,
    system_time_ms: i64 = 0,
}

pub struct FakeTimerAdvanceResult {
    fired_count: usize = 0,
    recorded_count: usize = 0,
    now_ms: i64 = 0,
    monotonic_ms: i64 = 0,
}

/// Normalizes a timer delay per HTML/Bun spec: delays <= 0ms are clamped to 1ms.
pub fn normalizeFakeDelayMs(delay_ms: i64): i64 {
    return if (delay_ms <= 0) 1 else delay_ms;
}

/// Pure-CP zero-allocation fake timer scheduler and mock system clock.
pub struct FakeTimerClock {
    active: bool = false,
    monotonic_ms: i64 = 0,
    date_now_offset_ms: i64 = 0,
    next_id: u64 = 1,
    next_seq: u64 = 1,
    active_count: usize = 0,
    max_run_limit: usize = FAKE_TIMERS_DEFAULT_RUN_LIMIT,
    slots: [FAKE_TIMERS_MAX_SLOTS]FakeTimerSlot = [FakeTimerSlot{}; FAKE_TIMERS_MAX_SLOTS],

    static fn init(): FakeTimerClock {
        return FakeTimerClock{};
    }

    /// Returns whether fake timers are currently active (mirrors `isFakeTimers()`).
    fn isFakeTimers(): bool {
        return _.active;
    }

    /// Activates fake timers and anchors the mock system clock at `initial_system_time_ms`.
    /// Resets monotonic offset to 0 and clears any previously scheduled timers.
    fn useFakeTimers(*_, initial_system_time_ms: i64) {
        _.clearSlotsInternal();
        _.active = true;
        _.monotonic_ms = 0;
        _.date_now_offset_ms = initial_system_time_ms;
        _.next_id = 1;
        _.next_seq = 1;
    }

    /// Deactivates fake timers and clears all pending timers (mirrors `useRealTimers()`).
    fn useRealTimers(*_) {
        _.clearSlotsInternal();
        _.active = false;
        _.monotonic_ms = 0;
        _.date_now_offset_ms = 0;
    }

    /// Overrides the mocked system wall-clock timestamp without shifting pending monotonic timers.
    /// Mirrors Bun's `CurrentTime.set`: `date_now_offset = new_system_time_ms - monotonic_ms`.
    fn setSystemTime(*_, new_system_time_ms: i64) {
        _.date_now_offset_ms = new_system_time_ms - _.monotonic_ms;
    }

    /// Returns the current mocked wall-clock timestamp in milliseconds (`Date.now()` / `jest.now()`).
    fn now(): i64 {
        return _.date_now_offset_ms + _.monotonic_ms;
    }

    /// Returns the current monotonic virtual clock in milliseconds (`performance.now()`).
    fn monotonicNow(): i64 {
        return _.monotonic_ms;
    }

    /// Configures the upper bound on timer firings per `runAllTimers` / `advanceTimersByTime` call
    /// to protect against non-terminating interval loops.
    fn setRunLimit(*_, limit: usize) {
        _.max_run_limit = if (limit == 0) 1 else limit;
    }

    /// Returns the number of currently scheduled timers (mirrors `getTimerCount()`).
    fn getTimerCount(): usize ! FakeTimerError {
        if (!_.active) return err.NotActive;
        return _.active_count;
    }

    /// Returns the earliest scheduled due timestamp on the monotonic clock, if any.
    fn peekNextDueMs(): i64? {
        if (idx := _.findMinIndex()) {
            return _.slots[idx].due_ms;
        }
        return null;
    }

    /// Returns the latest scheduled due timestamp on the monotonic clock, if any.
    fn peekLastDueMs(): i64? {
        if (idx := _.findMaxIndex()) {
            return _.slots[idx].due_ms;
        }
        return null;
    }

    /// Schedules a one-shot timeout after `delay_ms` (clamped to >= 1ms per HTML/Bun spec).
    fn setTimeout(*_, delay_ms: i64, tag: u64): u64 ! FakeTimerError {
        if (!_.active) return err.NotActive;
        const effective = normalizeFakeDelayMs(delay_ms);
        return _.insertSlot(.Timeout, _.monotonic_ms + effective, 0, tag);
    }

    /// Schedules a repeating interval every `interval_ms` (clamped to >= 1ms per HTML/Bun spec).
    fn setInterval(*_, interval_ms: i64, tag: u64): u64 ! FakeTimerError {
        if (!_.active) return err.NotActive;
        const effective = normalizeFakeDelayMs(interval_ms);
        return _.insertSlot(.Interval, _.monotonic_ms + effective, effective, tag);
    }

    /// Cancels a scheduled timeout or interval by ID. Returns `true` if found and removed.
    fn clearTimer(*_, id: u64): bool ! FakeTimerError {
        if (!_.active) return err.NotActive;
        if (id == 0) return false;
        let i: usize = 0;
        while (i < FAKE_TIMERS_MAX_SLOTS) {
            if (_.slots[i].active && _.slots[i].id == id) {
                _.slots[i].active = false;
                if (_.active_count > 0) _.active_count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Clears all pending timers and returns how many timers were cancelled (mirrors `clearAllTimers()`).
    fn clearAllTimers(*_): usize ! FakeTimerError {
        if (!_.active) return err.NotActive;
        const prev = _.active_count;
        _.clearSlotsInternal();
        return prev;
    }

    /// Advances the virtual clock to the earliest pending timer and fires it (mirrors `advanceTimersToNextTimer()`).
    fn advanceTimersToNextTimer(*_, out_events: []FakeTimerFired): FakeTimerAdvanceResult ! FakeTimerError {
        if (!_.active) return err.NotActive;
        let res = FakeTimerAdvanceResult{
            fired_count: 0,
            recorded_count: 0,
            now_ms: _.now(),
            monotonic_ms: _.monotonic_ms,
        };
        if (idx := _.findMinIndex()) {
            const ev = _.fireSlotAt(idx);
            res.fired_count = 1;
            if (out_events.len > 0) {
                out_events[0] = ev;
                res.recorded_count = 1;
            }
        }
        res.now_ms = _.now();
        res.monotonic_ms = _.monotonic_ms;
        return res;
    }

    /// Advances the virtual clock by `delta_ms`, firing all timers due at or before `monotonic_ms + effective_delta`.
    /// Mirrors Bun's `advanceTimersByTime`:
    /// - Rejects `delta_ms < 0` or `delta_ms > FAKE_TIMERS_MAX_ADVANCE_MS` (`u32::MAX`)
    /// - Clamps `delta_ms == 0` to `1ms` so `setTimeout(fn, 0)` immediate timers fire
    fn advanceTimersByTime(*_, delta_ms: i64, out_events: []FakeTimerFired): FakeTimerAdvanceResult ! FakeTimerError {
        if (!_.active) return err.NotActive;
        if (delta_ms < 0 || delta_ms > FAKE_TIMERS_MAX_ADVANCE_MS) {
            return err.AdvanceOutOfRange;
        }
        const effective: i64 = if (delta_ms == 0) 1 else delta_ms;
        const target = _.monotonic_ms + effective;
        let res = _.executeUntilInternal(target, out_events)?;
        if (target > _.monotonic_ms) {
            _.monotonic_ms = target;
        }
        res.now_ms = _.now();
        res.monotonic_ms = _.monotonic_ms;
        return res;
    }

    /// Executes all timers currently pending up to the maximum currently scheduled due timestamp.
    /// Mirrors Bun's `executeOnlyPendingTimers`: intervals stop once their next due time exceeds `until`.
    fn runOnlyPendingTimers(*_, out_events: []FakeTimerFired): FakeTimerAdvanceResult ! FakeTimerError {
        if (!_.active) return err.NotActive;
        if (max_idx := _.findMaxIndex()) {
            const until = _.slots[max_idx].due_ms;
            let res = _.executeUntilInternal(until, out_events)?;
            res.now_ms = _.now();
            res.monotonic_ms = _.monotonic_ms;
            return res;
        }
        return FakeTimerAdvanceResult{
            fired_count: 0,
            recorded_count: 0,
            now_ms: _.now(),
            monotonic_ms: _.monotonic_ms,
        };
    }

    /// Executes timers in chronological order until the queue is empty or `max_run_limit` is reached.
    /// Mirrors Bun's `executeAllTimers`, adding an infinite-loop guard for repeating intervals.
    fn runAllTimers(*_, out_events: []FakeTimerFired): FakeTimerAdvanceResult ! FakeTimerError {
        if (!_.active) return err.NotActive;
        let res = FakeTimerAdvanceResult{
            fired_count: 0,
            recorded_count: 0,
            now_ms: _.now(),
            monotonic_ms: _.monotonic_ms,
        };
        while (true) {
            if (idx := _.findMinIndex()) {
                if (res.fired_count >= _.max_run_limit) {
                    return err.RunLimitExceeded;
                }
                const ev = _.fireSlotAt(idx);
                if (res.recorded_count < out_events.len) {
                    out_events[res.recorded_count] = ev;
                    res.recorded_count += 1;
                }
                res.fired_count += 1;
            } else {
                break;
            }
        }
        res.now_ms = _.now();
        res.monotonic_ms = _.monotonic_ms;
        return res;
    }

    // --- Internal helpers ---

    fn clearSlotsInternal(*_) {
        let i: usize = 0;
        while (i < FAKE_TIMERS_MAX_SLOTS) {
            _.slots[i].active = false;
            i += 1;
        }
        _.active_count = 0;
    }

    fn insertSlot(*_, kind: FakeTimerKind, due_ms: i64, interval_ms: i64, tag: u64): u64 ! FakeTimerError {
        let i: usize = 0;
        while (i < FAKE_TIMERS_MAX_SLOTS) {
            if (!_.slots[i].active) {
                const id = _.next_id;
                _.next_id += 1;
                const seq = _.next_seq;
                _.next_seq += 1;
                _.slots[i] = FakeTimerSlot{
                    active: true,
                    id: id,
                    kind: kind,
                    due_ms: due_ms,
                    interval_ms: interval_ms,
                    tag: tag,
                    seq: seq,
                };
                _.active_count += 1;
                return id;
            }
            i += 1;
        }
        return err.CapacityExceeded;
    }

    fn findMinIndex(): usize? {
        let found: bool = false;
        let best_idx: usize = 0;
        let i: usize = 0;
        while (i < FAKE_TIMERS_MAX_SLOTS) {
            if (_.slots[i].active) {
                if (!found) {
                    found = true;
                    best_idx = i;
                } else {
                    const cur = _.slots[i];
                    const best = _.slots[best_idx];
                    if (cur.due_ms < best.due_ms || (cur.due_ms == best.due_ms && cur.seq < best.seq)) {
                        best_idx = i;
                    }
                }
            }
            i += 1;
        }
        return if (found) best_idx else null;
    }

    fn findMaxIndex(): usize? {
        let found: bool = false;
        let best_idx: usize = 0;
        let i: usize = 0;
        while (i < FAKE_TIMERS_MAX_SLOTS) {
            if (_.slots[i].active) {
                if (!found) {
                    found = true;
                    best_idx = i;
                } else {
                    const cur = _.slots[i];
                    const best = _.slots[best_idx];
                    if (cur.due_ms > best.due_ms || (cur.due_ms == best.due_ms && cur.seq > best.seq)) {
                        best_idx = i;
                    }
                }
            }
            i += 1;
        }
        return if (found) best_idx else null;
    }

    fn fireSlotAt(*_, idx: usize): FakeTimerFired {
        const slot = _.slots[idx];
        if (slot.due_ms > _.monotonic_ms) {
            _.monotonic_ms = slot.due_ms;
        }
        const ev = FakeTimerFired{
            id: slot.id,
            kind: slot.kind,
            tag: slot.tag,
            fired_at_ms: _.monotonic_ms,
            system_time_ms: _.now(),
        };
        if (slot.kind == .Interval) {
            const next_seq = _.next_seq;
            _.next_seq += 1;
            _.slots[idx].due_ms = slot.due_ms + slot.interval_ms;
            _.slots[idx].seq = next_seq;
        } else {
            _.slots[idx].active = false;
            if (_.active_count > 0) _.active_count -= 1;
        }
        return ev;
    }

    fn executeUntilInternal(*_, until_ms: i64, out_events: []FakeTimerFired): FakeTimerAdvanceResult ! FakeTimerError {
        let res = FakeTimerAdvanceResult{
            fired_count: 0,
            recorded_count: 0,
            now_ms: _.now(),
            monotonic_ms: _.monotonic_ms,
        };
        while (true) {
            if (idx := _.findMinIndex()) {
                if (_.slots[idx].due_ms > until_ms) {
                    break;
                }
                if (res.fired_count >= _.max_run_limit) {
                    return err.RunLimitExceeded;
                }
                const ev = _.fireSlotAt(idx);
                if (res.recorded_count < out_events.len) {
                    out_events[res.recorded_count] = ev;
                    res.recorded_count += 1;
                }
                res.fired_count += 1;
            } else {
                break;
            }
        }
        res.now_ms = _.now();
        res.monotonic_ms = _.monotonic_ms;
        return res;
    }
}
