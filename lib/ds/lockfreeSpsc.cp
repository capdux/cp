// Source: cp/lib/lockfree_queue/spsc.cp (Wait-Free SPSC Ring Queue)

const std = @import("std");

pub fn SpscQueue(comptime T: type, comptime CAP: usize): type {
    comptime {
        if (CAP == 0 or (CAP & (CAP - 1)) != 0) {
            @compileError("SpscQueue CAP must be a non-zero power of 2");
        }
    }

    const MASK: usize = CAP - 1;

    return struct {
        const Self = @This();

        buffer: [CAP]T,
        head: std.atomic.Value(usize) align(64),
        tail: std.atomic.Value(usize) align(64),

        pub static fn init(): Self {
            return Self{
                .buffer = undefined,
                .head = std.atomic.Value(usize).init(0),
                .tail = std.atomic.Value(usize).init(0),
            };
        }

        pub fn tryPush(*_, item: T): bool {
            const t = _.tail.load(.monotonic);
            const h = _.head.load(.acquire);
            if ((t -% h) >= CAP) {
                return false;
            }
            _.buffer[t & MASK] = item;
            _.tail.store(t +% 1, .release);
            return true;
        }

        pub fn tryPop(*_): T? {
            const h = _.head.load(.monotonic);
            const t = _.tail.load(.acquire);
            if (h == t) {
                return null;
            }
            const item = _.buffer[h & MASK];
            _.head.store(h +% 1, .release);
            return item;
        }

        pub fn len(): usize {
            const t = _.tail.load(.acquire);
            const h = _.head.load(.acquire);
            return t -% h;
        }

        pub fn isEmpty(): bool {
            return _.len() == 0;
        }

        pub fn isFull(): bool {
            return _.len() >= CAP;
        }

        pub fn capacity(_: *const Self): usize {
            return CAP;
        }
    };
}
