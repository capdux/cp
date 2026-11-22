// Origin: Pure-CP Stdlib (AI Inference PagedAttention KV-Cache Block Manager, CoW & Prefix Sharing) | Created: 2026-11-22
// Source: cp/lib/attention/kv_cache.cp (Autoregressive KV-Cache with Rollback & PagedAttention Block Manager)
const std = @import("std");
import { AttnError } from "./sdpa.cp";

pub struct PagedKvSlot {
    pub layer: usize,
    pub seq_offset: usize,
    pub num_tokens: usize,
}

pub struct KvCache {
    alloc: std.mem.Allocator,
    num_layers: usize,
    num_kv_heads: usize,
    head_dim: usize,
    max_seq_len: usize,
    layer_lens: []usize,
    k_buf: []f32,
    v_buf: []f32,

    pub fn init(
        alloc: std.mem.Allocator,
        num_layers: usize,
        num_kv_heads: usize,
        head_dim: usize,
        max_seq_len: usize,
    ): AttnError!KvCache {
        if (num_layers == 0 or num_kv_heads == 0 or head_dim == 0 or max_seq_len == 0) {
            return AttnError.InvalidDimension;
        }

        let layer_lens = alloc.alloc(usize, num_layers) catch return AttnError.OutOfMemory;
        errdefer alloc.free(layer_lens);
        @memset(layer_lens, 0);

        const total_elems = num_layers * max_seq_len * num_kv_heads * head_dim;
        let k_buf = alloc.alloc(f32, total_elems) catch return AttnError.OutOfMemory;
        errdefer alloc.free(k_buf);
        @memset(k_buf, 0.0);

        let v_buf = alloc.alloc(f32, total_elems) catch return AttnError.OutOfMemory;
        errdefer alloc.free(v_buf);
        @memset(v_buf, 0.0);

        return KvCache{
            .alloc = alloc,
            .num_layers = num_layers,
            .num_kv_heads = num_kv_heads,
            .head_dim = head_dim,
            .max_seq_len = max_seq_len,
            .layer_lens = layer_lens,
            .k_buf = k_buf,
            .v_buf = v_buf,
        };
    }

    pub fn deinit(*_) {
        _.alloc.free(_.layer_lens);
        _.alloc.free(_.k_buf);
        _.alloc.free(_.v_buf);
    }

    pub fn appendStep(
        *_,
        layer: usize,
        k_vec: []const f32,
        v_vec: []const f32,
    ): AttnError!PagedKvSlot {
        if (layer >= _.num_layers) {
            return AttnError.InvalidLayer;
        }
        const kv_stride = _.num_kv_heads * _.head_dim;
        if (k_vec.len != kv_stride or v_vec.len != kv_stride) {
            return AttnError.InvalidDimension;
        }
        const cur_len = _.layer_lens[layer];
        if (cur_len >= _.max_seq_len) {
            return AttnError.CacheFull;
        }

        const layer_stride = _.max_seq_len * kv_stride;
        const dst_off = layer * layer_stride + cur_len * kv_stride;
        @memcpy(_.k_buf[dst_off .. dst_off + kv_stride], k_vec);
        @memcpy(_.v_buf[dst_off .. dst_off + kv_stride], v_vec);
        _.layer_lens[layer] = cur_len + 1;

        return PagedKvSlot{
            .layer = layer,
            .seq_offset = cur_len,
            .num_tokens = 1,
        };
    }

    pub fn seqLen(layer: usize): usize {
        if (layer >= _.num_layers) {
            return 0;
        }
        return _.layer_lens[layer];
    }

    pub fn keysSlice(layer: usize): []const f32 {
        if (layer >= _.num_layers) {
            return _.k_buf[0..0];
        }
        const kv_stride = _.num_kv_heads * _.head_dim;
        const layer_stride = _.max_seq_len * kv_stride;
        const base = layer * layer_stride;
        const count = _.layer_lens[layer] * kv_stride;
        return _.k_buf[base .. base + count];
    }

    pub fn valuesSlice(layer: usize): []const f32 {
        if (layer >= _.num_layers) {
            return _.v_buf[0..0];
        }
        const kv_stride = _.num_kv_heads * _.head_dim;
        const layer_stride = _.max_seq_len * kv_stride;
        const base = layer * layer_stride;
        const count = _.layer_lens[layer] * kv_stride;
        return _.v_buf[base .. base + count];
    }

    pub fn truncateTo(*_, new_len: usize) {
        let i: usize = 0;
        while (i < _.num_layers) : (i += 1) {
            if (_.layer_lens[i] > new_len) {
                _.layer_lens[i] = new_len;
            }
        }
    }

    pub fn reset(*_) {
        @memset(_.layer_lens, 0);
    }
}

// ============================================================================
// Zero-Heap-Allocation PagedAttention KV-Cache Block Allocator, CoW & Prefix Sharing
// ============================================================================

/// Sentinel invalid physical block ID.
pub const KV_INVALID_BLOCK: u32 = 0xFFFFFFFF;

