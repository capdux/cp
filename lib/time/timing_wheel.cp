// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Hierarchical Hashed Timing Wheel O(1) Timer Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation 3-level 64-slot Hierarchical Hashed Timing Wheel (262,144-tick span):
//   - O(1) scheduling (`schedule` / `schedule_periodic`), O(1) ABA-safe handle cancellation (`cancel`)
//   - Intrusive doubly-linked bucket lists backed by fixed 256-slot inline pool & free-index stack
//   - Automatic Level 2 -> Level 1 -> Level 0 cascade demotion on tick boundaries
//   - Clock-regression guard (`target_tick < current_tick` no-op) & periodic auto-rescheduling

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Number of hash buckets per wheel level (power of two: 64).
pub const TW_SLOTS_PER_LEVEL: usize = 64;
/// Number of hierarchical wheel levels (64^3 = 262,144 ticks max span).
pub const TW_LEVELS: usize = 3;
/// Maximum concurrent active timers stored inline (zero dynamic heap allocations).
pub const TW_MAX_TIMERS: usize = 256;
/// Sentinel index representing a null intrusive list pointer.
pub const TW_INVALID_IDX: u16 = 0xFFFF;

const TW_MAX_DELAY_TICKS: u64 = 262_143; // 64 * 64 * 64 - 1

/// ABA-protected handle identifying a scheduled timer slot.
pub struct TimerHandle {
    slot_idx: u16 = TW_INVALID_IDX;
    generation: u32 = 0;
}

/// Inline timer node containing intrusive doubly-linked list pointers and metadata.
pub struct WheelTimerEntry {
    user_tag: u64 = 0;
    deadline_tick: u64 = 0;
    interval_ticks: u64 = 0;
    generation: u32 = 1;
    active: bool = false;
    prev_idx: u16 = TW_INVALID_IDX;
    next_idx: u16 = TW_INVALID_IDX;
    level: u8 = 0;
    bucket: u8 = 0;
}

/// Operational statistics snapshot for `HierarchicalTimingWheel`.
pub struct TimingWheelStats {
    current_tick: u64 = 0;
    active_timers: usize = 0;
    total_scheduled: u64 = 0;
    total_fired: u64 = 0;
    total_canceled: u64 = 0;
    total_cascades: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc 3-Level Hierarchical Hashed Timing Wheel
// ============================================================================

/// Zero-heap-allocation 3-level hierarchical hashed timing wheel with O(1) schedule and cancel.
pub struct HierarchicalTimingWheel {
    current_tick: u64 = 0;
    heads: [TW_LEVELS][TW_SLOTS_PER_LEVEL]u16 = [[TW_INVALID_IDX; TW_SLOTS_PER_LEVEL]; TW_LEVELS];
    entries: [TW_MAX_TIMERS]WheelTimerEntry = [WheelTimerEntry{}; TW_MAX_TIMERS];
    free_stack: [TW_MAX_TIMERS]u16 = [0; TW_MAX_TIMERS];
    free_top: usize = TW_MAX_TIMERS;

    active_timers: usize = 0;
    total_scheduled: u64 = 0;
    total_fired: u64 = 0;
    total_canceled: u64 = 0;
    total_cascades: u64 = 0;

    /// Initializes an empty timing wheel positioned at `start_tick`.
    static fn init(start_tick: u64): HierarchicalTimingWheel {
        let w = HierarchicalTimingWheel{
            current_tick: start_tick,
            heads: [[TW_INVALID_IDX; TW_SLOTS_PER_LEVEL]; TW_LEVELS],
            entries: [WheelTimerEntry{}; TW_MAX_TIMERS],
            free_stack: [0; TW_MAX_TIMERS],
            free_top: TW_MAX_TIMERS,
            active_timers: 0,
            total_scheduled: 0,
            total_fired: 0,
            total_canceled: 0,
            total_cascades: 0,
        };
        let i: usize = 0;
        while (i < TW_MAX_TIMERS) : (i += 1) {
            w.free_stack[i] = (TW_MAX_TIMERS - 1 - i) as u16;
            w.entries[i].generation = 1;
        }
        return w;
    }

    fn allocSlot(*_): u16? {
        if (_.free_top == 0) {
            return null;
        }
        _.free_top -= 1;
        return _.free_stack[_.free_top];
    }

    fn releaseSlot(*_, idx: u16) {
        const i = idx as usize;
        _.entries[i].active = false;
        _.entries[i].prev_idx = TW_INVALID_IDX;
        _.entries[i].next_idx = TW_INVALID_IDX;
        let next_gen = _.entries[i].generation +% 1;
        if (next_gen == 0) {
            next_gen = 1;
        }
        _.entries[i].generation = next_gen;
        if (_.free_top < TW_MAX_TIMERS) {
            _.free_stack[_.free_top] = idx;
            _.free_top += 1;
        }
    }

