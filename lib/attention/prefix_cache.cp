// Origin: Pure-CP Stdlib (AI Inference Block-Level KV Prefix Cache & LRU Eviction) | Created: 2026-11-22
// Pure CP zero-allocation block-level KV prefix cache (SGLang RadixAttention / vLLM APC style):
// chained FNV-1a 64-bit prefix hashing, longest-prefix matching, ref-counted in-flight pinning,
// and pinned-safe LRU eviction over a fixed caller-owned slot pool.

const std = @import("std");

/// Sentinel physical KV block ID indicating an invalid or unallocated block.
pub const PREFIX_INVALID_BLOCK: u32 = 0xFFFFFFFF;

/// Fixed-layout metadata for a single cached KV prefix block slot.
pub struct PrefixBlockEntry {
    prefix_hash: u64 = 0,
    parent_hash: u64 = 0,
    kv_block_id: u32 = PREFIX_INVALID_BLOCK,
    ref_count: u32 = 0,
    last_access_tick: u64 = 0,
    token_count: u16 = 0,
    occupied: bool = false,
}

/// Outcome of `KvPrefixCache.matchLongestPrefix`.
pub struct PrefixMatchResult {
    matched_blocks: usize = 0,
    matched_tokens: usize = 0,
    last_prefix_hash: u64 = 0,
}

/// Runtime telemetry snapshot for `KvPrefixCache`.
pub struct PrefixCacheStats {
    active_blocks: usize = 0,
    pinned_blocks: usize = 0,
    hits: u64 = 0,
    misses: u64 = 0,
    evictions: u64 = 0,
    tokens_saved: u64 = 0,
}

// ============================================================================
// 1. Chained FNV-1a 64-bit Block Prefix Hash
// ============================================================================

/// Computes a chained FNV-1a 64-bit hash combining `parent_hash` with `block_tokens`.
/// Guarantees that identical trailing blocks with different preceding prefixes produce
/// distinct hashes, preventing false prefix-cache hits across diverged prompts.
pub fn computePrefixBlockHash(parent_hash: u64, block_tokens: []const u32): u64 {
    let hasher = std.hash.Fnv1a_64.init();

    const p_bytes: [8]u8 = [
        (parent_hash & 0xFF) as u8,
        ((parent_hash >> 8) & 0xFF) as u8,
        ((parent_hash >> 16) & 0xFF) as u8,
        ((parent_hash >> 24) & 0xFF) as u8,
        ((parent_hash >> 32) & 0xFF) as u8,
        ((parent_hash >> 40) & 0xFF) as u8,
        ((parent_hash >> 48) & 0xFF) as u8,
        ((parent_hash >> 56) & 0xFF) as u8,
    ];
    hasher.update(p_bytes[0..]);

    const len64: u64 = block_tokens.len as u64;
    const len_bytes: [8]u8 = [
        (len64 & 0xFF) as u8,
        ((len64 >> 8) & 0xFF) as u8,
        ((len64 >> 16) & 0xFF) as u8,
        ((len64 >> 24) & 0xFF) as u8,
        ((len64 >> 32) & 0xFF) as u8,
        ((len64 >> 40) & 0xFF) as u8,
        ((len64 >> 48) & 0xFF) as u8,
        ((len64 >> 56) & 0xFF) as u8,
    ];
    hasher.update(len_bytes[0..]);

    let i: usize = 0;
    while (i < block_tokens.len) : (i += 1) {
        const tok: u32 = block_tokens[i];
        const t_bytes: [4]u8 = [
            (tok & 0xFF) as u8,
            ((tok >> 8) & 0xFF) as u8,
            ((tok >> 16) & 0xFF) as u8,
            ((tok >> 24) & 0xFF) as u8,
        ];
        hasher.update(t_bytes[0..]);
    }

    const h: u64 = hasher.final();
    // Reserve 0 exclusively as the root parent_hash sentinel.
    if (h == 0) {
        return 1;
    }
    return h;
}

