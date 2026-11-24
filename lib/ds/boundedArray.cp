// Source: Absorbed from /data/dev/bun/src/collections/bounded_array.cp | Synced: 2026-11-24T07:05:00Z
//! Stack-allocated fixed-capacity dynamic array (`BoundedArray<T, capacity>`),
//! zero heap allocations, with full slice/append/insert/remove operations.

import std from "std";

export err BoundedArrayError {
    Overflow,
    OutOfBounds,
}

export fn BoundedArray(comptime T: type, comptime buffer_capacity: usize): type {
    return struct {
        const Self = @This();
        pub const CAPACITY: usize = buffer_capacity;

        buffer: [buffer_capacity]T = undefined,
        len: usize = 0,

        pub static fn empty(): Self {
            return Self{ buffer: undefined, len: 0 };
        }

        pub static fn init(len: usize): Self! {
            if (len > buffer_capacity) return err.Overflow;
            return Self{ buffer: undefined, len: len };
        }

        pub static fn fromSlice(items: []const T): Self! {
            if (items.len > buffer_capacity) return err.Overflow;
            let list = Self{ buffer: undefined, len: items.len };
            if (items.len > 0) {
                @memcpy(list.buffer[0..items.len], items);
            }
            return list;
        }

        pub fn slice(*_): []T {
            return _.buffer[0.._.len];
        }

        pub fn constSlice(): []const T {
            return _.buffer[0.._.len];
        }

        pub fn capacity(): usize {
            return buffer_capacity;
        }

        pub fn isEmpty(): bool {
            return _.len == 0;
        }

        pub fn isFull(): bool {
            return _.len == buffer_capacity;
        }

        pub fn remainingCapacity(): usize {
            return buffer_capacity - _.len;
        }

        pub fn resize(*_, new_len: usize): void! {
            if (new_len > buffer_capacity) return err.Overflow;
            _.len = new_len;
        }

        pub fn clear(*_) {
            _.len = 0;
        }

        pub fn get(index: usize): T {
            return _.buffer[index];
        }

        pub fn getChecked(index: usize): T? {
            if (index >= _.len) return null;
            return _.buffer[index];
        }

        pub fn set(*_, index: usize, item: T): void! {
            if (index >= _.len) return err.OutOfBounds;
            _.buffer[index] = item;
        }

        pub fn append(*_, item: T): void! {
            if (_.len >= buffer_capacity) return err.Overflow;
            _.buffer[_.len] = item;
            _.len += 1;
        }

        pub fn appendSlice(*_, items: []const T): void! {
            if (items.len > buffer_capacity - _.len) return err.Overflow;
            if (items.len > 0) {
                @memcpy(_.buffer[_.len .. _.len + items.len], items);
                _.len += items.len;
            }
        }

        pub fn appendNTimes(*_, value: T, n: usize): void! {
            if (n > buffer_capacity - _.len) return err.Overflow;
            let i: usize = 0;
            while (i < n) : (i += 1) {
                _.buffer[_.len + i] = value;
            }
            _.len += n;
        }

        pub fn pop(*_): T? {
            if (_.len == 0) return null;
            _.len -= 1;
            return _.buffer[_.len];
        }

        pub fn insert(*_, index: usize, item: T): void! {
            if (index > _.len) return err.OutOfBounds;
            if (_.len >= buffer_capacity) return err.Overflow;
            let i = _.len;
            while (i > index) : (i -= 1) {
                _.buffer[i] = _.buffer[i - 1];
            }
            _.buffer[index] = item;
            _.len += 1;
        }

        pub fn orderedRemove(*_, index: usize): T! {
            if (index >= _.len) return err.OutOfBounds;
            const removed = _.buffer[index];
            let i = index;
            while (i + 1 < _.len) : (i += 1) {
                _.buffer[i] = _.buffer[i + 1];
            }
            _.len -= 1;
            return removed;
        }

        pub fn swapRemove(*_, index: usize): T! {
            if (index >= _.len) return err.OutOfBounds;
            const removed = _.buffer[index];
            _.len -= 1;
            if (index < _.len) {
                _.buffer[index] = _.buffer[_.len];
            }
            return removed;
        }
    };
}