    fn linkIntoBucket(*_, idx: u16, level: u8, bucket: u8) {
        const i = idx as usize;
        const lv = level as usize;
        const bk = bucket as usize;
        const old_head = _.heads[lv][bk];

        _.entries[i].level = level;
        _.entries[i].bucket = bucket;
        _.entries[i].prev_idx = TW_INVALID_IDX;
        _.entries[i].next_idx = old_head;
        if (old_head != TW_INVALID_IDX) {
            _.entries[old_head as usize].prev_idx = idx;
        }
        _.heads[lv][bk] = idx;
    }

    fn unlinkFromBucket(*_, idx: u16) {
        const i = idx as usize;
        const lv = _.entries[i].level as usize;
        const bk = _.entries[i].bucket as usize;
        const prev = _.entries[i].prev_idx;
        const next = _.entries[i].next_idx;

        if (prev != TW_INVALID_IDX) {
            _.entries[prev as usize].next_idx = next;
        } else if (_.heads[lv][bk] == idx) {
            _.heads[lv][bk] = next;
        }
        if (next != TW_INVALID_IDX) {
            _.entries[next as usize].prev_idx = prev;
        }
        _.entries[i].prev_idx = TW_INVALID_IDX;
        _.entries[i].next_idx = TW_INVALID_IDX;
    }

    fn placeEntry(*_, idx: u16) {
        const deadline = _.entries[idx as usize].deadline_tick;
        const delta: u64 = if (deadline > _.current_tick) deadline - _.current_tick else 0;

        if (delta < 64) {
            const bk: u8 = (deadline & 63) as u8;
            _.linkIntoBucket(idx, 0, bk);
        } else if (delta < 4096) {
            const bk: u8 = ((deadline >> 6) & 63) as u8;
            _.linkIntoBucket(idx, 1, bk);
        } else {
            const bk: u8 = ((deadline >> 12) & 63) as u8;
            _.linkIntoBucket(idx, 2, bk);
        }
    }

    fn cascadeBucket(*_, level: u8, bucket: u8) {
        const lv = level as usize;
        const bk = bucket as usize;
        let cur = _.heads[lv][bk];
        if (cur == TW_INVALID_IDX) {
            return;
        }
        _.heads[lv][bk] = TW_INVALID_IDX;
        _.total_cascades += 1;

        while (cur != TW_INVALID_IDX) {
            const next = _.entries[cur as usize].next_idx;
            _.entries[cur as usize].prev_idx = TW_INVALID_IDX;
            _.entries[cur as usize].next_idx = TW_INVALID_IDX;
            _.placeEntry(cur);
            cur = next;
        }
    }

    fn fireLevel0Bucket(*_, bucket: u8, out_fired_tags: *[]u64, fired_written: *usize) {
        const bk = bucket as usize;
        let cur = _.heads[0][bk];
        if (cur == TW_INVALID_IDX) {
            return;
        }
        _.heads[0][bk] = TW_INVALID_IDX;

        while (cur != TW_INVALID_IDX) {
            const next = _.entries[cur as usize].next_idx;
            _.entries[cur as usize].prev_idx = TW_INVALID_IDX;
            _.entries[cur as usize].next_idx = TW_INVALID_IDX;

            const ci = cur as usize;
            if (_.entries[ci].deadline_tick <= _.current_tick) {
                const tag = _.entries[ci].user_tag;
                if (fired_written.* < out_fired_tags.len) {
                    out_fired_tags[fired_written.*] = tag;
                    fired_written.* += 1;
                }
                _.total_fired += 1;

                const interval = _.entries[ci].interval_ticks;
                if (interval > 0 && interval <= TW_MAX_DELAY_TICKS) {
                    _.entries[ci].deadline_tick = _.current_tick + interval;
                    _.placeEntry(cur);
                } else {
                    _.active_timers -= 1;
                    _.releaseSlot(cur);
                }
            } else {
                // Not due yet (safety re-insert)
                _.placeEntry(cur);
            }

            cur = next;
        }
    }

