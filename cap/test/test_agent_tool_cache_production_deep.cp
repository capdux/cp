// Origin: Pure-CP Stdlib Test Suite (AI Agent Idempotent Tool-Call Memoization Cache) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/tool_cache.cp
// Covers:
//   1. Hit/miss lookup, argument distinction, scope isolation & key overwrite
//   2. TTL expiration on lookup, active purgeExpired & zero-TTL non-expiring entries
//   3. Side-effect gating (MutatingBypass, MutatingInvalidateScope) & invalidateTool
//   4. LRU eviction under slot pressure & in-place payload compaction under byte pressure
//   5. Cumulative telemetry (saved latency, hits/misses/evictions/invalidations) & zero-capacity guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ToolCacheKey,
    computeToolCacheKey,
    ToolSideEffectMode,
    ToolCacheEntry,
    ToolCacheStats,
    ToolCallCache,
} from "../../lib/agent/index.cp";

fn testHitMissAndScopeIsolation()! {
    let slots_buf: [8]ToolCacheEntry = [ToolCacheEntry{}; 8];
    let payload_buf: [256]u8 = [0; 256];
    let slots: []ToolCacheEntry = slots_buf[0..];
    let payload: []u8 = payload_buf[0..];
    let cache = ToolCallCache.init(slots, payload);

    const k_s1 = computeToolCacheKey("read_file", "{\"path\":\"a.txt\"}", 1);
    const k_s2 = computeToolCacheKey("read_file", "{\"path\":\"a.txt\"}", 2);
    const k_other_arg = computeToolCacheKey("read_file", "{\"path\":\"b.txt\"}", 1);
    const k_empty = computeToolCacheKey("stat_file", "{}", 1);

    // Initial misses
    expect_eq(cache.lookup(k_s1, 1000) == null, true);

    // Put isolated entries
    expect_eq(cache.put(k_s1, ToolSideEffectMode.ReadOnlyCacheable, "alpha-content", 5000, 120, 1000), true);
    expect_eq(cache.put(k_s2, ToolSideEffectMode.ReadOnlyCacheable, "beta-scope2", 5000, 80, 1000), true);
    expect_eq(cache.put(k_other_arg, ToolSideEffectMode.ReadOnlyCacheable, "file-b", 5000, 50, 1000), true);
    expect_eq(cache.put(k_empty, ToolSideEffectMode.ReadOnlyCacheable, "", 5000, 10, 1000), true);

    expect_eq(std.mem.eql(u8, cache.lookup(k_s1, 1100) ?? "", "alpha-content"), true);
    expect_eq(std.mem.eql(u8, cache.lookup(k_s2, 1100) ?? "", "beta-scope2"), true);
    expect_eq(std.mem.eql(u8, cache.lookup(k_other_arg, 1100) ?? "", "file-b"), true);
    expect_eq(cache.lookup(k_empty, 1100) != null, true);
    const empty_val: string = cache.lookup(k_empty, 1100) ?? "x";
    expect_eq(empty_val.len, 0);

    // Overwrite same key updates payload without duplicating slot
    expect_eq(cache.put(k_s1, ToolSideEffectMode.ReadOnlyCacheable, "alpha-v2", 5000, 150, 1200), true);
    expect_eq(cache.stats().entries, 4);
    expect_eq(std.mem.eql(u8, cache.lookup(k_s1, 1250) ?? "", "alpha-v2"), true);
}

fn testTtlExpirationAndPurge()! {
    let slots_buf: [4]ToolCacheEntry = [ToolCacheEntry{}; 4];
    let payload_buf: [128]u8 = [0; 128];
    let slots: []ToolCacheEntry = slots_buf[0..];
    let payload: []u8 = payload_buf[0..];
    let cache = ToolCallCache.init(slots, payload);

    const k_short = computeToolCacheKey("web_get", "u1", 1);
    const k_med = computeToolCacheKey("web_get", "u2", 1);
    const k_perm = computeToolCacheKey("web_get", "u3", 1);

    expect_eq(cache.put(k_short, ToolSideEffectMode.ReadOnlyCacheable, "short", 500, 100, 1000), true);
    expect_eq(cache.put(k_med, ToolSideEffectMode.ReadOnlyCacheable, "medium", 2000, 200, 1000), true);
    expect_eq(cache.put(k_perm, ToolSideEffectMode.ReadOnlyCacheable, "forever", 0, 50, 1000), true);

    // Before expiry (t=1499): all 3 hit
    expect_eq(std.mem.eql(u8, cache.lookup(k_short, 1499) ?? "", "short"), true);
    expect_eq(std.mem.eql(u8, cache.lookup(k_med, 1499) ?? "", "medium"), true);
    expect_eq(std.mem.eql(u8, cache.lookup(k_perm, 1499) ?? "", "forever"), true);

    // At t=1500: k_short expires on lookup and releases its slot
    expect_eq(cache.lookup(k_short, 1500) == null, true);
    expect_eq(cache.stats().entries, 2);

    // At t=3000: purgeExpired removes k_med, leaving k_perm intact
    expect_eq(cache.purgeExpired(3000), 1);
    expect_eq(cache.stats().entries, 1);
    expect_eq(cache.lookup(k_med, 3000) == null, true);
    expect_eq(std.mem.eql(u8, cache.lookup(k_perm, 999999) ?? "", "forever"), true);
}

