// Source: cp/cap/test/test_lru_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:lru & cap.lru

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { LruCache, LruStats, LruError } from "cap:lru";

test("1. basic put/get/peek eviction order: get refreshes LRU while peek does not") {
    const alloc = std.testing.allocator;
    let cache = LruCache<i32>.init(alloc, 3, 0)?;
    defer cache.deinit();

    // Ownership-safe key copying: mutate caller buffer immediately after put
    let key_buf: [8]u8 = undefined;
    @memcpy(key_buf[0..3], "k_a");
    cache.put(key_buf[0..3], 100)?;
    @memcpy(key_buf[0..3], "XXX");
    expect_eq(cache.contains("k_a", 0), true);
    expect_eq(cache.contains("XXX", 0), false);

    cache.put("k_b", 200)?;
    cache.put("k_c", 300)?;
    expect_eq(cache.len(), 3);

    // peek() must NOT refresh LRU order and must NOT change hits/misses
    expect_eq(cache.peek("k_a", 0).?, 100);
    let st0 = cache.stats();
    expect_eq(st0.hits, 0);
    expect_eq(st0.misses, 0);

    // get("k_a", 0) MUST promote "k_a" to MRU, leaving "k_b" as LRU victim
    expect_eq(cache.get("k_a", 0).?, 100);

    // Inserting "k_d" must evict "k_b"
    cache.put("k_d", 400)?;
    expect_eq(cache.len(), 3);
    expect_eq(cache.contains("k_b", 0), false);
    expect_eq(cache.contains("k_a", 0), true);
    expect_eq(cache.contains("k_c", 0), true);
    expect_eq(cache.contains("k_d", 0), true);

    let st1 = cache.stats();
    expect_eq(st1.hits, 1);
    expect_eq(st1.misses, 0);
    expect_eq(st1.evictions, 1);
}

test("2. max_weight byte-weight limit multi-eviction and EntryTooLarge/InvalidWeight guards") {
    const alloc = std.testing.allocator;
    let cache = LruCache<i32>.init(alloc, 10, 100)?;
    defer cache.deinit();

    // InvalidWeight when weight == 0
    let invalid_w = false;
    * = cache.putWeighted("bad_w", 1, 0, 0, 0) catch {
        invalid_w = true;
    };
    expect_eq(invalid_w, true);

    // EntryTooLarge when weight > max_weight
    let too_large = false;
    * = cache.putWeighted("huge", 999, 101, 0, 0) catch {
        too_large = true;
    };
    expect_eq(too_large, true);
    expect_eq(cache.len(), 0);
    expect_eq(cache.currentWeight(), 0);

    // Insert 3 entries totaling weight 90 (30 + 30 + 30)
    cache.putWeighted("w1", 1, 30, 0, 0)?;
    cache.putWeighted("w2", 2, 30, 0, 0)?;
    cache.putWeighted("w3", 3, 30, 0, 0)?;
    expect_eq(cache.currentWeight(), 90);
    expect_eq(cache.len(), 3);

    // Insert "w4" with weight 70 -> must evict both "w1" and "w2" (30 + 70 = 100 <= 100)
    cache.putWeighted("w4", 4, 70, 0, 0)?;
    expect_eq(cache.len(), 2);
    expect_eq(cache.currentWeight(), 100);
    expect_eq(cache.contains("w1", 0), false);
    expect_eq(cache.contains("w2", 0), false);
    expect_eq(cache.contains("w3", 0), true);
    expect_eq(cache.contains("w4", 0), true);

    let st = cache.stats();
    expect_eq(st.evictions, 2);
    expect_eq(st.current_weight, 100);
    expect_eq(st.max_weight, 100);
}

