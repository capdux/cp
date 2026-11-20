// Source: cp/lib/bun_alloc/scratch.cp (scoped arena with mark/rewind checkpoints)

import std from "std";

const Allocator = std.mem.Allocator;
const Alignment = std.mem.Alignment;

pub struct Checkpoint {
    chunk_index: usize,
    offset: usize,
}

const Chunk = struct {
    buffer: []u8,
    used: usize,
};

pub struct ScratchArena {
    backing_allocator: Allocator,
    chunks: std.ArrayList(Chunk),
    current_chunk: usize,
    default_chunk_size: usize,

    pub static fn init(backing: Allocator): ScratchArena {
        return ScratchArena.initWithChunkSize(backing, 4096);
    }

    pub static fn initWithChunkSize(backing: Allocator, chunk_size: usize): ScratchArena {
        const effective = if (chunk_size < 64) 64 else chunk_size;
        return ScratchArena{
            backing_allocator: backing,
            chunks: std.ArrayList(Chunk).empty,
            current_chunk: 0,
            default_chunk_size: effective,
        };
    }

    pub fn deinit(*_) {
        for (chunk of _.chunks.items) {
            _.backing_allocator.free(chunk.buffer);
        }
        _.chunks.deinit(_.backing_allocator);
        _.current_chunk = 0;
    }

    pub fn mark(): Checkpoint {
        if (_.chunks.items.len == 0) {
            return Checkpoint{ chunk_index: 0, offset: 0 };
        }
        return Checkpoint{
            chunk_index: _.current_chunk,
            offset: _.chunks.items[_.current_chunk].used,
        };
    }

    pub fn rewind(*_, cp: Checkpoint) {
        if (_.chunks.items.len == 0) {
            return;
        }
        if (cp.chunk_index >= _.chunks.items.len) {
            return;
        }
        _.chunks.items[cp.chunk_index].used = cp.offset;
        let i: usize = cp.chunk_index + 1;
        while (i < _.chunks.items.len) {
            _.chunks.items[i].used = 0;
            i += 1;
        }
        _.current_chunk = cp.chunk_index;
    }

    pub fn resetRetainingCapacity(*_) {
        let i: usize = 0;
        while (i < _.chunks.items.len) {
            _.chunks.items[i].used = 0;
            i += 1;
        }
        _.current_chunk = 0;
    }

    pub fn usedBytes(): usize {
        let total: usize = 0;
        for (chunk of _.chunks.items) {
            total += chunk.used;
        }
        return total;
    }

    pub fn capacityBytes(): usize {
        let total: usize = 0;
        for (chunk of _.chunks.items) {
            total += chunk.buffer.len;
        }
        return total;
    }

    pub fn chunkCount(): usize {
        return _.chunks.items.len;
    }

    static fn tryAllocInChunk(chunk: *Chunk, len: usize, align_bytes: usize): ([*]u8)? {
        const base_addr = @intFromPtr(chunk.buffer.ptr);
        const curr_addr = base_addr + chunk.used;
        const aligned_addr = std.mem.alignForward(usize, curr_addr, align_bytes);
        const offset = aligned_addr - base_addr;
        if (offset > chunk.buffer.len or len > chunk.buffer.len - offset) {
            return null;
        }
        chunk.used = offset + len;
        return chunk.buffer.ptr + offset;
    }

    pub fn alloc(*_, len: usize, alignment: Alignment): ([*]u8)? {
        const align_bytes = alignment.toByteUnits();
        if (_.chunks.items.len > 0) {
            const cur_ptr = tryAllocInChunk(&_.chunks.items[_.current_chunk], len, align_bytes);
            if (cur_ptr != null) {
                return cur_ptr;
            }
            let next_idx = _.current_chunk + 1;
            while (next_idx < _.chunks.items.len) {
                _.chunks.items[next_idx].used = 0;
                const next_ptr = tryAllocInChunk(&_.chunks.items[next_idx], len, align_bytes);
                if (next_ptr != null) {
                    _.current_chunk = next_idx;
                    return next_ptr;
                }
                next_idx += 1;
            }
        }

        const min_needed = len + align_bytes;
        const new_size = if (min_needed > _.default_chunk_size) min_needed else _.default_chunk_size;
        const buf = _.backing_allocator.alloc(u8, new_size) catch return null;
        _.chunks.append(_.backing_allocator, Chunk{
            buffer: buf,
            used: 0,
        }) catch {
            _.backing_allocator.free(buf);
            return null;
        };
        _.current_chunk = _.chunks.items.len - 1;
        return tryAllocInChunk(&_.chunks.items[_.current_chunk], len, align_bytes);
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
            .vtable = &ScratchArena.vtable,
        };
    }

    static fn vtableAlloc(ctx: *!, len: usize, alignment: Alignment, _: usize): ([*]u8)? {
        const inst: *ScratchArena = unsafe { @ptrCast(@alignCast(ctx)) };
        return inst.alloc(len, alignment);
    }

    static fn vtableResize(_: *!, buf: []u8, _: Alignment, new_len: usize, _: usize): bool {
        return new_len <= buf.len;
    }

    static fn vtableRemap(_: *!, memory: []u8, _: Alignment, new_len: usize, _: usize): ([*]u8)? {
        if (new_len <= memory.len) {
            return memory.ptr;
        }
        return null;
    }

    static fn vtableFree(_: *!, _: []u8, _: Alignment, _: usize) {}
}
