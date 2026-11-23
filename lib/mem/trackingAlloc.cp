// Source: cp/lib/bun_alloc/tracking.cp (auditing & leak-detecting wrapper allocator)

import std from "std";

const Allocator = std.mem.Allocator;
const Alignment = std.mem.Alignment;

pub struct TrackingAllocator {
    child_allocator: Allocator,
    alloc_count: usize,
    free_count: usize,
    realloc_count: usize,
    live_bytes: usize,
    peak_bytes: usize,
    total_allocated_bytes: usize,

    pub static fn init(child_allocator: Allocator): TrackingAllocator {
        return TrackingAllocator{
            child_allocator: child_allocator,
            alloc_count: 0,
            free_count: 0,
            realloc_count: 0,
            live_bytes: 0,
            peak_bytes: 0,
            total_allocated_bytes: 0,
        };
    }

    pub fn hasLeaks(): bool {
        return _.live_bytes != 0;
    }

    pub fn resetStats(*_) {
        _.alloc_count = 0;
        _.free_count = 0;
        _.realloc_count = 0;
        _.live_bytes = 0;
        _.peak_bytes = 0;
        _.total_allocated_bytes = 0;
    }

    pub fn formatStats(alloc: Allocator): ![]u8 {
        return std.fmt.allocPrint(
            alloc,
            "allocs={d} frees={d} reallocs={d} live={d} peak={d} total={d}",
            .{
                _.alloc_count,
                _.free_count,
                _.realloc_count,
                _.live_bytes,
                _.peak_bytes,
                _.total_allocated_bytes,
            },
        );
    }

    const vtable = Allocator.VTable{
        .alloc = &vtableAlloc,
        .resize = &vtableResize,
        .remap = &vtableRemap,
        .free = &vtableFree,
    };

    pub fn allocator(*_): Allocator {
        return Allocator{
            .ptr = _,
            .vtable = &TrackingAllocator.vtable,
        };
    }

    fn recordGrowth(*_, old_len: usize, new_len: usize) {
        if (new_len > old_len) {
            const delta = new_len - old_len;
            _.live_bytes += delta;
            _.total_allocated_bytes += delta;
            if (_.live_bytes > _.peak_bytes) {
                _.peak_bytes = _.live_bytes;
            }
        } else {
            const delta = old_len - new_len;
            if (_.live_bytes >= delta) {
                _.live_bytes -= delta;
            } else {
                _.live_bytes = 0;
            }
        }
    }

    static fn vtableAlloc(ctx: *!, len: usize, alignment: Alignment, ra: usize): ([*]u8)? {
        const inst: *TrackingAllocator = unsafe { @ptrCast(@alignCast(ctx)) };
        const ptr = inst.child_allocator.rawAlloc(len, alignment, ra) orelse return null;
        inst.alloc_count += 1;
        inst.live_bytes += len;
        inst.total_allocated_bytes += len;
        if (inst.live_bytes > inst.peak_bytes) {
            inst.peak_bytes = inst.live_bytes;
        }
        return ptr;
    }

    static fn vtableResize(ctx: *!, buf: []u8, alignment: Alignment, new_len: usize, ra: usize): bool {
        const inst: *TrackingAllocator = unsafe { @ptrCast(@alignCast(ctx)) };
        if (!inst.child_allocator.rawResize(buf, alignment, new_len, ra)) {
            return false;
        }
        inst.realloc_count += 1;
        inst.recordGrowth(buf.len, new_len);
        return true;
    }

    static fn vtableRemap(ctx: *!, memory: []u8, alignment: Alignment, new_len: usize, ra: usize): ([*]u8)? {
        const inst: *TrackingAllocator = unsafe { @ptrCast(@alignCast(ctx)) };
        const ptr = inst.child_allocator.rawRemap(memory, alignment, new_len, ra) orelse return null;
        inst.realloc_count += 1;
        inst.recordGrowth(memory.len, new_len);
        return ptr;
    }

    static fn vtableFree(ctx: *!, buf: []u8, alignment: Alignment, ra: usize) {
        const inst: *TrackingAllocator = unsafe { @ptrCast(@alignCast(ctx)) };
        inst.child_allocator.rawFree(buf, alignment, ra);
        inst.free_count += 1;
        if (inst.live_bytes >= buf.len) {
            inst.live_bytes -= buf.len;
        } else {
            inst.live_bytes = 0;
        }
    }
}
