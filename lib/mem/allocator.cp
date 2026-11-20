// Source: /data/dev/bun/src/bun_alloc/MaxHeapAllocator.cp & /data/dev/bun/src/bun_alloc/BufferFallbackAllocator.cp
// lib/mem/allocator.cp
// High-performance Arena, StackFallbackAllocator, BumpArena, MaybeOwnedBuf, and SharedTensor
// Absorbed from bun/src/bun_alloc/ (BufferFallbackAllocator, MaxHeapAllocator, maybe_owned, MimallocArena)

pub class Arena {
    pub fn alloc(size: usize): *! {
        return @alloc(size);
    }
    pub fn free() {
        @free();
    }
}

pub fn get_arena(): Arena {
    return @get_arena();
}

pub class SharedTensor {
    pub size: usize = 0;
    pub ref_count: usize = 0;
    pub ptr: [*]u8! = undefined;

    pub fn init(*_, size: usize) {
        _.size = size;
        _.ref_count = 1;
        if (size > 0) {
            zig {
                const buf = std.heap.page_allocator.alloc(u8, size) catch unreachable;
                @memset(buf, 0);
                self.ptr = buf.ptr;
            }
        }
    }

    pub fn retain(*_) {
        if (_.ref_count > 0) {
            zig {
                _ = @atomicRmw(usize, &self.ref_count, .Add, 1, .monotonic);
            }
        }
    }

    pub fn release(*_) {
        if (_.ref_count == 0) {
            return;
        }
        let prev: usize = 0;
        zig {
            prev = @atomicRmw(usize, &self.ref_count, .Sub, 1, .acq_rel);
        }
        if (prev == 1) {
            zig {
                if (self.size > 0) {
                    std.heap.page_allocator.free(self.ptr[0..self.size]);
                }
                self.size = 0;
            }
        }
    }

    pub fn get_size(): usize {
        return _.size;
    }

    pub fn get_ref_count(): usize {
        return _.ref_count;
    }
}

// Stack-first allocator with seamless fallback to heap when stack capacity is exceeded
// (Absorbed from bun/src/bun_alloc/BufferFallbackAllocator.cp)
pub struct StackFallbackAllocator<const CAP: usize> {
    buffer: [CAP]u8 = undefined;
    stack_used: usize = 0;
    heap_allocs: usize = 0;
    heap_bytes: usize = 0;

    pub static fn init(): StackFallbackAllocator<CAP> {
        return StackFallbackAllocator<CAP>{
            buffer: undefined,
            stack_used: 0,
            heap_allocs: 0,
            heap_bytes: 0,
        };
    }

    pub fn allocStack(*_, len: usize): ([]u8)? {
        if (_.stack_used + len > CAP) {
            return null;
        }
        const start = _.stack_used;
        _.stack_used += len;
        return _.buffer[start .. start + len];
    }

    pub fn recordFallback(*_, len: usize) {
        _.heap_allocs += 1;
        _.heap_bytes += len;
    }

    pub fn stackRemaining(): usize {
        return CAP - _.stack_used;
    }

    pub fn reset(*_) {
        _.stack_used = 0;
    }
}

// Contiguous region bump allocator (Absorbed from bun/src/bun_alloc/MaxHeapAllocator.cp)
pub struct BumpArena {
    ptr: [*]u8! = undefined;
    cap: usize = 0;
    offset: usize = 0;

    pub static fn initCapacity(capacity: usize): BumpArena {
        const cap = if (capacity == 0) 4096 else capacity;
        zig {
            const buf = std.heap.page_allocator.alloc(u8, cap) catch unreachable;
            return BumpArena{
                .ptr = buf.ptr,
                .cap = cap,
                .offset = 0,
            };
        }
    }

    pub fn allocBytes(*_, len: usize): ([]u8)? {
        if (_.offset + len > _.cap) {
            return null;
        }
        const start = _.offset;
        _.offset += len;
        let out: []u8 = [];
        zig {
            out = self.ptr[start .. start + len];
        }
        return out;
    }

