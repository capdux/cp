// Origin: Pure-CP Stdlib (vLLM-Style PagedAttention Physical Block Pool, CoW Forking & SIMD @dot/@fma Decode Kernel) | Created: 2026-11-22
// Pure CP zero-heap-allocation Paged KV-Cache & PagedAttention engine:
//   1. `PagedKvBlockPool`: fixed-capacity physical block pool with reference counting, high-watermark & CoW telemetry
//   2. `SeqBlockTable`: logical-to-physical block table with `appendTokenSlot`, zero-copy `forkSequence` & Copy-on-Write (`ensureWritableBlock`)
//   3. `pagedAttentionDecodeF32`: single-head online-softmax PagedAttention decode kernel powered by 8-lane SIMD `@dot` and `@fma`

const std = @import("std");

/// Sentinel representing an unmapped physical KV block slot.
pub const PAGED_KV_INVALID_BLOCK: u32 = 0xFFFFFFFF;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

// ============================================================================
// 1. Physical Block Pool with Reference Counting (`PagedKvBlockPool`)
// ============================================================================

/// Telemetry snapshot for `PagedKvBlockPool`.
pub struct PagedKvPoolStats {
    total_blocks: usize = 0;
    allocated_blocks: usize = 0;
    free_blocks: usize = 0;
    high_watermark: usize = 0;
    utilization: f32 = 0.0;
    cow_copies: u64 = 0;
}

/// Zero-heap-allocation physical KV-cache block allocator with reference counting.
/// `ref_counts[block_id] == 0` indicates a free block; `1` indicates exclusive ownership;
/// `>= 2` indicates a Copy-on-Write shared block across forked sequences.
pub struct PagedKvBlockPool {
    ref_counts: []u32;
    allocated_count: usize = 0;
    high_watermark: usize = 0;
    cow_copies: u64 = 0;

    /// Initializes a block pool over caller-provided `ref_counts` storage (1 slot per physical block).
    /// Returns `null` if `ref_counts.len == 0`.
    static fn init(ref_counts: *[]u32): PagedKvBlockPool? {
        if (ref_counts.len == 0) return null;
        let i: usize = 0;
        while (i < ref_counts.len) : (i += 1) {
            ref_counts[i] = 0;
        }
        return PagedKvBlockPool{
            ref_counts: ref_counts,
            allocated_count: 0,
            high_watermark: 0,
            cow_copies: 0,
        };
    }

    /// Allocates the first free physical block with `ref_count = 1`.
    /// Returns `null` if all physical blocks are currently allocated.
    fn allocBlock(*_): u32? {
        let i: usize = 0;
        while (i < _.ref_counts.len) : (i += 1) {
            if (_.ref_counts[i] == 0) {
                _.ref_counts[i] = 1;
                _.allocated_count += 1;
                if (_.allocated_count > _.high_watermark) {
                    _.high_watermark = _.allocated_count;
                }
                return i as u32;
            }
        }
        return null;
    }

    /// Increments the reference count of an already-allocated physical block (used by CoW fork).
    /// Returns `false` if `block_id` is out of range, free (`ref_count == 0`), or at `u32` max.
    fn retainBlock(*_, block_id: u32): bool {
        const idx: usize = block_id as usize;
        if (idx >= _.ref_counts.len) return false;
        if (_.ref_counts[idx] == 0 || _.ref_counts[idx] == std.math.maxInt(u32)) return false;
        _.ref_counts[idx] += 1;
        return true;
    }

    /// Decrements the reference count of a physical block, reclaiming it to the free pool when it reaches `0`.
    /// Returns `false` if `block_id` is out of range or already free.
    fn releaseBlock(*_, block_id: u32): bool {
        const idx: usize = block_id as usize;
        if (idx >= _.ref_counts.len) return false;
        if (_.ref_counts[idx] == 0) return false;
        _.ref_counts[idx] -= 1;
        if (_.ref_counts[idx] == 0 && _.allocated_count > 0) {
            _.allocated_count -= 1;
        }
        return true;
    }

    /// Returns the current reference count of `block_id` (`0` if free or out of range).
    fn refCount(block_id: u32): u32 {
        const idx: usize = block_id as usize;
        if (idx >= _.ref_counts.len) return 0;
        return _.ref_counts[idx];
    }

    /// Returns the number of free physical blocks remaining.
    fn freeBlocks(): usize {
        return _.ref_counts.len - _.allocated_count;
    }

