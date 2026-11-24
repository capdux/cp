// Source: Absorbed from /data/dev/bun/src/threading/guarded.cp | Synced: 2026-11-24T04:46:35Z
// Zero-allocation mutex-guarded value containers (`Guarded<T>` & `GuardedBy<T, Lock>`).

import { SpinLock } from "./spinLock.cp";

/// A value synchronized by an arbitrary lock type `Lock` implementing
/// `lock(*_)`, `tryLock(*_): bool`, and `unlock(*_)`.
pub struct GuardedBy<T, Lock> {
    /// Raw protected value. Only access while holding `mutex` or when uniquely owned.
    unsynchronized_value: T,
    mutex: Lock,

    /// Create a guarded container with an explicitly provided lock instance.
    static fn initWithLock(value: T, mutex: Lock): GuardedBy<T, Lock> {
        return GuardedBy<T, Lock>{
            unsynchronized_value: value,
            mutex: mutex,
        };
    }

    /// Acquire the lock and return a mutable borrow to the protected value.
    /// Caller must pair with `unlock()`.
    fn lock(*_): *T {
        _.mutex.lock();
        return *_.unsynchronized_value;
    }

    /// Attempt to acquire the lock without blocking.
    /// Returns a mutable borrow on success, or `null` if already locked.
    fn tryLock(*_): (*T)? {
        if (!_.mutex.tryLock()) {
            return null;
        }
        return *_.unsynchronized_value;
    }

    /// Release the underlying lock.
    fn unlock(*_) {
        _.mutex.unlock();
    }

    /// Lock, read a copy of the protected value, and unlock automatically.
    fn getCopy(*_): T {
        _.mutex.lock();
        const v = _.unsynchronized_value;
        _.mutex.unlock();
        return v;
    }

    /// Lock, overwrite the protected value, and unlock automatically.
    fn set(*_, new_val: T) {
        _.mutex.lock();
        _.unsynchronized_value = new_val;
        _.mutex.unlock();
    }

    /// Lock, swap the protected value with `new_val`, unlock, and return the previous value.
    fn swap(*_, new_val: T): T {
        _.mutex.lock();
        const prev = _.unsynchronized_value;
        _.unsynchronized_value = new_val;
        _.mutex.unlock();
        return prev;
    }

    /// Extract the inner value without locking (caller must guarantee exclusive access).
    fn intoUnprotected(*_): T {
        return _.unsynchronized_value;
    }
}

/// Default mutex-guarded value container backed by `cap:threading`'s adaptive `SpinLock`.
pub struct Guarded<T> {
    /// Raw protected value. Only access while holding `mutex` or when uniquely owned.
    unsynchronized_value: T,
    mutex: SpinLock = SpinLock.init(),

    /// Create a `Guarded<T>` with a default unlocked `SpinLock`.
    static fn init(value: T): Guarded<T> {
        return Guarded<T>{
            unsynchronized_value: value,
            mutex: SpinLock.init(),
        };
    }

    /// Acquire the `SpinLock` and return a mutable borrow to the protected value.
    fn lock(*_): *T {
        _.mutex.lock();
        return *_.unsynchronized_value;
    }

    /// Attempt to acquire the `SpinLock` without blocking.
    fn tryLock(*_): (*T)? {
        if (!_.mutex.tryLock()) {
            return null;
        }
        return *_.unsynchronized_value;
    }

    /// Release the `SpinLock`.
    fn unlock(*_) {
        _.mutex.unlock();
    }

    /// Query whether the underlying `SpinLock` is currently held.
    fn isLocked(*_): bool {
        return _.mutex.isHeld();
    }

    /// Lock, read a copy of the protected value, and unlock automatically.
    fn getCopy(*_): T {
        _.mutex.lock();
        const v = _.unsynchronized_value;
        _.mutex.unlock();
        return v;
    }

    /// Lock, overwrite the protected value, and unlock automatically.
    fn set(*_, new_val: T) {
        _.mutex.lock();
        _.unsynchronized_value = new_val;
        _.mutex.unlock();
    }

    /// Lock, swap the protected value with `new_val`, unlock, and return the previous value.
    fn swap(*_, new_val: T): T {
        _.mutex.lock();
        const prev = _.unsynchronized_value;
        _.unsynchronized_value = new_val;
        _.mutex.unlock();
        return prev;
    }

    /// Extract the inner value without locking (caller must guarantee exclusive access).
    fn intoUnprotected(*_): T {
        return _.unsynchronized_value;
    }
}
