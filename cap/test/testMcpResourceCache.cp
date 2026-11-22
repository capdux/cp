// Origin: Pure-CP Stdlib Test Suite (lib/mcp/resourceCache.cp) | Created: 2026-11-23
// Production Deep Verification Suite for MCP Resource Cache & Subscription Invalidation:
//   1. Insert, `hit_fresh`, SWR grace-window `hit_stale_revalidate`, `miss_expired` & version increment
//   2. ETag `revalidateNotModified` zero-copy TTL renewal & mismatch rejection
//   3. Exact `invalidateUri` & directory `invalidatePrefix` batch invalidation
//   4. Byte-budget (`max_bytes`) LRU eviction evicting least-recently-accessed entries
//   5. `pinned` protection against LRU eviction & atomic rejection when pinned bytes saturate budget
//   6. Oversized payload rejection, `clear()` & full `ResCacheStats` accounting
//   7. `cap.mcp` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_RES_CACHE_MAX_ENTRIES,
    MCP_RES_URI_MAX_LEN,
    MCP_RES_ETAG_MAX_LEN,
    MCP_RES_BODY_MAX_LEN,
    ResCacheLookupKind,
    ResCachePutStatus,
    McpResourceCache,
} from "cap:mcp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("ResourceCache: put, hit_fresh, SWR hit_stale_revalidate, miss_expired & version bump") {
    let cache = McpResourceCache.init(1024, 5000, 2000); // TTL=5s, SWR=2s

    const st1 = cache.put("file:///docs/readme.md", "etag-v1", "hello world", 1000, 0, false);
    expect_true(st1 == ResCachePutStatus.inserted);

    // t=4000 <= 6000 -> hit_fresh
    const r_fresh = cache.lookup("file:///docs/readme.md", 4000);
    expect_true(r_fresh.kind == ResCacheLookupKind.hit_fresh);
    expect_eq(r_fresh.version, @as(u64, 1));
    expect_eq(r_fresh.body_len, @as(usize, 11));
    if (entry := cache.entryAt(r_fresh.entry_index)) {
        expect_true(strEq(entry.body(), "hello world"));
        expect_true(strEq(entry.etag(), "etag-v1"));
        expect_true(entry.content_hash != 0);
    } else {
        expect_true(false);
    }

    // t=7000 in (6000, 8000] -> hit_stale_revalidate (SWR grace window)
    const r_stale = cache.lookup("file:///docs/readme.md", 7000);
    expect_true(r_stale.kind == ResCacheLookupKind.hit_stale_revalidate);

    // t=8001 > 8000 -> miss_expired
    const r_exp = cache.lookup("file:///docs/readme.md", 8001);
    expect_true(r_exp.kind == ResCacheLookupKind.miss_expired);

    // Update same URI -> updated & version increments to 2
    const st2 = cache.put("file:///docs/readme.md", "etag-v2", "updated content", 9000, 0, false);
    expect_true(st2 == ResCachePutStatus.updated);
    const r_upd = cache.lookup("file:///docs/readme.md", 9500);
    expect_true(r_upd.kind == ResCacheLookupKind.hit_fresh);
    expect_eq(r_upd.version, @as(u64, 2));
}

test("ResourceCache: ETag revalidateNotModified renews TTL without recopying body") {
    let cache = McpResourceCache.init(1024, 3000, 1000);
    * = cache.put("file:///cfg/app.json", "\"abc123\"", "{\"env\":\"prod\"}", 1000, 0, false);

    // At t=4500 it is past TTL (4000), inside SWR (5000)
    const r_stale = cache.lookup("file:///cfg/app.json", 4500);
    expect_true(r_stale.kind == ResCacheLookupKind.hit_stale_revalidate);

    // Wrong ETag rejected
    expect_true(!cache.revalidateNotModified("file:///cfg/app.json", "\"wrong\"", 4600));
    expect_true(!cache.revalidateNotModified("file:///cfg/app.json", "", 4600));

    // Matching ETag renews expires_at = 4600 + 3000 = 7600
    expect_true(cache.revalidateNotModified("file:///cfg/app.json", "\"abc123\"", 4600));
    const r_renewed = cache.lookup("file:///cfg/app.json", 7000);
    expect_true(r_renewed.kind == ResCacheLookupKind.hit_fresh);
    expect_eq(r_renewed.version, @as(u64, 1)); // body untouched
}

test("ResourceCache: invalidateUri & invalidatePrefix batch invalidation") {
    let cache = McpResourceCache.init(2048, 60000, 0);
    * = cache.put("file:///repo/src/a.cp", "e1", "aaa", 1000, 0, false);
    * = cache.put("file:///repo/src/b.cp", "e2", "bbb", 1000, 0, false);
    * = cache.put("file:///repo/docs/c.md", "e3", "ccc", 1000, 0, false);

    // Exact invalidate on docs/c.md
    expect_true(cache.invalidateUri("file:///repo/docs/c.md"));
    expect_true(!cache.invalidateUri("file:///repo/missing"));
    const rc = cache.lookup("file:///repo/docs/c.md", 1500);
    expect_true(rc.kind == ResCacheLookupKind.miss_invalidated);

    // Prefix invalidate on file:///repo/src/ -> invalidates both a.cp and b.cp
    const n_inv = cache.invalidatePrefix("file:///repo/src/");
    expect_eq(n_inv, @as(usize, 2));
    expect_true(cache.lookup("file:///repo/src/a.cp", 1500).kind == ResCacheLookupKind.miss_invalidated);
    expect_true(cache.lookup("file:///repo/src/b.cp", 1500).kind == ResCacheLookupKind.miss_invalidated);

    // Re-putting an invalidated URI clears invalidated and restores hit_fresh
    const st_re = cache.put("file:///repo/src/a.cp", "e1-new", "aaa2", 2000, 0, false);
    expect_true(st_re == ResCachePutStatus.updated);
    expect_true(cache.lookup("file:///repo/src/a.cp", 2100).kind == ResCacheLookupKind.hit_fresh);
    expect_eq(cache.stats().invalidations, @as(u64, 3));
}

