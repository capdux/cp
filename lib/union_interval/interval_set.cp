// Source: cp/lib/union_interval/interval_set.cp (Pure CP Merged Disjoint Interval Set)
const std = @import("std");

pub const RangeError = error{
    InvalidRange,
};

/// Half-open or closed integer interval [start, end)
pub fn Range(comptime T: type): type {
    return struct {
        start: T,
        end: T,
    };
}

pub fn IntervalSet(comptime T: type): type {
    const Span = Range(T);

    return struct {
        ranges: std.ArrayList(Span),
        alloc: std.mem.Allocator,

        const Self = @This();

        pub fn init(alloc: std.mem.Allocator): Self {
            return Self{
                .ranges = std.ArrayList(Span).empty,
                .alloc = alloc,
            };
        }

        pub fn deinit(self: *Self) {
            self.ranges.deinit(self.alloc);
        }

        pub fn count(self: Self): usize {
            return self.ranges.items.len;
        }

        pub fn clear(self: *Self) {
            self.ranges.clearRetainingCapacity();
        }

        pub fn items(self: Self): []const Span {
            return self.ranges.items;
        }

        /// Add half-open range [start, end), automatically merging overlapping or adjacent spans
        pub fn addRange(self: *Self, start: T, end: T): !void {
            if (start > end) { return RangeError.InvalidRange; }
            if (start == end) { return; }

            let next = std.ArrayList(Span).empty;
            errdefer next.deinit(self.alloc);

            let new_start = start;
            let new_end = end;
            let inserted = false;

            for (r of self.ranges.items) {
                if (r.end < new_start) {
                    try next.append(self.alloc, r);
                } else if (new_end < r.start) {
                    if (!inserted) {
                        try next.append(self.alloc, Span{ .start = new_start, .end = new_end });
                        inserted = true;
                    }
                    try next.append(self.alloc, r);
                } else {
                    new_start = @min(new_start, r.start);
                    new_end = @max(new_end, r.end);
                }
            }
            if (!inserted) {
                try next.append(self.alloc, Span{ .start = new_start, .end = new_end });
            }

            self.ranges.deinit(self.alloc);
            self.ranges = next;
        }

        /// Remove half-open range [start, end), splitting or trimming existing spans
        pub fn removeRange(self: *Self, start: T, end: T): !void {
            if (start > end) { return RangeError.InvalidRange; }
            if (start == end) { return; }

            let next = std.ArrayList(Span).empty;
            errdefer next.deinit(self.alloc);

            for (r of self.ranges.items) {
                if (r.end <= start or r.start >= end) {
                    try next.append(self.alloc, r);
                } else {
                    if (r.start < start) {
                        try next.append(self.alloc, Span{ .start = r.start, .end = start });
                    }
                    if (end < r.end) {
                        try next.append(self.alloc, Span{ .start = end, .end = r.end });
                    }
                }
            }

            self.ranges.deinit(self.alloc);
            self.ranges = next;
        }

        pub fn containsPoint(self: Self, point: T): bool {
            let lo: usize = 0;
            let hi: usize = self.ranges.items.len;
            while (lo < hi) {
                const mid = lo + (hi - lo) / 2;
                const r = self.ranges.items[mid];
                if (point < r.start) {
                    hi = mid;
                } else if (point >= r.end) {
                    lo = mid + 1;
                } else {
                    return true;
                }
            }
            return false;
        }

        pub fn totalCovered(self: Self): T {
            let total: T = 0;
            for (r of self.ranges.items) {
                total += (r.end - r.start);
            }
            return total;
        }
    };
}
