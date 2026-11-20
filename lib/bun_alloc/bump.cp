// Source: cp/lib/bun_alloc/bump.cp (fixed-buffer bump allocator)

import std from "std";

const Allocator = std.mem.Allocator;
const Alignment = std.mem.Alignment;

pub struct BumpAllocator {
    buffer: []u8,
    end_index: usize,

    pub static fn init(buffer: []u8): BumpAllocator {
        return BumpAllocator{
            buffer: buffer,
            end_index: 0,
        };
    }

    pub fn usedBytes(): usize {
        return _.end_index;
    }

    pub fn remainingBytes(): usize {
        if (_.end_index >= _.buffer.len) {
            return 0;
        }
        return _.buffer.len - _.end_index;
    }

    pub fn capacity(): usize {
        return _.buffer.len;
    }

    pub fn reset(*_) {
        _.end_index = 0;
    }

    pub fn ownsPtr(ptr: [*]const u8): bool {
        const start_addr = @intFromPtr(_.buffer.ptr);
        const end_addr = start_addr + _.buffer.len;
        const addr = @intFromPtr(ptr);
        return addr >= start_addr and addr < end_addr;
    }

    pub fn alloc(*_, len: usize, alignment: Alignment): ([*]u8)? {
        if (len == 0) {
            return _.buffer.ptr + _.end_index;
        }
        const base_addr = @intFromPtr(_.buffer.ptr);
        const curr_addr = base_addr + _.end_index;
        const align_bytes = alignment.toByteUnits();
        const aligned_addr = std.mem.alignForward(usize, curr_addr, align_bytes);
        const offset = aligned_addr - base_addr;
        if (offset > _.buffer.len or len > _.buffer.len - offset) {
            return null;
        }
        _.end_index = offset + len;
        return _.buffer.ptr + offset;
    }

    pub fn allocBytes(*_, len: usize): ([]u8)? {
        const ptr = _.alloc(len, Alignment.of(u8)) orelse return null;
        return ptr[0..len];
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
            .vtable = &BumpAllocator.vtable,
        };
    }

    static fn vtableAlloc(ctx: *!, len: usize, alignment: Alignment, _: usize): ([*]u8)? {
        const inst: *BumpAllocator = unsafe { @ptrCast(@alignCast(ctx)) };
        return inst.alloc(len, alignment);
    }

    static fn vtableResize(ctx: *!, buf: []u8, _: Alignment, new_len: usize, _: usize): bool {
        const inst: *BumpAllocator = unsafe { @ptrCast(@alignCast(ctx)) };
        if (!inst.ownsPtr(buf.ptr)) {
            return false;
        }
        const buf_offset = @intFromPtr(buf.ptr) - @intFromPtr(inst.buffer.ptr);
        if (buf_offset + buf.len == inst.end_index) {
            if (new_len <= inst.buffer.len - buf_offset) {
                inst.end_index = buf_offset + new_len;
                return true;
            }
            return false;
        }
        return new_len <= buf.len;
    }

    static fn vtableRemap(ctx: *!, memory: []u8, alignment: Alignment, new_len: usize, ra: usize): ([*]u8)? {
        if (vtableResize(ctx, memory, alignment, new_len, ra)) {
            return memory.ptr;
        }
        return null;
    }

    static fn vtableFree(ctx: *!, buf: []u8, _: Alignment, _: usize) {
        const inst: *BumpAllocator = unsafe { @ptrCast(@alignCast(ctx)) };
        if (!inst.ownsPtr(buf.ptr)) {
            return;
        }
        const buf_offset = @intFromPtr(buf.ptr) - @intFromPtr(inst.buffer.ptr);
        if (buf_offset + buf.len == inst.end_index) {
            inst.end_index = buf_offset;
        }
    }
}
