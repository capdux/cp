// Origin: Pure-CP Stdlib Test Suite (AI Agent Semantic Cache: Cosine Threshold, TTL & LRU) | Created: 2026-11-22
// Deep Production Verification Suite for lib/vector/semantic_cache.cp
// Covers:
//   1. Cosine threshold lookup returning highest-similarity hit vs low-similarity miss
//   2. TTL expiration, per-entry ttl_ms_override, purgeExpired & invalidate
//   3. Full-pool eviction priority: expired-first, then LRU with lookup refreshing recency
//   4. In-place key_id update & exact stats() hit_rate / eviction telemetry
//   5. Zero vector, dimension mismatch, illegal threshold, NaN/Inf & empty pool defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SemanticEntry,
    SemanticLookupHit,
    SemanticCacheStats,
    SemanticCache,
} from "../../lib/vector/index.cp";

const EPS_F64: f64 = 0.000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testCosineThresholdLookupAndBestMatch()! {
    let entries_buf: [4]SemanticEntry = [SemanticEntry{}; 4];
    let embs_buf: [16]f64 = [0.0; 16];
    let entries: []SemanticEntry = entries_buf[0..];
    let embs: []f64 = embs_buf[0..];

    let cache = SemanticCache.init(entries, embs, 4, 0.90, 60000) ?? return;

    const v1: [4]f64 = [1.0, 0.0, 0.0, 0.0];
    const v2: [4]f64 = [0.95, 0.31, 0.0, 0.0];
    const v3: [4]f64 = [0.0, 0.0, 1.0, 0.0];

    expect_eq(cache.put(101, 1001, v1[0..], 1000, 0), 0);
    expect_eq(cache.put(102, 1002, v2[0..], 1000, 0), 1);
    expect_eq(cache.put(103, 1003, v3[0..], 1000, 0), 2);

    // Near-synonym query closer to v1 (~0.9987) than v2 (~0.964): must pick v1 (highest sim)
    const q_hit: [4]f64 = [0.99, 0.05, 0.0, 0.0];
    const hit_opt = cache.lookup(q_hit[0..], 1500);
    expect_eq(hit_opt != null, true);
    const hit = hit_opt ?? SemanticLookupHit{};
    expect_eq(hit.key_id, 101);
    expect_eq(hit.payload_id, 1001);
    expect_eq(hit.similarity > 0.99, true);

    // Low-similarity query (< 0.90): must miss
    const q_miss: [4]f64 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(cache.lookup(q_miss[0..], 1600) == null, true);

    const st = cache.stats();
    expect_eq(st.active_entries, 3);
    expect_eq(st.hits, 1);
    expect_eq(st.misses, 1);
    expect_eq(approxF64(st.hit_rate, 0.5, EPS_F64), true);
}

fn testTtlExpirationPurgeAndInvalidate()! {
    let entries_buf: [4]SemanticEntry = [SemanticEntry{}; 4];
    let embs_buf: [8]f64 = [0.0; 8];
    let entries: []SemanticEntry = entries_buf[0..];
    let embs: []f64 = embs_buf[0..];

    let cache = SemanticCache.init(entries, embs, 2, 0.95, 5000) ?? return;

    const v1: [2]f64 = [1.0, 0.0];
    const v2: [2]f64 = [0.0, 1.0];

    // key 1 uses default TTL 5000 (expires 6000); key 2 overrides TTL 2000 (expires 3000)
    expect_eq(cache.put(1, 10, v1[0..], 1000, 0), 0);
    expect_eq(cache.put(2, 20, v2[0..], 1000, 2000), 1);

    // Before expiry: both hit
    expect_eq(cache.lookup(v1[0..], 2999) != null, true);
    expect_eq(cache.lookup(v2[0..], 2999) != null, true);

    // At t = 3000: key 2 expired -> miss; key 1 still live -> hit
    expect_eq(cache.lookup(v2[0..], 3000) == null, true);
    expect_eq(cache.lookup(v1[0..], 3000) != null, true);

    // purgeExpired reclaims key 2
    expect_eq(cache.purgeExpired(3000), 1);
    const st1 = cache.stats();
    expect_eq(st1.active_entries, 1);
    expect_eq(st1.expired_evictions, 1);
    expect_eq(st1.evictions, 1);

    // invalidate removes key 1
    expect_eq(cache.invalidate(1), true);
    expect_eq(cache.invalidate(999), false);
    expect_eq(cache.stats().active_entries, 0);
}

