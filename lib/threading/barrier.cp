// Source: cp/lib/threading/barrier.cp (pure CP reusable Barrier)
const std = @import("std");

pub struct Barrier {
    parties: usize,
    arrived: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    generation: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),

    pub static fn init(parties: usize): Barrier {
        return .{
            .parties = if (parties == 0) 1 else parties,
            .arrived = std.atomic.Value(usize).init(0),
            .generation = std.atomic.Value(u32).init(0),
        };
    }

    pub fn wait(*_) {
        const gen = this.generation.load(.acquire);
        const prev = this.arrived.fetchAdd(1, .acq_rel);
        if (prev + 1 == this.parties) {
            this.arrived.store(0, .release);
            * = this.generation.fetchAdd(1, .release);
            std.Thread.Futex.wake(&this.generation, std.math.maxInt(u32));
            return;
        }
        while (this.generation.load(.acquire) == gen) {
            std.Thread.Futex.wait(&this.generation, gen);
        }
    }
}