// ============================================================================
// 2. KvPrefixCache: Zero-Allocation Block-Level Prefix Cache with LRU Eviction
// ============================================================================

/// Zero-allocation block-level KV prefix cache over a fixed caller-owned slot pool.
pub struct KvPrefixCache {
    entries: []PrefixBlockEntry,
    block_size: u16 = 0,
    hits: u64 = 0,
    misses: u64 = 0,
    evictions: u64 = 0,
    tokens_saved: u64 = 0,

    /// Initializes a `KvPrefixCache` over `entries` with fixed `block_size` tokens per block.
    /// Resets all slots so uninitialized stack buffers are safe.
    /// Returns `null` if `entries.len == 0` or `block_size == 0`.
    pub static fn init(
        entries: *[]PrefixBlockEntry,
        block_size: u16,
    ): KvPrefixCache? {
        if (entries.len == 0 || block_size == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            entries[i] = PrefixBlockEntry{};
        }
        return KvPrefixCache{
            entries: entries,
            block_size: block_size,
            hits: 0,
            misses: 0,
            evictions: 0,
            tokens_saved: 0,
        };
    }

    fn findByPrefixHash(prefix_hash: u64): usize? {
        if (prefix_hash == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].prefix_hash == prefix_hash) {
                return i;
            }
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (!_.entries[i].occupied) {
                return i;
            }
        }
        return null;
    }

    /// Matches the longest contiguous prefix of full `block_size` blocks from `tokens`.
    /// Writes matched physical `kv_block_id`s into `out_block_ids`, updates `last_access_tick`,
    /// and accumulates `hits`, `misses`, and `tokens_saved` telemetry.
    pub fn matchLongestPrefix(
        *_,
        tokens: []const u32,
        tick: u64,
        out_block_ids: *[]u32,
    ): PrefixMatchResult {
        const bs: usize = _.block_size as usize;
        if (bs == 0 || tokens.len < bs) {
            return PrefixMatchResult{};
        }

        const total_full_blocks: usize = tokens.len / bs;
        let max_blocks: usize = total_full_blocks;
        if (out_block_ids.len < max_blocks) {
            max_blocks = out_block_ids.len;
        }

        let parent_hash: u64 = 0;
        let matched_blocks: usize = 0;
        let matched_tokens: usize = 0;

        let b: usize = 0;
        while (b < max_blocks) : (b += 1) {
            const base: usize = b * bs;
            const h: u64 = computePrefixBlockHash(parent_hash, tokens[base .. base + bs]);
            if (idx := _.findByPrefixHash(h)) {
                const entry = _.entries[idx];
                if (entry.parent_hash == parent_hash && entry.token_count == _.block_size) {
                    _.entries[idx].last_access_tick = tick;
                    out_block_ids[matched_blocks] = entry.kv_block_id;
                    matched_blocks += 1;
                    matched_tokens += bs;
                    parent_hash = h;
                    _.hits += 1;
                    _.tokens_saved += bs as u64;
                    continue;
                }
            }
            _.misses += 1;
            break;
        }

        return PrefixMatchResult{
            matched_blocks: matched_blocks,
            matched_tokens: matched_tokens,
            last_prefix_hash: parent_hash,
        };
    }

    /// Inserts or refreshes a chain of full prefix blocks from `tokens` mapped to `block_ids`.
    /// When the slot pool is full, evicts the least-recently-used unpinned (`ref_count == 0`) slot.
    /// If all slots are pinned (`ref_count > 0`), refuses to overwrite any pinned block and stops.
    /// Returns the number of prefix blocks successfully inserted or refreshed.
    pub fn insertPrefixBlocks(
        *_,
        tokens: []const u32,
        block_ids: []const u32,
        tick: u64,
    ): usize {
        const bs: usize = _.block_size as usize;
        if (bs == 0 || tokens.len < bs || block_ids.len == 0) {
            return 0;
        }

        let n_blocks: usize = tokens.len / bs;
        if (block_ids.len < n_blocks) {
            n_blocks = block_ids.len;
        }

        let parent_hash: u64 = 0;
        let inserted: usize = 0;

        let b: usize = 0;
        while (b < n_blocks) : (b += 1) {
            const kv_id: u32 = block_ids[b];
            if (kv_id == PREFIX_INVALID_BLOCK) {
                break;
            }
            const base: usize = b * bs;
            const h: u64 = computePrefixBlockHash(parent_hash, tokens[base .. base + bs]);

            if (existing_idx := _.findByPrefixHash(h)) {
                _.entries[existing_idx].kv_block_id = kv_id;
                _.entries[existing_idx].last_access_tick = tick;
                parent_hash = h;
                inserted += 1;
                continue;
            }

            let target_slot_opt: usize? = _.findFreeSlot();
            if (target_slot_opt == null) {
                if (evicted_id := _.evictLruUnpinned()) {
                    * = evicted_id;
                    target_slot_opt = _.findFreeSlot();
                } else {
                    // All slots are pinned; refuse to overwrite any pinned entry.
                    break;
                }
            }

            if (slot_idx := target_slot_opt) {
                _.entries[slot_idx] = PrefixBlockEntry{
                    prefix_hash: h,
                    parent_hash: parent_hash,
                    kv_block_id: kv_id,
                    ref_count: 0,
                    last_access_tick: tick,
                    token_count: _.block_size,
                    occupied: true,
                };
                parent_hash = h;
                inserted += 1;
            } else {
                break;
            }
        }

        return inserted;
    }

    /// Increments the in-flight reference count of a cached prefix block to protect it from eviction.
    pub fn pinBlock(*_, prefix_hash: u64): bool {
        if (idx := _.findByPrefixHash(prefix_hash)) {
            if (_.entries[idx].ref_count == std.math.maxInt(u32)) {
                return false;
            }
            _.entries[idx].ref_count += 1;
            return true;
        }
        return false;
    }

    /// Decrements the in-flight reference count of a cached prefix block.
    /// Returns `false` if not found or `ref_count` is already 0.
    pub fn unpinBlock(*_, prefix_hash: u64): bool {
        if (idx := _.findByPrefixHash(prefix_hash)) {
            if (_.entries[idx].ref_count == 0) {
                return false;
            }
            _.entries[idx].ref_count -= 1;
            return true;
        }
        return false;
    }

    /// Evicts the least-recently-used unpinned (`ref_count == 0`) block and returns its `kv_block_id`.
    /// Returns `null` if no unpinned occupied block exists.
    pub fn evictLruUnpinned(*_): u32? {
        let best_idx: usize? = null;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            const e = _.entries[i];
            if (!e.occupied || e.ref_count > 0) {
                continue;
            }
            if (cur := best_idx) {
                if (e.last_access_tick < _.entries[cur].last_access_tick) {
                    best_idx = i;
                }
            } else {
                best_idx = i;
            }
        }

        if (victim := best_idx) {
            const evicted_kv_id: u32 = _.entries[victim].kv_block_id;
            _.entries[victim] = PrefixBlockEntry{};
            _.evictions += 1;
            return evicted_kv_id;
        }
        return null;
    }

    /// Returns a telemetry snapshot of occupancy, pinned count, hit/miss counters, evictions, and tokens saved.
    pub fn stats(): PrefixCacheStats {
        let active: usize = 0;
        let pinned: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            const e = _.entries[i];
            if (e.occupied) {
                active += 1;
                if (e.ref_count > 0) {
                    pinned += 1;
                }
            }
        }
        return PrefixCacheStats{
            active_blocks: active,
            pinned_blocks: pinned,
            hits: _.hits,
            misses: _.misses,
            evictions: _.evictions,
            tokens_saved: _.tokens_saved,
        };
    }
}
