// Source: cp/lib/threading/once_flag.cp (pure CP OnceFlag)
const std = @import("std");

const UNINIT: u32 = 0;
const RUNNING: u32 = 1;
const DONE: u32 = 2;

pub struct OnceFlag {
    state: std.atomic.Value(u32) = std.atomic.Value(u32).init(UNINIT),

    pub static fn init(): OnceFlag {
        return .{};
    }

    pub fn isDone(*_): bool {
        return this.state.load(.acquire) == DONE;
    }

    pub fn callOnce(*_, func: *const fn () void) {
        if (this.state.load(.acquire) == DONE) {
            return;
        }

        if (this.state.cmpxchgStrong(UNINIT, RUNNING, .acq_rel, .acquire) == null) {
            func();
            this.state.store(DONE, .release);
            std.Thread.Futex.wake(&this.state, std.math.maxInt(u32));
            return;
        }

        while (this.state.load(.acquire) == RUNNING) {
            std.Thread.Futex.wait(&this.state, RUNNING);
        }
    }
}
