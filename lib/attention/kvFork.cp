// Origin: Pure-CP Stdlib (Zero-Alloc Best-of-N / Tree-of-Thoughts Shared KV-Cache RefCount & CoW Fork Manager) | Created: 2026-11-23
// Pure CP zero-heap-allocation shared KV-Cache reference counting & Copy-on-Write (CoW) fork manager:
//   - Fixed 256-block physical KV pool with O(1) free-stack allocator + up to 32 concurrent sequences
//     (up to 32 logical blocks each), supporting content-hash (`block_hash: u64`) zero-copy prefix dedup
//   - O(1) logical-table copy sequence forking (`forkSequence`) incrementing shared physical block
//     refcounts for Best-of-N, Beam Search, and Tree-of-Thoughts parallel exploration
//   - Copy-on-Write write preparation (`prepareWriteBlock`): in-place write when `refcount == 1`
//     (clearing stale content hash), or atomic new-block allocation + old-refcount decrement returning
//     `{ old_phys, new_phys, copied: true }` when `refcount > 1`
//   - Leak-free sequence release (`releaseSequence`) reclaiming zero-refcount blocks immediately,
//     plus `memorySavingsPermille()` and full telemetry snapshot

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum physical KV blocks managed inline (zero dynamic heap allocation).
pub const KV_FORK_MAX_PHYS_BLOCKS: usize = 256;

/// Maximum concurrent inference sequences tracked inline.
pub const KV_FORK_MAX_SEQS: usize = 32;

/// Maximum logical blocks per sequence.
pub const KV_FORK_MAX_LOGICAL_BLOCKS: usize = 32;

/// Sentinel indicating an invalid / unassigned physical block index.
pub const KV_FORK_INVALID_BLOCK: u16 = 0xFFFF;

/// Operation status codes for KV fork manager mutations.
pub enum KvForkOpStatus {
    ok,
    invalid_arg,
    seq_not_found,
    duplicate_seq_id,
    seq_capacity_full,
    seq_blocks_full,
    out_of_phys_blocks,
    invalid_logical_index,
}

/// Inline metadata for a single physical KV block.
pub struct KvForkPhysBlock {
    refcount: u16 = 0,
    /// Optional content hash (`0` = unhashed/private mutable block; non-zero = shareable immutable prefix).
    block_hash: u64 = 0,
}

/// Inline logical-to-physical block table for a single inference sequence.
pub struct KvForkSequence {
    active: bool = false,
    seq_id: u64 = 0,
    num_blocks: usize = 0,
    logical_to_phys: [KV_FORK_MAX_LOGICAL_BLOCKS]u16 = [KV_FORK_INVALID_BLOCK; KV_FORK_MAX_LOGICAL_BLOCKS],
}

/// Result returned by `appendBlock`.
pub struct KvForkAppendOutcome {
    status: KvForkOpStatus = KvForkOpStatus.ok,
    phys_block: u16 = KV_FORK_INVALID_BLOCK,
    reused_by_hash: bool = false,
}

/// Write plan returned by `prepareWriteBlock` (Copy-on-Write decision).
pub struct KvForkWritePlan {
    status: KvForkOpStatus = KvForkOpStatus.ok,
    old_phys: u16 = KV_FORK_INVALID_BLOCK,
    new_phys: u16 = KV_FORK_INVALID_BLOCK,
    /// When `true`, caller must copy KV tensor data from `old_phys` into `new_phys` before mutating.
    copied: bool = false,
}

/// Global memory efficiency & reference-counting telemetry snapshot.
pub struct KvForkStats {
    phys_capacity: usize = 0,
    phys_used: usize = 0,
    phys_free: usize = 0,
    active_sequences: usize = 0,
    total_logical_refs: usize = 0,
    memory_savings_permille: u16 = 0,
    total_forks: u64 = 0,
    total_cow_copies: u64 = 0,
    total_hash_dedups: u64 = 0,
    total_releases: u64 = 0,
}

// ============================================================================
// 2. Core Zero-Alloc Shared KV-Cache CoW Fork Manager
// ============================================================================

