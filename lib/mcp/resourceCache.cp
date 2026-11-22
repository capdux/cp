// Origin: Pure-CP Stdlib (MCP Resource URI Version/ETag Cache, SWR & Byte-Budget LRU Eviction) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation MCP resource cache engine:
//   - Fixed-capacity 32-entry URI/ETag/body cache with FNV-1a 64-bit content hashing & monotonic versions
//   - Stale-While-Revalidate (SWR) grace window & ETag zero-copy TTL renewal (`revalidateNotModified`)
//   - Exact (`invalidateUri`) and directory-prefix (`invalidatePrefix`) invalidation for
//     MCP `notifications/resources/updated`
//   - Byte-budget (`max_bytes`) and slot-cap LRU eviction with strict `pinned` entry protection

// ============================================================================
// 1. Constants & Enums
// ============================================================================

/// Fixed maximum number of cached MCP resource entries.
pub const MCP_RES_CACHE_MAX_ENTRIES: usize = 32;

/// Maximum inline byte length for a resource URI.
pub const MCP_RES_URI_MAX_LEN: usize = 96;

/// Maximum inline byte length for a resource ETag / revision token.
pub const MCP_RES_ETAG_MAX_LEN: usize = 48;

/// Maximum inline byte length for a cached resource payload body.
pub const MCP_RES_BODY_MAX_LEN: usize = 512;

/// Outcome classification returned by `McpResourceCache.lookup`.
pub enum ResCacheLookupKind {
    hit_fresh,
    hit_stale_revalidate,
    miss_not_found,
    miss_expired,
    miss_invalidated,
}

/// Insertion/update status returned by `McpResourceCache.put`.
pub enum ResCachePutStatus {
    inserted,
    updated,
    evicted_and_inserted,
    rejected_too_large,
}

// ============================================================================
// 2. Data Models
// ============================================================================

/// Single cached MCP resource entry with inline URI, ETag, body, version, and TTL metadata.
pub struct ResCacheEntry {
    active: bool = false;
    uri_buf: [MCP_RES_URI_MAX_LEN]u8 = [0; MCP_RES_URI_MAX_LEN];
    uri_len: usize = 0;
    etag_buf: [MCP_RES_ETAG_MAX_LEN]u8 = [0; MCP_RES_ETAG_MAX_LEN];
    etag_len: usize = 0;
    body_buf: [MCP_RES_BODY_MAX_LEN]u8 = [0; MCP_RES_BODY_MAX_LEN];
    body_len: usize = 0;
    content_hash: u64 = 0;
    fetched_at_ms: u64 = 0;
    expires_at_ms: u64 = 0;
    last_access_ms: u64 = 0;
    version: u64 = 0;
    pinned: bool = false;
    invalidated: bool = false;

    /// Returns the cached resource URI slice.
    fn uri(): []const u8 {
        return uri_buf[0..uri_len];
    }

    /// Returns the cached ETag slice.
    fn etag(): []const u8 {
        return etag_buf[0..etag_len];
    }

    /// Returns the cached resource payload body slice.
    fn body(): []const u8 {
        return body_buf[0..body_len];
    }
}

/// Lookup descriptor returned by `McpResourceCache.lookup`.
pub struct ResCacheLookupResult {
    kind: ResCacheLookupKind = ResCacheLookupKind.miss_not_found;
    entry_index: usize = MCP_RES_CACHE_MAX_ENTRIES;
    etag_len: usize = 0;
    body_len: usize = 0;
    version: u64 = 0;
}

/// Aggregate cache usage and hit/miss/eviction telemetry.
pub struct ResCacheStats {
    entry_count: usize = 0;
    used_bytes: usize = 0;
    max_bytes: usize = 0;
    hits: u64 = 0;
    stale_hits: u64 = 0;
    misses: u64 = 0;
    evictions: u64 = 0;
    invalidations: u64 = 0;
}

// ============================================================================
// 3. Internal Helpers
// ============================================================================

fn fnv1aHash64(data: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h = (h ^ @as(u64, data[i])) *% 1099511628211;
    }
    return h;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn startsWithBytes(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
}

