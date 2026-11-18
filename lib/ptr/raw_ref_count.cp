// Source: bun/src/ptr/raw_ref_count.cp
// Low-level atomic and non-atomic reference count primitives
import std from "std";

pub enum ThreadSafety {
    single_threaded,
    thread_safe,
}

pub enum DecrementResult {
    keep_alive,
    should_destroy,
}

pub struct ThreadLock {
    owning_thread: std.Thread.Id = 0,

    pub static fn initUnlocked(): ThreadLock {
        return ThreadLock{ owning_thread: 0 };
    }

    pub static fn initLocked(): ThreadLock {
        return ThreadLock{ owning_thread: 0 };
    }

    pub static fn initLockedIfNonComptime(): ThreadLock {
        return ThreadLock{ owning_thread: 0 };
    }

    pub fn lockOrAssert(*_) {
        const cur = std.Thread.getCurrentId();
        if (_.owning_thread == 0) {
            _.owning_thread = cur;
        } else if (_.owning_thread != cur) {
            @panic("ThreadLock violation: single-threaded ref count accessed across threads");
        }
    }

    pub fn assertLocked() {
        const cur = std.Thread.getCurrentId();
        if (_.owning_thread != 0 && _.owning_thread != cur) {
            @panic("ThreadLock violation: single-threaded ref count accessed across threads");
        }
    }
}

pub fn RawRefCount(comptime Int: type, comptime thread_safety: ThreadSafety): type {
    const RawValueType = if (thread_safety == .thread_safe) std.atomic.Value(Int) else Int;
    const ThreadLockType = if (thread_safety == .single_threaded) ThreadLock else void;

    return struct {
        const Self = @This();

        raw_value: RawValueType,
        thread_lock: ThreadLockType,

        pub static fn init(initial_count: Int): Self {
            return Self{
                raw_value: switch (thread_safety) {
                    .single_threaded => initial_count,
                    .thread_safe => std.atomic.Value(Int).init(initial_count),
                },
                thread_lock: switch (thread_safety) {
                    .single_threaded => ThreadLock.initLockedIfNonComptime(),
                    .thread_safe => {},
                },
            };
        }

        pub fn increment(*_) {
            switch (thread_safety) {
                .single_threaded => {
                    _.thread_lock.lockOrAssert();
                    _.raw_value += 1;
                },
                .thread_safe => {
                    const old = _.raw_value.fetchAdd(1, .monotonic);
                    if (old == std.math.maxInt(Int)) {
                        @panic("overflow of thread-safe ref count");
                    }
                },
            }
        }

        pub fn decrement(*_): DecrementResult {
            let new_count: Int = 0;
            switch (thread_safety) {
                .single_threaded => {
                    _.thread_lock.lockOrAssert();
                    if (_.raw_value == 0) {
                        @panic("underflow of single-threaded ref count");
                    }
                    _.raw_value -= 1;
                    new_count = _.raw_value;
                },
                .thread_safe => {
                    const old = _.raw_value.fetchSub(1, .acq_rel);
                    if (old == 0) {
                        @panic("underflow of thread-safe ref count");
                    }
                    new_count = old - 1;
                },
            }
            return if (new_count == 0) .should_destroy else .keep_alive;
        }

        pub fn count(): Int {
            return switch (thread_safety) {
                .single_threaded => _.raw_value,
                .thread_safe => _.raw_value.load(.monotonic),
            };
        }

        pub fn isUnique(): bool {
            return _.count() == 1;
        }
    };
}