/// Zero-heap-allocation shared KV-Cache reference-counting & Copy-on-Write fork manager.
pub struct KvForkManager {
    phys_capacity: usize = KV_FORK_MAX_PHYS_BLOCKS,
    phys_blocks: [KV_FORK_MAX_PHYS_BLOCKS]KvForkPhysBlock = [KvForkPhysBlock{}; KV_FORK_MAX_PHYS_BLOCKS],
    free_stack: [KV_FORK_MAX_PHYS_BLOCKS]u16 = [0; KV_FORK_MAX_PHYS_BLOCKS],
    free_top: usize = 0,
    seqs: [KV_FORK_MAX_SEQS]KvForkSequence = [KvForkSequence{}; KV_FORK_MAX_SEQS],
    active_seqs: usize = 0,
    total_forks: u64 = 0,
    total_cow_copies: u64 = 0,
    total_hash_dedups: u64 = 0,
    total_releases: u64 = 0,

    /// Initializes a manager with `pool_blocks` physical KV blocks (`1..=KV_FORK_MAX_PHYS_BLOCKS`).
    /// Returns `null` if `pool_blocks == 0` or `pool_blocks > KV_FORK_MAX_PHYS_BLOCKS`.
    pub static fn init(pool_blocks: usize): KvForkManager? {
        if (pool_blocks == 0 || pool_blocks > KV_FORK_MAX_PHYS_BLOCKS) return null;
        let mgr = KvForkManager{
            phys_capacity: pool_blocks,
            free_top: pool_blocks,
        };
        // Populate free stack so block 0 is popped first
        let i: usize = 0;
        while (i < pool_blocks) : (i += 1) {
            mgr.free_stack[i] = (pool_blocks - 1 - i) as u16;
        }
        return mgr;
    }

    /// Initializes a manager with the full 256-block physical KV pool.
    pub static fn initDefault(): KvForkManager {
        return KvForkManager.init(KV_FORK_MAX_PHYS_BLOCKS) ?? KvForkManager{};
    }

    // ---- Internal Helpers ----

    fn findSeqSlot(seq_id: u64): usize? {
        if (seq_id == 0) return null;
        let i: usize = 0;
        while (i < KV_FORK_MAX_SEQS) : (i += 1) {
            if (_.seqs[i].active && _.seqs[i].seq_id == seq_id) return i;
        }
        return null;
    }

    fn findFreeSeqSlot(): usize? {
        let i: usize = 0;
        while (i < KV_FORK_MAX_SEQS) : (i += 1) {
            if (!_.seqs[i].active) return i;
        }
        return null;
    }

    fn allocPhysBlock(*_, block_hash: u64): u16? {
        if (_.free_top == 0) return null;
        _.free_top -= 1;
        const phys = _.free_stack[_.free_top];
        _.phys_blocks[phys as usize] = KvForkPhysBlock{
            refcount: 1,
            block_hash: block_hash,
        };
        return phys;
    }

    fn releasePhysBlock(*_, phys: u16) {
        const idx = phys as usize;
        if (idx >= _.phys_capacity) return;
        if (_.phys_blocks[idx].refcount == 0) return;

        _.phys_blocks[idx].refcount -= 1;
        if (_.phys_blocks[idx].refcount == 0) {
            _.phys_blocks[idx].block_hash = 0;
            if (_.free_top < _.phys_capacity) {
                _.free_stack[_.free_top] = phys;
                _.free_top += 1;
            }
        }
    }

    fn findPhysByHash(block_hash: u64): u16? {
        if (block_hash == 0) return null;
        let i: usize = 0;
        while (i < _.phys_capacity) : (i += 1) {
            if (_.phys_blocks[i].refcount > 0 && _.phys_blocks[i].block_hash == block_hash) {
                return i as u16;
            }
        }
        return null;
    }

    // ---- Sequence & KV Block Lifecycle API ----

    /// Creates a new empty inference sequence with unique non-zero `seq_id`.
    pub fn createSequence(*_, seq_id: u64): KvForkOpStatus {
        if (seq_id == 0) return KvForkOpStatus.invalid_arg;
        if (_.findSeqSlot(seq_id) != null) return KvForkOpStatus.duplicate_seq_id;
        const slot = _.findFreeSeqSlot() ?? return KvForkOpStatus.seq_capacity_full;

        _.seqs[slot] = KvForkSequence{
            active: true,
            seq_id: seq_id,
            num_blocks: 0,
        };
        _.active_seqs += 1;
        return KvForkOpStatus.ok;
    }

