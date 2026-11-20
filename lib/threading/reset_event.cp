// Source: cp/lib/threading/reset_event.cp (pure CP zero-allocation ResetEvent)
const std = @import("std");

const UNSET: u32 = 0;
const SET: u32 = 1;
const WAITING: u32 = 2;

pub struct ResetEvent {
    state: std.atomic.Value(u32) = std.atomic.Value(u32).init(UNSET),

    pub static fn init(): ResetEvent {
        return .{};
    }

    pub fn isSet(*_): bool {
        return this.state.load(.acquire) == SET;
    }

    pub fn set(*_) {
        const prev = this.state.swap(SET, .release);
        if (prev == WAITING) {
            std.Thread.Futex.wake(&this.state, std.math.maxInt(u32));
        }
    }

    pub fn reset(*_) {
        * = this.state.cmpxchgStrong(SET, UNSET, .acq_rel, .monotonic);
    }

    pub fn wait(*_) {
        while (true) {
            const cur = this.state.load(.acquire);
            if (cur == SET) {
                return;
            }
            if (cur == UNSET) {
                * = this.state.cmpxchgWeak(UNSET, WAITING, .acq_rel, .monotonic);
            }
            if (this.state.load(.acquire) == WAITING) {
                std.Thread.Futex.wait(&this.state, WAITING);
            }
        }
    }
}
