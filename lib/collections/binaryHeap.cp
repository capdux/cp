// Source: cp/lib/collections/binary_heap.cp (Generic BinaryHeap)
const std = @import("std");

pub fn BinaryHeap(comptime T: type, comptime cmp: fn (a: T, b: T) std.math.Order): type {
    return struct {
        const Self = @This();

        alloc: std.mem.Allocator,
        items: std.ArrayList(T),

        pub fn init(alloc: std.mem.Allocator): Self {
            return Self{
                alloc: alloc,
                items: std.ArrayList(T).empty,
            };
        }

        pub fn deinit(*_) {
            _.items.deinit(_.alloc);
        }

        pub fn len(): usize {
            return _.items.items.len;
        }

        pub fn isEmpty(): bool {
            return _.items.items.len == 0;
        }

        pub fn clear(*_) {
            _.items.clearRetainingCapacity();
        }

        pub fn peek(): T? {
            if (_.items.items.len == 0) {
                return null;
            }
            return _.items.items[0];
        }

        pub fn push(*_, item: T): !void {
            try _.items.append(_.alloc, item);
            _.siftUp(_.items.items.len - 1);
        }

        pub fn pop(*_): T? {
            const n = _.items.items.len;
            if (n == 0) {
                return null;
            }
            if (n == 1) {
                return _.items.pop();
            }
            const top = _.items.items[0];
            _.items.items[0] = _.items.pop() orelse unreachable;
            _.siftDown(0);
            return top;
        }

        fn siftUp(*_, start_idx: usize) {
            let idx = start_idx;
            while (idx > 0) {
                const parent = (idx - 1) / 2;
                if (cmp(_.items.items[idx], _.items.items[parent]) == .lt) {
                    const tmp = _.items.items[idx];
                    _.items.items[idx] = _.items.items[parent];
                    _.items.items[parent] = tmp;
                    idx = parent;
                } else {
                    break;
                }
            }
        }

        fn siftDown(*_, start_idx: usize) {
            let idx = start_idx;
            const n = _.items.items.len;
            while (true) {
                const left = idx * 2 + 1;
                if (left >= n) {
                    break;
                }
                let best = left;
                const right = left + 1;
                if (right < n and cmp(_.items.items[right], _.items.items[left]) == .lt) {
                    best = right;
                }
                if (cmp(_.items.items[best], _.items.items[idx]) == .lt) {
                    const tmp = _.items.items[idx];
                    _.items.items[idx] = _.items.items[best];
                    _.items.items[best] = tmp;
                    idx = best;
                } else {
                    break;
                }
            }
        }
    };
}
