// Source: Absorbed from /data/dev/bun/src/bake/DevServer/WatcherAtomics.cp | Synced: 2026-11-23T21:03:55Z
// Zero-allocation triple-buffered atomic watcher <-> dev-server hot-reload event relay.

const std = @import("std");

/// Fixed triple-buffer slot count (one active in server, one pending, one staging in watcher).
pub const WATCHER_RELAY_SLOTS: usize = 3;

/// Atomic state sentinel: consumer is running and no next slot is pending (`maxInt(u8) - 1`).
pub const NEXT_EVENT_WAITING: u8 = 254;

/// Atomic state sentinel: consumer is idle (`maxInt(u8)`).
pub const NEXT_EVENT_DONE: u8 = 255;

/// Protocol misuse error set for `WatcherAtomicRelay`.
pub err WatcherRelayError {
    AlreadyAcquired,
    NotAcquired,
    NoActiveServerEvent,
}

/// Result of submitting a watcher slot via `watcherReleaseAndSubmit`.
pub enum WatcherSubmitOutcome: u8 {
    /// Slot contained no file changes; consumer was not woken.
    EmptyIgnored = 0,
    /// Consumer was idle (`NEXT_EVENT_DONE`); caller must enqueue/wake the consumer with this slot.
    ScheduledImmediate = 1,
    /// Consumer was busy (`NEXT_EVENT_WAITING`); slot is queued to run immediately after current finishes.
    QueuedBehindActive = 2,
    /// A previously queued (not yet started) pending slot was coalesced/replaced with this newer slot.
    ReplacedPending = 3,
}

/// Triple-buffered atomic event slot relay absorbed from Bun `DevServer/WatcherAtomics.cp`.
/// Recycles 3 fixed slots (`0`, `1`, `2`) between a producer (watcher thread) and a consumer
/// (dev server / bundler thread) with zero dynamic heap allocations.
pub struct WatcherAtomicRelay<T> {
    slots: [3]T,
    next_event: std.atomic.Value(u8) = std.atomic.Value(u8).init(NEXT_EVENT_DONE),
    current_event: u8? = null,
    pending_event: u8? = null,
    watcher_acquired_slot: u8? = null,
    server_active_slot: u8? = null,
    total_submitted: u64 = 0,
    total_replaced: u64 = 0,
    total_recycled: u64 = 0,

    /// Initialize all 3 reusable slots with `default_val` in idle (`NEXT_EVENT_DONE`) state.
    static fn init(default_val: T): WatcherAtomicRelay<T> {
        return WatcherAtomicRelay<T>{
            slots: [default_val, default_val, default_val],
            next_event: std.atomic.Value(u8).init(NEXT_EVENT_DONE),
        };
    }

    /// Return a mutable borrow to slot `idx` (`0..2`).
    fn slotMut(*_, idx: u8): *T {
        return *_.slots[usize(idx)];
    }

    /// Read a copy of slot `idx` (`0..2`).
    fn slotCopy(idx: u8): T {
        return _.slots[usize(idx)];
    }

    /// True if the relay is currently idle (`NEXT_EVENT_DONE`).
    fn isIdle(): bool {
        return _.next_event.load(.acquire) == NEXT_EVENT_DONE;
    }

    /// Producer side: acquire a free slot index (`0..2`) not currently owned by consumer or pending queue.
    fn watcherAcquire(*_): u8 ! WatcherRelayError {
        if (_.watcher_acquired_slot != null) {
            return err.AlreadyAcquired;
        }
        let available: [3]bool = [true, true, true];
        if (cur := _.current_event) {
            if (cur < 3) available[usize(cur)] = false;
        }
        if (pend := _.pending_event) {
            if (pend < 3) available[usize(pend)] = false;
        }

        let chosen: u8 = 0;
        for (i of 0..WATCHER_RELAY_SLOTS) {
            if (available[i]) {
                chosen = u8(i);
                break;
            }
        }
        _.watcher_acquired_slot = chosen;
        return chosen;
    }

    /// Producer side: release the acquired slot and atomically publish it if `has_changes` is true.
    fn watcherReleaseAndSubmit(*_, slot_idx: u8, has_changes: bool): WatcherSubmitOutcome ! WatcherRelayError {
        if (acq := _.watcher_acquired_slot) {
            if (acq != slot_idx) return err.NotAcquired;
        } else {
            return err.NotAcquired;
        }
        _.watcher_acquired_slot = null;

        if (!has_changes) {
            return .EmptyIgnored;
        }

        _.total_submitted += 1;
        const old_next = _.next_event.swap(slot_idx, .acq_rel);
        if (old_next == NEXT_EVENT_DONE) {
            // Consumer was idle: schedule this slot immediately and move atomic state to WAITING.
            _.current_event = slot_idx;
            _.pending_event = null;
            _.server_active_slot = slot_idx;
            _.next_event.store(NEXT_EVENT_WAITING, .monotonic);
            return .ScheduledImmediate;
        } else if (old_next == NEXT_EVENT_WAITING) {
            // Consumer is running current_event: queue slot_idx as pending.
            if (_.pending_event != null) {
                _.current_event = _.pending_event;
            }
            _.pending_event = slot_idx;
            return .QueuedBehindActive;
        } else {
            // Previous pending slot had not been picked up yet; replace it with the newer slot.
            _.pending_event = slot_idx;
            _.total_replaced += 1;
            return .ReplacedPending;
        }
    }

    /// Consumer side: mark `finished_slot` complete and atomically claim the next pending slot (if any).
    /// Returns the next slot index to process, or `null` if the relay transitioned back to idle (`NEXT_EVENT_DONE`).
    fn serverRecycle(*_, finished_slot: u8): u8? ! WatcherRelayError {
        if (active := _.server_active_slot) {
            if (active != finished_slot) return err.NoActiveServerEvent;
        } else {
            return err.NoActiveServerEvent;
        }
        _.server_active_slot = null;
        _.total_recycled += 1;

        while (true) {
            const next = _.next_event.swap(NEXT_EVENT_WAITING, .acq_rel);
            if (next == NEXT_EVENT_WAITING) {
                if (_.next_event.cmpxchgWeak(NEXT_EVENT_WAITING, NEXT_EVENT_DONE, .release, .monotonic) != null) {
                    continue;
                }
                return null;
            }
            if (next == NEXT_EVENT_DONE) {
                return null;
            }
            _.server_active_slot = next;
            return next;
        }
    }
}