    /// Appends a logical KV block to `seq_id`.
    /// When `block_hash != 0` and a live physical block with the same hash exists, reuses it
    /// zero-copy by incrementing its refcount (`reused_by_hash = true`).
    pub fn appendBlock(*_, seq_id: u64, block_hash: u64): KvForkAppendOutcome {
        const slot = _.findSeqSlot(seq_id) ?? return KvForkAppendOutcome{
            status: KvForkOpStatus.seq_not_found,
        };
        if (_.seqs[slot].num_blocks >= KV_FORK_MAX_LOGICAL_BLOCKS) {
            return KvForkAppendOutcome{
                status: KvForkOpStatus.seq_blocks_full,
            };
        }

        // Content-hash zero-copy deduplication lookup
        if (existing := _.findPhysByHash(block_hash)) {
            _.phys_blocks[existing as usize].refcount += 1;
            const lidx = _.seqs[slot].num_blocks;
            _.seqs[slot].logical_to_phys[lidx] = existing;
            _.seqs[slot].num_blocks += 1;
            _.total_hash_dedups += 1;
            return KvForkAppendOutcome{
                status: KvForkOpStatus.ok,
                phys_block: existing,
                reused_by_hash: true,
            };
        }

        const phys = _.allocPhysBlock(block_hash) ?? return KvForkAppendOutcome{
            status: KvForkOpStatus.out_of_phys_blocks,
        };
        const lidx = _.seqs[slot].num_blocks;
        _.seqs[slot].logical_to_phys[lidx] = phys;
        _.seqs[slot].num_blocks += 1;
        return KvForkAppendOutcome{
            status: KvForkOpStatus.ok,
            phys_block: phys,
            reused_by_hash: false,
        };
    }

    /// Zero-copy forks `src_seq_id` into `new_seq_id`, duplicating the logical block table
    /// and incrementing refcounts on all shared physical blocks (Best-of-N / Beam / ToT).
    pub fn forkSequence(*_, src_seq_id: u64, new_seq_id: u64): KvForkOpStatus {
        if (src_seq_id == 0 || new_seq_id == 0) return KvForkOpStatus.invalid_arg;
        const src_slot = _.findSeqSlot(src_seq_id) ?? return KvForkOpStatus.seq_not_found;
        if (_.findSeqSlot(new_seq_id) != null) return KvForkOpStatus.duplicate_seq_id;
        const dst_slot = _.findFreeSeqSlot() ?? return KvForkOpStatus.seq_capacity_full;

        const n = _.seqs[src_slot].num_blocks;
        _.seqs[dst_slot] = KvForkSequence{
            active: true,
            seq_id: new_seq_id,
            num_blocks: n,
        };

        let i: usize = 0;
        while (i < n) : (i += 1) {
            const phys = _.seqs[src_slot].logical_to_phys[i];
            _.seqs[dst_slot].logical_to_phys[i] = phys;
            if ((phys as usize) < _.phys_capacity) {
                _.phys_blocks[phys as usize].refcount += 1;
            }
        }

        _.active_seqs += 1;
        _.total_forks += 1;
        return KvForkOpStatus.ok;
    }

    /// Prepares logical block `logical_idx` of `seq_id` for mutation (Copy-on-Write):
    ///   - If `refcount == 1`: allows in-place write (`copied = false`, clears stale `block_hash`).
    ///   - If `refcount > 1`: atomically allocates a fresh physical block, decrements the old block's
    ///     refcount, updates the logical table, and returns `{ old_phys, new_phys, copied: true }`.
    pub fn prepareWriteBlock(*_, seq_id: u64, logical_idx: usize): KvForkWritePlan {
        const slot = _.findSeqSlot(seq_id) ?? return KvForkWritePlan{
            status: KvForkOpStatus.seq_not_found,
        };
        if (logical_idx >= _.seqs[slot].num_blocks) {
            return KvForkWritePlan{
                status: KvForkOpStatus.invalid_logical_index,
            };
        }

        const old_phys = _.seqs[slot].logical_to_phys[logical_idx];
        const rc = _.phys_blocks[old_phys as usize].refcount;
        if (rc <= 1) {
            // Exclusive owner: safe to mutate in place; invalidate cached content hash
            _.phys_blocks[old_phys as usize].block_hash = 0;
            return KvForkWritePlan{
                status: KvForkOpStatus.ok,
                old_phys: old_phys,
                new_phys: old_phys,
                copied: false,
            };
        }

        // Shared block: Copy-on-Write into a fresh physical block
        const new_phys = _.allocPhysBlock(0) ?? return KvForkWritePlan{
            status: KvForkOpStatus.out_of_phys_blocks,
            old_phys: old_phys,
            new_phys: KV_FORK_INVALID_BLOCK,
            copied: false,
        };

        _.releasePhysBlock(old_phys);
        _.seqs[slot].logical_to_phys[logical_idx] = new_phys;
        _.total_cow_copies += 1;

        return KvForkWritePlan{
            status: KvForkOpStatus.ok,
            old_phys: old_phys,
            new_phys: new_phys,
            copied: true,
        };
    }