test("3. deterministic now_ms TTL lazy expiration, purgeExpired and expirations stats") {
    const alloc = std.testing.allocator;
    let cache = LruCache<i32>.init(alloc, 8, 500)?;
    defer cache.deinit();

    cache.putWeighted("short", 10, 50, 100, 1000)?;   // expires at 1100
    cache.putWeighted("medium", 20, 60, 300, 1000)?;  // expires at 1300
    cache.putWeighted("persist", 30, 70, 0, 1000)?;   // no TTL

    // Before expiration
    expect_eq(cache.get("short", 1099).?, 10);
    expect_eq(cache.peek("medium", 1299).?, 20);

    // At 1100, "short" is expired: peek/contains see null/false without mutating, get lazily removes
    expect_eq(cache.contains("short", 1100), false);
    expect_eq(cache.peek("short", 1100) == null, true);
    expect_eq(cache.len(), 3);

    expect_eq(cache.get("short", 1100) == null, true);
    expect_eq(cache.len(), 2);
    expect_eq(cache.currentWeight(), 130); // 60 + 70

    // purgeExpired at 1300 removes "medium" while keeping "persist"
    const purged = cache.purgeExpired(1300);
    expect_eq(purged, 1);
    expect_eq(cache.len(), 1);
    expect_eq(cache.currentWeight(), 70);
    expect_eq(cache.get("persist", 5000).?, 30);

    let st = cache.stats();
    expect_eq(st.expirations, 2);
    expect_eq(st.hits, 2);
    expect_eq(st.misses, 1);
}

test("4. overwrite update, removeEntry, resize shrink eviction, InvalidCapacity and hitRate") {
    const alloc = std.testing.allocator;

    // InvalidCapacity guard
    let invalid_cap = false;
    * = LruCache<i32>.init(alloc, 0, 100) catch {
        invalid_cap = true;
    };
    expect_eq(invalid_cap, true);

    let cache = LruCache<i32>.init(alloc, 4, 200)?;
    defer cache.deinit();

    cache.putWeighted("a", 1, 40, 0, 0)?;
    cache.putWeighted("b", 2, 40, 0, 0)?;
    cache.putWeighted("c", 3, 40, 0, 0)?;
    cache.putWeighted("d", 4, 40, 0, 0)?;
    expect_eq(cache.currentWeight(), 160);

    // Overwrite "b" with weight 100 -> total would be 220 > 200, so LRU ("a") is evicted
    cache.putWeighted("b", 22, 100, 0, 0)?;
    expect_eq(cache.contains("a", 0), false);
    expect_eq(cache.peek("b", 0).?, 22);
    expect_eq(cache.currentWeight(), 180); // c(40) + d(40) + b(100)

    // removeEntry returns old value and frees weight
    expect_eq(cache.removeEntry("c").?, 3);
    expect_eq(cache.removeEntry("c") == null, true);
    expect_eq(cache.currentWeight(), 140); // d(40) + b(100)
    expect_eq(cache.len(), 2);

    // resize shrink: max_entries = 2, max_weight = 100 -> evicts LRU "d", keeps MRU "b"
    cache.resize(2, 100)?;
    expect_eq(cache.capacity(), 2);
    expect_eq(cache.maxWeight(), 100);
    expect_eq(cache.len(), 1);
    expect_eq(cache.currentWeight(), 100);
    expect_eq(cache.contains("d", 0), false);
    expect_eq(cache.get("b", 0).?, 22);
    expect_eq(cache.get("missing", 0) == null, true);

    let st = cache.stats();
    expect_eq(st.hits, 1);
    expect_eq(st.misses, 1);
    expect_eq(st.hitRate(), 0.5);

    cache.clear();
    expect_eq(cache.isEmpty(), true);
    expect_eq(cache.currentWeight(), 0);
}

test("5. cap.lru dynamic mounting and std.testing.allocator zero-leak verification") {
    const alloc = std.testing.allocator;
    let cache = cap.lru.LruCache<i64>.init(alloc, 2, 0)?;
    defer cache.deinit();

    cache.put("x", 10)?;
    cache.put("y", 20)?;
    expect_eq(cache.get("x", 1).?, 10);
    cache.put("z", 30)?; // evicts "y"

    expect_eq(cache.contains("y", 1), false);
    expect_eq(cache.get("x", 1).?, 10);
    expect_eq(cache.get("z", 1).?, 30);

    // Resize up and down to verify zero leaks in reallocation path
    cache.resize(5, 10)?;
    cache.putWeighted("w", 40, 5, 100, 10)?;
    expect_eq(cache.get("w", 50).?, 40);
    cache.resize(1, 0)?;
    expect_eq(cache.len(), 1);
    expect_eq(cache.get("w", 50).?, 40);
}