fn testSideEffectGatingAndInvalidation()! {
    let slots_buf: [6]ToolCacheEntry = [ToolCacheEntry{}; 6];
    let payload_buf: [256]u8 = [0; 256];
    let slots: []ToolCacheEntry = slots_buf[0..];
    let payload: []u8 = payload_buf[0..];
    let cache = ToolCallCache.init(slots, payload);

    const k_read_s1 = computeToolCacheKey("read_file", "a", 10);
    const k_list_s1 = computeToolCacheKey("list_dir", ".", 10);
    const k_read_s2 = computeToolCacheKey("read_file", "a", 20);
    const k_log_s1 = computeToolCacheKey("append_log", "msg", 10);
    const k_write_s1 = computeToolCacheKey("write_file", "a", 10);

    expect_eq(cache.put(k_read_s1, ToolSideEffectMode.ReadOnlyCacheable, "r1", 0, 50, 100), true);
    expect_eq(cache.put(k_list_s1, ToolSideEffectMode.ReadOnlyCacheable, "l1", 0, 50, 100), true);
    expect_eq(cache.put(k_read_s2, ToolSideEffectMode.ReadOnlyCacheable, "r2", 0, 50, 100), true);
    expect_eq(cache.stats().entries, 3);

    // MutatingBypass: rejected from cache, does not invalidate existing scope entries
    expect_eq(cache.put(k_log_s1, ToolSideEffectMode.MutatingBypass, "ok", 0, 20, 110), false);
    expect_eq(cache.stats().bypassed_mutating, 1);
    expect_eq(cache.stats().entries, 3);
    expect_eq(cache.lookup(k_read_s1, 120) != null, true);

    // MutatingInvalidateScope: rejected & automatically invalidates scope 10 only (scope 20 survives)
    expect_eq(cache.put(k_write_s1, ToolSideEffectMode.MutatingInvalidateScope, "written", 0, 30, 130), false);
    expect_eq(cache.stats().bypassed_mutating, 2);
    expect_eq(cache.stats().invalidations, 2);
    expect_eq(cache.stats().entries, 1);
    expect_eq(cache.lookup(k_read_s1, 140) == null, true);
    expect_eq(cache.lookup(k_list_s1, 140) == null, true);
    expect_eq(std.mem.eql(u8, cache.lookup(k_read_s2, 140) ?? "", "r2"), true);

    // invalidateTool removes all entries for "read_file" across all scopes
    expect_eq(cache.put(k_read_s1, ToolSideEffectMode.ReadOnlyCacheable, "r1-new", 0, 50, 150), true);
    expect_eq(cache.invalidateTool("read_file"), 2);
    expect_eq(cache.stats().entries, 0);
    expect_eq(cache.stats().invalidations, 4);
}

