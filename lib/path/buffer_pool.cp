// Source: bun/src/paths/path_buffer_pool.cp
// ============================================================================
// lib/path/buffer_pool.cp — Preallocated Buffer Pool for Zero-Allocation Path Ops
// ============================================================================

const std = @import("std");

pub const MAX_PATH_BYTES: usize = 4096;
pub const PathBuffer = [MAX_PATH_BYTES]u8;

const POOL_SIZE: usize = 8;
let pool_buffers: [POOL_SIZE]PathBuffer = undefined;
let pool_in_use: [POOL_SIZE]bool = [false; POOL_SIZE];

pub const PathBufferPool = struct {
    pub fn get(): *PathBuffer {
        let i: usize = 0;
        while (i < POOL_SIZE) {
            if (!pool_in_use[i]) {
                pool_in_use[i] = true;
                return &pool_buffers[i];
            }
            i += 1;
        }
        // Fallback to slot 0 if all slots are in use
        return &pool_buffers[0];
    }

    pub fn put(buf: *const PathBuffer): void {
        let ptr_val = usize(@intFromPtr(buf));
        let base_val = usize(@intFromPtr(&pool_buffers[0]));
        if (ptr_val >= base_val and ptr_val < base_val + POOL_SIZE * MAX_PATH_BYTES) {
            let idx = (ptr_val - base_val) / MAX_PATH_BYTES;
            if (idx < POOL_SIZE) {
                pool_in_use[idx] = false;
            }
        }
    }
};
