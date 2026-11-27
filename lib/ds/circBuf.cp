// Source: cp/lib/circ_buf/circ_buf.cp (Pure CP Generic Circular Ring Buffer)
const std = @import("std");

pub const RingError = error{
    InvalidCapacity,
    BufferFull,
    BufferEmpty,
    IndexOutOfBounds,
};

pub fn RingBuffer(comptime T: type): type {
    return struct {
        buf: []T,
        head: usize,
        tail: usize,
        count: usize,
        alloc: std.mem.Allocator,

        const Self = @This();

        pub fn init(alloc: std.mem.Allocator, cap_size: usize): !Self {
            if (cap_size == 0) { return RingError.InvalidCapacity; }
            const buf = try alloc.alloc(T, cap_size);
            return Self{
                .buf = buf,
                .head = 0,
                .tail = 0,
                .count = 0,
                .alloc = alloc,
            };
        }

        pub fn deinit(*_) {
            _.alloc.free(_.buf);
            _.count = 0;
        }

        pub fn capacity(): usize {
            return _.buf.len;
        }

        pub fn len(): usize {
            return _.count;
        }

        pub fn isEmpty(): bool {
            return _.count == 0;
        }

        pub fn isFull(): bool {
            return _.count == _.buf.len;
        }

        pub fn clear(*_) {
            _.head = 0;
            _.tail = 0;
            _.count = 0;
        }

        pub fn push(*_, item: T): RingError!void {
            if (_.count == _.buf.len) { return RingError.BufferFull; }
            _.buf[_.tail] = item;
            _.tail = (_.tail + 1) % _.buf.len;
            _.count += 1;
        }

        /// Overwrites oldest element when full, returning the evicted element if any
        pub fn pushOverwrite(*_, item: T): T? {
            if (_.count == _.buf.len) {
                const evicted = _.buf[_.head];
                _.buf[_.head] = item;
                _.head = (_.head + 1) % _.buf.len;
                _.tail = _.head;
                return evicted;
            }
            _.buf[_.tail] = item;
            _.tail = (_.tail + 1) % _.buf.len;
            _.count += 1;
            return null;
        }

        pub fn popFront(*_): T? {
            if (_.count == 0) { return null; }
            const val = _.buf[_.head];
            _.head = (_.head + 1) % _.buf.len;
            _.count -= 1;
            return val;
        }

        pub fn popBack(*_): T? {
            if (_.count == 0) { return null; }
            _.tail = if (_.tail == 0) _.buf.len - 1 else _.tail - 1;
            _.count -= 1;
            return _.buf[_.tail];
        }

        pub fn peekFront(): T? {
            if (_.count == 0) { return null; }
            return _.buf[_.head];
        }

        pub fn peekBack(): T? {
            if (_.count == 0) { return null; }
            const idx = if (_.tail == 0) _.buf.len - 1 else _.tail - 1;
            return _.buf[idx];
        }

        pub fn get(idx: usize): RingError!T {
            if (idx >= _.count) { return RingError.IndexOutOfBounds; }
            const real_idx = (_.head + idx) % _.buf.len;
            return _.buf[real_idx];
        }

        pub fn toOwnedSlice(alloc: std.mem.Allocator): ![]T {
            let out = try alloc.alloc(T, _.count);
            let i: usize = 0;
            while (i < _.count) : (i += 1) {
                out[i] = _.buf[(_.head + i) % _.buf.len];
            }
            return out;
        }
    };
}
