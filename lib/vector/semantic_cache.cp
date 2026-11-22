// Origin: Pure-CP Stdlib (AI Agent Semantic Cache: Cosine Threshold Lookup, TTL & LRU Eviction) | Created: 2026-11-22
// Zero-heap-allocation LLM Semantic Cache backed by caller-owned fixed entry and flat embedding pools.
// Supports exact cosine-similarity threshold lookup, TTL expiration, expired-first then LRU eviction,
// in-place key updates, and hit-rate / eviction telemetry.

const std = @import("std");

/// Fixed-slot metadata record for a cached semantic entry.
pub struct SemanticEntry {
    occupied: bool = false;
    key_id: u64 = 0;
    payload_id: u64 = 0;
    created_ms: i64 = 0;
    last_hit_ms: i64 = 0;
    expires_ms: i64 = 0;
    hit_count: u32 = 0;
    norm: f64 = 0.0;
}

/// Successful semantic cache lookup result.
pub struct SemanticLookupHit {
    slot: usize = 0;
    key_id: u64 = 0;
    payload_id: u64 = 0;
    similarity: f64 = 0.0;
}

/// Telemetry snapshot returned by `SemanticCache.stats`.
pub struct SemanticCacheStats {
    active_entries: usize = 0;
    capacity: usize = 0;
    hits: u64 = 0;
    misses: u64 = 0;
    evictions: u64 = 0;
    expired_evictions: u64 = 0;
    hit_rate: f64 = 0.0;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn computeNonZeroNormF64(vec: []const f64): f64? {
    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < vec.len) : (i += 1) {
        const v = vec[i];
        if (!isFiniteF64(v)) return null;
        sum_sq += v * v;
    }
    if (!isFiniteF64(sum_sq) || sum_sq == 0.0) return null;
    const norm = std.math.sqrt(sum_sq);
    if (!isFiniteF64(norm) || norm == 0.0) return null;
    return norm;
}

fn saturatingAddI64(a: i64, b: i64): i64 {
    const max_i64 = std.math.maxInt(i64);
    if (b > 0 && a > max_i64 - b) {
        return max_i64;
    }
    return a + b;
}

/// Zero-heap-allocation LLM Semantic Cache with cosine-similarity threshold lookup, TTL, and LRU eviction.
pub struct SemanticCache {
    entries: []SemanticEntry;
    embeddings_flat: []f64;
    dim: usize = 0;
    similarity_threshold: f64 = 0.9;
    default_ttl_ms: i64 = 0;
    active_count: usize = 0;
    hits: u64 = 0;
    misses: u64 = 0;
    evictions: u64 = 0;
    expired_evictions: u64 = 0;

    /// Initializes a semantic cache over caller-provided `entries` and `embeddings_flat` pools.
    /// Requires `dim > 0`, `embeddings_flat.len >= entries.len * dim`,
    /// `similarity_threshold` in `(-1.0, 1.0]`, and `default_ttl_ms >= 0` (`0` = no expiry).
    static fn init(
        entries: *[]SemanticEntry,
        embeddings_flat: *[]f64,
        dim: usize,
        similarity_threshold: f64,
        default_ttl_ms: i64,
    ): SemanticCache? {
        if (dim == 0) return null;
        if (!isFiniteF64(similarity_threshold) || similarity_threshold <= -1.0 || similarity_threshold > 1.0) {
            return null;
        }
        if (default_ttl_ms < 0) return null;
        if (entries.len > 0 && entries.len > std.math.maxInt(usize) / dim) return null;
        if (embeddings_flat.len < entries.len * dim) return null;

        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            entries[i] = SemanticEntry{};
        }

