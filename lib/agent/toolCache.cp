// Origin: Pure-CP Stdlib (AI Agent Idempotent Tool-Call Memoization Cache) | Created: 2026-11-22
// Zero-heap-allocation idempotent tool-call result memoization cache with scope isolation,
// side-effect gating, TTL expiration, LRU eviction, and in-place payload compaction.

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = (u64(0xcbf29ce4) << 32) | u64(0x84222325);
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h = h ^ u64(data[i]);
        h = h *% 0x100000001b3;
    }
    return h;
}

/// Composite lookup key isolating cached tool results by tool name, canonical arguments, and scope.
pub struct ToolCacheKey {
    tool_name_hash: u64 = 0;
    args_hash: u64 = 0;
    scope_id: u32 = 0;

    /// Returns `true` when two keys match across tool name, arguments, and scope.
    fn equals(other: ToolCacheKey): bool {
        return _.tool_name_hash == other.tool_name_hash &&
            _.args_hash == other.args_hash &&
            _.scope_id == other.scope_id;
    }
}

/// Computes a deterministic FNV-1a 64-bit `ToolCacheKey` for `(tool_name, args_canonical, scope_id)`.
pub fn computeToolCacheKey(
    tool_name: string,
    args_canonical: string,
    scope_id: u32,
): ToolCacheKey {
    return ToolCacheKey{
        tool_name_hash: fnv1a64(tool_name),
        args_hash: fnv1a64(args_canonical),
        scope_id: scope_id,
    };
}

/// Side-effect classification governing cache admission and scope invalidation.
pub enum ToolSideEffectMode {
    ReadOnlyCacheable,
    MutatingBypass,
    MutatingInvalidateScope,
}

/// Fixed-slot metadata record for a cached tool-call result.
pub struct ToolCacheEntry {
    key: ToolCacheKey = ToolCacheKey{};
    result_hash: u64 = 0;
    result_offset: usize = 0;
    result_len: usize = 0;
    created_ms: i64 = 0;
    expires_ms: i64 = 0;
    last_hit_ms: i64 = 0;
    last_access_seq: u64 = 0;
    hit_count: u64 = 0;
    saved_latency_ms: u64 = 0;
    occupied: bool = false;
}

/// Cumulative operational telemetry for `ToolCallCache`.
pub struct ToolCacheStats {
    entries: usize = 0;
    hits: u64 = 0;
    misses: u64 = 0;
    evictions: u64 = 0;
    invalidations: u64 = 0;
    bypassed_mutating: u64 = 0;
    total_saved_latency_ms: u64 = 0;
}

fn isEntryExpired(entry: ToolCacheEntry, safe_now: i64): bool {
    return entry.expires_ms > 0 && safe_now >= entry.expires_ms;
}

/// Zero-heap-allocation idempotent tool-call memoization cache backed by caller-owned slot and byte buffers.
pub struct ToolCallCache {
    slots: []ToolCacheEntry;
    payload_buf: []u8;
    payload_used: usize = 0;
    access_seq: u64 = 0;
    metrics: ToolCacheStats = ToolCacheStats{};