/// Metadata record for a single physical KV-Cache block.
pub struct KvBlockMeta {
    ref_count: u32 = 0;
    prefix_hash: u64 = 0;
    has_hash: bool = false;
    filled_tokens: u16 = 0;
    last_used_tick: u64 = 0;
}

/// Telemetry snapshot for `PagedKvAllocator`.
/// `free_blocks` counts all allocatable blocks (`ref_count == 0`, including `cached_evictable_blocks`).
pub struct KvAllocStats {
    total_blocks: usize = 0;
    free_blocks: usize = 0;
    cached_evictable_blocks: usize = 0;
    prefix_hits: u64 = 0;
    prefix_misses: u64 = 0;
    cow_copies: u64 = 0;
}

/// Fixed-pool PagedAttention physical KV block manager supporting prefix hash sharing,
/// reference-counted Copy-on-Write (CoW), and LRU eviction of unreferenced prefix cache blocks.
pub struct PagedKvAllocator {
    blocks: []KvBlockMeta;
    block_size: u16 = 16;
    prefix_hits: u64 = 0;
    prefix_misses: u64 = 0;
    cow_copies: u64 = 0;

    /// Initializes the block allocator over caller-owned `blocks` pool.
    /// Returns `null` if `blocks.len == 0`, `block_size == 0`, or pool exceeds `KV_INVALID_BLOCK`.
    static fn init(blocks: *[]KvBlockMeta, block_size: u16): PagedKvAllocator? {
        if (blocks.len == 0 || block_size == 0) return null;
        if (blocks.len >= (KV_INVALID_BLOCK as usize)) return null;

        let i: usize = 0;
        while (i < blocks.len) : (i += 1) {
            blocks[i] = KvBlockMeta{};
        }

        return PagedKvAllocator{
            blocks: blocks,
            block_size: block_size,
            prefix_hits: 0,
            prefix_misses: 0,
            cow_copies: 0,
        };
    }

    fn pickAllocatableSlot(): usize? {
        // 1. Prefer an unhashed free block (ref_count == 0 && !has_hash).
        let i: usize = 0;
        while (i < _.blocks.len) : (i += 1) {
            if (_.blocks[i].ref_count == 0 && !_.blocks[i].has_hash) {
                return i;
            }
        }

        // 2. Otherwise evict the LRU cached prefix block (ref_count == 0 && has_hash, smallest last_used_tick).
        let found = false;
        let best_idx: usize = 0;
        let best_tick: u64 = 0;
        let j: usize = 0;
        while (j < _.blocks.len) : (j += 1) {
            if (_.blocks[j].ref_count == 0 && _.blocks[j].has_hash) {
                if (!found || _.blocks[j].last_used_tick < best_tick) {
                    found = true;
                    best_tick = _.blocks[j].last_used_tick;
                    best_idx = j;
                }
            }
        }
        if (found) return best_idx;
        return null;
    }

    /// Allocates or zero-copy shares a physical block matching `prefix_hash`.
    /// On prefix hit (active or cached-evictable), increments `ref_count` and `prefix_hits`.
    /// On miss, allocates an unhashed free block or LRU-evicts an unreferenced cached block.
    /// Returns `null` if all blocks are actively referenced (`ref_count > 0`).
    fn allocateOrSharePrefix(*_, prefix_hash: u64, tick: u64): u32? {
        let i: usize = 0;
        while (i < _.blocks.len) : (i += 1) {
            if (_.blocks[i].has_hash && _.blocks[i].prefix_hash == prefix_hash) {
                _.blocks[i].ref_count += 1;
                _.blocks[i].last_used_tick = tick;
                _.prefix_hits += 1;
                return i as u32;
            }
        }

        const slot_opt = _.pickAllocatableSlot();
        if (slot_opt == null) return null;
        const slot = slot_opt ?? 0;

        _.blocks[slot] = KvBlockMeta{
            ref_count: 1,
            prefix_hash: prefix_hash,
            has_hash: true,
            filled_tokens: 0,
            last_used_tick: tick,
        };
        _.prefix_misses += 1;
        return slot as u32;
    }

    /// Allocates a fresh unhashed exclusive block (`ref_count = 1`), or `null` if pool is full.
    fn allocateFresh(*_, tick: u64): u32? {
        const slot_opt = _.pickAllocatableSlot();
        if (slot_opt == null) return null;
        const slot = slot_opt ?? 0;

        _.blocks[slot] = KvBlockMeta{
            ref_count: 1,
            prefix_hash: 0,
            has_hash: false,
            filled_tokens: 0,
            last_used_tick: tick,
        };
        return slot as u32;
    }

    /// Increments `ref_count` on an active block (for Beam Search / Parallel Sampling).
    /// Returns `false` if `block_id` is invalid or currently unreferenced.
    fn retainBlock(*_, block_id: u32, tick: u64): bool {
        if (block_id == KV_INVALID_BLOCK) return false;
        const idx: usize = block_id as usize;
        if (idx >= _.blocks.len) return false;
        if (_.blocks[idx].ref_count == 0) return false;

        _.blocks[idx].ref_count += 1;
        _.blocks[idx].last_used_tick = tick;
        return true;
    }

