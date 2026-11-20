// Source: cp/cap/test/test_cache_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:cache & cap.cache

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CacheErr,
    CacheStats,
    LruCache,
    BloomFilter,
    CountMinSketch,
} from "cap:cache";

fn formatNumKey(buf: []u8, prefix: []const u8, idx: usize): []const u8 {
    return std.fmt.bufPrint(buf, "{s}{d}", .{ prefix, idx }) catch unreachable;
}

fn testLruO1EvictionOrderAndPeek(): !void {
    const alloc = std.heap.page_allocator;

    // Invalid capacity guard
    let invalid_res = false;
    * = LruCache<i32>.init(alloc, 0) catch {
        invalid_res = true;
    };
    expect_eq(invalid_res, true);

    let cache = LruCache<i32>.init(alloc, 3)?;
    defer cache.deinit();

    expect_eq(cache.capacity(), 3);
    expect_eq(cache.len(), 0);
    expect_eq(cache.isEmpty(), true);

    // Ownership-safe key copying: overwrite caller's mutable buffer right after put()
    let key_buf: [16]u8 = undefined;
    @memcpy(key_buf[0..4], "k_a1");
    cache.put(key_buf[0..4], 101)?;
    @memcpy(key_buf[0..4], "XXXX");
    expect_eq(cache.contains("k_a1"), true);
    expect_eq(cache.contains("XXXX"), false);
    expect_eq(cache.peek("k_a1").?, 101);

    // Fill to capacity 3: order MRU -> LRU is ["k_c", "k_b", "k_a1"]
    cache.put("k_b", 202)?;
    cache.put("k_c", 303)?;
    expect_eq(cache.len(), 3);

    // peek() must NOT refresh LRU order ("k_a1" remains LRU) or alter hit/miss counters
    expect_eq(cache.peek("k_a1").?, 101);
    let st0 = cache.stats();
    expect_eq(st0.hits, 0);
    expect_eq(st0.misses, 0);

    // get("k_a1") MUST refresh "k_a1" to MRU, making "k_b" the LRU victim
    expect_eq(cache.get("k_a1").?, 101);

    // Inserting "k_d" must evict "k_b" (not "k_a1")
    cache.put("k_d", 404)?;
    expect_eq(cache.len(), 3);
    expect_eq(cache.contains("k_b"), false);
    expect_eq(cache.contains("k_a1"), true);
    expect_eq(cache.contains("k_c"), true);
    expect_eq(cache.contains("k_d"), true);

    // Updating an existing key updates value and promotes to MRU without eviction
    cache.put("k_c", 333)?;
    expect_eq(cache.peek("k_c").?, 333);
    cache.put("k_e", 505)?; // evicts "k_a1" (which was LRU after "k_c" promoted and "k_d" added)
    expect_eq(cache.contains("k_a1"), false);
    expect_eq(cache.contains("k_c"), true);
    expect_eq(cache.contains("k_d"), true);
    expect_eq(cache.contains("k_e"), true);

    // Explicit remove() & reuse of freed slot
    expect_eq(cache.remove("k_d"), true);
    expect_eq(cache.remove("k_d"), false);
    expect_eq(cache.len(), 2);
    cache.put("k_f", 606)?;
    expect_eq(cache.len(), 3);
    expect_eq(cache.get("k_f").?, 606);

    // Miss check & stats verification
    expect_eq(cache.get("missing") == null, true);
    let st1 = cache.stats();
    expect_eq(st1.hits, 2);
    expect_eq(st1.misses, 1);
    expect_eq(st1.evictions, 2);

    // clear() retains capacity and drops entries
    cache.clear();
    expect_eq(cache.len(), 0);
    expect_eq(cache.isEmpty(), true);
    expect_eq(cache.contains("k_f"), false);
    cache.put("after_clear", 777)?;
    expect_eq(cache.get("after_clear").?, 777);
}

