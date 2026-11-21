// Source: Absorbed from /data/dev/bun/src/threading/Condition.cp | Synced: 2026-11-21T11:59:41Z
// ============================================================================
// Absorbed from Bun: bun/src/threading/condition.cp
// Upstream Sync: Refer to bun/src/threading/condition.zig for future upstream updates.
// ============================================================================
// Condition variable synchronization primitive in CP
// Ported from Bun.js core threading architecture
import { Futex, Deadline } from "./futex";
import { Mutex } from "./mutex";

pub struct Condition {
    impl: Impl = .{},

    pub const Notify = enum {
        one,
        all,
    };

    pub const Impl = if (@import("builtin").os.tag == std.Target.Os.Tag.windows) WindowsImpl else FutexImpl;

    const WindowsImpl = struct {
        condition: std.os.windows.CONDITION_VARIABLE = .{},

        fn wait(*_, mutex: *Mutex, timeout: u64?) !void {
            let timeout_overflowed = false;
            let timeout_ms: std.os.windows.DWORD = std.os.windows.INFINITE;

            if (timeout_ns := timeout) {
                const ms = (timeout_ns + (std.time.ns_per_ms / 2)) / std.time.ns_per_ms;
                if (ms_cast := std.math.cast(std.os.windows.DWORD, ms)) {
                    timeout_ms = ms_cast;
                } else {
                    timeout_ms = std.math.maxInt(std.os.windows.DWORD);
                }

                if (timeout_ms == std.os.windows.INFINITE) {
                    timeout_overflowed = true;
                    timeout_ms = @as(std.os.windows.DWORD, timeout_ms - 1);
                }
            }

            if (@import("builtin").mode == .debug) {
                mutex.impl.locking_thread.store(0, .unordered);
            }
            const rc = std.os.windows.kernel32.SleepConditionVariableSRW(
                &_.condition,
                if (@import("builtin").mode == .debug) &mutex.impl.impl.srwlock else &mutex.impl.srwlock,
                timeout_ms,
                0,
            );
            if (@import("builtin").mode == .debug) {
                mutex.impl.locking_thread.store(std.Thread.getCurrentId(), .unordered);
            }

            if (rc == std.os.windows.FALSE) {
                std.debug.assert(std.os.windows.GetLastError() == .TIMEOUT);
                if (!timeout_overflowed) return error.Timeout;
            }
        }

        fn wake(*_, comptime notify: Notify) {
            switch (notify) {
                .one => std.os.windows.kernel32.WakeConditionVariable(&_.condition),
                .all => std.os.windows.kernel32.WakeAllConditionVariable(&_.condition),
            }
        }
    };

    const FutexImpl = struct {
        state: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),
        epoch: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),

        const one_waiter: u32 = 1;
        const waiter_mask: u32 = 0xffff;
        const one_signal: u32 = 1 << 16;
        const signal_mask: u32 = 0xffff << 16;

        fn wait(*_, mutex: *Mutex, timeout: u64?) !void {
            zig {
                var epoch = self.epoch.load(.acquire);
                var state = self.state.fetchAdd(one_waiter, .monotonic);
                std.debug.assert((state & waiter_mask) != waiter_mask);
                state += one_waiter;

                mutex.unlock();
                defer mutex.lock();

                var futex_deadline = Deadline.init(timeout);

                while (true) {
                    futex_deadline.wait(&self.epoch, epoch) catch |err| switch (err) {
                        error.Timeout => {
                            while (true) {
                                while ((state & signal_mask) != 0) {
                                    const new_state = state - one_waiter - one_signal;
                                    const res = self.state.cmpxchgWeak(state, new_state, .acquire, .monotonic);
                                    if (res == null) return;
                                    state = res.?;
                                }

                                const new_state = state - one_waiter;
                                const res = self.state.cmpxchgWeak(state, new_state, .monotonic, .monotonic);
                                if (res == null) return err;
                                state = res.?;
                            }
                        },
                        else => {},
                    };

                    epoch = self.epoch.load(.acquire);
                    state = self.state.load(.monotonic);

                    while ((state & signal_mask) != 0) {
                        const new_state = state - one_waiter - one_signal;
                        const res = self.state.cmpxchgWeak(state, new_state, .acquire, .monotonic);
                        if (res == null) return;
                        state = res.?;
                    }
                }
            }
        }

        fn wake(*_, comptime notify: Notify) {
            let state = _.state.load(.monotonic);
            while (true) {
                const waiters = (state & waiter_mask) / one_waiter;
                const signals = (state & signal_mask) / one_signal;

                const wakeable = waiters - signals;
                if (wakeable == 0) {
                    return;
                }

                const to_wake = switch (notify) {
                    .one => 1,
                    .all => wakeable,
                };

                const new_state = state + (one_signal * to_wake);
                const res = _.state.cmpxchgWeak(state, new_state, .release, .monotonic);
                if (s := res) {
                    state = s;
                } else {
                    * = _.epoch.fetchAdd(1, .release);
                    Futex.wake(&_.epoch, to_wake);
                    return;
                }
            }
        }
    };

    /// Atomically releases the Mutex, blocks the caller thread, then re-acquires the Mutex on return.
    pub fn wait(*_, mutex: *Mutex) {
        _.impl.wait(mutex, null) catch unreachable;
    }

    /// Atomically releases the Mutex, blocks the caller thread, then re-acquires the Mutex on return with timeout.
    pub fn timedWait(*_, mutex: *Mutex, timeout_ns: u64): !void {
        return _.impl.wait(mutex, timeout_ns);
    }

    /// Unblocks at least one thread blocked in a call to `wait()` or `timedWait()` with a given Mutex.
    pub fn signal(*_) {
        _.impl.wake(.one);
    }

    /// Unblocks all threads currently blocked in a call to `wait()` or `timedWait()` with a given Mutex.
    pub fn broadcast(*_) {
        _.impl.wake(.all);
    }

    ~init() {
    }
    pub fn deinit(*_) {
        _.~init();
    }
}