fn testLruEvictionAndPayloadCompaction()! {
    // 1. Slot-pressure LRU eviction (capacity = 3)
    let slots_buf: [3]ToolCacheEntry = [ToolCacheEntry{}; 3];
    let payload_buf: [128]u8 = [0; 128];
    let slots: []ToolCacheEntry = slots_buf[0..];
    let payload: []u8 = payload_buf[0..];
    let cache = ToolCallCache.init(slots, payload);

    const ka = computeToolCacheKey("t", "a", 1);
    const kb = computeToolCacheKey("t", "b", 1);
    const kc = computeToolCacheKey("t", "c", 1);
    const kd = computeToolCacheKey("t", "d", 1);

    expect_eq(cache.put(ka, ToolSideEffectMode.ReadOnlyCacheable, "valA", 0, 10, 100), true);
    expect_eq(cache.put(kb, ToolSideEffectMode.ReadOnlyCacheable, "valB", 0, 10, 101), true);
    expect_eq(cache.put(kc, ToolSideEffectMode.ReadOnlyCacheable, "valC", 0, 10, 102), true);

    // Touch ka at t=105 -> kb is now LRU
    expect_eq(std.mem.eql(u8, cache.lookup(ka, 105) ?? "", "valA"), true);

    // Insert kd -> evicts kb
    expect_eq(cache.put(kd, ToolSideEffectMode.ReadOnlyCacheable, "valD", 0, 10, 106), true);
    expect_eq(cache.stats().evictions, 1);
    expect_eq(cache.lookup(kb, 107) == null, true);
    expect_eq(std.mem.eql(u8, cache.lookup(ka, 107) ?? "", "valA"), true);
    expect_eq(std.mem.eql(u8, cache.lookup(kc, 107) ?? "", "valC"), true);
    expect_eq(std.mem.eql(u8, cache.lookup(kd, 107) ?? "", "valD"), true);

    // 2. Byte-pressure in-place compaction & LRU eviction on tight 16-byte payload buffer
    let tight_slots_buf: [4]ToolCacheEntry = [ToolCacheEntry{}; 4];
    let tight_payload_buf: [16]u8 = [0; 16];
    let tight_slots: []ToolCacheEntry = tight_slots_buf[0..];
    let tight_payload: []u8 = tight_payload_buf[0..];
    let tight = ToolCallCache.init(tight_slots, tight_payload);

    expect_eq(tight.put(ka, ToolSideEffectMode.ReadOnlyCacheable, "12345678", 0, 10, 10), true); // 8B
    expect_eq(tight.put(kb, ToolSideEffectMode.ReadOnlyCacheable, "abcdefgh", 0, 10, 11), true); // 8B -> 16B full
    // Inserting 8B kc triggers compaction + LRU eviction of ka (8B freed)
    expect_eq(tight.put(kc, ToolSideEffectMode.ReadOnlyCacheable, "ABCDEFGH", 0, 10, 12), true);
    expect_eq(tight.stats().evictions, 1);
    expect_eq(tight.lookup(ka, 13) == null, true);
    expect_eq(std.mem.eql(u8, tight.lookup(kb, 13) ?? "", "abcdefgh"), true);
    expect_eq(std.mem.eql(u8, tight.lookup(kc, 13) ?? "", "ABCDEFGH"), true);

    // Oversized single result (> 16B) safely rejected without corrupting existing entries
    expect_eq(tight.put(kd, ToolSideEffectMode.ReadOnlyCacheable, "01234567890123456", 0, 10, 14), false);
    expect_eq(std.mem.eql(u8, tight.lookup(kb, 15) ?? "", "abcdefgh"), true);
}

fn testStatsLatencyAccountingAndZeroCapacity()! {
    let slots_buf: [4]ToolCacheEntry = [ToolCacheEntry{}; 4];
    let payload_buf: [64]u8 = [0; 64];
    let slots: []ToolCacheEntry = slots_buf[0..];
    let payload: []u8 = payload_buf[0..];
    let cache = ToolCallCache.init(slots, payload);

    const k1 = computeToolCacheKey("search", "q1", 1);
    const k2 = computeToolCacheKey("search", "q2", 1);

    expect_eq(cache.put(k1, ToolSideEffectMode.ReadOnlyCacheable, "res1", 0, 250, 100), true);
    expect_eq(cache.put(k2, ToolSideEffectMode.ReadOnlyCacheable, "res2", 0, 400, 100), true);

    // 2 hits on k1 (500ms saved), 1 hit on k2 (400ms saved) => 900ms total saved
    expect_eq(cache.lookup(k1, 110) != null, true);
    expect_eq(cache.lookup(k1, 120) != null, true);
    expect_eq(cache.lookup(k2, 130) != null, true);
    expect_eq(cache.lookup(computeToolCacheKey("search", "miss", 1), 140) == null, true);

    const st = cache.stats();
    expect_eq(st.entries, 2);
    expect_eq(st.hits, 3);
    expect_eq(st.misses, 1);
    expect_eq(st.total_saved_latency_ms, 900);

    // Zero-capacity slot & payload boundary guards
    let z_slots_buf: [0]ToolCacheEntry = undefined;
    let z_payload_buf: [0]u8 = undefined;
    let z_slots: []ToolCacheEntry = z_slots_buf[0..];
    let z_payload: []u8 = z_payload_buf[0..];
    let z_cache = ToolCallCache.init(z_slots, z_payload);
    expect_eq(z_cache.put(k1, ToolSideEffectMode.ReadOnlyCacheable, "x", 0, 10, 100), false);
    expect_eq(z_cache.lookup(k1, 100) == null, true);
    expect_eq(z_cache.purgeExpired(100), 0);
    expect_eq(z_cache.invalidateScope(1), 0);
}

fn main() {
    test("1. agent tool_cache: hit/miss, scope isolation & key overwrite", () => {
        testHitMissAndScopeIsolation()?;
    });
    test("2. agent tool_cache: TTL expiration, purgeExpired & non-expiring entries", () => {
        testTtlExpirationAndPurge()?;
    });
    test("3. agent tool_cache: side-effect gating, scope invalidation & tool invalidation", () => {
        testSideEffectGatingAndInvalidation()?;
    });
    test("4. agent tool_cache: LRU eviction & in-place payload compaction", () => {
        testLruEvictionAndPayloadCompaction()?;
    });
    test("5. agent tool_cache: saved latency telemetry & zero-capacity boundary guards", () => {
        testStatsLatencyAccountingAndZeroCapacity()?;
    });
}