fn testLruDeterministicTtlEvictAndStats(): !void {
    const alloc = std.heap.page_allocator;
    let cache = LruCache<i64>.init(alloc, 4)?;
    defer cache.deinit();

    // Insert entries with deterministic now_ms = 1000
    cache.putWithTtl("short_1", 10, 500, 1000)?;  // expires at 1500
    cache.putWithTtl("short_2", 20, 800, 1000)?;  // expires at 1800
    cache.putWithTtl("long_1", 30, 5000, 1000)?;  // expires at 6000
    cache.put("perm_1", 40)?;                     // never expires

    // At now_ms = 1499, all entries are live
    expect_eq(cache.getAt("short_1", 1499).?, 10);
    expect_eq(cache.peekAt("short_2", 1499).?, 20);
    expect_eq(cache.containsAt("short_1", 1499), true);

    // At exact boundary now_ms = 1500, short_1 is lazily expired & removed on getAt
    expect_eq(cache.containsAt("short_1", 1500), false);
    expect_eq(cache.peekAt("short_1", 1500) == null, true);
    expect_eq(cache.len(), 4); // peek/contains must not mutate
    expect_eq(cache.getAt("short_1", 1500) == null, true);
    expect_eq(cache.len(), 3); // lazy eviction freed slot

    // At now_ms = 2000, evictExpired removes short_2 (expired at 1800), keeps long_1 & perm_1
    const swept = cache.evictExpired(2000);
    expect_eq(swept, 1);
    expect_eq(cache.len(), 2);
    expect_eq(cache.contains("short_2"), false);
    expect_eq(cache.getAt("long_1", 2000).?, 30);
    expect_eq(cache.getAt("perm_1", 9999999).?, 40);

    // Verify stats telemetry & hitRate calculation
    // Hits: short_1 (1499), long_1 (2000), perm_1 (9999999) => 3 hits
    // Misses: short_1 (1500) => 1 miss
    // Expirations: short_1 (lazy) + short_2 (evictExpired) => 2
    const st = cache.stats();
    expect_eq(st.hits, 3);
    expect_eq(st.misses, 1);
    expect_eq(st.expirations, 2);
    expect_eq(st.evictions, 0);
    expect_eq(st.len, 2);
    expect_eq(st.capacity, 4);
    expect_eq(st.totalRequests(), 4);
    const hr = st.hitRate();
    expect_eq(hr > 0.749 && hr < 0.751, true);
}

fn testBloomZeroFnLowFpAndUnion(): !void {
    const alloc = std.heap.page_allocator;

    // Optimal sizing for 1,000 items at 1% target false positive rate
    let bf_a = BloomFilter.initOptimal(alloc, 1000, 0.01)?;
    defer bf_a.deinit();

    expect_eq(bf_a.bitCount() >= 9000, true);
    expect_eq(bf_a.numHashes() >= 5 && bf_a.numHashes() <= 10, true);
    expect_eq(bf_a.fillRatio(), 0.0);
    expect_eq(bf_a.estimatedCount(), 0.0);

    let key_buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 500) : (i += 1) {
        const k = formatNumKey(&key_buf, "item_a:", i);
        bf_a.insert(k);
    }
    expect_eq(bf_a.insertedCount(), 500);

    // Zero false negatives for all 500 inserted items
    i = 0;
    while (i < 500) : (i += 1) {
        const k = formatNumKey(&key_buf, "item_a:", i);
        expect_eq(bf_a.mightContain(k), true);
    }

    // Measure empirical false positive rate on 2,000 unseen items (should be < 3%)
    let fp_count: usize = 0;
    i = 0;
    while (i < 2000) : (i += 1) {
        const k = formatNumKey(&key_buf, "unseen_probe:", i);
        if (bf_a.mightContain(k)) {
            fp_count += 1;
        }
    }
    expect_eq(fp_count < 60, true);

    // Check estimatedCount() is within reasonable range around 500
    const est_a = bf_a.estimatedCount();
    expect_eq(est_a > 400.0 && est_a < 600.0, true);
    expect_eq(bf_a.fillRatio() > 0.0 && bf_a.fillRatio() < 0.6, true);

    // Union with second identically sized Bloom filter
    let bf_b = BloomFilter.initOptimal(alloc, 1000, 0.01)?;
    defer bf_b.deinit();
    i = 500;
    while (i < 900) : (i += 1) {
        const k = formatNumKey(&key_buf, "item_b:", i);
        bf_b.insert(k);
    }

    bf_a.unionWith(bf_b)?;
    i = 0;
    while (i < 500) : (i += 1) {
        const k = formatNumKey(&key_buf, "item_a:", i);
        expect_eq(bf_a.mightContain(k), true);
    }
    i = 500;
    while (i < 900) : (i += 1) {
        const k = formatNumKey(&key_buf, "item_b:", i);
        expect_eq(bf_a.mightContain(k), true);
    }

    // Mismatched filter union must fail with SizeMismatch
    let bf_small = BloomFilter.initRaw(alloc, 128, 3)?;
    defer bf_small.deinit();
    let mismatch_err = false;
    bf_a.unionWith(bf_small) catch {
        mismatch_err = true;
    };
    expect_eq(mismatch_err, true);

    // clear() resets all bits
    bf_a.clear();
    expect_eq(bf_a.fillRatio(), 0.0);
    expect_eq(bf_a.insertedCount(), 0);
    expect_eq(bf_a.mightContain("item_a:0"), false);
}

