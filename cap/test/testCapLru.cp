// cap/test/testCapLru.cp
// Comprehensive unit tests for @cap/lru: O(1) LRU eviction, peek vs get, update promotion & weight,
// TTL expiration & purgeExpired, max_weight multi-entry eviction, hash collision & tombstone reuse/rehash,
// hitRate observability stats, and boundary guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MAX_CAPACITY,
    MAX_KEY_LEN,
    MAX_VAL_LEN,
    LruConfig,
    LruStats,
    LruCache,
} from "../../packages/lru-cache/src/index.cp";

fn eqStr(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn formatPrefixedInt(buf: []u8, prefix: []const u8, n: usize): []const u8 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        buf[pos] = prefix[i];
        pos += 1;
    }
    if (n == 0) {
        buf[pos] = '0';
        pos += 1;
        return buf[0..pos];
    }
    let tmp: [20]u8 = [0; 20];
    let dlen: usize = 0;
    let v = n;
    while (v > 0) {
        tmp[dlen] = ('0' as u8) + ((v % 10) as u8);
        dlen += 1;
        v /= 10;
    }
    let j: usize = dlen;
    while (j > 0) {
        j -= 1;
        buf[pos] = tmp[j];
        pos += 1;
    }
    return buf[0..pos];
}

fn main() {
    test("1. @cap/lru — Basic Put/Get/Peek/Contains & O(1) LRU Eviction Order", () => {
        let cache = LruCache.init(LruConfig{ max_entries: 3 });
        expect_eq(cache.isEmpty(), true);
        expect_eq(cache.len(), 0 as usize);
        expect_eq(cache.mostRecentKey() == null, true);
        expect_eq(cache.leastRecentKey() == null, true);

        expect_eq(cache.put("a", "1", 0), true);
        expect_eq(cache.put("b", "2", 0), true);
        expect_eq(cache.put("c", "3", 0), true);
        expect_eq(cache.len(), 3 as usize);
        expect_eq(cache.isEmpty(), false);
        expect_eq(eqStr(cache.mostRecentKey().?, "c"), true);
        expect_eq(eqStr(cache.leastRecentKey().?, "a"), true);

        // peek must NOT alter LRU order or stats
        expect_eq(eqStr(cache.peek("a", 0).?, "1"), true);
        expect_eq(cache.contains("a", 0), true);
        expect_eq(eqStr(cache.leastRecentKey().?, "a"), true);
        expect_eq(cache.stats().hits, 0 as u64);

        // get promotes "a" to MRU; LRU becomes "b"
        expect_eq(eqStr(cache.get("a", 0).?, "1"), true);
        expect_eq(eqStr(cache.mostRecentKey().?, "a"), true);
        expect_eq(eqStr(cache.leastRecentKey().?, "b"), true);

        // Inserting "d" evicts LRU "b"
        expect_eq(cache.put("d", "4", 0), true);
        expect_eq(cache.len(), 3 as usize);
        expect_eq(cache.contains("b", 0), false);
        expect_eq(cache.get("b", 0) == null, true);
        expect_eq(eqStr(cache.mostRecentKey().?, "d"), true);
        expect_eq(eqStr(cache.leastRecentKey().?, "c"), true);
        expect_eq(cache.stats().evictions, 1 as u64);
    });

    test("2. @cap/lru — Update Existing Key Promotes Position & Adjusts Weight", () => {
        let cache = LruCache.init(LruConfig{ max_entries: 3, max_weight: 20 });
        expect_eq(cache.putWithOptions("k1", "v1", 0, 5, 1), true);
        expect_eq(cache.putWithOptions("k2", "v2", 0, 5, 2), true);
        expect_eq(cache.putWithOptions("k3", "v3", 0, 5, 3), true);
        expect_eq(cache.totalWeight(), 15 as u64);
        expect_eq(eqStr(cache.leastRecentKey().?, "k1"), true);

        // Update "k1" with higher weight (12): total would be 22 > 20, so "k2" (now LRU) gets evicted
        expect_eq(cache.putWithOptions("k1", "v1_updated", 0, 12, 4), true);
        expect_eq(eqStr(cache.mostRecentKey().?, "k1"), true);
        expect_eq(eqStr(cache.peek("k1", 4).?, "v1_updated"), true);
        expect_eq(cache.contains("k2", 4), false);
        expect_eq(cache.contains("k3", 4), true);
        expect_eq(cache.len(), 2 as usize);
        expect_eq(cache.totalWeight(), 17 as u64);
    });

    test("3. @cap/lru — TTL Expiration, Lazy Eviction & purgeExpired", () => {
        let cache = LruCache.init(LruConfig{ max_entries: 16, default_ttl_ns: 100 });
        expect_eq(cache.put("short", "s", 1000), true); // expires at 1100
        expect_eq(cache.putWithOptions("med", "m", 250, 0, 1000), true); // expires at 1250
        expect_eq(cache.putWithOptions("forever", "f", 0, 0, 1000), true); // never expires

        // At t=1099 all valid
        expect_eq(cache.contains("short", 1099), true);
        expect_eq(eqStr(cache.peek("short", 1099).?, "s"), true);

        // At t=1100 "short" is expired: peek/contains see null/false, get lazily evicts & counts expiration
        expect_eq(cache.peek("short", 1100) == null, true);
        expect_eq(cache.contains("short", 1100), false);
        expect_eq(cache.len(), 3 as usize); // peek did not mutate
        expect_eq(cache.get("short", 1100) == null, true);
        expect_eq(cache.len(), 2 as usize);
        expect_eq(cache.stats().expirations, 1 as u64);

        // Add another expiring entry and purgeExpired at t=1300
        expect_eq(cache.putWithOptions("tmp", "t", 50, 0, 1200), true); // expires at 1250
        expect_eq(cache.len(), 3 as usize);
        const purged = cache.purgeExpired(1300);
        expect_eq(purged, 2 as usize);
        expect_eq(cache.len(), 1 as usize);
        expect_eq(eqStr(cache.get("forever", 999999).?, "f"), true);
        expect_eq(cache.stats().expirations, 3 as u64);
    });

    test("4. @cap/lru — Byte-Weight Multi-Entry Eviction & Oversized Rejection", () => {
        let cache = LruCache.init(LruConfig{ max_entries: 10, max_weight: 50 });
        expect_eq(cache.putWithOptions("a", "1", 0, 15, 0), true);
        expect_eq(cache.putWithOptions("b", "2", 0, 15, 0), true);
        expect_eq(cache.putWithOptions("c", "3", 0, 15, 0), true);
        expect_eq(cache.totalWeight(), 45 as u64);

        // Entry of weight 35 requires evicting both "a" and "b" (remaining: "c"=15 + "d"=35 = 50)
        expect_eq(cache.putWithOptions("d", "4", 0, 35, 0), true);
        expect_eq(cache.len(), 2 as usize);
        expect_eq(cache.totalWeight(), 50 as u64);
        expect_eq(cache.contains("a", 0), false);
        expect_eq(cache.contains("b", 0), false);
        expect_eq(cache.contains("c", 0), true);
        expect_eq(cache.contains("d", 0), true);

        // Oversized entry (> max_weight) is rejected without disturbing cache
        expect_eq(cache.putWithOptions("huge", "x", 0, 51, 0), false);
        expect_eq(cache.len(), 2 as usize);
        expect_eq(cache.totalWeight(), 50 as u64);
    });

    test("5. @cap/lru — Remove, popLru, Clear & ResetStats", () => {
        let cache = LruCache.init(LruConfig{ max_entries: 4 });
        expect_eq(cache.put("k1", "v1", 0), true);
        expect_eq(cache.put("k2", "v2", 0), true);
        expect_eq(cache.put("k3", "v3", 0), true);

        // Remove middle node
        expect_eq(cache.remove("k2"), true);
        expect_eq(cache.remove("k2"), false);
        expect_eq(cache.len(), 2 as usize);
        expect_eq(eqStr(cache.mostRecentKey().?, "k3"), true);
        expect_eq(eqStr(cache.leastRecentKey().?, "k1"), true);

        // popLru evicts "k1"
        expect_eq(cache.popLru(), true);
        expect_eq(cache.len(), 1 as usize);
        expect_eq(eqStr(cache.mostRecentKey().?, "k3"), true);
        expect_eq(eqStr(cache.leastRecentKey().?, "k3"), true);

        // clear empties entries and weight while keeping stats
        cache.clear();
        expect_eq(cache.isEmpty(), true);
        expect_eq(cache.totalWeight(), 0 as u64);
        expect_eq(cache.popLru(), false);
        expect_eq(cache.stats().evictions, 1 as u64);

        cache.resetStats();
        expect_eq(cache.stats().evictions, 0 as u64);
    });

    test("6. @cap/lru — High-Churn Tombstone Reuse & Automatic Rehash Integrity", () => {
        let cache = LruCache.init(LruConfig{ max_entries: 64 });
        let key_buf: [16]u8 = [0; 16];
        let val_buf: [16]u8 = [0; 16];

        // 600 rounds of insert+delete exceeds HASH_BUCKETS (512) and triggers automatic rehash
        let round: usize = 0;
        while (round < 600) : (round += 1) {
            const k = formatPrefixedInt(key_buf[0..], "key_", round);
            const v = formatPrefixedInt(val_buf[0..], "val_", round);
            expect_eq(cache.put(k, v, 0), true);
            expect_eq(eqStr(cache.get(k, 0).?, v), true);
            if (round % 3 != 0) {
                expect_eq(cache.remove(k), true);
            }
        }

        // Round 597 fills cache to 64; round 598 evicts 1 then removes itself -> 63; round 599 adds then removes -> 63
        expect_eq(cache.len(), 63 as usize);
        expect_eq(cache.contains("key_598", 0), false);
        expect_eq(cache.contains("key_599", 0), false);
        // Verify last kept key (597) is MRU and retrievable
        expect_eq(eqStr(cache.mostRecentKey().?, "key_597"), true);
        expect_eq(eqStr(cache.get("key_597", 0).?, "val_597"), true);
    });

    test("7. @cap/lru — HitRate Stats & Boundary Guards", () => {
        let empty_stats = LruStats{};
        expect_eq(empty_stats.hitRate(), 0.0);

        let cache = LruCache.init(LruConfig{ max_entries: 4 });
        expect_eq(cache.put("ok", "", 0), true); // empty value is allowed
        expect_eq(cache.get("ok", 0).?.len, 0 as usize); // hit 1
        expect_eq(cache.get("ok", 0) != null, true); // hit 2
        expect_eq(cache.get("ok", 0) != null, true); // hit 3
        expect_eq(cache.get("miss", 0) == null, true); // miss 1

        const st = cache.stats();
        expect_eq(st.hits, 3 as u64);
        expect_eq(st.misses, 1 as u64);
        expect_eq(st.hitRate(), 0.75);

        // Boundary guards: empty key, oversized key (> 64), oversized val (> 256)
        expect_eq(cache.put("", "v", 0), false);
        let big_key: [MAX_KEY_LEN + 1]u8 = ['a'; MAX_KEY_LEN + 1];
        expect_eq(cache.put(big_key[0..], "v", 0), false);
        let big_val: [MAX_VAL_LEN + 1]u8 = ['b'; MAX_VAL_LEN + 1];
        expect_eq(cache.put("k", big_val[0..], 0), false);

        // Zero-capacity cache rejects all inserts safely
        let zero_cache = LruCache.init(LruConfig{ max_entries: 0 });
        expect_eq(zero_cache.capacity(), 0 as usize);
        expect_eq(zero_cache.put("a", "b", 0), false);

        // Over-capacity config clamps to MAX_CAPACITY
        let clamped = LruCache.init(LruConfig{ max_entries: 9999 });
        expect_eq(clamped.capacity(), MAX_CAPACITY);
    });
}