fn testEvictionPriorityExpiredFirstThenLru()! {
    let entries_buf: [2]SemanticEntry = [SemanticEntry{}; 2];
    let embs_buf: [4]f64 = [0.0; 4];
    let entries: []SemanticEntry = entries_buf[0..];
    let embs: []f64 = embs_buf[0..];

    let cache = SemanticCache.init(entries, embs, 2, 0.95, 10000) ?? return;

    const v1: [2]f64 = [1.0, 0.0];
    const v2: [2]f64 = [0.0, 1.0];
    const v3: [2]f64 = [-1.0, 0.0];
    const v4: [2]f64 = [0.0, -1.0];

    expect_eq(cache.put(1, 100, v1[0..], 100, 0), 0); // last_hit = 100
    expect_eq(cache.put(2, 200, v2[0..], 200, 0), 1); // last_hit = 200

    // Lookup refreshes key 1's last_hit_ms to 500 -> key 2 becomes LRU (200)
    expect_eq(cache.lookup(v1[0..], 500) != null, true);

    // Full pool, all live -> must evict LRU key 2 (slot 1)
    expect_eq(cache.put(3, 300, v3[0..], 600, 0), 1);
    expect_eq(cache.lookup(v2[0..], 610) == null, true);
    expect_eq(cache.lookup(v1[0..], 610) != null, true);
    expect_eq(cache.lookup(v3[0..], 610) != null, true);

    const st_lru = cache.stats();
    expect_eq(st_lru.evictions, 1);
    expect_eq(st_lru.expired_evictions, 0);

    // Update key 1 in-place with short TTL 100ms (expires at 750, last_hit = 650 > key 3's 610)
    expect_eq(cache.put(1, 101, v1[0..], 650, 100), 0);

    // At t = 800, key 1 is EXPIRED while key 3 is still live (expires at 10600).
    // Even though key 3 has older last_hit_ms, expired-first must evict slot 0 (key 1)!
    expect_eq(cache.put(4, 400, v4[0..], 800, 0), 0);
    expect_eq(cache.lookup(v3[0..], 810) != null, true);
    expect_eq(cache.lookup(v4[0..], 810) != null, true);

    const st_exp = cache.stats();
    expect_eq(st_exp.evictions, 2);
    expect_eq(st_exp.expired_evictions, 1);
}

