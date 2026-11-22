// Origin: Pure-CP Stdlib (Zero-Alloc Deterministic Arena SkipList KV Engine for LSM MemTable) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation Arena SkipList ordered KV engine:
//   - Fixed node pool (`SKIPLIST_MAX_NODES = 256`, `SKIPLIST_MAX_LEVEL = 12`) + inline 8 KiB byte arena
//   - Deterministic xorshift64 PRNG (seedable for reproducible tests) with 1/4 branching factor
//   - O(log N) lexicographic `put`, `get`, tombstone `delete`, `seek` / `nextNode` forward cursor,
//     and `rangeScan(start_key, end_key, include_tombstones, *out_entries)`
//   - In-place value overwrite when `new_val.len <= val_cap` (0 extra arena bytes), atomic pre-checks
//     preventing partial writes on pool/arena exhaustion, `shouldFlush(threshold_permille)`, and telemetry

/// Maximum number of key-value nodes stored inline in `ArenaSkipList`.
pub const SKIPLIST_MAX_NODES: usize = 256;

/// Maximum skiplist tower height (levels `0..11`).
pub const SKIPLIST_MAX_LEVEL: usize = 12;

/// Inline byte arena capacity for storing keys and values (8 KiB).
pub const SKIPLIST_ARENA_BYTES: usize = 8192;

/// Sentinel node index indicating end of list / missing node.
pub const SKIPLIST_NO_NODE: u16 = 0xFFFF;

/// Default non-zero PRNG seed for deterministic level generation.
pub const SKIPLIST_DEFAULT_SEED: u64 = 0x9E3779B97F4A7C15;

// ============================================================================
// 1. Enums & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Result status for `put` and `delete` operations.
pub enum SkipListWriteStatus: u8 {
    ok = 0,
    empty_key = 1,
    node_pool_full = 2,
    arena_full = 3,
}

/// Lookup classification returned by `get` / `getConst`.
pub enum SkipListGetKind: u8 {
    found = 0,
    tombstone = 1,
    not_found = 2,
}

/// Internal fixed-size skiplist node stored in the inline node pool.
pub struct SkipListNode {
    occupied: bool = false;
    tombstone: bool = false;
    level: u8 = 1;
    version: u64 = 0;
    key_off: u16 = 0;
    key_len: u16 = 0;
    val_off: u16 = 0;
    val_len: u16 = 0;
    val_cap: u16 = 0;
    next: [SKIPLIST_MAX_LEVEL]u16 = [SKIPLIST_NO_NODE; SKIPLIST_MAX_LEVEL];
}

/// Lightweight value descriptor referencing a node's key/value slices in the arena.
pub struct SkipListEntryView {
    node_idx: u16 = SKIPLIST_NO_NODE;
    version: u64 = 0;
    tombstone: bool = false;
    key_off: u16 = 0;
    key_len: u16 = 0;
    val_off: u16 = 0;
    val_len: u16 = 0;
}

/// Lookup result returned by `get` and `getConst`.
pub struct SkipListGetResult {
    kind: SkipListGetKind = SkipListGetKind.not_found;
    version: u64 = 0;
    entry: SkipListEntryView = SkipListEntryView{};
}

/// Complete zero-heap telemetry snapshot of `ArenaSkipList`.
pub struct SkipListStats {
    node_capacity: usize = SKIPLIST_MAX_NODES;
    arena_capacity: usize = SKIPLIST_ARENA_BYTES;
    total_nodes: usize = 0;
    live_keys: usize = 0;
    tombstone_keys: usize = 0;
    current_max_level: u8 = 1;
    arena_used_bytes: usize = 0;
    /// Byte arena utilization in permille (`0..=1000`).
    arena_util_permille: u32 = 0;
    /// Node pool utilization in permille (`0..=1000`).
    node_util_permille: u32 = 0;
    total_comparisons: u64 = 0;
    next_version: u64 = 1;
}

// ============================================================================
// 2. Internal Helpers (Lexicographic Compare & xorshift64)
// ============================================================================