    /// Returns the number of currently allocated physical blocks.
    fn allocatedBlocks(): usize {
        return _.allocated_count;
    }

    /// Returns a utilization and CoW telemetry snapshot.
    fn stats(): PagedKvPoolStats {
        const total: usize = _.ref_counts.len;
        const util: f32 = if (total == 0) 0.0 else (_.allocated_count as f32) / (total as f32);
        return PagedKvPoolStats{
            total_blocks: total,
            allocated_blocks: _.allocated_count,
            free_blocks: total - _.allocated_count,
            high_watermark: _.high_watermark,
            utilization: util,
            cow_copies: _.cow_copies,
        };
    }
}

// ============================================================================
// 2. Logical Sequence Block Table & Copy-on-Write Forking (`SeqBlockTable`)
// ============================================================================

/// Physical KV address returned by `appendTokenSlot` or `ensureWritableBlock`.
pub struct PagedTokenSlot {
    logical_block: usize = 0;
    physical_block: u32 = PAGED_KV_INVALID_BLOCK;
    slot_in_block: usize = 0;
    did_cow: bool = false;
}

/// Per-sequence logical-to-physical block mapping table supporting zero-copy CoW forking
/// (Beam Search / parallel sampling) and automatic Copy-on-Write separation on mutation.
pub struct SeqBlockTable {
    block_ids: []u32;
    num_blocks: usize = 0;
    seq_len: usize = 0;
    block_size: usize = 16;

    /// Initializes an empty sequence block table over caller-provided `block_ids`.
    /// Returns `null` if `block_size == 0` or `block_ids.len == 0`.
    static fn init(block_ids: *[]u32, block_size: usize): SeqBlockTable? {
        if (block_size == 0 || block_ids.len == 0) return null;
        let i: usize = 0;
        while (i < block_ids.len) : (i += 1) {
            block_ids[i] = PAGED_KV_INVALID_BLOCK;
        }
        return SeqBlockTable{
            block_ids: block_ids,
            num_blocks: 0,
            seq_len: 0,
            block_size: block_size,
        };
    }

    /// Returns the physical block ID mapped to `logical_idx`, or `null` if out of range.
    fn physicalBlockAt(logical_idx: usize): u32? {
        if (logical_idx >= _.num_blocks) return null;
        return _.block_ids[logical_idx];
    }

    /// Ensures logical block `logical_idx` is exclusively writable (`ref_count == 1`).
    /// If the physical block is shared (`ref_count > 1`), allocates a fresh physical block,
    /// copies both K and V data for that block, decrements the old block's refcount, and updates the table.
    fn ensureWritableBlock(
        *_,
        logical_idx: usize,
        pool: *PagedKvBlockPool,
        k_cache_flat: *[]f32,
        v_cache_flat: *[]f32,
        head_dim: usize,
    ): PagedTokenSlot? {
        if (logical_idx >= _.num_blocks || head_dim == 0) return null;
        if (_.block_size > std.math.maxInt(usize) / head_dim) return null;
        const block_elems: usize = _.block_size * head_dim;

        const old_phys: u32 = _.block_ids[logical_idx];
        const rc: u32 = pool.refCount(old_phys);
        if (rc == 0) return null;
        if (rc == 1) {
            return PagedTokenSlot{
                logical_block: logical_idx,
                physical_block: old_phys,
                slot_in_block: 0,
                did_cow: false,
            };
        }

        // Copy-on-Write: allocate a fresh block and duplicate K/V contents before releasing shared block.
        const new_phys: u32 = pool.allocBlock() ?? return null;
        const old_idx: usize = old_phys as usize;
        const new_idx: usize = new_phys as usize;

        if (old_idx > std.math.maxInt(usize) / block_elems || new_idx > std.math.maxInt(usize) / block_elems) {
            * = pool.releaseBlock(new_phys);
            return null;
        }
        const old_off: usize = old_idx * block_elems;
        const new_off: usize = new_idx * block_elems;
        if (old_off + block_elems > k_cache_flat.len || new_off + block_elems > k_cache_flat.len ||
            old_off + block_elems > v_cache_flat.len || new_off + block_elems > v_cache_flat.len) {
            * = pool.releaseBlock(new_phys);
            return null;
        }

        let i: usize = 0;
        while (i < block_elems) : (i += 1) {
            k_cache_flat[new_off + i] = k_cache_flat[old_off + i];
            v_cache_flat[new_off + i] = v_cache_flat[old_off + i];
        }

        * = pool.releaseBlock(old_phys);
        _.block_ids[logical_idx] = new_phys;
        pool.cow_copies += 1;

        return PagedTokenSlot{
            logical_block: logical_idx,
            physical_block: new_phys,
            slot_in_block: 0,
            did_cow: true,
        };
    }