fn copyExact(src: []const u8, dst: *[]u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

// ============================================================================
// 4. Core Engine: `McpResourceCache`
// ============================================================================

/// Zero-heap-allocation MCP resource cache with ETag revalidation, SWR grace window,
/// subscription-driven invalidation, and byte-budget LRU eviction.
pub struct McpResourceCache {
    entries: [MCP_RES_CACHE_MAX_ENTRIES]ResCacheEntry = [ResCacheEntry{}; MCP_RES_CACHE_MAX_ENTRIES];
    max_bytes: usize = 4096;
    default_ttl_ms: u64 = 60000;
    stale_while_revalidate_ms: u64 = 30000;
    entry_count: usize = 0;
    used_bytes: usize = 0;
    hits: u64 = 0;
    stale_hits: u64 = 0;
    misses: u64 = 0;
    evictions: u64 = 0;
    invalidations: u64 = 0;

    /// Initializes a cache with payload byte budget `max_bytes`, default TTL, and SWR grace window.
    static fn init(
        max_bytes: usize,
        default_ttl_ms: u64,
        stale_while_revalidate_ms: u64,
    ): McpResourceCache {
        return McpResourceCache{
            max_bytes: max_bytes,
            default_ttl_ms: default_ttl_ms,
            stale_while_revalidate_ms: stale_while_revalidate_ms,
        };
    }

    fn findByUri(uri: []const u8): usize? {
        let i: usize = 0;
        while (i < MCP_RES_CACHE_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active && bytesEqual(entries[i].uri(), uri)) {
                return i;
            }
        }
        return null;
    }

    fn findLruEvictionVictim(exclude_idx: usize): usize? {
        let victim: usize = MCP_RES_CACHE_MAX_ENTRIES;
        let oldest_access: u64 = 0;
        let i: usize = 0;
        while (i < MCP_RES_CACHE_MAX_ENTRIES) : (i += 1) {
            if (!entries[i].active || i == exclude_idx || entries[i].pinned) continue;
            if (victim == MCP_RES_CACHE_MAX_ENTRIES || entries[i].last_access_ms < oldest_access) {
                victim = i;
                oldest_access = entries[i].last_access_ms;
            }
        }
        if (victim == MCP_RES_CACHE_MAX_ENTRIES) return null;
        return victim;
    }

    fn evictSlot(*_, idx: usize) {
        if (!entries[idx].active) return;
        used_bytes -= entries[idx].body_len;
        entry_count -= 1;
        entries[idx] = ResCacheEntry{};
        evictions += 1;
    }

    /// Inserts or updates a cached resource entry.
    ///
    /// - Rejects empty/oversized `uri`, oversized `etag`/`body`, or `body.len > max_bytes`
    ///   with `rejected_too_large`.
    /// - Evicts least-recently-accessed non-`pinned` entries when needed to satisfy `max_bytes`
    ///   and the 32-slot capacity; refuses atomically if `pinned` entries block sufficient space.
    /// - Updating an existing URI increments `version` and clears `invalidated`.
    fn put(
        *_,
        uri: []const u8,
        etag: []const u8,
        body: []const u8,
        now_ms: u64,
        ttl_override_ms: u64,
        pinned: bool,
    ): ResCachePutStatus {
        if (uri.len == 0 || uri.len > MCP_RES_URI_MAX_LEN) return ResCachePutStatus.rejected_too_large;
        if (etag.len > MCP_RES_ETAG_MAX_LEN) return ResCachePutStatus.rejected_too_large;
        if (body.len > MCP_RES_BODY_MAX_LEN || body.len > max_bytes) return ResCachePutStatus.rejected_too_large;

        const eff_ttl: u64 = if (ttl_override_ms > 0) ttl_override_ms else default_ttl_ms;
        const exp_ms: u64 = now_ms + eff_ttl;
        const hash: u64 = fnv1aHash64(body);

        // 1. Update existing URI in-place
        if (existing_idx := findByUri(uri)) {
            const old_len: usize = entries[existing_idx].body_len;
            // Verify pinned bytes (excluding self) allow the new body size
            let pinned_other_bytes: usize = 0;
            let k: usize = 0;
            while (k < MCP_RES_CACHE_MAX_ENTRIES) : (k += 1) {
                if (entries[k].active && k != existing_idx && entries[k].pinned) {
                    pinned_other_bytes += entries[k].body_len;
                }
            }
            if (pinned_other_bytes + body.len > max_bytes) {
                return ResCachePutStatus.rejected_too_large;
            }

            while ((used_bytes - old_len) + body.len > max_bytes) {
                const vic: usize = findLruEvictionVictim(existing_idx) ?? return ResCachePutStatus.rejected_too_large;
                evictSlot(vic);
            }

            used_bytes = (used_bytes - old_len) + body.len;
            let etag_dst: []u8 = entries[existing_idx].etag_buf[0..];
            copyExact(etag, *etag_dst);
            entries[existing_idx].etag_len = etag.len;
            let body_dst: []u8 = entries[existing_idx].body_buf[0..];
            copyExact(body, *body_dst);
            entries[existing_idx].body_len = body.len;
            entries[existing_idx].content_hash = hash;
            entries[existing_idx].fetched_at_ms = now_ms;
            entries[existing_idx].expires_at_ms = exp_ms;
            entries[existing_idx].last_access_ms = now_ms;
            entries[existing_idx].version += 1;
            entries[existing_idx].pinned = pinned;
            entries[existing_idx].invalidated = false;
            return ResCachePutStatus.updated;
        }

        // 2. Atomic feasibility check before evicting for new insert
        let pinned_count: usize = 0;
        let pinned_bytes: usize = 0;
        let ci: usize = 0;
        while (ci < MCP_RES_CACHE_MAX_ENTRIES) : (ci += 1) {
            if (entries[ci].active && entries[ci].pinned) {
                pinned_count += 1;
                pinned_bytes += entries[ci].body_len;
            }
        }
        if (pinned_count >= MCP_RES_CACHE_MAX_ENTRIES || pinned_bytes + body.len > max_bytes) {
            return ResCachePutStatus.rejected_too_large;
        }

        let did_evict: bool = false;
        while (entry_count >= MCP_RES_CACHE_MAX_ENTRIES || used_bytes + body.len > max_bytes) {
            const vic: usize = findLruEvictionVictim(MCP_RES_CACHE_MAX_ENTRIES) ?? return ResCachePutStatus.rejected_too_large;
            evictSlot(vic);
            did_evict = true;
        }

        let free_idx: usize = MCP_RES_CACHE_MAX_ENTRIES;
        let fi: usize = 0;
        while (fi < MCP_RES_CACHE_MAX_ENTRIES) : (fi += 1) {
            if (!entries[fi].active) {
                free_idx = fi;
                break;
            }
        }
        if (free_idx == MCP_RES_CACHE_MAX_ENTRIES) return ResCachePutStatus.rejected_too_large;

        let e = ResCacheEntry{
            active: true,
            uri_len: uri.len,
            etag_len: etag.len,
            body_len: body.len,
            content_hash: hash,
            fetched_at_ms: now_ms,
            expires_at_ms: exp_ms,
            last_access_ms: now_ms,
            version: 1,
            pinned: pinned,
            invalidated: false,
        };
        let uri_dst: []u8 = e.uri_buf[0..];
        copyExact(uri, *uri_dst);
        let etag_dst: []u8 = e.etag_buf[0..];
        copyExact(etag, *etag_dst);
        let body_dst: []u8 = e.body_buf[0..];
        copyExact(body, *body_dst);

        entries[free_idx] = e;
        entry_count += 1;
        used_bytes += body.len;
        return if (did_evict) ResCachePutStatus.evicted_and_inserted else ResCachePutStatus.inserted;
    }

    /// Looks up `uri` at `now_ms`, distinguishing `hit_fresh`, `hit_stale_revalidate`,
    /// `miss_invalidated`, `miss_expired`, and `miss_not_found`.
    fn lookup(*_, uri: []const u8, now_ms: u64): ResCacheLookupResult {
        let idx: usize = 0;
        if (found := findByUri(uri)) {
            idx = found;
        } else {
            misses += 1;
            return ResCacheLookupResult{ kind: ResCacheLookupKind.miss_not_found };
        }

        if (entries[idx].invalidated) {
            misses += 1;
            return ResCacheLookupResult{
                kind: ResCacheLookupKind.miss_invalidated,
                entry_index: idx,
                etag_len: entries[idx].etag_len,
                body_len: entries[idx].body_len,
                version: entries[idx].version,
            };
        }

        if (now_ms <= entries[idx].expires_at_ms) {
            entries[idx].last_access_ms = now_ms;
            hits += 1;
            return ResCacheLookupResult{
                kind: ResCacheLookupKind.hit_fresh,
                entry_index: idx,
                etag_len: entries[idx].etag_len,
                body_len: entries[idx].body_len,
                version: entries[idx].version,
            };
        }

        if (stale_while_revalidate_ms > 0 && now_ms <= entries[idx].expires_at_ms + stale_while_revalidate_ms) {
            entries[idx].last_access_ms = now_ms;
            stale_hits += 1;
            return ResCacheLookupResult{
                kind: ResCacheLookupKind.hit_stale_revalidate,
                entry_index: idx,
                etag_len: entries[idx].etag_len,
                body_len: entries[idx].body_len,
                version: entries[idx].version,
            };
        }

        misses += 1;
        return ResCacheLookupResult{
            kind: ResCacheLookupKind.miss_expired,
            entry_index: idx,
            etag_len: entries[idx].etag_len,
            body_len: entries[idx].body_len,
            version: entries[idx].version,
        };
    }

    /// Marks an exact `uri` entry as invalidated (e.g. upon `notifications/resources/updated`).
    fn invalidateUri(*_, uri: []const u8): bool {
        if (idx := findByUri(uri)) {
            if (!entries[idx].invalidated) {
                entries[idx].invalidated = true;
                invalidations += 1;
            }
            return true;
        }
        return false;
    }

    /// Marks all active entries whose URI starts with `uri_prefix` as invalidated.
    /// Returns the number of entries matched.
    fn invalidatePrefix(*_, uri_prefix: []const u8): usize {
        if (uri_prefix.len == 0) return 0;
        let matched: usize = 0;
        let i: usize = 0;
        while (i < MCP_RES_CACHE_MAX_ENTRIES) : (i += 1) {
            if (entries[i].active && startsWithBytes(entries[i].uri(), uri_prefix)) {
                if (!entries[i].invalidated) {
                    entries[i].invalidated = true;
                    invalidations += 1;
                }
                matched += 1;
            }
        }
        return matched;
    }

    /// Renews TTL and restores fresh state without recopying `body` when `etag` matches
    /// the stored entry's ETag (304 Not Modified semantics).
    fn revalidateNotModified(*_, uri: []const u8, etag: []const u8, now_ms: u64): bool {
        if (etag.len == 0) return false;
        const idx: usize = findByUri(uri) ?? return false;
        if (!bytesEqual(entries[idx].etag(), etag)) return false;

        entries[idx].fetched_at_ms = now_ms;
        entries[idx].expires_at_ms = now_ms + default_ttl_ms;
        entries[idx].last_access_ms = now_ms;
        entries[idx].invalidated = false;
        return true;
    }

    /// Returns a copy of the cache entry at slot `idx`, or `null` if out of bounds or empty.
    fn entryAt(idx: usize): ResCacheEntry? {
        if (idx >= MCP_RES_CACHE_MAX_ENTRIES || !entries[idx].active) return null;
        return entries[idx];
    }

    /// Returns aggregate cache usage and hit/miss/eviction statistics.
    fn stats(): ResCacheStats {
        return ResCacheStats{
            entry_count: entry_count,
            used_bytes: used_bytes,
            max_bytes: max_bytes,
            hits: hits,
            stale_hits: stale_hits,
            misses: misses,
            evictions: evictions,
            invalidations: invalidations,
        };
    }

    /// Removes all cached entries and resets occupancy (preserves configuration).
    fn clear(*_) {
        let i: usize = 0;
        while (i < MCP_RES_CACHE_MAX_ENTRIES) : (i += 1) {
            entries[i] = ResCacheEntry{};
        }
        entry_count = 0;
        used_bytes = 0;
    }
}