fn testCountMinSketchNoUnderestimateAndMerge(): !void {
    const alloc = std.heap.page_allocator;

    // initWithError(epsilon=0.005, delta=0.001) -> width >= 544, depth >= 7
    let cms_a = CountMinSketch.initWithError(alloc, 0.005, 0.001)?;
    defer cms_a.deinit();
    expect_eq(cms_a.width() >= 540, true);
    expect_eq(cms_a.depth() >= 7, true);

    let cms_b = CountMinSketch.initWithError(alloc, 0.005, 0.001)?;
    defer cms_b.deinit();

    // Heavy hitters + background noise across two distributed shards
    cms_a.increment("prompt:gpt4", 150);
    cms_a.increment("prompt:claude", 80);
    cms_b.increment("prompt:gpt4", 250);
    cms_b.increment("prompt:gemini", 60);

    let key_buf: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 500) : (i += 1) {
        const k = formatNumKey(&key_buf, "tail_key:", i);
        cms_a.increment(k, 1);
    }

    expect_eq(cms_a.totalCount(), 730);
    expect_eq(cms_b.totalCount(), 310);

    // Never underestimates true counts
    expect_eq(cms_a.estimate("prompt:gpt4") >= 150, true);
    expect_eq(cms_a.estimate("prompt:claude") >= 80, true);
    expect_eq(cms_a.estimate("prompt:gpt4") <= 165, true);

    i = 0;
    while (i < 500) : (i += 1) {
        const k = formatNumKey(&key_buf, "tail_key:", i);
        expect_eq(cms_a.estimate(k) >= 1, true);
    }

    // Merge shard B into shard A
    cms_a.merge(cms_b)?;
    expect_eq(cms_a.totalCount(), 1040);
    expect_eq(cms_a.estimate("prompt:gpt4") >= 400, true);
    expect_eq(cms_a.estimate("prompt:gpt4") <= 420, true);
    expect_eq(cms_a.estimate("prompt:claude") >= 80, true);
    expect_eq(cms_a.estimate("prompt:gemini") >= 60, true);

    // Dimension mismatch must return error
    let cms_other = CountMinSketch.init(alloc, 64, 4)?;
    defer cms_other.deinit();
    let merge_err = false;
    cms_a.merge(cms_other) catch {
        merge_err = true;
    };
    expect_eq(merge_err, true);

    cms_a.clear();
    expect_eq(cms_a.totalCount(), 0);
    expect_eq(cms_a.estimate("prompt:gpt4"), 0);
}

fn testCapCacheAutomaticLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    // Verify `import cap from "cap"; cap.cache.*` automatic lazy mounting
    let hub_lru = cap.cache.LruCache(i32).init(alloc, 2)?;
    defer hub_lru.deinit();
    hub_lru.putWithTtl("k1", 11, 1000, 100)?;
    hub_lru.put("k2", 22)?;
    expect_eq(hub_lru.getAt("k1", 500).?, 11);
    expect_eq(hub_lru.getAt("k1", 1100) == null, true);

    const hub_stats: cap.cache.CacheStats = hub_lru.stats();
    expect_eq(hub_stats.hits, 1);
    expect_eq(hub_stats.expirations, 1);

    let hub_bf = cap.cache.BloomFilter.initRaw(alloc, 512, 4)?;
    defer hub_bf.deinit();
    hub_bf.insert("embedding:vec_42");
    expect_eq(hub_bf.mightContain("embedding:vec_42"), true);

    let hub_cms = cap.cache.CountMinSketch.init(alloc, 128, 4)?;
    defer hub_cms.deinit();
    hub_cms.increment("route:/v1/chat", 9);
    expect_eq(hub_cms.estimate("route:/v1/chat") >= 9, true);
}

fn main() {
    test("1. LruCache O(1) Eviction, Access Refresh, Ownership Copy & Peek", () => {
        testLruO1EvictionOrderAndPeek()?;
    });
    test("2. LruCache Deterministic TTL (now_ms), evictExpired & CacheStats", () => {
        testLruDeterministicTtlEvictAndStats()?;
    });
    test("3. BloomFilter Zero False Negatives, Low False Positives & unionWith", () => {
        testBloomZeroFnLowFpAndUnion()?;
    });
    test("4. CountMinSketch Zero Underestimation & Distributed Merge", () => {
        testCountMinSketchNoUnderestimateAndMerge()?;
    });
    test("5. cap.cache Automatic Dynamic Lazy Mounting Verification", () => {
        testCapCacheAutomaticLazyMount()?;
    });
}