    /// Reserves a writable physical token slot at the end of this sequence.
    /// - Allocates a new physical block when crossing a `block_size` boundary.
    /// - Triggers Copy-on-Write (`ensureWritableBlock`) when appending into a shared partial tail block.
    /// Returns `null` if the block table or physical pool is exhausted.
    fn appendTokenSlot(
        *_,
        pool: *PagedKvBlockPool,
        k_cache_flat: *[]f32,
        v_cache_flat: *[]f32,
        head_dim: usize,
    ): PagedTokenSlot? {
        const logical_idx: usize = _.seq_len / _.block_size;
        const slot_in_block: usize = _.seq_len % _.block_size;

        if (slot_in_block == 0) {
            if (_.num_blocks >= _.block_ids.len) return null;
            const phys: u32 = pool.allocBlock() ?? return null;
            _.block_ids[_.num_blocks] = phys;
            _.num_blocks += 1;
            _.seq_len += 1;
            return PagedTokenSlot{
                logical_block: logical_idx,
                physical_block: phys,
                slot_in_block: 0,
                did_cow: false,
            };
        }

        if (writable := _.ensureWritableBlock(logical_idx, pool, k_cache_flat, v_cache_flat, head_dim)) {
            _.seq_len += 1;
            return PagedTokenSlot{
                logical_block: logical_idx,
                physical_block: writable.physical_block,
                slot_in_block: slot_in_block,
                did_cow: writable.did_cow,
            };
        }
        return null;
    }

    /// Zero-copy Copy-on-Write sequence fork (for Beam Search / parallel sampling).
    /// Shares all physical blocks of this sequence with the child table and increments their refcounts.
    /// Returns `null` if `child_block_ids` is smaller than `num_blocks` or retaining fails.
    fn forkSequence(pool: *PagedKvBlockPool, child_block_ids: *[]u32): SeqBlockTable? {
        if (child_block_ids.len < _.num_blocks) return null;

        let init_i: usize = 0;
        while (init_i < child_block_ids.len) : (init_i += 1) {
            child_block_ids[init_i] = PAGED_KV_INVALID_BLOCK;
        }

        let i: usize = 0;
        while (i < _.num_blocks) : (i += 1) {
            const phys: u32 = _.block_ids[i];
            if (!pool.retainBlock(phys)) {
                // Rollback retained blocks on failure
                let rb: usize = 0;
                while (rb < i) : (rb += 1) {
                    * = pool.releaseBlock(child_block_ids[rb]);
                    child_block_ids[rb] = PAGED_KV_INVALID_BLOCK;
                }
                return null;
            }
            child_block_ids[i] = phys;
        }

        return SeqBlockTable{
            block_ids: child_block_ids,
            num_blocks: _.num_blocks,
            seq_len: _.seq_len,
            block_size: _.block_size,
        };
    }

    /// Releases all physical blocks referenced by this sequence back to `pool` (zero leak).
    fn releaseAll(*_, pool: *PagedKvBlockPool) {
        let i: usize = 0;
        while (i < _.num_blocks) : (i += 1) {
            * = pool.releaseBlock(_.block_ids[i]);
            _.block_ids[i] = PAGED_KV_INVALID_BLOCK;
        }
        _.num_blocks = 0;
        _.seq_len = 0;
    }

    /// Writes a single token's K and V vectors into the physical KV cache at `slot`.
    fn writeKvToken(
        slot: PagedTokenSlot,
        k_vec: []const f32,
        v_vec: []const f32,
        k_cache_flat: *[]f32,
        v_cache_flat: *[]f32,
        head_dim: usize,
    ): bool {
        if (head_dim == 0 || k_vec.len != head_dim || v_vec.len != head_dim) return false;
        if (slot.slot_in_block >= _.block_size) return false;
        if (slot.physical_block == PAGED_KV_INVALID_BLOCK) return false;

        const block_elems: usize = _.block_size * head_dim;
        const base: usize = (slot.physical_block as usize) * block_elems + slot.slot_in_block * head_dim;
        if (base + head_dim > k_cache_flat.len || base + head_dim > v_cache_flat.len) return false;

        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            k_cache_flat[base + d] = k_vec[d];
            v_cache_flat[base + d] = v_vec[d];
        }
        return true;
    }
}

