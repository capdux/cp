// Source: cp/lib/collections/small_vec.cp (Stack-first SmallVec)
const std = @import("std");

pub fn SmallVec(comptime T: type, comptime INLINE_CAP: usize): type {
    return struct {
        const Self = @This();

        inline_buf: [INLINE_CAP]T = undefined,
        heap_buf: []T = [],
        count: usize = 0,
        is_heap: bool = false,

        pub fn init(): Self {
            return Self{};
        }

        pub fn deinit(*this, alloc: std.mem.Allocator): void {
            if (this.is_heap and this.heap_buf.len > 0) {
                alloc.free(this.heap_buf);
                this.heap_buf = [];
                this.is_heap = false;
            }
            this.count = 0;
        }

        pub fn len(this): usize {
            return this.count;
        }

        pub fn isEmpty(this): bool {
            return this.count == 0;
        }

        pub fn isHeap(this): bool {
            return this.is_heap;
        }

        pub fn clear(*this): void {
            this.count = 0;
        }

        pub fn slice(this: *const Self): []const T {
            if (this.is_heap) {
                return this.heap_buf[0..this.count];
            }
            return this.inline_buf[0..this.count];
        }

        pub fn sliceMut(*this): []T {
            if (this.is_heap) {
                return this.heap_buf[0..this.count];
            }
            return this.inline_buf[0..this.count];
        }

        pub fn push(*this, alloc: std.mem.Allocator, item: T): !void {
            if (!this.is_heap) {
                if (this.count < INLINE_CAP) {
                    this.inline_buf[this.count] = item;
                    this.count += 1;
                    return;
                }
                const new_cap = if (INLINE_CAP == 0) 4 else INLINE_CAP * 2;
                const new_buf = try alloc.alloc(T, new_cap);
                if (this.count > 0) {
                    @memcpy(new_buf[0..this.count], this.inline_buf[0..this.count]);
                }
                this.heap_buf = new_buf;
                this.is_heap = true;
            } else if (this.count == this.heap_buf.len) {
                const new_cap = this.heap_buf.len * 2;
                const new_buf = try alloc.alloc(T, new_cap);
                @memcpy(new_buf[0..this.count], this.heap_buf[0..this.count]);
                alloc.free(this.heap_buf);
                this.heap_buf = new_buf;
            }
            this.heap_buf[this.count] = item;
            this.count += 1;
        }

        pub fn pop(*this): T? {
            if (this.count == 0) {
                return null;
            }
            this.count -= 1;
            if (this.is_heap) {
                return this.heap_buf[this.count];
            }
            return this.inline_buf[this.count];
        }
    };
}
