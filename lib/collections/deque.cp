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

        pub static fn init(): Self {
            return Self{};
        }

        pub fn len(): usize {
            return _.count;
        }

        pub fn capacity(): usize {
            * = _;
            return N;
        }

        pub fn isEmpty(): bool {
            return _.count == 0;
        }

        pub fn isFull(): bool {
            return _.count == N;
        }

        pub fn clear(*_) {
            _.head = 0;
            _.tail = 0;
            _.count = 0;
        }

        pub fn pushFront(*_, item: T): CollectionError!void {
            if (_.count == N) {
                return CollectionError.Full;
            }
            _.head = if (_.head == 0) N - 1 else _.head - 1;
            _.buf[_.head] = item;
            _.count += 1;
        }

        pub fn pushBack(*_, item: T): CollectionError!void {
            if (_.count == N) {
                return CollectionError.Full;
            }
            _.buf[_.tail] = item;
            _.tail = (_.tail + 1) % N;
            _.count += 1;
        }

        pub fn popFront(*_): T? {
            if (_.count == 0) {
                return null;
            }
            const val = _.buf[_.head];
            _.head = (_.head + 1) % N;
            _.count -= 1;
            return val;
        }

        pub fn popBack(*_): T? {
            if (_.count == 0) {
                return null;
            }
            _.tail = if (_.tail == 0) N - 1 else _.tail - 1;
            _.count -= 1;
            return _.buf[_.tail];
        }

        pub fn peekFront(): T? {
            if (_.count == 0) {
                return null;
            }
            return _.buf[_.head];
        }

        pub fn peekBack(): T? {
            if (_.count == 0) {
                return null;
            }
            const idx = if (_.tail == 0) N - 1 else _.tail - 1;
            return _.buf[idx];
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

        pub static fn init(alloc: std.mem.Allocator): Self {
            return Self{
                alloc: alloc,
                buf: [],
                head: 0,
                tail: 0,
                count: 0,
            };
        }

        pub fn deinit(*_) {
            if (_.buf.len > 0) {
                _.alloc.free(_.buf);
                _.buf = [];
            }
            _.head = 0;
            _.tail = 0;
            _.count = 0;
        }

        pub fn len(): usize {
            return _.count;
        }

        pub fn isEmpty(): bool {
            return _.count == 0;
        }

        pub fn clear(*_) {
            _.head = 0;
            _.tail = 0;
            _.count = 0;
        }

        fn ensureCapacity(*_): !void {
            if (_.count < _.buf.len) {
                return;
            }
            const old_cap = _.buf.len;
            const new_cap = if (old_cap == 0) 8 else old_cap * 2;
            let new_buf = try _.alloc.alloc(T, new_cap);
            if (_.count > 0) {
                for (i of 0..this.count) {
                    const src_idx = (_.head + i) % old_cap;
                    new_buf[i] = _.buf[src_idx];
                }
            }
            if (old_cap > 0) {
                _.alloc.free(_.buf);
            }
            _.buf = new_buf;
            _.head = 0;
            _.tail = _.count;
        }

        pub fn pushFront(*_, item: T): !void {
            try _.ensureCapacity();
            const cap = _.buf.len;
            _.head = if (_.head == 0) cap - 1 else _.head - 1;
            _.buf[_.head] = item;
            _.count += 1;
        }

        pub fn pushBack(*_, item: T): !void {
            try _.ensureCapacity();
            const cap = _.buf.len;
            _.buf[_.tail] = item;
            _.tail = (_.tail + 1) % cap;
            _.count += 1;
        }

        pub fn popFront(*_): T? {
            if (_.count == 0) {
                return null;
            }
            const val = _.buf[_.head];
            _.head = (_.head + 1) % _.buf.len;
            _.count -= 1;
            return val;
        }

        pub fn popBack(*_): T? {
            if (_.count == 0) {
                return null;
            }
            _.tail = if (_.tail == 0) _.buf.len - 1 else _.tail - 1;
            _.count -= 1;
            return _.buf[_.tail];
        }

        pub fn peekFront(): T? {
            if (_.count == 0) {
                return null;
            }
            return _.buf[_.head];
        }

        pub fn peekBack(): T? {
            if (_.count == 0) {
                return null;
            }
            const idx = if (_.tail == 0) _.buf.len - 1 else _.tail - 1;
            return _.buf[idx];
        }
    };
}