// ============================================================================
// 3. SIMD @dot/@fma PagedAttention Decode Kernel (`pagedAttentionDecodeF32`)
// ============================================================================

/// Zero-heap-allocation single-head PagedAttention decode kernel powered by 8-lane SIMD `@dot` & `@fma`.
///
/// Traverses non-contiguous physical K/V blocks via `seq_table.block_ids` with numerically stable
/// single-pass online Softmax (zero `O(seq_len)` scratch buffer):
///   `out = sum_t softmax_t((query · K_t) * scale) * V_t`
///
/// - Physical K/V layout: `[num_physical_blocks, block_size, head_dim]` flattened.
/// - If `scale <= 0.0`, automatically uses `1.0 / sqrt(head_dim)`.
/// - Returns `false` on empty sequence, dimension mismatch, invalid block mapping, out-of-bounds cache, or `NaN`/`Inf`.
pub fn pagedAttentionDecodeF32(
    query: []const f32,
    seq_table: SeqBlockTable,
    k_cache_flat: []const f32,
    v_cache_flat: []const f32,
    head_dim: usize,
    scale: f32,
    out_vec: *[]f32,
): bool {
    if (head_dim == 0 || query.len != head_dim || out_vec.len < head_dim) return false;
    if (seq_table.seq_len == 0 || seq_table.block_size == 0) return false;
    if (!isFiniteF32(scale)) return false;

    const needed_blocks: usize = (seq_table.seq_len + seq_table.block_size - 1) / seq_table.block_size;
    if (needed_blocks > seq_table.num_blocks) return false;
    if (seq_table.block_size > std.math.maxInt(usize) / head_dim) return false;
    const block_elems: usize = seq_table.block_size * head_dim;

    let qd: usize = 0;
    while (qd < head_dim) : (qd += 1) {
        if (!isFiniteF32(query[qd])) return false;
        out_vec[qd] = 0.0;
    }

    const eff_scale: f32 = if (scale > 0.0) scale else (1.0 / @sqrt(head_dim as f32));
    let running_max: f32 = 0.0 - std.math.inf(f32);
    let running_sum: f32 = 0.0;

    let tok: usize = 0;
    while (tok < seq_table.seq_len) : (tok += 1) {
        const logical_blk: usize = tok / seq_table.block_size;
        const slot: usize = tok % seq_table.block_size;
        const phys_blk: u32 = seq_table.block_ids[logical_blk];
        if (phys_blk == PAGED_KV_INVALID_BLOCK) return false;

        const phys_idx: usize = phys_blk as usize;
        if (phys_idx > std.math.maxInt(usize) / block_elems) return false;
        const base: usize = phys_idx * block_elems + slot * head_dim;
        if (base + head_dim > k_cache_flat.len || base + head_dim > v_cache_flat.len) return false;

        const k_vec: []const f32 = k_cache_flat[base .. base + head_dim];
        const v_vec: []const f32 = v_cache_flat[base .. base + head_dim];

        const dot_qk: f32 = @dot(query, k_vec);
        if (!isFiniteF32(dot_qk)) return false;
        const score: f32 = dot_qk * eff_scale;

        if (score > running_max) {
            const alpha: f32 = if (tok == 0) 0.0 else std.math.exp(running_max - score);
            running_sum = running_sum * alpha + 1.0;
            let d: usize = 0;
            while (d < head_dim) : (d += 1) {
                if (!isFiniteF32(v_vec[d])) return false;
                out_vec[d] = @fma(out_vec[d], alpha, v_vec[d]);
            }
            running_max = score;
        } else {
            const w: f32 = std.math.exp(score - running_max);
            running_sum += w;
            let d: usize = 0;
            while (d < head_dim) : (d += 1) {
                if (!isFiniteF32(v_vec[d])) return false;
                out_vec[d] = @fma(w, v_vec[d], out_vec[d]);
            }
        }
    }

    if (!(running_sum > 0.0) || !isFiniteF32(running_sum)) return false;
    const inv_sum: f32 = 1.0 / running_sum;
    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        out_vec[d] = out_vec[d] * inv_sum;
    }
    return true;
}
