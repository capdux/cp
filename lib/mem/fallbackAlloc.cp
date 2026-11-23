// Source: Absorbed from /data/dev/bun/src/bun_alloc/BufferFallbackAllocator.cp | Synced: 2026-11-22T12:43:45Z
//! Stack-first / fixed-buffer allocator that transparently falls back to a backing allocator
//! when the inline buffer is exhausted.

import std from "std";

const Allocator = std.mem.Allocator;
const FixedBufferAllocator = std.heap.FixedBufferAllocator;

export struct BufferFallbackAllocator {
    fallback_allocator: Allocator,
    fixed_buffer_allocator: FixedBufferAllocator,

    pub static fn init(buffer: []u8, fallback_allocator: Allocator): BufferFallbackAllocator {
        return BufferFallbackAllocator{
            fallback_allocator: fallback_allocator,
            fixed_buffer_allocator: FixedBufferAllocator.init(buffer),
        };
    }

    const vtable = Allocator.VTable{
        alloc: &allocFn,
        resize: &resizeFn,
        remap: &remapFn,
        free: &freeFn,
    };

    pub fn allocator(*_): Allocator {
        return Allocator{
            ptr: _,
            vtable: &BufferFallbackAllocator.vtable,
        };
    }

    pub fn ownsBufferPtr(ptr: [*]const u8): bool {
        return _.fixed_buffer_allocator.ownsPtr(ptr);
    }

    pub fn usedBytes(): usize {
        return _.fixed_buffer_allocator.end_index;
    }

    pub fn capacity(): usize {
        return _.fixed_buffer_allocator.buffer.len;
    }

    pub fn remainingBytes(): usize {
        return _.fixed_buffer_allocator.buffer.len - _.fixed_buffer_allocator.end_index;
    }

    pub fn reset(*_) {
        _.fixed_buffer_allocator.reset();
    }

    static fn allocFn(ctx: *!, len: usize, alignment: std.mem.Alignment, ra: usize): ([*]u8!)? {
        const inst: *BufferFallbackAllocator! = unsafe { @ptrCast(@alignCast(ctx)) };
        return FixedBufferAllocator.alloc(
            &inst.fixed_buffer_allocator,
            len,
            alignment,
            ra,
        ) ?? inst.fallback_allocator.rawAlloc(len, alignment, ra);
    }

    static fn resizeFn(ctx: *!, buf: []u8, alignment: std.mem.Alignment, new_len: usize, ra: usize): bool {
        const inst: *BufferFallbackAllocator! = unsafe { @ptrCast(@alignCast(ctx)) };
        if (inst.fixed_buffer_allocator.ownsPtr(buf.ptr)) {
            return FixedBufferAllocator.resize(
                &inst.fixed_buffer_allocator,
                buf,
                alignment,
                new_len,
                ra,
            );
        }
        return inst.fallback_allocator.rawResize(buf, alignment, new_len, ra);
    }

    static fn remapFn(ctx: *!, memory: []u8, alignment: std.mem.Alignment, new_len: usize, ra: usize): ([*]u8!)? {
        const inst: *BufferFallbackAllocator! = unsafe { @ptrCast(@alignCast(ctx)) };
        if (inst.fixed_buffer_allocator.ownsPtr(memory.ptr)) {
            return FixedBufferAllocator.remap(
                &inst.fixed_buffer_allocator,
                memory,
                alignment,
                new_len,
                ra,
            );
        }
        return inst.fallback_allocator.rawRemap(memory, alignment, new_len, ra);
    }

    static fn freeFn(ctx: *!, buf: []u8, alignment: std.mem.Alignment, ra: usize) {
        const inst: *BufferFallbackAllocator! = unsafe { @ptrCast(@alignCast(ctx)) };
        if (inst.fixed_buffer_allocator.ownsPtr(buf.ptr)) {
            FixedBufferAllocator.free(
                &inst.fixed_buffer_allocator,
                buf,
                alignment,
                ra,
            );
            return;
        }
        inst.fallback_allocator.rawFree(buf, alignment, ra);
    }
}
