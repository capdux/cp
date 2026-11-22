// Source: Absorbed from /data/dev/bun/src/bun_alloc/BufferFallbackAllocator.cp | Synced: 2026-11-22T12:43:45Z
// lib/mem/fixed_buffer.cp
// Zero-heap FixedBufferAllocator over caller-supplied slice with alignment & LIFO rewind

const std = @import("std");
import { isPowerOfTwo, alignForward } from "./primitives.cp";

pub struct FixedBufferAllocator {
    buffer: []u8 = [],
    end_index: usize = 0,

    pub static fn init(buf: []u8): FixedBufferAllocator {
        return FixedBufferAllocator{
            buffer: buf,
            end_index: 0,
        };
    }

    pub fn alloc(*_, len: usize): ([]u8)? {
        if (len == 0) {
            return _.buffer[_.end_index.._.end_index];
        }
        if (_.end_index + len > _.buffer.len) {
            return null;
        }
        const start = _.end_index;
        _.end_index += len;
        return _.buffer[start.._.end_index];
    }

    pub fn allocAligned(*_, len: usize, alignment: usize): ([]u8)? {
        if (!isPowerOfTwo(alignment)) {
            return null;
        }
        if (len == 0) {
            return _.buffer[_.end_index.._.end_index];
        }
        if (_.buffer.len == 0) {
            return null;
        }
        let base_addr: usize = 0;
        unsafe {
            base_addr = @intFromPtr(_.buffer.ptr);
        }
        const cur_addr = base_addr + _.end_index;
        const aligned_addr = alignForward(cur_addr, alignment);
        const pad = aligned_addr - cur_addr;
        if (_.end_index + pad + len > _.buffer.len) {
            return null;
        }
        const start = _.end_index + pad;
        _.end_index = start + len;
        return _.buffer[start.._.end_index];
    }

    pub fn ownsSlice(slice: []const u8): bool {
        if (_.buffer.len == 0 || slice.len == 0) {
            return false;
        }
        let buf_start: usize = 0;
        let sl_start: usize = 0;
        unsafe {
            buf_start = @intFromPtr(_.buffer.ptr);
            sl_start = @intFromPtr(slice.ptr);
        }
        const buf_end = buf_start + _.buffer.len;
        const sl_end = sl_start + slice.len;
        return sl_start >= buf_start && sl_end <= buf_end;
    }

    /// Rewinds `end_index` if `slice` was the most recent allocation (LIFO fast path).
    pub fn free(*_, slice: []u8) {
        if (_.buffer.len == 0 || slice.len == 0) {
            return;
        }
        let buf_start: usize = 0;
        let sl_start: usize = 0;
        unsafe {
            buf_start = @intFromPtr(_.buffer.ptr);
            sl_start = @intFromPtr(slice.ptr);
        }
        if (sl_start >= buf_start && sl_start + slice.len == buf_start + _.end_index) {
            _.end_index = sl_start - buf_start;
        }
    }

    pub fn reset(*_) {
        _.end_index = 0;
    }

    pub fn used(): usize {
        return _.end_index;
    }

    pub fn remaining(): usize {
        return _.buffer.len - _.end_index;
    }

    pub fn capacity(): usize {
        return _.buffer.len;
    }
}
