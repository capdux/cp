// Reference Only: Functional overlap with lib/ds/segtree.cp (LazySegmentTree) | Kept for comparison/analysis
// Source: cp/lib/segment_tree/segment_tree.cp (Pure CP Lazy-Propagation Segment Tree)
const std = @import("std");

pub const SegError = error{
    EmptyTree,
    IndexOutOfBounds,
    InvalidRange,
};

pub fn SegmentTree(comptime T: type): type {
    const Node = struct {
        sum: T,
        min_val: T,
        max_val: T,
        lazy_add: T,
    };

    return struct {
        nodes: []Node,
        n: usize,
        alloc: std.mem.Allocator,

        const Self = @This();

        pub fn init(alloc: std.mem.Allocator, values: []const T): !Self {
            if (values.len == 0) { return SegError.EmptyTree; }
            const nodes = try alloc.alloc(Node, 4 * values.len);
            errdefer alloc.free(nodes);
            let st = Self{
                .nodes = nodes,
                .n = values.len,
                .alloc = alloc,
            };
            st.build(1, 0, values.len - 1, values);
            return st;
        }

        pub fn deinit(self: *Self) {
            self.alloc.free(self.nodes);
        }

        pub fn len(self: Self): usize {
            return self.n;
        }

        fn pull(self: *Self, idx: usize) {
            const left = self.nodes[idx * 2];
            const right = self.nodes[idx * 2 + 1];
            self.nodes[idx].sum = left.sum + right.sum;
            self.nodes[idx].min_val = @min(left.min_val, right.min_val);
            self.nodes[idx].max_val = @max(left.max_val, right.max_val);
        }

        fn applyAdd(self: *Self, idx: usize, l: usize, r: usize, delta: T) {
            const count: T = @intCast(r - l + 1);
            self.nodes[idx].sum += delta * count;
            self.nodes[idx].min_val += delta;
            self.nodes[idx].max_val += delta;
            self.nodes[idx].lazy_add += delta;
        }

        fn push(self: *Self, idx: usize, l: usize, r: usize) {
            const lz = self.nodes[idx].lazy_add;
            if (lz != 0 and l < r) {
                const mid = l + (r - l) / 2;
                self.applyAdd(idx * 2, l, mid, lz);
                self.applyAdd(idx * 2 + 1, mid + 1, r, lz);
                self.nodes[idx].lazy_add = 0;
            }
        }

        fn build(self: *Self, idx: usize, l: usize, r: usize, values: []const T) {
            self.nodes[idx].lazy_add = 0;
            if (l == r) {
                const v = values[l];
                self.nodes[idx].sum = v;
                self.nodes[idx].min_val = v;
                self.nodes[idx].max_val = v;
                return;
            }
            const mid = l + (r - l) / 2;
            self.build(idx * 2, l, mid, values);
            self.build(idx * 2 + 1, mid + 1, r, values);
            self.pull(idx);
        }

        fn updateRange(self: *Self, idx: usize, l: usize, r: usize, ql: usize, qr: usize, delta: T) {
            if (ql <= l and r <= qr) {
                self.applyAdd(idx, l, r, delta);
                return;
            }
            self.push(idx, l, r);
            const mid = l + (r - l) / 2;
            if (ql <= mid) {
                self.updateRange(idx * 2, l, mid, ql, qr, delta);
            }
            if (qr > mid) {
                self.updateRange(idx * 2 + 1, mid + 1, r, ql, qr, delta);
            }
            self.pull(idx);
        }

        pub fn rangeAdd(self: *Self, left: usize, right: usize, delta: T): SegError!void {
            if (left > right) { return SegError.InvalidRange; }
            if (right >= self.n) { return SegError.IndexOutOfBounds; }
            self.updateRange(1, 0, self.n - 1, left, right, delta);
        }

        fn querySumNode(self: *Self, idx: usize, l: usize, r: usize, ql: usize, qr: usize): T {
            if (ql <= l and r <= qr) { return self.nodes[idx].sum; }
            self.push(idx, l, r);
            const mid = l + (r - l) / 2;
            let res: T = 0;
            if (ql <= mid) { res += self.querySumNode(idx * 2, l, mid, ql, qr); }
            if (qr > mid) { res += self.querySumNode(idx * 2 + 1, mid + 1, r, ql, qr); }
            return res;
        }

        pub fn rangeSum(self: *Self, left: usize, right: usize): SegError!T {
            if (left > right) { return SegError.InvalidRange; }
            if (right >= self.n) { return SegError.IndexOutOfBounds; }
            return self.querySumNode(1, 0, self.n - 1, left, right);
        }

        fn queryMinNode(self: *Self, idx: usize, l: usize, r: usize, ql: usize, qr: usize): T {
            if (ql <= l and r <= qr) { return self.nodes[idx].min_val; }
            self.push(idx, l, r);
            const mid = l + (r - l) / 2;
            if (qr <= mid) { return self.queryMinNode(idx * 2, l, mid, ql, qr); }
            if (ql > mid) { return self.queryMinNode(idx * 2 + 1, mid + 1, r, ql, qr); }
            const a = self.queryMinNode(idx * 2, l, mid, ql, qr);
            const b = self.queryMinNode(idx * 2 + 1, mid + 1, r, ql, qr);
            return @min(a, b);
        }

        pub fn rangeMin(self: *Self, left: usize, right: usize): SegError!T {
            if (left > right) { return SegError.InvalidRange; }
            if (right >= self.n) { return SegError.IndexOutOfBounds; }
            return self.queryMinNode(1, 0, self.n - 1, left, right);
        }

        fn queryMaxNode(self: *Self, idx: usize, l: usize, r: usize, ql: usize, qr: usize): T {
            if (ql <= l and r <= qr) { return self.nodes[idx].max_val; }
            self.push(idx, l, r);
            const mid = l + (r - l) / 2;
            if (qr <= mid) { return self.queryMaxNode(idx * 2, l, mid, ql, qr); }
            if (ql > mid) { return self.queryMaxNode(idx * 2 + 1, mid + 1, r, ql, qr); }
            const a = self.queryMaxNode(idx * 2, l, mid, ql, qr);
            const b = self.queryMaxNode(idx * 2 + 1, mid + 1, r, ql, qr);
            return @max(a, b);
        }

        pub fn rangeMax(self: *Self, left: usize, right: usize): SegError!T {
            if (left > right) { return SegError.InvalidRange; }
            if (right >= self.n) { return SegError.IndexOutOfBounds; }
            return self.queryMaxNode(1, 0, self.n - 1, left, right);
        }
    };
}
