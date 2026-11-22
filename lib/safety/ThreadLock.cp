// Source: Absorbed from /data/dev/bun/src/safety/ThreadLock.cp | Synced: 2026-11-22T12:49:17Z
import std from "std";
import { invalid } from "./threadId.cp";

const thread_lock_enabled: bool = true;

export struct ThreadLock {
    owning_thread: std.Thread.Id = invalid,

    pub static fn initUnlocked(): ThreadLock {
        return ThreadLock{ owning_thread: invalid };
    }

    pub static fn initLocked(): ThreadLock {
        let lock_val = ThreadLock.initUnlocked();
        lock_val.lock();
        return lock_val;
    }

    pub fn lock(*_) {
        const current = std.Thread.getCurrentId();
        if (_.owning_thread != invalid) {
            @panic("tried to lock ThreadLock, but was already locked");
        }
        _.owning_thread = current;
    }

    pub fn unlock(*_) {
        _.assertLocked();
        _.owning_thread = invalid;
    }

    pub fn assertLocked() {
        if (_.owning_thread == invalid) {
            @panic("ThreadLock is not locked");
        }
        const current = std.Thread.getCurrentId();
        if (_.owning_thread != current) {
            @panic("ThreadLock is locked by another thread");
        }
    }

    pub fn assertNotLocked() {
        if (_.owning_thread != invalid) {
            @panic("ThreadLock is unexpectedly locked");
        }
    }

    pub fn lockOrAssert(*_) {
        if (_.owning_thread == invalid) {
            _.lock();
        } else {
            _.assertLocked();
        }
    }

    pub fn isLocked(): bool {
        return _.owning_thread != invalid;
    }

    pub fn isHeldByCurrentThread(): bool {
        return _.owning_thread == std.Thread.getCurrentId();
    }
}
