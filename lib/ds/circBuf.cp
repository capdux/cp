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

        pub fn deinit(self: *Self) {
            self.alloc.free(self.buf);
            self.count = 0;
        }

        pub fn capacity(self: Self): usize {
            return self.buf.len;
        }

        pub fn len(self: Self): usize {
            return self.count;
        }

        pub fn isEmpty(self: Self): bool {
            return self.count == 0;
        }

        pub fn isFull(self: Self): bool {
            return self.count == self.buf.len;
        }

        pub fn clear(self: *Self) {
            self.head = 0;
            self.tail = 0;
            self.count = 0;
        }

        pub fn push(self: *Self, item: T): RingError!void {
            if (self.count == self.buf.len) { return RingError.BufferFull; }
            self.buf[self.tail] = item;
            self.tail = (self.tail + 1) % self.buf.len;
            self.count += 1;
        }

        /// Overwrites oldest element when full, returning the evicted element if any
        pub fn pushOverwrite(self: *Self, item: T): T? {
            if (self.count == self.buf.len) {
                const evicted = self.buf[self.head];
                self.buf[self.head] = item;
                self.head = (self.head + 1) % self.buf.len;
                self.tail = self.head;
                return evicted;
            }
            self.buf[self.tail] = item;
            self.tail = (self.tail + 1) % self.buf.len;
            self.count += 1;
            return null;
        }

        pub fn popFront(self: *Self): T? {
            if (self.count == 0) { return null; }
            const val = self.buf[self.head];
            self.head = (self.head + 1) % self.buf.len;
            self.count -= 1;
            return val;
        }

        pub fn popBack(self: *Self): T? {
            if (self.count == 0) { return null; }
            self.tail = if (self.tail == 0) self.buf.len - 1 else self.tail - 1;
            self.count -= 1;
            return self.buf[self.tail];
        }

        pub fn peekFront(self: Self): T? {
            if (self.count == 0) { return null; }
            return self.buf[self.head];
        }

        pub fn peekBack(self: Self): T? {
            if (self.count == 0) { return null; }
            const idx = if (self.tail == 0) self.buf.len - 1 else self.tail - 1;
            return self.buf[idx];
        }

        pub fn get(self: Self, idx: usize): RingError!T {
            if (idx >= self.count) { return RingError.IndexOutOfBounds; }
            const real_idx = (self.head + idx) % self.buf.len;
            return self.buf[real_idx];
        }

        pub fn toOwnedSlice(self: Self, alloc: std.mem.Allocator): ![]T {
            let out = try alloc.alloc(T, self.count);
            let i: usize = 0;
            while (i < self.count) : (i += 1) {
                out[i] = self.buf[(self.head + i) % self.buf.len];
            }
            return out;
        }
    };
}