fn testInPlaceUpdateAndStatsPrecision()! {
    let entries_buf: [3]SemanticEntry = [SemanticEntry{}; 3];
    let embs_buf: [6]f64 = [0.0; 6];
    let entries: []SemanticEntry = entries_buf[0..];
    let embs: []f64 = embs_buf[0..];

    let cache = SemanticCache.init(entries, embs, 2, 0.90, 0) ?? return; // default_ttl = 0 (no expiry)

    const v_old: [2]f64 = [1.0, 0.0];
    const v_new: [2]f64 = [0.0, 1.0];

    expect_eq(cache.put(50, 500, v_old[0..], 100, 0), 0);
    // Same key_id updates slot 0 in-place without eviction or increasing active_entries
    expect_eq(cache.put(50, 501, v_new[0..], 200, 0), 0);

    const hit_opt = cache.lookup(v_new[0..], 999999999);
    expect_eq(hit_opt != null, true);
    expect_eq((hit_opt ?? SemanticLookupHit{}).payload_id, 501);
    expect_eq(cache.lookup(v_old[0..], 999999999) == null, true);

    const st = cache.stats();
    expect_eq(st.active_entries, 1);
    expect_eq(st.capacity, 3);
    expect_eq(st.evictions, 0);
    expect_eq(st.hits, 1);
    expect_eq(st.misses, 1);
    expect_eq(approxF64(st.hit_rate, 0.5, EPS_F64), true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_v = std.math.nan(f64);
    const inf_v = std.math.inf(f64);

    let entries_buf: [2]SemanticEntry = [SemanticEntry{}; 2];
    let embs_ok_buf: [4]f64 = [0.0; 4];
    let embs_short_buf: [3]f64 = [0.0; 3];
    let entries: []SemanticEntry = entries_buf[0..];
    let embs_ok: []f64 = embs_ok_buf[0..];
    let embs_short: []f64 = embs_short_buf[0..];

    // 1. init validation failures
    expect_eq(SemanticCache.init(entries, embs_ok, 0, 0.9, 1000) == null, true);      // dim == 0
    expect_eq(SemanticCache.init(entries, embs_ok, 2, -1.0, 1000) == null, true);     // threshold <= -1
    expect_eq(SemanticCache.init(entries, embs_ok, 2, 1.01, 1000) == null, true);     // threshold > 1
    expect_eq(SemanticCache.init(entries, embs_ok, 2, nan_v, 1000) == null, true);    // NaN threshold
    expect_eq(SemanticCache.init(entries, embs_ok, 2, 0.9, -1) == null, true);        // negative default TTL
    expect_eq(SemanticCache.init(entries, embs_short, 2, 0.9, 1000) == null, true);   // embeddings buffer too small

    // 2. Empty 0-slot cache
    let empty_entries: []SemanticEntry = entries_buf[0..0];
    let empty_cache = SemanticCache.init(empty_entries, embs_ok, 2, 0.9, 1000) ?? return;
    const good_v: [2]f64 = [1.0, 0.0];
    expect_eq(empty_cache.stats().capacity, 0);
    expect_eq(empty_cache.put(1, 1, good_v[0..], 100, 0) == null, true);
    expect_eq(empty_cache.lookup(good_v[0..], 100) == null, true);
    expect_eq(empty_cache.purgeExpired(100), 0);

    // 3. put / lookup malformed vector defenses
    let cache = SemanticCache.init(entries, embs_ok, 2, 0.9, 1000) ?? return;
    const zero_v: [2]f64 = [0.0, 0.0];
    const wrong_dim_v: [3]f64 = [1.0, 0.0, 0.0];
    const nan_vec: [2]f64 = [1.0, nan_v];
    const inf_vec: [2]f64 = [inf_v, 0.0];

    expect_eq(cache.put(1, 1, zero_v[0..], 100, 0) == null, true);
    expect_eq(cache.put(1, 1, wrong_dim_v[0..], 100, 0) == null, true);
    expect_eq(cache.put(1, 1, nan_vec[0..], 100, 0) == null, true);
    expect_eq(cache.put(1, 1, inf_vec[0..], 100, 0) == null, true);
    expect_eq(cache.put(1, 1, good_v[0..], 100, -5) == null, true);

    expect_eq(cache.lookup(zero_v[0..], 100) == null, true);
    expect_eq(cache.lookup(wrong_dim_v[0..], 100) == null, true);
    expect_eq(cache.lookup(nan_vec[0..], 100) == null, true);
    expect_eq(cache.lookup(inf_vec[0..], 100) == null, true);
    expect_eq(cache.stats().active_entries, 0);
    expect_eq(cache.stats().misses, 4);
}

fn main() {
    test("1. semantic cache: cosine threshold lookup & highest-similarity hit", () => {
        testCosineThresholdLookupAndBestMatch()?;
    });
    test("2. semantic cache: TTL expiration, ttl_ms_override, purgeExpired & invalidate", () => {
        testTtlExpirationPurgeAndInvalidate()?;
    });
    test("3. semantic cache: full-pool expired-first then LRU eviction & recency refresh", () => {
        testEvictionPriorityExpiredFirstThenLru()?;
    });
    test("4. semantic cache: in-place key_id update & stats() precision", () => {
        testInPlaceUpdateAndStatsPrecision()?;
    });
    test("5. semantic cache: zero-vector/dim-mismatch/threshold/NaN/empty-pool defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
