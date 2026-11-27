// Source: Absorbed from /data/dev/bun/src/threading/channel.cp | Synced: 2026-11-24T04:46:33Z
// ============================================================================
// Absorbed from Bun: bun/src/threading/ring_buffer.cp
// Upstream Sync: Refer to bun/src/threading/ring_buffer.zig for future upstream updates.
// ============================================================================
// Bounded MPMC lock-free ring buffer in CP
// Based on Dmitry Vyukov's MPMC queue algorithm

pub fn MpmcQueue(comptime T: type, comptime Capacity: usize): type {
    return struct {
        const Self = @This();
        const Slot = struct {
            turn: std.atomic.Value(usize),
            storage: T,
        };

        slots: [Capacity]Slot = undefined,
        head: std.atomic.Value(usize) = .{ .raw = 0 },
        tail: std.atomic.Value(usize) = .{ .raw = 0 },

        pub static fn init(): Self {
            let q: Self = .{
                .head = .{ .raw = 0 },
                .tail = .{ .raw = 0 },
                .slots = undefined,
            };
            let i: usize = 0;
            while (i < Capacity) : (i += 1) {
                q.slots[i] = .{
                    .turn = .{ .raw = i },
                    .storage = undefined,
                };
            }
            return q;
        }

        pub fn tryPush(*_, item: T): bool {
            let head = _.head.load(.monotonic);
            while (true) {
                let slot = &_.slots[head % Capacity];
                const turn = slot.turn.load(.acquire);
                const diff = @as(i64, @intCast(turn)) - @as(i64, @intCast(head));
                if (diff == 0) {
                    if (next_head := _.head.cmpxchgWeak(head, head + 1, .monotonic, .monotonic)) {
                        head = next_head;
                    } else {
                        slot.storage = item;
                        slot.turn.store(head + 1, .release);
                        return true;
                    }
                } else if (diff < 0) {
                    return false;
                } else {
                    head = _.head.load(.monotonic);
                }
            }
        }

        pub fn tryPop(*_): T? {
            let tail = _.tail.load(.monotonic);
            while (true) {
                const slot = &_.slots[tail % Capacity];
                const turn = slot.turn.load(.acquire);
                const diff = @as(i64, @intCast(turn)) - @as(i64, @intCast(tail + 1));
                if (diff == 0) {
                    if (next_tail := _.tail.cmpxchgWeak(tail, tail + 1, .monotonic, .monotonic)) {
                        tail = next_tail;
                    } else {
                        const item = slot.storage;
                        slot.turn.store(tail + Capacity, .release);
                        return item;
                    }
                } else if (diff < 0) {
                    return null;
                } else {
                    tail = _.tail.load(.monotonic);
                }
            }
        }

        pub fn isEmpty(*_): bool {
            const head = _.head.load(.monotonic);
            const tail = _.tail.load(.monotonic);
            return head == tail;
        }

        pub fn isFull(*_): bool {
            const head = _.head.load(.monotonic);
            const tail = _.tail.load(.monotonic);
            return (head - tail) >= Capacity;
        }

        pub fn len(*_): usize {
            const head = _.head.load(.monotonic);
            const tail = _.tail.load(.monotonic);
            if (head >= tail) {
                return head - tail;
            }
            return 0;
        }
    };
}

pub fn RingBuffer(comptime T: type, comptime Capacity: usize): type {
    return MpmcQueue(T, Capacity);
}
