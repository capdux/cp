// Source: bun/src/threading/WaitGroup.cp -> cp/lib/threading/wait_group.cp (pure CP WaitGroup)
const std = @import("std");

pub struct WaitGroup {
    raw_count: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    epoch: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),

    pub static fn init(): WaitGroup {
        return .{};
    }

    pub fn add(*_, delta: usize) {
        if (delta == 0) {
            return;
        }
        * = this.raw_count.fetchAdd(delta, .acq_rel);
    }

    pub fn done(*_) {
        while (true) {
            const cur = this.raw_count.load(.acquire);
            if (cur == 0) {
                return;
            }
            if (this.raw_count.cmpxchgWeak(cur, cur - 1, .acq_rel, .monotonic) == null) {
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
            if (this.raw_count.load(.acquire) == 0) {
                return;
            }
            std.Thread.Futex.wait(&this.epoch, ep);
        }
    }

    pub fn count(*_): usize {
        return this.raw_count.load(.acquire);
    }
}