    /// Initializes a cache using caller-owned `slots` and `payload_buf`.
    static fn init(slots: *[]ToolCacheEntry, payload_buf: *[]u8): ToolCallCache {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = ToolCacheEntry{};
        }
        return ToolCallCache{
            slots: slots,
            payload_buf: payload_buf,
            payload_used: 0,
            access_seq: 0,
            metrics: ToolCacheStats{},
        };
    }

    /// Returns a snapshot of current cache metrics.
    fn stats(): ToolCacheStats {
        return _.metrics;
    }

    /// Clears all cached entries and resets the payload cursor while retaining cumulative counters.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            _.slots[i] = ToolCacheEntry{};
        }
        _.payload_used = 0;
        _.metrics.entries = 0;
    }

    fn nextSeq(*_): u64 {
        _.access_seq += 1;
        return _.access_seq;
    }

    fn findSlot(key: ToolCacheKey): usize? {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].key.equals(key)) {
                return i;
            }
        }
        return null;
    }

    fn releaseSlot(*_, idx: usize) {
        if (idx < _.slots.len && _.slots[idx].occupied) {
            _.slots[idx] = ToolCacheEntry{};
            if (_.metrics.entries > 0) {
                _.metrics.entries -= 1;
            }
        }
    }

    /// Compacts all live payloads in-place toward offset 0 in ascending `(result_offset, slot_idx)` order.
    fn compactPayload(*_) {
        let new_used: usize = 0;
        let has_prev = false;
        let prev_off: usize = 0;
        let prev_idx: usize = 0;
        let moved_count: usize = 0;

        while (moved_count < _.metrics.entries) {
            let best_idx: usize? = null;
            let best_off: usize = 0;
            let i: usize = 0;
            while (i < _.slots.len) : (i += 1) {
                if (!_.slots[i].occupied) continue;
                const off = _.slots[i].result_offset;
                if (has_prev) {
                    if (off < prev_off || (off == prev_off && i <= prev_idx)) continue;
                }
                if (best_idx == null || off < best_off) {
                    best_idx = i;
                    best_off = off;
                }
            }
            if (idx := best_idx) {
                const len = _.slots[idx].result_len;
                const src_off = _.slots[idx].result_offset;
                if (src_off != new_used && len > 0) {
                    let k: usize = 0;
                    while (k < len) : (k += 1) {
                        _.payload_buf[new_used + k] = _.payload_buf[src_off + k];
                    }
                }
                _.slots[idx].result_offset = new_used;
                new_used += len;
                has_prev = true;
                prev_off = src_off;
                prev_idx = idx;
                moved_count += 1;
            } else {
                break;
            }
        }
        _.payload_used = new_used;
    }

    /// Finds the least-recently-used occupied slot index (optionally skipping `exclude_idx`).
    fn findLruSlot(exclude_idx: usize?): usize? {
        let victim: usize? = null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            if (ex := exclude_idx) {
                if (i == ex) continue;
            }
            if (v := victim) {
                const cur = _.slots[i];
                const best = _.slots[v];
                if (cur.last_hit_ms < best.last_hit_ms ||
                    (cur.last_hit_ms == best.last_hit_ms && cur.last_access_seq < best.last_access_seq))
                {
                    victim = i;
                }
            } else {
                victim = i;
            }
        }
        return victim;
    }

    /// Looks up a cached tool result by `key` at timestamp `now_ms`.
    /// Returns a zero-copy slice of the cached result on hit (updating LRU & saved latency stats),
    /// or `null` on miss or TTL expiration.
    fn lookup(*_, key: ToolCacheKey, now_ms: i64): string? {
        const safe_now: i64 = if (now_ms < 0) 0 else now_ms;
        if (idx := _.findSlot(key)) {
            if (isEntryExpired(_.slots[idx], safe_now)) {
                _.releaseSlot(idx);
                _.metrics.misses += 1;
                return null;
            }
            _.slots[idx].last_hit_ms = safe_now;
            _.slots[idx].last_access_seq = _.nextSeq();
            _.slots[idx].hit_count += 1;
            _.metrics.hits += 1;
            _.metrics.total_saved_latency_ms += _.slots[idx].saved_latency_ms;
            const off = _.slots[idx].result_offset;
            const len = _.slots[idx].result_len;
            return _.payload_buf[off..off + len];
        }
        _.metrics.misses += 1;
        return null;
    }

    /// Admits a tool result into the cache subject to `mode`:
    /// - `MutatingBypass`: refuses caching and increments `bypassed_mutating`.
    /// - `MutatingInvalidateScope`: refuses caching, increments `bypassed_mutating`, and invalidates all entries in `key.scope_id`.
    /// - `ReadOnlyCacheable`: stores/updates result with `ttl_ms` (`<= 0` means no expiration), evicting LRU entries if full.
    /// Returns `true` if cached, `false` if bypassed or exceeding buffer capacity.
    fn put(
        *_,
        key: ToolCacheKey,
        mode: ToolSideEffectMode,
        result: []const u8,
        ttl_ms: i64,
        latency_ms: u64,
        now_ms: i64,
    ): bool {
        if (mode == ToolSideEffectMode.MutatingBypass) {
            _.metrics.bypassed_mutating += 1;
            return false;
        }
        if (mode == ToolSideEffectMode.MutatingInvalidateScope) {
            _.metrics.bypassed_mutating += 1;
            * = _.invalidateScope(key.scope_id);
            return false;
        }
        if (_.slots.len == 0 || result.len > _.payload_buf.len) {
            return false;
        }

        const safe_now: i64 = if (now_ms < 0) 0 else now_ms;

        // 1. Choose target slot: existing key > empty slot > expired slot > LRU victim
        let target_idx: usize? = _.findSlot(key);
        if (existing := target_idx) {
            _.releaseSlot(existing);
        } else {
            let i: usize = 0;
            while (i < _.slots.len) : (i += 1) {
                if (!_.slots[i].occupied) {
                    target_idx = i;
                    break;
                }
            }
            if (target_idx == null) {
                let j: usize = 0;
                while (j < _.slots.len) : (j += 1) {
                    if (_.slots[j].occupied && isEntryExpired(_.slots[j], safe_now)) {
                        _.releaseSlot(j);
                        target_idx = j;
                        break;
                    }
                }
            }
            if (target_idx == null) {
                if (victim := _.findLruSlot(null)) {
                    _.releaseSlot(victim);
                    _.metrics.evictions += 1;
                    target_idx = victim;
                } else {
                    return false;
                }
            }
        }

        // 2. Ensure sufficient payload buffer space via in-place compaction & LRU eviction
        if (_.payload_used + result.len > _.payload_buf.len) {
            _.compactPayload();
        }
        while (_.payload_used + result.len > _.payload_buf.len) {
            if (victim := _.findLruSlot(null)) {
                _.releaseSlot(victim);
                _.metrics.evictions += 1;
                _.compactPayload();
            } else {
                return false;
            }
        }

        const idx = target_idx ?? 0;
        const write_off = _.payload_used;
        let k: usize = 0;
        while (k < result.len) : (k += 1) {
            _.payload_buf[write_off + k] = result[k];
        }
        _.payload_used += result.len;

        const expires_ms: i64 = if (ttl_ms > 0) safe_now + ttl_ms else 0;
        _.slots[idx] = ToolCacheEntry{
            key: key,
            result_hash: fnv1a64(result),
            result_offset: write_off,
            result_len: result.len,
            created_ms: safe_now,
            expires_ms: expires_ms,
            last_hit_ms: safe_now,
            last_access_seq: _.nextSeq(),
            hit_count: 0,
            saved_latency_ms: latency_ms,
            occupied: true,
        };
        _.metrics.entries += 1;
        return true;
    }

    /// Invalidates all cached entries belonging to `scope_id` and returns the count removed.
    fn invalidateScope(*_, scope_id: u32): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].key.scope_id == scope_id) {
                _.releaseSlot(i);
                removed += 1;
            }
        }
        _.metrics.invalidations += u64(removed);
        return removed;
    }

    /// Invalidates all cached entries for `tool_name` across all scopes and returns the count removed.
    fn invalidateTool(*_, tool_name: string): usize {
        const target_hash = fnv1a64(tool_name);
        let removed: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].key.tool_name_hash == target_hash) {
                _.releaseSlot(i);
                removed += 1;
            }
        }
        _.metrics.invalidations += u64(removed);
        return removed;
    }

    /// Purges all TTL-expired entries as of `now_ms` and returns the count removed.
    fn purgeExpired(*_, now_ms: i64): usize {
        const safe_now: i64 = if (now_ms < 0) 0 else now_ms;
        let purged: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && isEntryExpired(_.slots[i], safe_now)) {
                _.releaseSlot(i);
                purged += 1;
            }
        }
        return purged;
    }
}
