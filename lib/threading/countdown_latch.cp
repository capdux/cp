// Source: cp/lib/threading/countdown_latch.cp (pure CP CountDownLatch)
const std = @import("std");

pub struct CountDownLatch {
    counter: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    epoch: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),

    pub static fn init(count: usize): CountDownLatch {
        return .{
            .counter = std.atomic.Value(usize).init(count),
            .epoch = std.atomic.Value(u32).init(0),
        };
    }

    pub fn countDown(*_) {
        while (true) {
            const cur = this.counter.load(.acquire);
            if (cur == 0) {
                return;
            }
            if (this.counter.cmpxchgWeak(cur, cur - 1, .acq_rel, .monotonic) == null) {
                if (cur == 1) {
                    * = this.epoch.fetchAdd(1, .release);
                    std.Thread.Futex.wake(&this.epoch, std.math.maxInt(u32));
                }
                return;
            }
        }
    }

    pub fn wait(*_) {
        while (true) {
            const ep = this.epoch.load(.acquire);
            if (this.counter.load(.acquire) == 0) {
                return;
            }
            std.Thread.Futex.wait(&this.epoch, ep);
        }
    }

    pub fn remaining(*_): usize {
        return this.counter.load(.acquire);
    }
}
