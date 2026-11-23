// Source: Absorbed from /data/dev/bun/src/safety/ThreadLock.cp | Synced: 2026-11-23T01:34:02Z (Single-thread ownership assertion lock ThreadLock: lock/unlock/assertLocked/lockOrAssert, invalid thread ID guard & ownership transfer)
// Pure-CP zero-allocation thread ownership assertion lock aligned with Bun `safety/ThreadLock.cp`.

/// Sentinel thread identifier representing an unlocked `ThreadLock` (aligned with Bun `thread_id.invalid`).
pub const THREAD_LOCK_INVALID_ID: u64 = 0;

/// Error set for `ThreadLock` ownership checks and transitions.
pub err ThreadLockError {
    InvalidThreadId,
    AlreadyLocked,
    NotLocked,
    WrongThread,
}

/// Zero-allocation debug/runtime thread ownership assertion lock (aligned with Bun `safety/ThreadLock.cp`).
/// Ensures a non-thread-safe resource is accessed exclusively by one owning thread at a time
/// without OS mutex overhead or heap allocation.
pub struct ThreadLock {
    owning_thread: u64 = THREAD_LOCK_INVALID_ID,
    enabled: bool = true,
    acquire_count: u64 = 0,
    violation_count: u64 = 0,

    /// Initializes an unlocked `ThreadLock` with runtime assertions enabled.
    static fn initUnlocked(): ThreadLock {
        return ThreadLock{
            owning_thread: THREAD_LOCK_INVALID_ID,
            enabled: true,
        };
    }

    /// Initializes a disabled (no-op) `ThreadLock` for release/unchecked fast paths.
    static fn initDisabled(): ThreadLock {
        return ThreadLock{
            owning_thread: THREAD_LOCK_INVALID_ID,
            enabled: false,
        };
    }

    /// Initializes a `ThreadLock` immediately owned by `thread_id`.
    static fn initLocked(thread_id: u64): ThreadLock ! ThreadLockError {
        let lk = ThreadLock.initUnlocked();
        * = lk.lock(thread_id)?;
        return lk;
    }

    fn isLocked(): bool {
        return _.owning_thread != THREAD_LOCK_INVALID_ID;
    }

    fn owningThread(): u64 {
        return _.owning_thread;
    }

    /// Acquires exclusive thread ownership for `thread_id` (aligned with Bun `ThreadLock.lock`).
    /// Fails with `AlreadyLocked` if any thread (including `thread_id`) already holds the lock.
    fn lock(*_, thread_id: u64): bool ! ThreadLockError {
        if (!_.enabled) return true;
        if (thread_id == THREAD_LOCK_INVALID_ID) {
            _.violation_count +|= 1;
            return ThreadLockError.InvalidThreadId;
        }
        if (_.owning_thread != THREAD_LOCK_INVALID_ID) {
            _.violation_count +|= 1;
            return ThreadLockError.AlreadyLocked;
        }
        _.owning_thread = thread_id;
        _.acquire_count +|= 1;
        return true;
    }

    /// Verifies that `thread_id` currently owns the lock (aligned with Bun `ThreadLock.assertLocked`).
    fn assertLocked(*_, thread_id: u64): bool ! ThreadLockError {
        if (!_.enabled) return true;
        if (thread_id == THREAD_LOCK_INVALID_ID) {
            _.violation_count +|= 1;
            return ThreadLockError.InvalidThreadId;
        }
        if (_.owning_thread == THREAD_LOCK_INVALID_ID) {
            _.violation_count +|= 1;
            return ThreadLockError.NotLocked;
        }
        if (_.owning_thread != thread_id) {
            _.violation_count +|= 1;
            return ThreadLockError.WrongThread;
        }
        return true;
    }

    /// Read-only ownership predicate without mutating `violation_count`.
    fn isOwnedBy(thread_id: u64): bool {
        if (!_.enabled) return true;
        return thread_id != THREAD_LOCK_INVALID_ID && _.owning_thread == thread_id;
    }

    /// Releases the lock after asserting `thread_id` is the current owner (aligned with Bun `ThreadLock.unlock`).
    fn unlock(*_, thread_id: u64): bool ! ThreadLockError {
        if (!_.enabled) return true;
        * = _.assertLocked(thread_id)?;
        _.owning_thread = THREAD_LOCK_INVALID_ID;
        return true;
    }

    /// Acquires the lock if unlocked; otherwise asserts `thread_id` already owns it
    /// (aligned with Bun `ThreadLock.lockOrAssert`).
    fn lockOrAssert(*_, thread_id: u64): bool ! ThreadLockError {
        if (!_.enabled) return true;
        if (_.owning_thread == THREAD_LOCK_INVALID_ID) {
            return _.lock(thread_id);
        }
        return _.assertLocked(thread_id);
    }

    /// Explicitly transfers ownership from `from_thread` to `to_thread` when handing a resource across threads.
    fn transfer(*_, from_thread: u64, to_thread: u64): bool ! ThreadLockError {
        if (!_.enabled) return true;
        if (to_thread == THREAD_LOCK_INVALID_ID) {
            _.violation_count +|= 1;
            return ThreadLockError.InvalidThreadId;
        }
        * = _.assertLocked(from_thread)?;
        _.owning_thread = to_thread;
        _.acquire_count +|= 1;
        return true;
    }
}
