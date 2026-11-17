// Source: Custom implementation by Antigravity AI
// lib/col/priority_queue.cp
// Generic Priority Queue - Industrial Strength (Based on std.PriorityQueue)

const std = @import("std");

pub struct PriorityQueue<T> {
    fn cmp(_: void, a: T, b: T): std.math.Order {
        return if (a < b) .lt else if (a > b) .gt else .eq;
    }

    inner: std.PriorityQueue(T, void, cmp),

    pub static fn init(allocator: std.mem.Allocator): PriorityQueue<T> {
        return PriorityQueue<T> {
            .inner = std.PriorityQueue(T, void, cmp).init(allocator, {}),
        };
    }

    pub fn deinit(*_) {
        _.inner.deinit();
    }

    pub fn add(*_, item: T) !void {
        try _.inner.add(item);
    }

    pub fn pop(*_): T? {
        return _.inner.popOrNull();
    }

    pub fn len(*_): usize {
        return _.inner.len;
    }
}
