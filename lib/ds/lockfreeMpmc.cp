// Source: cp/lib/lockfree_queue/mpmc.cp (Dmitry Vyukov Bounded MPMC Lock-Free Queue)

const std = @import("std");

pub fn MpmcQueue(comptime T: type, comptime CAP: usize): type {
    comptime {
        if (CAP < 2 or (CAP & (CAP - 1)) != 0) {
            @compileError("MpmcQueue CAP must be a power of 2 >= 2");
        }
    }

    const MASK: usize = CAP - 1;

    const Slot = struct {
        sequence: std.atomic.Value(usize) align(64),
        data: T,
    };

    return struct {
        const Self = @This();

        buffer: [CAP]Slot,
        enqueue_pos: std.atomic.Value(usize) align(64),
        dequeue_pos: std.atomic.Value(usize) align(64),

        pub fn init(): Self {
            let q: Self = undefined;
            q.enqueue_pos = std.atomic.Value(usize).init(0);
            q.dequeue_pos = std.atomic.Value(usize).init(0);
            let i: usize = 0;
            while (i < CAP) {
                q.buffer[i].sequence = std.atomic.Value(usize).init(i);
                i += 1;
            }
            return q;
        }

        pub fn tryPush(self: *Self, item: T): bool {
            let pos = self.enqueue_pos.load(.monotonic);
            while (true) {
                let slot = &self.buffer[pos & MASK];
                const seq = slot.sequence.load(.acquire);
                const diff = @as(isize, @bitCast(seq)) -% @as(isize, @bitCast(pos));
                if (diff == 0) {
                    const res = self.enqueue_pos.cmpxchgWeak(pos, pos +% 1, .monotonic, .monotonic);
                    if (res == null) {
                        slot.data = item;
                        slot.sequence.store(pos +% 1, .release);
                        return true;
                    } else {
                        pos = res.?;
                    }
                } else if (diff < 0) {
                    return false;
                } else {
                    pos = self.enqueue_pos.load(.monotonic);
                }
            }
        }

        pub fn tryPop(self: *Self): T? {
            let pos = self.dequeue_pos.load(.monotonic);
            while (true) {
                const slot = &self.buffer[pos & MASK];
                const seq = slot.sequence.load(.acquire);
                const diff = @as(isize, @bitCast(seq)) -% @as(isize, @bitCast(pos +% 1));
                if (diff == 0) {
                    const res = self.dequeue_pos.cmpxchgWeak(pos, pos +% 1, .monotonic, .monotonic);
                    if (res == null) {
                        const item = slot.data;
                        slot.sequence.store(pos +% CAP, .release);
                        return item;
                    } else {
                        pos = res.?;
                    }
                } else if (diff < 0) {
                    return null;
                } else {
                    pos = self.dequeue_pos.load(.monotonic);
                }
            }
        }

        pub fn len(self: *const Self): usize {
            const enq = self.enqueue_pos.load(.acquire);
            const deq = self.dequeue_pos.load(.acquire);
            const diff = enq -% deq;
            if (diff > CAP) {
                return CAP;
            }
            return diff;
        }

        pub fn isEmpty(self: *const Self): bool {
            return self.len() == 0;
        }

        pub fn isFull(self: *const Self): bool {
            return self.len() >= CAP;
        }

        pub fn capacity(_: *const Self): usize {
            return CAP;
        }
    };
}
