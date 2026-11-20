// Source: bun/src/collections/bounded_array.cp -> cp/lib/collections/deque.cp (BoundedDeque & RingDeque)
const std = @import("std");
import { CollectionError } from "./errors.cp";

pub fn BoundedDeque(comptime T: type, comptime N: usize): type {
    return struct {
        const Self = @This();

        buf: [N]T = undefined,
        head: usize = 0,
        tail: usize = 0,
        count: usize = 0,

        pub fn init(): Self {
            return Self{};
        }

        pub fn len(this): usize {
            return this.count;
        }

        pub fn capacity(this): usize {
            _ = this;
            return N;
        }

        pub fn isEmpty(this): bool {
            return this.count == 0;
        }

        pub fn isFull(this): bool {
            return this.count == N;
        }

        pub fn clear(*this): void {
            this.head = 0;
            this.tail = 0;
            this.count = 0;
        }

        pub fn pushFront(*this, item: T): CollectionError!void {
            if (this.count == N) {
                return CollectionError.Full;
            }
            this.head = if (this.head == 0) N - 1 else this.head - 1;
            this.buf[this.head] = item;
            this.count += 1;
        }

        pub fn pushBack(*this, item: T): CollectionError!void {
            if (this.count == N) {
                return CollectionError.Full;
            }
            this.buf[this.tail] = item;
            this.tail = (this.tail + 1) % N;
            this.count += 1;
        }

        pub fn popFront(*this): T? {
            if (this.count == 0) {
                return null;
            }
            const val = this.buf[this.head];
            this.head = (this.head + 1) % N;
            this.count -= 1;
            return val;
        }

        pub fn popBack(*this): T? {
            if (this.count == 0) {
                return null;
            }
            this.tail = if (this.tail == 0) N - 1 else this.tail - 1;
            this.count -= 1;
            return this.buf[this.tail];
        }

        pub fn peekFront(this): T? {
            if (this.count == 0) {
                return null;
            }
            return this.buf[this.head];
        }

        pub fn peekBack(this): T? {
            if (this.count == 0) {
                return null;
            }
            const idx = if (this.tail == 0) N - 1 else this.tail - 1;
            return this.buf[idx];
        }
    };
}

pub fn RingDeque(comptime T: type): type {
    return struct {
        const Self = @This();

        alloc: std.mem.Allocator,
        buf: []T,
        head: usize,
        tail: usize,
        count: usize,

        pub fn init(alloc: std.mem.Allocator): Self {
            return Self{
                alloc: alloc,
                buf: [],
                head: 0,
                tail: 0,
                count: 0,
            };
        }

        pub fn deinit(*this): void {
            if (this.buf.len > 0) {
                this.alloc.free(this.buf);
                this.buf = [];
            }
            this.head = 0;
            this.tail = 0;
            this.count = 0;
        }

        pub fn len(this): usize {
            return this.count;
        }

        pub fn isEmpty(this): bool {
            return this.count == 0;
        }

        pub fn clear(*this): void {
            this.head = 0;
            this.tail = 0;
            this.count = 0;
        }

        fn ensureCapacity(*this): !void {
            if (this.count < this.buf.len) {
                return;
            }
            const old_cap = this.buf.len;
            const new_cap = if (old_cap == 0) 8 else old_cap * 2;
            let new_buf = try this.alloc.alloc(T, new_cap);
            if (this.count > 0) {
                for (i of 0..this.count) {
                    const src_idx = (this.head + i) % old_cap;
                    new_buf[i] = this.buf[src_idx];
                }
            }
            if (old_cap > 0) {
                this.alloc.free(this.buf);
            }
            this.buf = new_buf;
            this.head = 0;
            this.tail = this.count;
        }

        pub fn pushFront(*this, item: T): !void {
            try this.ensureCapacity();
            const cap = this.buf.len;
            this.head = if (this.head == 0) cap - 1 else this.head - 1;
            this.buf[this.head] = item;
            this.count += 1;
        }

        pub fn pushBack(*this, item: T): !void {
            try this.ensureCapacity();
            const cap = this.buf.len;
            this.buf[this.tail] = item;
            this.tail = (this.tail + 1) % cap;
            this.count += 1;
        }

        pub fn popFront(*this): T? {
            if (this.count == 0) {
                return null;
            }
            const val = this.buf[this.head];
            this.head = (this.head + 1) % this.buf.len;
            this.count -= 1;
            return val;
        }

        pub fn popBack(*this): T? {
            if (this.count == 0) {
                return null;
            }
            this.tail = if (this.tail == 0) this.buf.len - 1 else this.tail - 1;
            this.count -= 1;
            return this.buf[this.tail];
        }

        pub fn peekFront(this): T? {
            if (this.count == 0) {
                return null;
            }
            return this.buf[this.head];
        }

        pub fn peekBack(this): T? {
            if (this.count == 0) {
                return null;
            }
            const idx = if (this.tail == 0) this.buf.len - 1 else this.tail - 1;
            return this.buf[idx];
        }
    };
}
