// Reference Only: Functional overlap with lib/ds/segtree.cp (FenwickTree) | Kept for comparison/analysis
// Source: cp/lib/fenwick/fenwick.cp (Pure CP 1D & 2D Fenwick / Binary Indexed Tree)
const std = @import("std");

pub const FenwickError = error{
    IndexOutOfBounds,
    InvalidRange,
};

pub fn FenwickTree(comptime T: type): type {
    return struct {
        tree: []T,
        n: usize,
        alloc: std.mem.Allocator,

        const Self = @This();

        pub fn init(alloc: std.mem.Allocator, size: usize): !Self {
            const buf = try alloc.alloc(T, size + 1);
            @memset(buf, 0);
            return Self{
                .tree = buf,
                .n = size,
                .alloc = alloc,
            };
        }

        pub fn initFromSlice(alloc: std.mem.Allocator, values: []const T): !Self {
            let ft = try Self.init(alloc, values.len);
            errdefer ft.deinit();
            let i: usize = 0;
            while (i < values.len) : (i += 1) {
                try ft.add(i, values[i]);
            }
            return ft;
        }

        pub fn deinit(self: *Self) {
            self.alloc.free(self.tree);
        }

        pub fn len(self: Self): usize {
            return self.n;
        }

        /// 0-indexed point add: a[idx] += delta
        pub fn add(self: *Self, idx: usize, delta: T): FenwickError!void {
            if (idx >= self.n) { return FenwickError.IndexOutOfBounds; }
            let i = idx + 1;
            while (i <= self.n) {
                self.tree[i] += delta;
                i += i & (~i +% 1);
            }
        }

        /// Prefix sum of [0 .. idx] inclusive
        pub fn prefixSum(self: Self, idx: usize): FenwickError!T {
            if (idx >= self.n) { return FenwickError.IndexOutOfBounds; }
            let i = idx + 1;
            let sum: T = 0;
            while (i > 0) {
                sum += self.tree[i];
                i -= i & (~i +% 1);
            }
            return sum;
        }

        /// Inclusive range sum [left .. right]
        pub fn rangeSum(self: Self, left: usize, right: usize): FenwickError!T {
            if (left > right) { return FenwickError.InvalidRange; }
            if (right >= self.n) { return FenwickError.IndexOutOfBounds; }
            const r_sum = try self.prefixSum(right);
            if (left == 0) { return r_sum; }
            const l_sum = try self.prefixSum(left - 1);
            return r_sum - l_sum;
        }

        /// Point query a[idx]
        pub fn get(self: Self, idx: usize): FenwickError!T {
            return self.rangeSum(idx, idx);
        }

        /// Point set: a[idx] = val
        pub fn set(self: *Self, idx: usize, val: T): FenwickError!void {
            const cur = try self.get(idx);
            try self.add(idx, val - cur);
        }

        /// Smallest 0-based index `idx` such that prefixSum(idx) >= target (requires non-negative elements)
        pub fn lowerBound(self: Self, target: T): usize? {
            if (self.n == 0) { return null; }
            let idx: usize = 0;
            let acc: T = 0;
            let bit: usize = 1;
            while (bit * 2 <= self.n) {
                bit *= 2;
            }
            while (bit > 0) {
                const next = idx + bit;
                if (next <= self.n and acc + self.tree[next] < target) {
                    acc += self.tree[next];
                    idx = next;
                }
                bit >>= 1;
            }
            if (idx >= self.n) { return null; }
            return idx;
        }
    };
}
