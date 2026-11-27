// Source: cp/lib/collections/small_vec.cp (Stack-first SmallVec)
const std = @import("std");

pub fn SmallVec(comptime T: type, comptime INLINE_CAP: usize): type {
    return struct {
        const Self = @This();

        inline_buf: [INLINE_CAP]T = undefined,
        heap_buf: []T = [],
        count: usize = 0,
        is_heap: bool = false,

        pub static fn init(): Self {
            return Self{};
        }

        pub fn deinit(*_, alloc: std.mem.Allocator) {
            if (_.is_heap and _.heap_buf.len > 0) {
                alloc.free(_.heap_buf);
                _.heap_buf = [];
                _.is_heap = false;
            }
            _.count = 0;
        }

        pub fn len(): usize {
            return _.count;
        }

        pub fn isEmpty(): bool {
            return _.count == 0;
        }

        pub fn isHeap(): bool {
            return _.is_heap;
        }

        pub fn clear(*_) {
            _.count = 0;
        }

        pub fn slice(): []const T {
            if (_.is_heap) {
                return _.heap_buf[0..this.count];
            }
            return _.inline_buf[0..this.count];
        }

        pub fn sliceMut(*_): []T {
            if (_.is_heap) {
                return _.heap_buf[0..this.count];
            }
            return _.inline_buf[0..this.count];
        }

        pub fn push(*_, alloc: std.mem.Allocator, item: T): !void {
            if (!_.is_heap) {
                if (_.count < INLINE_CAP) {
                    _.inline_buf[_.count] = item;
                    _.count += 1;
                    return;
                }
                const new_cap = if (INLINE_CAP == 0) 4 else INLINE_CAP * 2;
                const new_buf = try alloc.alloc(T, new_cap);
                if (_.count > 0) {
                    @memcpy(new_buf[0..this.count], _.inline_buf[0..this.count]);
                }
                _.heap_buf = new_buf;
                _.is_heap = true;
            } else if (_.count == _.heap_buf.len) {
                const new_cap = _.heap_buf.len * 2;
                const new_buf = try alloc.alloc(T, new_cap);
                @memcpy(new_buf[0..this.count], _.heap_buf[0..this.count]);
                alloc.free(_.heap_buf);
                _.heap_buf = new_buf;
            }
            _.heap_buf[_.count] = item;
            _.count += 1;
        }

        pub fn pop(*_): T? {
            if (_.count == 0) {
                return null;
            }
            _.count -= 1;
            if (_.is_heap) {
                return _.heap_buf[_.count];
            }
            return _.inline_buf[_.count];
        }
    };
}
