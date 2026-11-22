// Source: cp/lib/interval_tree/interval_tree.cp (Pure CP Augmented Interval Tree for Range Overlap Queries)
const std = @import("std");

pub const IntervalError = error{
    InvalidInterval,
};

pub fn Interval(comptime T: type, comptime V: type): type {
    return struct {
        low: T,
        high: T,
        value: V,
    };
}

pub fn IntervalTree(comptime T: type, comptime V: type): type {
    const Node = struct {
        low: T,
        high: T,
        max_high: T,
        value: V,
        left: (*@This())?,
        right: (*@This())?,
    };

    return struct {
        root: (*Node)?,
        count: usize,
        alloc: std.mem.Allocator,

        const Self = @This();
        pub const Entry = Interval(T, V);

        pub fn init(alloc: std.mem.Allocator): Self {
            return Self{
                .root = null,
                .count = 0,
                .alloc = alloc,
            };
        }

        fn freeNode(alloc: std.mem.Allocator, maybe_node: (*Node)?) {
            if (n := maybe_node) {
                freeNode(alloc, n.left);
                freeNode(alloc, n.right);
                alloc.destroy(n);
            }
        }

        pub fn deinit(self: *Self) {
            freeNode(self.alloc, self.root);
            self.root = null;
            self.count = 0;
        }

        pub fn len(self: Self): usize {
            return self.count;
        }

        fn insertNode(alloc: std.mem.Allocator, maybe_node: (*Node)?, low: T, high: T, value: V): !*Node {
            if (n := maybe_node) {
                if (low < n.low) {
                    n.left = try insertNode(alloc, n.left, low, high, value);
                } else {
                    n.right = try insertNode(alloc, n.right, low, high, value);
                }
                if (high > n.max_high) {
                    n.max_high = high;
                }
                return n;
            }
            let node = try alloc.create(Node);
            node.* = Node{
                .low = low,
                .high = high,
                .max_high = high,
                .value = value,
                .left = null,
                .right = null,
            };
            return node;
        }

        pub fn insert(self: *Self, low: T, high: T, value: V): !void {
            if (low > high) { return IntervalError.InvalidInterval; }
            self.root = try insertNode(self.alloc, self.root, low, high, value);
            self.count += 1;
        }

        fn overlaps(a_low: T, a_high: T, b_low: T, b_high: T): bool {
            return a_low <= b_high and b_low <= a_high;
        }

        fn searchOverlap(maybe_node: (*Node)?, low: T, high: T, alloc: std.mem.Allocator, out: *std.ArrayList(Entry)): !void {
            if (n := maybe_node) {
                if (left := n.left) {
                    if (left.max_high >= low) {
                        try searchOverlap(left, low, high, alloc, out);
                    }
                }
                if (overlaps(n.low, n.high, low, high)) {
                    try out.append(alloc, Entry{
                        .low = n.low,
                        .high = n.high,
                        .value = n.value,
                    });
                }
                if (n.low <= high) {
                    try searchOverlap(n.right, low, high, alloc, out);
                }
            }
        }

        pub fn queryOverlap(self: Self, alloc: std.mem.Allocator, low: T, high: T): ![]Entry {
            if (low > high) { return IntervalError.InvalidInterval; }
            let out = std.ArrayList(Entry).empty;
            errdefer out.deinit(alloc);
            try searchOverlap(self.root, low, high, alloc, &out);
            return out.toOwnedSlice(alloc);
        }

        pub fn queryPoint(self: Self, alloc: std.mem.Allocator, point: T): ![]Entry {
            return self.queryOverlap(alloc, point, point);
        }

        fn hasOverlapNode(maybe_node: (*Node)?, low: T, high: T): bool {
            let cur = maybe_node;
            while (n := cur) {
                if (overlaps(n.low, n.high, low, high)) { return true; }
                if (left := n.left) {
                    if (left.max_high >= low) {
                        cur = left;
                        continue;
                    }
                }
                cur = n.right;
            }
            return false;
        }

        pub fn hasOverlap(self: Self, low: T, high: T): IntervalError!bool {
            if (low > high) { return IntervalError.InvalidInterval; }
            return hasOverlapNode(self.root, low, high);
        }
    };
}
