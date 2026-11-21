// Source: cp/lib/lockfree_queue/spsc.cp (Wait-Free SPSC Ring Queue)

const std = @import("std");

pub fn SpscQueue(comptime T: type, comptime CAP: usize): type {
    comptime {
        if (CAP == 0 or (CAP & (CAP - 1)) != 0) {
            @compileError("SpscQueue CAP must be a non-zero power of 2");
        }
    }

    const MASK: usize = CAP - 1;
    const OptHolder = struct {
        val: T?,
    };
    const OptT = @TypeOf(@as(OptHolder, undefined).val);

    return struct {
        const Self = @This();

        buffer: [CAP]T,
        head: std.atomic.Value(usize) align(64),
        tail: std.atomic.Value(usize) align(64),

        pub fn init(): Self {
            return Self{
                .buffer = undefined,
                .head = std.atomic.Value(usize).init(0),
                .tail = std.atomic.Value(usize).init(0),
            };
        }

        pub fn tryPush(self: *Self, item: T): bool {
            const t = self.tail.load(.monotonic);
            const h = self.head.load(.acquire);
            if ((t -% h) >= CAP) {
                return false;
            }
            self.buffer[t & MASK] = item;
            self.tail.store(t +% 1, .release);
            return true;
        }

        pub fn tryPop(self: *Self): OptT {
            const h = self.head.load(.monotonic);
            const t = self.tail.load(.acquire);
            if (h == t) {
                return null;
            }
            const item = self.buffer[h & MASK];
            self.head.store(h +% 1, .release);
            return item;
        }

        pub fn len(self: *const Self): usize {
            const t = self.tail.load(.acquire);
            const h = self.head.load(.acquire);
            return t -% h;
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
