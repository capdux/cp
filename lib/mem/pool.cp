// Source: Absorbed from /data/dev/bun/src/bun_alloc/bun_alloc.cp | Synced: 2026-11-21T12:32:04Z
// lib/mem/pool.cp
// O(1) Free-List SlabPool with alignment & double-free protection

const std = @import("std");
import { alignForward } from "./primitives.cp";

const FREE_NONE: usize = std.math.maxInt(usize);

pub struct SlabPool {
    data_ptr: [*]u8! = undefined,
    data_len: usize = 0,
    next_free: []usize = [],
    in_use: []bool = [],
    slot_size_req: usize = 0,
    slot_stride: usize = 0,
    slot_count: usize = 0,
    free_head: usize = FREE_NONE,
    used_count: usize = 0,
    initialized: bool = false,

    /// Initializes a SlabPool of `slot_count` slots, each of at least `slot_size` bytes.
    pub static fn init(slot_size: usize, slot_count: usize): SlabPool {
        if (slot_count == 0) {
            return SlabPool{};
        }
        const req_sz = if (slot_size == 0) 8 else slot_size;
        const stride = alignForward(req_sz, 8);
        const total_bytes = stride * slot_count;

        let d_ptr: [*]u8! = unsafe { @ptrFromInt(8) };
        let nf: []usize = [];
        let iu: []bool = [];
        zig {
            const mem = std.heap.page_allocator.alloc(u8, total_bytes) catch unreachable;
            @memset(mem, 0);
            d_ptr = mem.ptr;
            nf = std.heap.page_allocator.alloc(usize, slot_count) catch unreachable;
            iu = std.heap.page_allocator.alloc(bool, slot_count) catch unreachable;
            for (0..slot_count) |i| {
                nf[i] = if (i + 1 < slot_count) (i + 1) else FREE_NONE;
                iu[i] = false;
            }
        }
        return SlabPool{
            data_ptr: d_ptr,
            data_len: total_bytes,
            next_free: nf,
            in_use: iu,
            slot_size_req: req_sz,
            slot_stride: stride,
            slot_count: slot_count,
            free_head: 0,
            used_count: 0,
            initialized: true,
        };
    }

    /// O(1) free-list pop returning a mutable slice of `slotSize()` bytes, or `null` when full.
    pub fn alloc(*_): ([]u8)? {
        if (!_.initialized || _.free_head == FREE_NONE) {
            return null;
        }
        const idx = _.free_head;
        _.free_head = _.next_free[idx];
        _.next_free[idx] = FREE_NONE;
        _.in_use[idx] = true;
        _.used_count += 1;

        const off = idx * _.slot_stride;
        let out: []u8 = [];
        zig {
            out = self.data_ptr[off .. off + self.slot_size_req];
        }
        return out;
    }

    /// Checks whether `slice` points to a valid slot inside this pool.
    pub fn contains(slice: []const u8): bool {
        if (!_.initialized || _.data_len == 0 || slice.len == 0) {
            return false;
        }
        let base: usize = 0;
        let ptr_addr: usize = 0;
        unsafe {
            base = @intFromPtr(_.data_ptr);
            ptr_addr = @intFromPtr(slice.ptr);
        }
        if (ptr_addr < base || ptr_addr >= base + _.data_len) {
            return false;
        }
        const diff = ptr_addr - base;
        if ((diff % _.slot_stride) != 0) {
            return false;
        }
        return (diff / _.slot_stride) < _.slot_count;
    }

    /// O(1) free-list push with strict double-free & bounds validation.
    /// Returns `true` if freed cleanly, or `false` on double-free / foreign slice.
    pub fn free(*_, slice: []u8): bool {
        if (!_.initialized || _.data_len == 0 || slice.len == 0) {
            return false;
        }
        let base: usize = 0;
        let ptr_addr: usize = 0;
        unsafe {
            base = @intFromPtr(_.data_ptr);
            ptr_addr = @intFromPtr(slice.ptr);
        }
        if (ptr_addr < base || ptr_addr >= base + _.data_len) {
            return false;
        }
        const diff = ptr_addr - base;
        if ((diff % _.slot_stride) != 0) {
            return false;
        }
        const idx = diff / _.slot_stride;
        if (idx >= _.slot_count) {
            return false;
        }
        // Anti-double-free guard
        if (!_.in_use[idx]) {
            return false;
        }
        _.in_use[idx] = false;
        _.next_free[idx] = _.free_head;
        _.free_head = idx;
        _.used_count -= 1;
        return true;
    }

    pub fn slotSize(): usize {
        return _.slot_size_req;
    }

    pub fn capacity(): usize {
        return _.slot_count;
    }

    pub fn used(): usize {
        return _.used_count;
    }

    pub fn available(): usize {
        return _.slot_count - _.used_count;
    }

    pub fn reset(*_) {
        if (!_.initialized) {
            return;
        }
        let i: usize = 0;
        while (i < _.slot_count) {
            _.next_free[i] = if (i + 1 < _.slot_count) (i + 1) else FREE_NONE;
            _.in_use[i] = false;
            i += 1;
        }
        _.free_head = if (_.slot_count > 0) 0 else FREE_NONE;
        _.used_count = 0;
    }

    ~init() {
        if (_.initialized) {
            zig {
                if (self.data_len > 0) {
                    std.heap.page_allocator.free(self.data_ptr[0..self.data_len]);
                }
                if (self.next_free.len > 0) {
                    std.heap.page_allocator.free(self.next_free);
                }
                if (self.in_use.len > 0) {
                    std.heap.page_allocator.free(self.in_use);
                }
            }
            _.data_len = 0;
            _.slot_count = 0;
            _.used_count = 0;
            _.free_head = FREE_NONE;
            _.initialized = false;
        }
    }

    pub fn deinit(*_) {
        _.~init();
    }
}