        return SemanticCache{
            entries: entries,
            embeddings_flat: embeddings_flat,
            dim: dim,
            similarity_threshold: similarity_threshold,
            default_ttl_ms: default_ttl_ms,
            active_count: 0,
            hits: 0,
            misses: 0,
            evictions: 0,
            expired_evictions: 0,
        };
    }

    /// Inserts or updates a cached semantic entry.
    ///
    /// Slot resolution priority:
    /// 1. In-place update if an occupied slot already holds `key_id`.
    /// 2. First free slot if capacity permits.
    /// 3. Evict an expired slot (`now_ms >= expires_ms`) if any exists.
    /// 4. Evict the least-recently-hit slot (smallest `last_hit_ms`).
    ///
    /// Rejects empty pools, dimension mismatches, negative `ttl_ms_override`, zero vectors, or NaN/Inf.
    fn put(
        *_,
        key_id: u64,
        payload_id: u64,
        embedding: []const f64,
        now_ms: i64,
        ttl_ms_override: i64,
    ): usize? {
        if (_.entries.len == 0) return null;
        if (embedding.len != _.dim) return null;
        if (ttl_ms_override < 0) return null;

        const norm_opt = computeNonZeroNormF64(embedding);
        if (norm_opt == null) return null;
        const norm = norm_opt ?? 1.0;

        const eff_ttl = if (ttl_ms_override > 0) ttl_ms_override else _.default_ttl_ms;
        const expires_ms = if (eff_ttl == 0) std.math.maxInt(i64) else saturatingAddI64(now_ms, eff_ttl);

        // 1. Check for existing key_id (in-place update) and track first free slot.
        let target_slot: usize = 0;
        let found_existing = false;
        let found_free = false;
        let free_slot: usize = 0;

        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].occupied) {
                if (_.entries[i].key_id == key_id) {
                    target_slot = i;
                    found_existing = true;
                    break;
                }
            } else if (!found_free) {
                free_slot = i;
                found_free = true;
            }
        }

        if (!found_existing) {
            if (found_free) {
                target_slot = free_slot;
                _.active_count += 1;
            } else {
                // 3. Full pool: prefer evicting an expired slot first.
                let found_expired = false;
                let exp_slot: usize = 0;
                let j: usize = 0;
                while (j < _.entries.len) : (j += 1) {
                    if (_.entries[j].occupied && now_ms >= _.entries[j].expires_ms) {
                        exp_slot = j;
                        found_expired = true;
                        break;
                    }
                }

                if (found_expired) {
                    target_slot = exp_slot;
                    _.expired_evictions += 1;
                    _.evictions += 1;
                } else {
                    // 4. Full pool with all live entries: evict LRU (smallest last_hit_ms).
                    let lru_slot: usize = 0;
                    let lru_ts: i64 = _.entries[0].last_hit_ms;
                    let k: usize = 1;
                    while (k < _.entries.len) : (k += 1) {
                        if (_.entries[k].last_hit_ms < lru_ts) {
                            lru_ts = _.entries[k].last_hit_ms;
                            lru_slot = k;
                        }
                    }
                    target_slot = lru_slot;
                    _.evictions += 1;
                }
            }
        }

        // Write embedding slice into flat pool.
        const base = target_slot * _.dim;
        let d: usize = 0;
        while (d < _.dim) : (d += 1) {
            _.embeddings_flat[base + d] = embedding[d];
        }

        _.entries[target_slot] = SemanticEntry{
            occupied: true,
            key_id: key_id,
            payload_id: payload_id,
            created_ms: now_ms,
            last_hit_ms: now_ms,
            expires_ms: expires_ms,
            hit_count: 0,
            norm: norm,
        };

        return target_slot;
    }

    /// Searches all live (non-expired) entries for the highest cosine similarity >= `similarity_threshold`.
    /// On hit, updates `last_hit_ms`, increments `hit_count` and global `hits`; on miss, increments `misses`.
    fn lookup(*_, query_embedding: []const f64, now_ms: i64): SemanticLookupHit? {
        if (query_embedding.len != _.dim) {
            _.misses += 1;
            return null;
        }
        const q_norm_opt = computeNonZeroNormF64(query_embedding);
        if (q_norm_opt == null) {
            _.misses += 1;
            return null;
        }
        const q_norm = q_norm_opt ?? 1.0;

        let found = false;
        let best_slot: usize = 0;
        let best_sim: f64 = -2.0;

        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            const entry = _.entries[i];
            if (!entry.occupied) continue;
            if (now_ms >= entry.expires_ms) continue;

            const base = i * _.dim;
            let dot: f64 = 0.0;
            let d: usize = 0;
            while (d < _.dim) : (d += 1) {
                dot += query_embedding[d] * _.embeddings_flat[base + d];
            }

            const denom = q_norm * entry.norm;
            if (denom == 0.0 || !isFiniteF64(denom)) continue;

            let sim = dot / denom;
            if (sim > 1.0) sim = 1.0;
            if (sim < -1.0) sim = -1.0;

            if (sim >= _.similarity_threshold) {
                if (!found || sim > best_sim) {
                    found = true;
                    best_sim = sim;
                    best_slot = i;
                }
            }
        }

        if (!found) {
            _.misses += 1;
            return null;
        }

        _.entries[best_slot].last_hit_ms = now_ms;
        _.entries[best_slot].hit_count += 1;
        _.hits += 1;

        return SemanticLookupHit{
            slot: best_slot,
            key_id: _.entries[best_slot].key_id,
            payload_id: _.entries[best_slot].payload_id,
            similarity: best_sim,
        };
    }

    /// Removes the cached entry matching `key_id`. Returns `false` if not present.
    fn invalidate(*_, key_id: u64): bool {
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].occupied && _.entries[i].key_id == key_id) {
                _.entries[i].occupied = false;
                if (_.active_count > 0) _.active_count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Reclaims all expired entries (`now_ms >= expires_ms`) and returns the number purged.
    fn purgeExpired(*_, now_ms: i64): usize {
        let purged: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            if (_.entries[i].occupied && now_ms >= _.entries[i].expires_ms) {
                _.entries[i].occupied = false;
                if (_.active_count > 0) _.active_count -= 1;
                _.expired_evictions += 1;
                _.evictions += 1;
                purged += 1;
            }
        }
        return purged;
    }

    /// Returns a snapshot of active entries, capacity, hit/miss counters, evictions, and hit rate.
    fn stats(): SemanticCacheStats {
        const total = _.hits + _.misses;
        const rate: f64 = if (total == 0) 0.0 else (_.hits as f64) / (total as f64);
        return SemanticCacheStats{
            active_entries: _.active_count,
            capacity: _.entries.len,
            hits: _.hits,
            misses: _.misses,
            evictions: _.evictions,
            expired_evictions: _.expired_evictions,
            hit_rate: rate,
        };
    }
}
