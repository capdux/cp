// Source: cp/lib/threading/spin_lock.cp (pure CP adaptive SpinLock)
const std = @import("std");

const UNLOCKED: u32 = 0;
const LOCKED: u32 = 1;

pub struct SpinLock {
    state: std.atomic.Value(u32) = std.atomic.Value(u32).init(UNLOCKED),

    pub static fn init(): SpinLock {
        return .{};
    }

    pub fn tryLock(*_): bool {
        return this.state.cmpxchgWeak(UNLOCKED, LOCKED, .acquire, .monotonic) == null;
    }

    pub fn lock(*_) {
        let spins: usize = 0;
        while (this.state.cmpxchgWeak(UNLOCKED, LOCKED, .acquire, .monotonic) != null) {
            while (this.state.load(.monotonic) == LOCKED) {
                if (spins < 64) {
                    std.atomic.spinLoopHint();
                    spins += 1;
                } else {
                    std.Thread.yield() catch {};
                    spins = 0;
                }
            }
        }
    }

    pub fn unlock(*_) {
        this.state.store(UNLOCKED, .release);
    }

    pub fn isHeld(*_): bool {
        return this.state.load(.acquire) == LOCKED;
    }
}
