// Reference Only: Functional overlap with lib/time/interval.cp | Kept for comparison/analysis
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

        pub static fn init(alloc: std.mem.Allocator): Self {
            return Self{
                .ranges = std.ArrayList(Span).empty,
                .alloc = alloc,
            };
        }

        pub fn deinit(*_) {
            _.ranges.deinit(_.alloc);
        }

        pub fn count(): usize {
            return _.ranges.items.len;
        }

        pub fn clear(*_) {
            _.ranges.clearRetainingCapacity();
        }

        pub fn items(): []const Span {
            return _.ranges.items;
        }

        /// Add half-open range [start, end), automatically merging overlapping or adjacent spans
        pub fn addRange(*_, start: T, end: T): !void {
            if (start > end) { return RangeError.InvalidRange; }
            if (start == end) { return; }

            let next = std.ArrayList(Span).empty;
            errdefer next.deinit(_.alloc);

            let new_start = start;
            let new_end = end;
            let inserted = false;

            for (r of _.ranges.items) {
                if (r.end < new_start) {
                    try next.append(_.alloc, r);
                } else if (new_end < r.start) {
                    if (!inserted) {
                        try next.append(_.alloc, Span{ .start = new_start, .end = new_end });
                        inserted = true;
                    }
                    try next.append(_.alloc, r);
                } else {
                    new_start = @min(new_start, r.start);
                    new_end = @max(new_end, r.end);
                }
            }
            if (!inserted) {
                try next.append(_.alloc, Span{ .start = new_start, .end = new_end });
            }

            _.ranges.deinit(_.alloc);
            _.ranges = next;
        }

        /// Remove half-open range [start, end), splitting or trimming existing spans
        pub fn removeRange(*_, start: T, end: T): !void {
            if (start > end) { return RangeError.InvalidRange; }
            if (start == end) { return; }

            let next = std.ArrayList(Span).empty;
            errdefer next.deinit(_.alloc);

            for (r of _.ranges.items) {
                if (r.end <= start or r.start >= end) {
                    try next.append(_.alloc, r);
                } else {
                    if (r.start < start) {
                        try next.append(_.alloc, Span{ .start = r.start, .end = start });
                    }
                    if (end < r.end) {
                        try next.append(_.alloc, Span{ .start = end, .end = r.end });
                    }
                }
            }

            _.ranges.deinit(_.alloc);
            _.ranges = next;
        }

        pub fn containsPoint(point: T): bool {
            let lo: usize = 0;
            let hi: usize = _.ranges.items.len;
            while (lo < hi) {
                const mid = lo + (hi - lo) / 2;
                const r = _.ranges.items[mid];
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

        pub fn totalCovered(): T {
            let total: T = 0;
            for (r of _.ranges.items) {
                total += (r.end - r.start);
            }
            return total;
        }
    };
}
