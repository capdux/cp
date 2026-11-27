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

        pub static fn init(alloc: std.mem.Allocator, values: []const T): !Self {
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

        pub fn deinit(*_) {
            _.alloc.free(_.nodes);
        }

        pub fn len(): usize {
            return _.n;
        }

        fn pull(*_, idx: usize) {
            const left = _.nodes[idx * 2];
            const right = _.nodes[idx * 2 + 1];
            _.nodes[idx].sum = left.sum + right.sum;
            _.nodes[idx].min_val = @min(left.min_val, right.min_val);
            _.nodes[idx].max_val = @max(left.max_val, right.max_val);
        }

        fn applyAdd(*_, idx: usize, l: usize, r: usize, delta: T) {
            const count: T = @intCast(r - l + 1);
            _.nodes[idx].sum += delta * count;
            _.nodes[idx].min_val += delta;
            _.nodes[idx].max_val += delta;
            _.nodes[idx].lazy_add += delta;
        }

        fn push(*_, idx: usize, l: usize, r: usize) {
            const lz = _.nodes[idx].lazy_add;
            if (lz != 0 and l < r) {
                const mid = l + (r - l) / 2;
                _.applyAdd(idx * 2, l, mid, lz);
                _.applyAdd(idx * 2 + 1, mid + 1, r, lz);
                _.nodes[idx].lazy_add = 0;
            }
        }

        fn build(*_, idx: usize, l: usize, r: usize, values: []const T) {
            _.nodes[idx].lazy_add = 0;
            if (l == r) {
                const v = values[l];
                _.nodes[idx].sum = v;
                _.nodes[idx].min_val = v;
                _.nodes[idx].max_val = v;
                return;
            }
            const mid = l + (r - l) / 2;
            _.build(idx * 2, l, mid, values);
            _.build(idx * 2 + 1, mid + 1, r, values);
            _.pull(idx);
        }

        fn updateRange(*_, idx: usize, l: usize, r: usize, ql: usize, qr: usize, delta: T) {
            if (ql <= l and r <= qr) {
                _.applyAdd(idx, l, r, delta);
                return;
            }
            _.push(idx, l, r);
            const mid = l + (r - l) / 2;
            if (ql <= mid) {
                _.updateRange(idx * 2, l, mid, ql, qr, delta);
            }
            if (qr > mid) {
                _.updateRange(idx * 2 + 1, mid + 1, r, ql, qr, delta);
            }
            _.pull(idx);
        }

        pub fn rangeAdd(*_, left: usize, right: usize, delta: T): SegError!void {
            if (left > right) { return SegError.InvalidRange; }
            if (right >= _.n) { return SegError.IndexOutOfBounds; }
            _.updateRange(1, 0, _.n - 1, left, right, delta);
        }

        fn querySumNode(*_, idx: usize, l: usize, r: usize, ql: usize, qr: usize): T {
            if (ql <= l and r <= qr) { return _.nodes[idx].sum; }
            _.push(idx, l, r);
            const mid = l + (r - l) / 2;
            let res: T = 0;
            if (ql <= mid) { res += _.querySumNode(idx * 2, l, mid, ql, qr); }
            if (qr > mid) { res += _.querySumNode(idx * 2 + 1, mid + 1, r, ql, qr); }
            return res;
        }

        pub fn rangeSum(*_, left: usize, right: usize): SegError!T {
            if (left > right) { return SegError.InvalidRange; }
            if (right >= _.n) { return SegError.IndexOutOfBounds; }
            return _.querySumNode(1, 0, _.n - 1, left, right);
        }

        fn queryMinNode(*_, idx: usize, l: usize, r: usize, ql: usize, qr: usize): T {
            if (ql <= l and r <= qr) { return _.nodes[idx].min_val; }
            _.push(idx, l, r);
            const mid = l + (r - l) / 2;
            if (qr <= mid) { return _.queryMinNode(idx * 2, l, mid, ql, qr); }
            if (ql > mid) { return _.queryMinNode(idx * 2 + 1, mid + 1, r, ql, qr); }
            const a = _.queryMinNode(idx * 2, l, mid, ql, qr);
            const b = _.queryMinNode(idx * 2 + 1, mid + 1, r, ql, qr);
            return @min(a, b);
        }

        pub fn rangeMin(*_, left: usize, right: usize): SegError!T {
            if (left > right) { return SegError.InvalidRange; }
            if (right >= _.n) { return SegError.IndexOutOfBounds; }
            return _.queryMinNode(1, 0, _.n - 1, left, right);
        }

        fn queryMaxNode(*_, idx: usize, l: usize, r: usize, ql: usize, qr: usize): T {
            if (ql <= l and r <= qr) { return _.nodes[idx].max_val; }
            _.push(idx, l, r);
            const mid = l + (r - l) / 2;
            if (qr <= mid) { return _.queryMaxNode(idx * 2, l, mid, ql, qr); }
            if (ql > mid) { return _.queryMaxNode(idx * 2 + 1, mid + 1, r, ql, qr); }
            const a = _.queryMaxNode(idx * 2, l, mid, ql, qr);
            const b = _.queryMaxNode(idx * 2 + 1, mid + 1, r, ql, qr);
            return @max(a, b);
        }

        pub fn rangeMax(*_, left: usize, right: usize): SegError!T {
            if (left > right) { return SegError.InvalidRange; }
            if (right >= _.n) { return SegError.IndexOutOfBounds; }
            return _.queryMaxNode(1, 0, _.n - 1, left, right);
        }
    };
}