    /// Seals a finalized, exclusively-owned block with a content hash so future sequences can dedup against it.
    pub fn sealBlockHash(*_, seq_id: u64, logical_idx: usize, block_hash: u64): KvForkOpStatus {
        if (block_hash == 0) return KvForkOpStatus.invalid_arg;
        const slot = _.findSeqSlot(seq_id) ?? return KvForkOpStatus.seq_not_found;
        if (logical_idx >= _.seqs[slot].num_blocks) return KvForkOpStatus.invalid_logical_index;
        const phys = _.seqs[slot].logical_to_phys[logical_idx];
        _.phys_blocks[phys as usize].block_hash = block_hash;
        return KvForkOpStatus.ok;
    }

    /// Releases a sequence, decrementing refcounts on all its blocks and immediately reclaiming
    /// any physical block whose refcount drops to 0.
    pub fn releaseSequence(*_, seq_id: u64): KvForkOpStatus {
        const slot = _.findSeqSlot(seq_id) ?? return KvForkOpStatus.seq_not_found;
        const n = _.seqs[slot].num_blocks;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            _.releasePhysBlock(_.seqs[slot].logical_to_phys[i]);
        }

        _.seqs[slot] = KvForkSequence{};
        if (_.active_seqs > 0) _.active_seqs -= 1;
        _.total_releases += 1;
        return KvForkOpStatus.ok;
    }

    // ---- Queries & Memory Savings Telemetry ----

    /// Returns the physical block index mapped to `(seq_id, logical_idx)`, or `null` if not found / OOB.
    pub fn getPhysicalBlock(seq_id: u64, logical_idx: usize): u16? {
        const slot = _.findSeqSlot(seq_id) ?? return null;
        if (logical_idx >= _.seqs[slot].num_blocks) return null;
        return _.seqs[slot].logical_to_phys[logical_idx];
    }

    /// Returns the current reference count of physical block `phys_block` (`0` if free or OOB).
    pub fn blockRefcount(phys_block: u16): u16 {
        const idx = phys_block as usize;
        if (idx >= _.phys_capacity) return 0;
        return _.phys_blocks[idx].refcount;
    }

    /// Returns the number of logical blocks in `seq_id`, or `null` if not found.
    pub fn sequenceLength(seq_id: u64): usize? {
        const slot = _.findSeqSlot(seq_id) ?? return null;
        return _.seqs[slot].num_blocks;
    }

    /// Computes KV-cache memory savings in permille (`0..=1000`) achieved via sharing/dedup:
    /// `((total_logical_refs - phys_used) * 1000) / total_logical_refs`.
    pub fn memorySavingsPermille(): u16 {
        let logical_total: usize = 0;
        let i: usize = 0;
        while (i < KV_FORK_MAX_SEQS) : (i += 1) {
            if (_.seqs[i].active) logical_total += _.seqs[i].num_blocks;
        }
        const phys_used = _.phys_capacity - _.free_top;
        if (logical_total == 0 || logical_total <= phys_used) return 0;
        return (((logical_total - phys_used) * 1000) / logical_total) as u16;
    }

    /// Returns a complete global KV-cache sharing & telemetry snapshot.
    pub fn stats(): KvForkStats {
        let logical_total: usize = 0;
        let i: usize = 0;
        while (i < KV_FORK_MAX_SEQS) : (i += 1) {
            if (_.seqs[i].active) logical_total += _.seqs[i].num_blocks;
        }
        const phys_used = _.phys_capacity - _.free_top;
        const savings: u16 = if (logical_total == 0 || logical_total <= phys_used)
            0
        else
            (((logical_total - phys_used) * 1000) / logical_total) as u16;

        return KvForkStats{
            phys_capacity: _.phys_capacity,
            phys_used: phys_used,
            phys_free: _.free_top,
            active_sequences: _.active_seqs,
            total_logical_refs: logical_total,
            memory_savings_permille: savings,
            total_forks: _.total_forks,
            total_cow_copies: _.total_cow_copies,
            total_hash_dedups: _.total_hash_dedups,
            total_releases: _.total_releases,
        };
    }
}
