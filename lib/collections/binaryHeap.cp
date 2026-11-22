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

        pub fn deinit(*this) {
            this.items.deinit(this.alloc);
        }

        pub fn len(this): usize {
            return this.items.items.len;
        }

        pub fn isEmpty(this): bool {
            return this.items.items.len == 0;
        }

        pub fn clear(*this) {
            this.items.clearRetainingCapacity();
        }

        pub fn peek(this): T? {
            if (this.items.items.len == 0) {
                return null;
            }
            return this.items.items[0];
        }

        pub fn push(*this, item: T): !void {
            try this.items.append(this.alloc, item);
            this.siftUp(this.items.items.len - 1);
        }

        pub fn pop(*this): T? {
            const n = this.items.items.len;
            if (n == 0) {
                return null;
            }
            if (n == 1) {
                return this.items.pop();
            }
            const top = this.items.items[0];
            this.items.items[0] = this.items.pop() orelse unreachable;
            this.siftDown(0);
            return top;
        }

        fn siftUp(*this, start_idx: usize) {
            var idx = start_idx;
            while (idx > 0) {
                const parent = (idx - 1) / 2;
                if (cmp(this.items.items[idx], this.items.items[parent]) == .lt) {
                    const tmp = this.items.items[idx];
                    this.items.items[idx] = this.items.items[parent];
                    this.items.items[parent] = tmp;
                    idx = parent;
                } else {
                    break;
                }
            }
        }

        fn siftDown(*this, start_idx: usize) {
            var idx = start_idx;
            const n = this.items.items.len;
            while (true) {
                const left = idx * 2 + 1;
                if (left >= n) {
                    break;
                }
                var best = left;
                const right = left + 1;
                if (right < n and cmp(this.items.items[right], this.items.items[left]) == .lt) {
                    best = right;
                }
                if (cmp(this.items.items[best], this.items.items[idx]) == .lt) {
                    const tmp = this.items.items[idx];
                    this.items.items[idx] = this.items.items[best];
                    this.items.items[best] = tmp;
                    idx = best;
                } else {
                    break;
                }
            }
        }
    };
}