fn compareBytesLex(a: []const u8, b: []const u8): i32 {
    const min_len: usize = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

fn nextXorShift64(state: u64): u64 {
    let x: u64 = if (state == 0) SKIPLIST_DEFAULT_SEED else state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return x;
}

fn makeEntryView(idx: u16, node: SkipListNode): SkipListEntryView {
    return SkipListEntryView{
        node_idx: idx,
        version: node.version,
        tombstone: node.tombstone,
        key_off: node.key_off,
        key_len: node.key_len,
        val_off: node.val_off,
        val_len: node.val_len,
    };
}

// ============================================================================
// 3. ArenaSkipList Core Engine
// ============================================================================

/// Zero-heap deterministic Arena SkipList ordered KV engine for LSM MemTables.
pub struct ArenaSkipList {
    nodes: [SKIPLIST_MAX_NODES]SkipListNode = [SkipListNode{}; SKIPLIST_MAX_NODES];
    arena: [SKIPLIST_ARENA_BYTES]u8 = [0; SKIPLIST_ARENA_BYTES];
    head_next: [SKIPLIST_MAX_LEVEL]u16 = [SKIPLIST_NO_NODE; SKIPLIST_MAX_LEVEL];
    node_count: usize = 0;
    live_keys: usize = 0;
    tombstone_keys: usize = 0;
    arena_used: usize = 0;
    current_max_level: u8 = 1;
    prng_state: u64 = SKIPLIST_DEFAULT_SEED;
    initial_seed: u64 = SKIPLIST_DEFAULT_SEED;
    next_version: u64 = 1;
    total_comparisons: u64 = 0;

    /// Initializes an empty `ArenaSkipList` with the default deterministic seed.
    static fn init(): ArenaSkipList {
        return ArenaSkipList{};
    }

    /// Initializes an empty `ArenaSkipList` with an explicit PRNG seed for reproducible tower heights.
    static fn initWithSeed(seed: u64): ArenaSkipList {
        const eff_seed: u64 = if (seed == 0) SKIPLIST_DEFAULT_SEED else seed;
        return ArenaSkipList{
            prng_state: eff_seed,
            initial_seed: eff_seed,
        };
    }

    /// Clears all nodes, arena bytes, and telemetry while restoring the initial PRNG seed.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.node_count && i < SKIPLIST_MAX_NODES) {
            _.nodes[i] = SkipListNode{};
            i += 1;
        }
        let lvl: usize = 0;
        while (lvl < SKIPLIST_MAX_LEVEL) {
            _.head_next[lvl] = SKIPLIST_NO_NODE;
            lvl += 1;
        }
        _.node_count = 0;
        _.live_keys = 0;
        _.tombstone_keys = 0;
        _.arena_used = 0;
        _.current_max_level = 1;
        _.prng_state = _.initial_seed;
        _.next_version = 1;
        _.total_comparisons = 0;
    }

    fn nodeKeySlice(idx: u16): []const u8 {
        const u: usize = @as(usize, idx);
        const start: usize = @as(usize, _.nodes[u].key_off);
        const end: usize = start + @as(usize, _.nodes[u].key_len);
        return _.arena[start..end];
    }

    /// Returns a zero-copy read-only slice for `entry`'s key bytes.
    fn keySlice(entry: SkipListEntryView): []const u8 {
        const start: usize = @as(usize, entry.key_off);
        const end: usize = start + @as(usize, entry.key_len);
        if (end > _.arena_used || end > SKIPLIST_ARENA_BYTES) {
            return "";
        }
        return _.arena[start..end];
    }

    /// Returns a zero-copy read-only slice for `entry`'s value bytes (`""` for tombstones).
    fn valueSlice(entry: SkipListEntryView): []const u8 {
        if (entry.tombstone || entry.val_len == 0) {
            return "";
        }
        const start: usize = @as(usize, entry.val_off);
        const end: usize = start + @as(usize, entry.val_len);
        if (end > _.arena_used || end > SKIPLIST_ARENA_BYTES) {
            return "";
        }
        return _.arena[start..end];
    }

    fn randomLevel(*_): u8 {
        let lvl: u8 = 1;
        let rnd: u64 = nextXorShift64(_.prng_state);
        _.prng_state = rnd;
        // Branching factor 1/4 (2 bits per level promotion) matches LevelDB/RocksDB InlineSkipList.
        while (lvl < @as(u8, @intCast(SKIPLIST_MAX_LEVEL)) && (rnd & 3) == 0) {
            lvl += 1;
            rnd = rnd >> 2;
        }
        return lvl;
    }

    fn allocArenaBytes(*_, data: []const u8): u16 {
        const off: usize = _.arena_used;
        let i: usize = 0;
        while (i < data.len) {
            _.arena[off + i] = data[i];
            i += 1;
        }
        _.arena_used = off + data.len;
        return @as(u16, @intCast(off));
    }

    /// Inserts or updates `(key, value)` in strict lexicographic order with monotonic version bump.
    /// Performs atomic pre-checks before mutating any node or arena byte.
    fn put(*_, key: []const u8, value: []const u8): SkipListWriteStatus {
        if (key.len == 0) {
            return SkipListWriteStatus.empty_key;
        }

        let preds: [SKIPLIST_MAX_LEVEL]u16 = [SKIPLIST_NO_NODE; SKIPLIST_MAX_LEVEL];
        let cur: u16 = SKIPLIST_NO_NODE;
        let lvl: usize = @as(usize, _.current_max_level);

        while (lvl > 0) {
            lvl -= 1;
            while (true) {
                const next_idx: u16 = if (cur == SKIPLIST_NO_NODE)
                    _.head_next[lvl]
                else
                    _.nodes[@as(usize, cur)].next[lvl];
                if (next_idx == SKIPLIST_NO_NODE) {
                    break;
                }
                _.total_comparisons += 1;
                const cmp: i32 = compareBytesLex(_.nodeKeySlice(next_idx), key);
                if (cmp < 0) {
                    cur = next_idx;
                } else {
                    break;
                }
            }
            preds[lvl] = cur;
        }

        const cand: u16 = if (preds[0] == SKIPLIST_NO_NODE)
            _.head_next[0]
        else
            _.nodes[@as(usize, preds[0])].next[0];

        if (cand != SKIPLIST_NO_NODE) {
            _.total_comparisons += 1;
            if (compareBytesLex(_.nodeKeySlice(cand), key) == 0) {
                const cu: usize = @as(usize, cand);
                // Atomic space check: reuse value slot in-place if it fits, else verify arena headroom.
                if (value.len <= @as(usize, _.nodes[cu].val_cap)) {
                    const base: usize = @as(usize, _.nodes[cu].val_off);
                    let i: usize = 0;
                    while (i < value.len) {
                        _.arena[base + i] = value[i];
                        i += 1;
                    }
                    _.nodes[cu].val_len = @as(u16, @intCast(value.len));
                } else {
                    if (_.arena_used + value.len > SKIPLIST_ARENA_BYTES) {
                        return SkipListWriteStatus.arena_full;
                    }
                    _.nodes[cu].val_off = _.allocArenaBytes(value);
                    _.nodes[cu].val_len = @as(u16, @intCast(value.len));
                    _.nodes[cu].val_cap = @as(u16, @intCast(value.len));
                }

                if (_.nodes[cu].tombstone) {
                    _.nodes[cu].tombstone = false;
                    _.live_keys += 1;
                    if (_.tombstone_keys > 0) {
                        _.tombstone_keys -= 1;
                    }
                }
                _.nodes[cu].version = _.next_version;
                _.next_version += 1;
                return SkipListWriteStatus.ok;
            }
        }

        // New key: atomic capacity pre-checks before mutating state.
        if (_.node_count >= SKIPLIST_MAX_NODES) {
            return SkipListWriteStatus.node_pool_full;
        }
        if (_.arena_used + key.len + value.len > SKIPLIST_ARENA_BYTES) {
            return SkipListWriteStatus.arena_full;
        }

        const new_idx_u: usize = _.node_count;
        const new_idx: u16 = @as(u16, @intCast(new_idx_u));
        const new_lvl: u8 = _.randomLevel();
        if (new_lvl > _.current_max_level) {
            let fill: usize = @as(usize, _.current_max_level);
            while (fill < @as(usize, new_lvl)) {
                preds[fill] = SKIPLIST_NO_NODE;
                fill += 1;
            }
            _.current_max_level = new_lvl;
        }

        const k_off: u16 = _.allocArenaBytes(key);
        const v_off: u16 = if (value.len > 0) _.allocArenaBytes(value) else 0;

        let node = SkipListNode{
            occupied: true,
            tombstone: false,
            level: new_lvl,
            version: _.next_version,
            key_off: k_off,
            key_len: @as(u16, @intCast(key.len)),
            val_off: v_off,
            val_len: @as(u16, @intCast(value.len)),
            val_cap: @as(u16, @intCast(value.len)),
        };

        let link_lvl: usize = 0;
        while (link_lvl < @as(usize, new_lvl)) {
            if (preds[link_lvl] == SKIPLIST_NO_NODE) {
                node.next[link_lvl] = _.head_next[link_lvl];
                _.head_next[link_lvl] = new_idx;
            } else {
                const pu: usize = @as(usize, preds[link_lvl]);
                node.next[link_lvl] = _.nodes[pu].next[link_lvl];
                _.nodes[pu].next[link_lvl] = new_idx;
            }
            link_lvl += 1;
        }

        _.nodes[new_idx_u] = node;
        _.node_count += 1;
        _.live_keys += 1;
        _.next_version += 1;
        return SkipListWriteStatus.ok;
    }

    /// Writes a tombstone deletion marker for `key` (updating in place if present, or inserting a tombstone node).
    fn delete(*_, key: []const u8): SkipListWriteStatus {
        if (key.len == 0) {
            return SkipListWriteStatus.empty_key;
        }

        let preds: [SKIPLIST_MAX_LEVEL]u16 = [SKIPLIST_NO_NODE; SKIPLIST_MAX_LEVEL];
        let cur: u16 = SKIPLIST_NO_NODE;
        let lvl: usize = @as(usize, _.current_max_level);

        while (lvl > 0) {
            lvl -= 1;
            while (true) {
                const next_idx: u16 = if (cur == SKIPLIST_NO_NODE)
                    _.head_next[lvl]
                else
                    _.nodes[@as(usize, cur)].next[lvl];
                if (next_idx == SKIPLIST_NO_NODE) {
                    break;
                }
                _.total_comparisons += 1;
                const cmp: i32 = compareBytesLex(_.nodeKeySlice(next_idx), key);
                if (cmp < 0) {
                    cur = next_idx;
                } else {
                    break;
                }
            }
            preds[lvl] = cur;
        }

        const cand: u16 = if (preds[0] == SKIPLIST_NO_NODE)
            _.head_next[0]
        else
            _.nodes[@as(usize, preds[0])].next[0];

        if (cand != SKIPLIST_NO_NODE) {
            _.total_comparisons += 1;
            if (compareBytesLex(_.nodeKeySlice(cand), key) == 0) {
                const cu: usize = @as(usize, cand);
                if (!_.nodes[cu].tombstone) {
                    _.nodes[cu].tombstone = true;
                    _.tombstone_keys += 1;
                    if (_.live_keys > 0) {
                        _.live_keys -= 1;
                    }
                }
                _.nodes[cu].val_len = 0;
                _.nodes[cu].version = _.next_version;
                _.next_version += 1;
                return SkipListWriteStatus.ok;
            }
        }

        // Insert standalone tombstone so lower SSTable levels are masked during LSM lookup.
        if (_.node_count >= SKIPLIST_MAX_NODES) {
            return SkipListWriteStatus.node_pool_full;
        }
        if (_.arena_used + key.len > SKIPLIST_ARENA_BYTES) {
            return SkipListWriteStatus.arena_full;
        }

        const new_idx_u: usize = _.node_count;
        const new_idx: u16 = @as(u16, @intCast(new_idx_u));
        const new_lvl: u8 = _.randomLevel();
        if (new_lvl > _.current_max_level) {
            let fill: usize = @as(usize, _.current_max_level);
            while (fill < @as(usize, new_lvl)) {
                preds[fill] = SKIPLIST_NO_NODE;
                fill += 1;
            }
            _.current_max_level = new_lvl;
        }

        const k_off: u16 = _.allocArenaBytes(key);
        let node = SkipListNode{
            occupied: true,
            tombstone: true,
            level: new_lvl,
            version: _.next_version,
            key_off: k_off,
            key_len: @as(u16, @intCast(key.len)),
        };

        let link_lvl: usize = 0;
        while (link_lvl < @as(usize, new_lvl)) {
            if (preds[link_lvl] == SKIPLIST_NO_NODE) {
                node.next[link_lvl] = _.head_next[link_lvl];
                _.head_next[link_lvl] = new_idx;
            } else {
                const pu: usize = @as(usize, preds[link_lvl]);
                node.next[link_lvl] = _.nodes[pu].next[link_lvl];
                _.nodes[pu].next[link_lvl] = new_idx;
            }
            link_lvl += 1;
        }

        _.nodes[new_idx_u] = node;
        _.node_count += 1;
        _.tombstone_keys += 1;
        _.next_version += 1;
        return SkipListWriteStatus.ok;
    }

    /// O(log N) exact lookup for `key`, updating `total_comparisons` telemetry.
    fn get(*_, key: []const u8): SkipListGetResult {
        if (key.len == 0 || _.node_count == 0) {
            return SkipListGetResult{};
        }

        let cur: u16 = SKIPLIST_NO_NODE;
        let lvl: usize = @as(usize, _.current_max_level);
        while (lvl > 0) {
            lvl -= 1;
            while (true) {
                const next_idx: u16 = if (cur == SKIPLIST_NO_NODE)
                    _.head_next[lvl]
                else
                    _.nodes[@as(usize, cur)].next[lvl];
                if (next_idx == SKIPLIST_NO_NODE) {
                    break;
                }
                _.total_comparisons += 1;
                const cmp: i32 = compareBytesLex(_.nodeKeySlice(next_idx), key);
                if (cmp < 0) {
                    cur = next_idx;
                } else {
                    break;
                }
            }
        }

        const cand: u16 = if (cur == SKIPLIST_NO_NODE)
            _.head_next[0]
        else
            _.nodes[@as(usize, cur)].next[0];

        if (cand == SKIPLIST_NO_NODE) {
            return SkipListGetResult{};
        }
        _.total_comparisons += 1;
        if (compareBytesLex(_.nodeKeySlice(cand), key) != 0) {
            return SkipListGetResult{};
        }

        const node = _.nodes[@as(usize, cand)];
        return SkipListGetResult{
            kind: if (node.tombstone) SkipListGetKind.tombstone else SkipListGetKind.found,
            version: node.version,
            entry: makeEntryView(cand, node),
        };
    }

    /// Read-only O(log N) lookup for `key` (does not modify `total_comparisons`).
    fn getConst(key: []const u8): SkipListGetResult {
        if (key.len == 0 || _.node_count == 0) {
            return SkipListGetResult{};
        }
        const cand: u16 = _.seek(key);
        if (cand == SKIPLIST_NO_NODE) {
            return SkipListGetResult{};
        }
        if (compareBytesLex(_.nodeKeySlice(cand), key) != 0) {
            return SkipListGetResult{};
        }
        const node = _.nodes[@as(usize, cand)];
        return SkipListGetResult{
            kind: if (node.tombstone) SkipListGetKind.tombstone else SkipListGetKind.found,
            version: node.version,
            entry: makeEntryView(cand, node),
        };
    }

    /// O(log N) lower-bound seek: returns the index of the first node with `key >= lower_key`,
    /// or `SKIPLIST_NO_NODE` if all keys are smaller (or list is empty).
    fn seek(lower_key: []const u8): u16 {
        if (_.node_count == 0) {
            return SKIPLIST_NO_NODE;
        }
        if (lower_key.len == 0) {
            return _.head_next[0];
        }

        let cur: u16 = SKIPLIST_NO_NODE;
        let lvl: usize = @as(usize, _.current_max_level);
        while (lvl > 0) {
            lvl -= 1;
            while (true) {
                const next_idx: u16 = if (cur == SKIPLIST_NO_NODE)
                    _.head_next[lvl]
                else
                    _.nodes[@as(usize, cur)].next[lvl];
                if (next_idx == SKIPLIST_NO_NODE) {
                    break;
                }
                if (compareBytesLex(_.nodeKeySlice(next_idx), lower_key) < 0) {
                    cur = next_idx;
                } else {
                    break;
                }
            }
        }

        if (cur == SKIPLIST_NO_NODE) {
            return _.head_next[0];
        }
        return _.nodes[@as(usize, cur)].next[0];
    }

    /// Advances a forward cursor to the next node in sorted order (Level 0), or `SKIPLIST_NO_NODE`.
    fn nextNode(node_idx: u16): u16 {
        if (node_idx == SKIPLIST_NO_NODE || @as(usize, node_idx) >= _.node_count) {
            return SKIPLIST_NO_NODE;
        }
        return _.nodes[@as(usize, node_idx)].next[0];
    }

    /// Returns the `SkipListEntryView` at `node_idx`, or `null` if invalid.
    fn entryAt(node_idx: u16): SkipListEntryView? {
        if (node_idx == SKIPLIST_NO_NODE || @as(usize, node_idx) >= _.node_count) {
            return null;
        }
        const node = _.nodes[@as(usize, node_idx)];
        if (!node.occupied) {
            return null;
        }
        return makeEntryView(node_idx, node);
    }

    /// Scans keys in `[start_key, end_key)` (`""` = unbounded on either side) in strict lexicographic order.
    fn rangeScan(
        start_key: []const u8,
        end_key: []const u8,
        include_tombstones: bool,
        out_entries: *[]SkipListEntryView,
    ): usize {
        if (out_entries.len == 0 || _.node_count == 0) {
            return 0;
        }
        if (start_key.len > 0 && end_key.len > 0 && compareBytesLex(start_key, end_key) >= 0) {
            return 0;
        }

        let cur: u16 = _.seek(start_key);
        let written: usize = 0;
        while (cur != SKIPLIST_NO_NODE && written < out_entries.len) {
            if (end_key.len > 0 && compareBytesLex(_.nodeKeySlice(cur), end_key) >= 0) {
                break;
            }
            const node = _.nodes[@as(usize, cur)];
            if (include_tombstones || !node.tombstone) {
                out_entries[written] = makeEntryView(cur, node);
                written += 1;
            }
            cur = node.next[0];
        }
        return written;
    }

    /// Returns `true` if either node pool or byte arena utilization has reached `threshold_permille` (`0..=1000`).
    fn shouldFlush(threshold_permille: u32): bool {
        if (threshold_permille == 0) {
            return true;
        }
        const st = _.stats();
        return st.arena_util_permille >= threshold_permille || st.node_util_permille >= threshold_permille;
    }

    /// Returns a zero-heap telemetry snapshot of the skiplist.
    fn stats(): SkipListStats {
        const arena_permille: u32 = @as(u32, @intCast((@as(u64, _.arena_used) * 1000) / @as(u64, SKIPLIST_ARENA_BYTES)));
        const node_permille: u32 = @as(u32, @intCast((@as(u64, _.node_count) * 1000) / @as(u64, SKIPLIST_MAX_NODES)));
        return SkipListStats{
            node_capacity: SKIPLIST_MAX_NODES,
            arena_capacity: SKIPLIST_ARENA_BYTES,
            total_nodes: _.node_count,
            live_keys: _.live_keys,
            tombstone_keys: _.tombstone_keys,
            current_max_level: _.current_max_level,
            arena_used_bytes: _.arena_used,
            arena_util_permille: arena_permille,
            node_util_permille: node_permille,
            total_comparisons: _.total_comparisons,
            next_version: _.next_version,
        };
    }
}