    /// Schedules a one-shot timer firing after `delay_ticks` (`0..=262,143`).
    /// Returns `null` if `delay_ticks` exceeds the 3-level span or all 256 slots are full.
    fn schedule(*_, delay_ticks: u64, user_tag: u64): TimerHandle? {
        if (delay_ticks > TW_MAX_DELAY_TICKS) {
            return null;
        }
        if (delay_ticks > 0xFFFFFFFFFFFFFFFF - _.current_tick) {
            return null;
        }
        const idx = _.allocSlot() ?? return null;
        const i = idx as usize;
        const gen = _.entries[i].generation;
        _.entries[i].user_tag = user_tag;
        _.entries[i].deadline_tick = _.current_tick + delay_ticks;
        _.entries[i].interval_ticks = 0;
        _.entries[i].active = true;
        _.placeEntry(idx);

        _.active_timers += 1;
        _.total_scheduled += 1;
        return TimerHandle{ slot_idx: idx, generation: gen };
    }

    /// Schedules a periodic timer whose first firing occurs after `delay_ticks` and then
    /// automatically reschedules every `interval_ticks` (`1..=262,143`).
    fn schedule_periodic(
        *_,
        delay_ticks: u64,
        interval_ticks: u64,
        user_tag: u64,
    ): TimerHandle? {
        if (delay_ticks > TW_MAX_DELAY_TICKS || interval_ticks == 0 || interval_ticks > TW_MAX_DELAY_TICKS) {
            return null;
        }
        if (delay_ticks > 0xFFFFFFFFFFFFFFFF - _.current_tick) {
            return null;
        }
        const idx = _.allocSlot() ?? return null;
        const i = idx as usize;
        const gen = _.entries[i].generation;
        _.entries[i].user_tag = user_tag;
        _.entries[i].deadline_tick = _.current_tick + delay_ticks;
        _.entries[i].interval_ticks = interval_ticks;
        _.entries[i].active = true;
        _.placeEntry(idx);

        _.active_timers += 1;
        _.total_scheduled += 1;
        return TimerHandle{ slot_idx: idx, generation: gen };
    }

    /// Cancels a scheduled timer in O(1) using generation-checked ABA protection.
    /// Returns `false` if the handle is invalid, already fired, already canceled, or stale.
    fn cancel(*_, handle: TimerHandle): bool {
        if (handle.slot_idx >= (TW_MAX_TIMERS as u16)) {
            return false;
        }
        const i = handle.slot_idx as usize;
        if (!_.entries[i].active || _.entries[i].generation != handle.generation) {
            return false;
        }
        _.unlinkFromBucket(handle.slot_idx);
        _.active_timers -= 1;
        _.total_canceled += 1;
        _.releaseSlot(handle.slot_idx);
        return true;
    }

    /// Advances the wheel to `target_tick`, cascading higher levels and firing all due timers.
    /// - If `target_tick < current_tick`, performs a safe no-op and returns `0`.
    /// - Writes expired `user_tag`s into `out_fired_tags` and returns the number written.
    fn advance_to(*_, target_tick: u64, out_fired_tags: *[]u64): usize {
        if (target_tick < _.current_tick) {
            return 0;
        }

        let written: usize = 0;
        // Fire any 0-delay timers scheduled at current_tick before advancing
        const cur_bk: u8 = (_.current_tick & 63) as u8;
        _.fireLevel0Bucket(cur_bk, out_fired_tags, &written);

        while (_.current_tick < target_tick) {
            _.current_tick += 1;
            const l0_bk: u8 = (_.current_tick & 63) as u8;
            if (l0_bk == 0) {
                const l1_bk: u8 = ((_.current_tick >> 6) & 63) as u8;
                if (l1_bk == 0) {
                    const l2_bk: u8 = ((_.current_tick >> 12) & 63) as u8;
                    _.cascadeBucket(2, l2_bk);
                }
                _.cascadeBucket(1, l1_bk);
            }
            _.fireLevel0Bucket(l0_bk, out_fired_tags, &written);
        }

        return written;
    }

    /// Returns the earliest `deadline_tick` among all currently active timers, or `null` if empty.
    fn next_expiry_tick(): u64? {
        if (_.active_timers == 0) {
            return null;
        }
        let best: u64 = 0xFFFFFFFFFFFFFFFF;
        let found = false;
        let i: usize = 0;
        while (i < TW_MAX_TIMERS) : (i += 1) {
            if (_.entries[i].active) {
                if (!found || _.entries[i].deadline_tick < best) {
                    best = _.entries[i].deadline_tick;
                    found = true;
                }
            }
        }
        if (!found) {
            return null;
        }
        return best;
    }

    /// Returns a consistent statistics snapshot of the timing wheel.
    fn stats(): TimingWheelStats {
        return TimingWheelStats{
            current_tick: _.current_tick,
            active_timers: _.active_timers,
            total_scheduled: _.total_scheduled,
            total_fired: _.total_fired,
            total_canceled: _.total_canceled,
            total_cascades: _.total_cascades,
        };
    }
}
