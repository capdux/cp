// Source: Custom implementation by Antigravity AI
// ============================================================================
// lib/col/soa.cp
// High-performance SoA (Struct of Arrays) column-oriented container
// Underpinned by Zig native std.MultiArrayList(T) with SIMD-aligned contiguous column slices
// ============================================================================

pub struct Soa<T> {
    zig {
        raw: std.MultiArrayList(T) = .{},
        allocator: std.mem.Allocator = std.heap.page_allocator,

        pub fn items(self: anytype, comptime field: anytype) @TypeOf(self.raw.items(field)) {
            return self.raw.items(field);
        }
    }

    pub fn init(allocator: auto = null): Soa<T> {
        let s = Soa<T> {};
        zig {
            s.raw = .{};
            const ATy = @TypeOf(allocator);
            if (comptime ATy == @TypeOf(null)) {
                s.allocator = std.heap.page_allocator;
            } else if (comptime (@typeInfo(ATy) == .optional)) {
                s.allocator = allocator orelse std.heap.page_allocator;
            } else {
                s.allocator = allocator;
            }
        }
        return s;
    }

    pub fn initCapacity(initial_cap: usize, allocator: auto = null): Soa<T> {
        let s = Soa<T> {};
        zig {
            s.raw = .{};
            const ATy = @TypeOf(allocator);
            if (comptime ATy == @TypeOf(null)) {
                s.allocator = std.heap.page_allocator;
            } else if (comptime (@typeInfo(ATy) == .optional)) {
                s.allocator = allocator orelse std.heap.page_allocator;
            } else {
                s.allocator = allocator;
            }
            s.raw.ensureTotalCapacity(s.allocator, initial_cap) catch unreachable;
        }
        return s;
    }

    pub fn deinit(*_) {
        zig {
            self.raw.deinit(self.allocator);
        }
    }

    pub fn append(*_, item: T) {
        zig {
            self.raw.append(self.allocator, item) catch unreachable;
        }
    }

    pub fn get(index: usize): T {
        zig {
            return self.raw.get(index);
        }
    }

    pub fn set(*_, index: usize, item: T) {
        zig {
            self.raw.set(index, item);
        }
    }

    pub fn len(): usize {
        zig {
            return self.raw.len;
        }
    }

    pub fn capacity(): usize {
        zig {
            return self.raw.capacity;
        }
    }

    pub fn isEmpty(): bool {
        zig {
            return self.raw.len == 0;
        }
    }

    pub fn clearRetainingCapacity(*_) {
        zig {
            self.raw.clearRetainingCapacity();
        }
    }

    pub fn popOrNull(*_): T? {
        zig {
            return self.raw.pop();
        }
    }

    pub fn pop(*_): T? {
        zig {
            return self.raw.pop();
        }
    }

    pub fn ensureTotalCapacity(*_, new_capacity: usize) {
        zig {
            self.raw.ensureTotalCapacity(self.allocator, new_capacity) catch unreachable;
        }
    }

    pub fn ensureUnusedCapacity(*_, additional_count: usize) {
        zig {
            self.raw.ensureUnusedCapacity(self.allocator, additional_count) catch unreachable;
        }
    }

    pub fn swapRemove(*_, index: usize) {
        zig {
            self.raw.swapRemove(index);
        }
    }

    pub fn orderedRemove(*_, index: usize) {
        zig {
            self.raw.orderedRemove(index);
        }
    }
}