    pub fn allocAligned(*_, len: usize, align_pow2: usize): ([]u8)? {
        if (align_pow2 == 0 || (align_pow2 & (align_pow2 - 1)) != 0) {
            return null;
        }
        const mask = align_pow2 - 1;
        const aligned_offset = (_.offset + mask) & ~mask;
        if (aligned_offset + len > _.cap) {
            return null;
        }
        _.offset = aligned_offset + len;
        let out: []u8 = [];
        zig {
            out = self.ptr[aligned_offset .. aligned_offset + len];
        }
        return out;
    }

    pub fn used(): usize {
        return _.offset;
    }

    pub fn remaining(): usize {
        return _.cap - _.offset;
    }

    pub fn reset(*_) {
        _.offset = 0;
    }

    pub fn release(*_) {
        zig {
            if (self.cap > 0) {
                std.heap.page_allocator.free(self.ptr[0..self.cap]);
                self.cap = 0;
                self.offset = 0;
            }
        }
    }
}

// Ownership-aware buffer wrapper (Absorbed from bun/src/bun_alloc/maybe_owned.cp)
pub struct MaybeOwnedBuf {
    data: []const u8 = "";
    owned: bool = false;

    pub static fn initBorrowed(buf: []const u8): MaybeOwnedBuf {
        return MaybeOwnedBuf{
            data: buf,
            owned: false,
        };
    }

    pub static fn initOwnedCopy(buf: []const u8): MaybeOwnedBuf {
        let duped: []const u8 = "";
        zig {
            duped = std.heap.page_allocator.dupe(u8, buf) catch unreachable;
        }
        return MaybeOwnedBuf{
            data: duped,
            owned: true,
        };
    }

    pub fn isOwned(): bool {
        return _.owned;
    }

    pub fn slice(): []const u8 {
        return _.data;
    }

    pub fn len(): usize {
        return _.data.len;
    }

    pub fn release(*_) {
        if (_.owned && _.data.len > 0) {
            zig {
                std.heap.page_allocator.free(self.data);
            }
        }
        _.data = "";
        _.owned = false;
    }
}

// Single-allocation high-water-mark & quota-enforced reusable heap allocator
// (Absorbed from /data/dev/bun/src/bun_alloc/MaxHeapAllocator.cp)
pub struct MaxHeapAllocator {
    ptr: [*]u8! = undefined;
    capacity_bytes: usize = 0;
    current_len: usize = 0;
    max_limit: usize = 0;

    pub static fn init(max_limit: usize = 0): MaxHeapAllocator {
        return MaxHeapAllocator{
            ptr: undefined,
            capacity_bytes: 0,
            current_len: 0,
            max_limit: max_limit,
        };
    }

    pub fn alloc(*_, len: usize): ([]u8)? {
        if (_.max_limit > 0 && len > _.max_limit) {
            return null;
        }
        if (len == 0) {
            _.current_len = 0;
            let empty_out: []u8 = [];
            return empty_out;
        }
        if (len > _.capacity_bytes) {
            zig {
                if (self.capacity_bytes > 0) {
                    std.heap.page_allocator.free(self.ptr[0..self.capacity_bytes]);
                }
                const new_buf = std.heap.page_allocator.alloc(u8, len) catch return null;
                self.ptr = new_buf.ptr;
                self.capacity_bytes = len;
            }
        }
        _.current_len = len;
        let out: []u8 = [];
        zig {
            out = self.ptr[0..len];
        }
        return out;
    }

    pub fn capacity(): usize {
        return _.capacity_bytes;
    }

    pub fn used(): usize {
        return _.current_len;
    }

    pub fn maxQuota(): usize {
        return _.max_limit;
    }

    pub fn reset(*_) {
        _.current_len = 0;
    }

    pub fn deinit(*_) {
        zig {
            if (self.capacity_bytes > 0) {
                std.heap.page_allocator.free(self.ptr[0..self.capacity_bytes]);
                self.capacity_bytes = 0;
                self.current_len = 0;
            }
        }
    }

    pub fn release(*_) {
        _.deinit();
    }
}
