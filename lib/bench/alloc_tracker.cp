// Source: cp/lib/bench/alloc_tracker.cp (Counting Allocator for Memory Benchmarks)
// Zero-Overhead Instrumentation Wrapper over Any std.mem.Allocator for Heap Profiling

const std = @import("std");

/// Point-in-time snapshot of heap allocation metrics captured by `CountingAllocator`.
pub struct AllocSnapshot {
    pub alloc_count: u64 = 0;
    pub free_count: u64 = 0;
    pub realloc_count: u64 = 0;
    pub total_bytes_allocated: u64 = 0;
    pub live_bytes: u64 = 0;
    pub peak_bytes: u64 = 0;

    pub fn hasLeaks(): bool {
        return _.live_bytes > 0;
    }

    pub fn netAllocCount(): i64 {
        return (_.alloc_count as i64) - (_.free_count as i64);
    }
}

/// Non-intrusive `std.mem.Allocator` wrapper that tracks alloc/free/realloc counts,
/// total bytes allocated, live (unfreed) bytes, and high-water peak bytes.
pub struct CountingAllocator {
    backing: std.mem.Allocator;
    alloc_count: u64 = 0;
    free_count: u64 = 0;
    realloc_count: u64 = 0;
    total_bytes_allocated: u64 = 0;
    live_bytes: u64 = 0;
    peak_bytes: u64 = 0;

    pub static fn init(backing: std.mem.Allocator): CountingAllocator {
        return CountingAllocator {
            backing: backing,
            alloc_count: 0,
            free_count: 0,
            realloc_count: 0,
            total_bytes_allocated: 0,
            live_bytes: 0,
            peak_bytes: 0,
        };
    }

    pub fn reset(*_) {
        _.alloc_count = 0;
        _.free_count = 0;
        _.realloc_count = 0;
        _.total_bytes_allocated = 0;
        _.live_bytes = 0;
        _.peak_bytes = 0;
    }

    pub fn snapshot(): AllocSnapshot {
        return AllocSnapshot {
            alloc_count: _.alloc_count,
            free_count: _.free_count,
            realloc_count: _.realloc_count,
            total_bytes_allocated: _.total_bytes_allocated,
            live_bytes: _.live_bytes,
            peak_bytes: _.peak_bytes,
        };
    }

    fn recordGrow(*_, delta: u64) {
        _.total_bytes_allocated += delta;
        _.live_bytes += delta;
        if (_.live_bytes > _.peak_bytes) {
            _.peak_bytes = _.live_bytes;
        }
    }

    fn recordShrink(*_, delta: u64) {
        _.live_bytes = if (_.live_bytes >= delta) (_.live_bytes - delta) else 0;
    }

    static fn vtableAlloc(
        ctx: *!,
        len: usize,
        alignment: std.mem.Alignment,
        ret_addr: usize
    ): [*]u8?! {
        unsafe {
            const inst = ctx as *CountingAllocator!;
            if (ptr := inst.backing.rawAlloc(len, alignment, ret_addr)) {
                inst.alloc_count += 1;
                inst.recordGrow(len as u64);
                return ptr;
            }
            return null;
        }
    }

    static fn vtableResize(
        ctx: *!,
        memory: []u8,
        alignment: std.mem.Alignment,
        new_len: usize,
        ret_addr: usize
    ): bool {
        unsafe {
            const inst = ctx as *CountingAllocator!;
            if (!inst.backing.rawResize(memory, alignment, new_len, ret_addr)) {
                return false;
            }
            inst.realloc_count += 1;
            if (new_len >= memory.len) {
                inst.recordGrow((new_len - memory.len) as u64);
            } else {
                inst.recordShrink((memory.len - new_len) as u64);
            }
            return true;
        }
    }

    static fn vtableRemap(
        ctx: *!,
        memory: []u8,
        alignment: std.mem.Alignment,
        new_len: usize,
        ret_addr: usize
    ): [*]u8?! {
        unsafe {
            const inst = ctx as *CountingAllocator!;
            if (new_ptr := inst.backing.rawRemap(memory, alignment, new_len, ret_addr)) {
                inst.realloc_count += 1;
                if (new_len >= memory.len) {
                    inst.recordGrow((new_len - memory.len) as u64);
                } else {
                    inst.recordShrink((memory.len - new_len) as u64);
                }
                return new_ptr;
            }
            return null;
        }
    }

    static fn vtableFree(
        ctx: *!,
        buf: []u8,
        alignment: std.mem.Alignment,
        ret_addr: usize
    ) {
        unsafe {
            const inst = ctx as *CountingAllocator!;
            inst.backing.rawFree(buf, alignment, ret_addr);
            if (buf.len > 0) {
                inst.free_count += 1;
                inst.recordShrink(buf.len as u64);
            }
        }
    }

    /// Returns a `std.mem.Allocator` interface backed by this `CountingAllocator`.
    /// Note: the `CountingAllocator` must remain at a stable address while in use.
    pub fn allocator(*_): std.mem.Allocator {
        zig {
            const VTABLE = std.mem.Allocator.VTable{
                .alloc = &vtableAlloc,
                .resize = &vtableResize,
                .remap = &vtableRemap,
                .free = &vtableFree,
            };
            return std.mem.Allocator{
                .ptr = self,
                .vtable = &VTABLE,
            };
        }
    }
}
