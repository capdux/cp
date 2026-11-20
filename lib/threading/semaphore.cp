// Source: cp/lib/threading/semaphore.cp (pure CP counting Semaphore)
const std = @import("std");

pub struct Semaphore {
    permits: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    epoch: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),

    pub static fn init(permits: usize): Semaphore {
        return .{
            .permits = std.atomic.Value(usize).init(permits),
            .epoch = std.atomic.Value(u32).init(0),
        };
    }

    pub fn tryAcquire(*_): bool {
        while (true) {
            const cur = this.permits.load(.acquire);
            if (cur == 0) {
                return false;
            }
            if (this.permits.cmpxchgWeak(cur, cur - 1, .acq_rel, .monotonic) == null) {
                return true;
            }
        }
    }

    pub fn acquire(*_) {
        while (true) {
            const ep = this.epoch.load(.acquire);
            if (this.tryAcquire()) {
                return;
            }
            if (this.permits.load(.acquire) == 0) {
                std.Thread.Futex.wait(&this.epoch, ep);
            }
        }
    }

    pub fn post(*_) {
        * = this.permits.fetchAdd(1, .release);
        * = this.epoch.fetchAdd(1, .release);
        std.Thread.Futex.wake(&this.epoch, 1);
    }

    pub fn availablePermits(*_): usize {
        return this.permits.load(.acquire);
    }
}
