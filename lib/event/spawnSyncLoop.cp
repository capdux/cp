// Source: Absorbed from /data/dev/bun/src/event_loop/SpawnSyncEventLoop.cp | Synced: 2026-11-22T19:28:30Z (Synchronous child-wait sub-loop state machine with deadline timeout, parent handle suspend/restore & deterministic tick driver)
// Pure-CP zero-allocation synchronous sub-event-loop state machine for blocking wait with timeout & parent loop isolation.

/// Execution state returned by each `SpawnSyncLoop.tick` poll iteration.
pub enum SpawnSyncTickState: u8 {
    Running = 0,
    Completed = 1,
    Timeout = 2,
    Cancelled = 3,
}

/// Error set for `SpawnSyncLoop` lifecycle transitions.
pub err SpawnSyncLoopError {
    NotPrepared,
    AlreadyPrepared,
    InvalidSubHandle,
}

/// Zero-allocation monotonic millisecond deadline calculator (`timeout_ms == 0` means infinite).
pub struct SpawnSyncDeadline {
    start_ms: u64 = 0,
    timeout_ms: u64 = 0,

    /// Creates an infinite (non-expiring) deadline.
    static fn infinite(start_ms: u64): SpawnSyncDeadline {
        return SpawnSyncDeadline{
            start_ms: start_ms,
            timeout_ms: 0,
        };
    }

    /// Creates a finite deadline starting at `start_ms` with `timeout_ms` duration.
    static fn fromTimeout(start_ms: u64, timeout_ms: u64): SpawnSyncDeadline {
        return SpawnSyncDeadline{
            start_ms: start_ms,
            timeout_ms: timeout_ms,
        };
    }

    /// Returns true if no timeout limit is configured.
    fn isInfinite(): bool {
        return _.timeout_ms == 0;
    }

    /// Returns the absolute expiration timestamp (saturating addition on overflow).
    fn deadlineMs(): u64 {
        return _.start_ms +| _.timeout_ms;
    }

    /// Returns remaining milliseconds until expiration (`0` if already expired or infinite).
    fn remainingMs(now_ms: u64): u64 {
        if (_.timeout_ms == 0) return 0;
        const end = _.deadlineMs();
        if (now_ms >= end) return 0;
        return end - now_ms;
    }

    /// Returns true if a finite deadline has elapsed at `now_ms`.
    fn isExpired(now_ms: u64): bool {
        if (_.timeout_ms == 0) return false;
        return now_ms >= _.deadlineMs();
    }

    /// Returns elapsed milliseconds since `start_ms` (clamped to `0` on clock skew).
    fn elapsedMs(now_ms: u64): u64 {
        if (now_ms <= _.start_ms) return 0;
        return now_ms - _.start_ms;
    }
}

/// Zero-allocation synchronous sub-event-loop state machine.
/// Isolates a nested blocking operation (e.g., `spawnSync`) by saving and temporarily replacing
/// the parent event loop handle, driving deterministic deadline-bounded ticks, and restoring
/// the parent handle upon cleanup.
pub struct SpawnSyncLoop {
    sub_handle_id: u64 = 0,
    saved_parent_handle_id: u64 = 0,
    prepared: bool = false,
    did_timeout: bool = false,
    completed: bool = false,
    cancelled: bool = false,
    ticks: u64 = 0,
    deadline: SpawnSyncDeadline = SpawnSyncDeadline{},
    last_now_ms: u64 = 0,

    /// Initializes a fresh `SpawnSyncLoop` bound to `sub_handle_id`.
    static fn init(sub_handle_id: u64): SpawnSyncLoop {
        return SpawnSyncLoop{
            sub_handle_id: sub_handle_id,
        };
    }

    /// Suspends the parent event loop by saving `*active_handle_id` and installing `sub_handle_id`.
    /// Returns the saved parent handle ID.
    fn prepare(*_, active_handle_id: *u64, start_ms: u64, timeout_ms: u64): u64 ! SpawnSyncLoopError {
        if (_.prepared) return SpawnSyncLoopError.AlreadyPrepared;
        if (_.sub_handle_id == 0) return SpawnSyncLoopError.InvalidSubHandle;

        const prev = active_handle_id.*;
        _.saved_parent_handle_id = prev;
        active_handle_id.* = _.sub_handle_id;

        _.prepared = true;
        _.did_timeout = false;
        _.completed = false;
        _.cancelled = false;
        _.ticks = 0;
        _.deadline = SpawnSyncDeadline.fromTimeout(start_ms, timeout_ms);
        _.last_now_ms = start_ms;
        return prev;
    }

    /// Marks the waited synchronous operation as completed.
    fn markCompleted(*_) {
        _.completed = true;
    }

    /// Marks the waited synchronous operation as explicitly cancelled.
    fn cancel(*_) {
        _.cancelled = true;
    }

    /// Returns the current terminal or running state without advancing ticks.
    fn state(): SpawnSyncTickState {
        if (_.cancelled) return SpawnSyncTickState.Cancelled;
        if (_.completed) return SpawnSyncTickState.Completed;
        if (_.did_timeout) return SpawnSyncTickState.Timeout;
        return SpawnSyncTickState.Running;
    }

    /// Advances one synchronous poll iteration at monotonic timestamp `now_ms`.
    fn tick(*_, now_ms: u64): SpawnSyncTickState {
        _.last_now_ms = now_ms;
        if (_.cancelled) return SpawnSyncTickState.Cancelled;
        if (_.completed) return SpawnSyncTickState.Completed;
        if (_.deadline.isExpired(now_ms)) {
            _.did_timeout = true;
            return SpawnSyncTickState.Timeout;
        }
        _.ticks +|= 1;
        return SpawnSyncTickState.Running;
    }

    /// Returns remaining milliseconds before timeout at `now_ms` (`0` if infinite or expired).
    fn remainingTimeoutMs(now_ms: u64): u64 {
        return _.deadline.remainingMs(now_ms);
    }

    /// Returns elapsed milliseconds since `prepare()` at `now_ms`.
    fn elapsedMs(now_ms: u64): u64 {
        return _.deadline.elapsedMs(now_ms);
    }

    /// Restores the parent event loop handle into `*active_handle_id` and ends the prepared phase.
    /// Returns the restored parent handle ID.
    fn cleanup(*_, active_handle_id: *u64): u64 ! SpawnSyncLoopError {
        if (!_.prepared) return SpawnSyncLoopError.NotPrepared;
        const restored = _.saved_parent_handle_id;
        active_handle_id.* = restored;
        _.prepared = false;
        return restored;
    }
}