    /// Decrements `ref_count` on an active block.
    /// When `ref_count` reaches `0`, blocks with `has_hash == true` remain cached for prefix reuse
    /// and LRU eviction; unhashed blocks become immediately free.
    /// Returns `false` on invalid `block_id` or double-release (`ref_count == 0`).
    fn releaseBlock(*_, block_id: u32, tick: u64): bool {
        if (block_id == KV_INVALID_BLOCK) return false;
        const idx: usize = block_id as usize;
        if (idx >= _.blocks.len) return false;
        if (_.blocks[idx].ref_count == 0) return false;

        _.blocks[idx].ref_count -= 1;
        _.blocks[idx].last_used_tick = tick;
        if (_.blocks[idx].ref_count == 0 && !_.blocks[idx].has_hash) {
            _.blocks[idx].filled_tokens = 0;
        }
        return true;
    }

    /// Copy-on-Write for mutating a block:
    /// - If `ref_count == 1` (exclusive): invalidates prefix hash and returns `block_id` zero-copy.
    /// - If `ref_count > 1` (shared): allocates a fresh exclusive block, decrements the original
    ///   block's `ref_count`, increments `cow_copies`, and returns the new block ID.
    /// Returns `null` if `block_id` is invalid/unreferenced or if the pool is full on split.
    fn copyOnWrite(*_, block_id: u32, tick: u64): u32? {
        if (block_id == KV_INVALID_BLOCK) return null;
        const idx: usize = block_id as usize;
        if (idx >= _.blocks.len) return null;
        if (_.blocks[idx].ref_count == 0) return null;

        if (_.blocks[idx].ref_count == 1) {
            _.blocks[idx].has_hash = false;
            _.blocks[idx].prefix_hash = 0;
            _.blocks[idx].last_used_tick = tick;
            return block_id;
        }

        const new_id_opt = _.allocateFresh(tick);
        if (new_id_opt == null) return null;
        const new_id = new_id_opt ?? KV_INVALID_BLOCK;

        _.blocks[new_id as usize].filled_tokens = _.blocks[idx].filled_tokens;
        _.blocks[idx].ref_count -= 1;
        _.blocks[idx].last_used_tick = tick;
        _.cow_copies += 1;
        return new_id;
    }

    /// Returns a snapshot of pool utilization, prefix sharing hits/misses, and CoW copies.
    fn stats(): KvAllocStats {
        let free_cnt: usize = 0;
        let cached_cnt: usize = 0;
        let i: usize = 0;
        while (i < _.blocks.len) : (i += 1) {
            if (_.blocks[i].ref_count == 0) {
                free_cnt += 1;
                if (_.blocks[i].has_hash) {
                    cached_cnt += 1;
                }
            }
        }
        return KvAllocStats{
            total_blocks: _.blocks.len,
            free_blocks: free_cnt,
            cached_evictable_blocks: cached_cnt,
            prefix_hits: _.prefix_hits,
            prefix_misses: _.prefix_misses,
            cow_copies: _.cow_copies,
        };
    }
}

/// Fixed-capacity logical-to-physical block mapping table for a single sequence.
pub struct BlockTable {
    phys_blocks: []u32;
    len: usize = 0;
    block_size: u16 = 16;

    /// Initializes a `BlockTable` backed by caller-owned `phys_blocks` slice.
    /// Returns `null` if `block_size == 0`.
    static fn init(phys_blocks: *[]u32, block_size: u16): BlockTable? {
        if (block_size == 0) return null;
        let i: usize = 0;
        while (i < phys_blocks.len) : (i += 1) {
            phys_blocks[i] = KV_INVALID_BLOCK;
        }
        return BlockTable{
            phys_blocks: phys_blocks,
            len: 0,
            block_size: block_size,
        };
    }

    /// Appends a physical block ID to the sequence's logical block table.
    /// Returns `false` if `phys_block_id == KV_INVALID_BLOCK` or table is full.
    fn appendBlock(*_, phys_block_id: u32): bool {
        if (phys_block_id == KV_INVALID_BLOCK) return false;
        if (_.len >= _.phys_blocks.len) return false;
        _.phys_blocks[_.len] = phys_block_id;
        _.len += 1;
        return true;
    }

    /// Maps a logical token offset `logical_pos` to `[phys_block_id, slot_offset]`.
    /// Returns `null` if `logical_pos` lies beyond allocated logical blocks.
    fn mapTokenToPhys(logical_pos: usize): ([2]u32)? {
        if (_.block_size == 0) return null;
        const bs: usize = _.block_size as usize;
        const blk_idx = logical_pos / bs;
        if (blk_idx >= _.len) return null;
        const slot_off = logical_pos % bs;
        const phys_id = _.phys_blocks[blk_idx];
        if (phys_id == KV_INVALID_BLOCK) return null;
        const pair: [2]u32 = [phys_id, slot_off as u32];
        return pair;
    }
}