test("ResourceCache: byte-budget LRU eviction evicts oldest accessed non-pinned entry") {
    // Budget = 20 bytes
    let cache = McpResourceCache.init(20, 60000, 0);
    const body10: []const u8 = "0123456789"; // 10 bytes

    expect_true(cache.put("res://1", "e1", body10, 1000, 0, false) == ResCachePutStatus.inserted);
    expect_true(cache.put("res://2", "e2", body10, 1100, 0, false) == ResCachePutStatus.inserted);
    expect_eq(cache.stats().used_bytes, @as(usize, 20));

    // Touch res://1 at t=1500 so res://2 becomes the LRU victim
    expect_true(cache.lookup("res://1", 1500).kind == ResCacheLookupKind.hit_fresh);

    // Insert res://3 (10 bytes) -> must evict res://2, keeping res://1
    const st3 = cache.put("res://3", "e3", body10, 1600, 0, false);
    expect_true(st3 == ResCachePutStatus.evicted_and_inserted);
    expect_eq(cache.stats().used_bytes, @as(usize, 20));
    expect_eq(cache.stats().evictions, @as(u64, 1));
    expect_true(cache.lookup("res://2", 1700).kind == ResCacheLookupKind.miss_not_found);
    expect_true(cache.lookup("res://1", 1700).kind == ResCacheLookupKind.hit_fresh);
    expect_true(cache.lookup("res://3", 1700).kind == ResCacheLookupKind.hit_fresh);
}

test("ResourceCache: pinned protection & atomic rejection when pinned saturates budget") {
    // Budget = 20 bytes
    let cache = McpResourceCache.init(20, 60000, 0);
    const body10: []const u8 = "abcdefghij";

    // Pin res://sys (oldest) and insert unpinned res://tmp (newer)
    expect_true(cache.put("res://sys", "e1", body10, 1000, 0, true) == ResCachePutStatus.inserted);
    expect_true(cache.put("res://tmp", "e2", body10, 2000, 0, false) == ResCachePutStatus.inserted);

    // Insert res://new (10 bytes) -> skips older pinned res://sys and evicts res://tmp
    expect_true(cache.put("res://new", "e3", body10, 3000, 0, true) == ResCachePutStatus.evicted_and_inserted);
    expect_true(cache.lookup("res://sys", 3100).kind == ResCacheLookupKind.hit_fresh);
    expect_true(cache.lookup("res://tmp", 3100).kind == ResCacheLookupKind.miss_not_found);

    // Now both res://sys and res://new are pinned (20/20 bytes) -> 3rd insert rejected atomically
    expect_true(cache.put("res://denied", "e4", "12345", 4000, 0, false) == ResCachePutStatus.rejected_too_large);
    expect_eq(cache.stats().entry_count, @as(usize, 2));
}

test("ResourceCache: oversized payload rejection, clear() & stats accounting") {
    let cache = McpResourceCache.init(16, 10000, 0);

    // Body > max_bytes rejected
    expect_true(cache.put("res://big", "e", "12345678901234567", 1000, 0, false) == ResCachePutStatus.rejected_too_large);
    // Empty URI rejected
    expect_true(cache.put("", "e", "ok", 1000, 0, false) == ResCachePutStatus.rejected_too_large);

    expect_true(cache.put("res://ok", "e", "1234", 1000, 0, false) == ResCachePutStatus.inserted);
    expect_eq(cache.stats().entry_count, @as(usize, 1));
    expect_eq(cache.stats().used_bytes, @as(usize, 4));

    cache.clear();
    expect_eq(cache.stats().entry_count, @as(usize, 0));
    expect_eq(cache.stats().used_bytes, @as(usize, 0));
    expect_true(cache.lookup("res://ok", 1100).kind == ResCacheLookupKind.miss_not_found);
}

test("ResourceCache: cap.mcp facade parity") {
    expect_eq(cap.mcp.MCP_RES_CACHE_MAX_ENTRIES, @as(usize, 32));
    expect_eq(cap.mcp.MCP_RES_URI_MAX_LEN, @as(usize, 96));
    expect_eq(cap.mcp.MCP_RES_ETAG_MAX_LEN, @as(usize, 48));
    expect_eq(cap.mcp.MCP_RES_BODY_MAX_LEN, @as(usize, 512));
    expect_eq(MCP_RES_CACHE_MAX_ENTRIES, @as(usize, 32));
    expect_eq(MCP_RES_URI_MAX_LEN, @as(usize, 96));
    expect_eq(MCP_RES_ETAG_MAX_LEN, @as(usize, 48));
    expect_eq(MCP_RES_BODY_MAX_LEN, @as(usize, 512));
}
