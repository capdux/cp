// Source: cp/cap/test/test_lfu_production_deep.cp (Production deep tests for cap:lfu)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TINYLFU_ROWS,
    TINYLFU_MAX_COUNT,
    TinyLfuSketch,
    LfuSlot,
    LfuCache,
} from "cap:db";

test("1. LfuCache evicts lowest frequency first & breaks equal-frequency ties by LRU", () => {
    let slots: [3]LfuSlot = std.mem.zeroes([3]LfuSlot);
    let cache = LfuCache.init(slots[0..]) ?? unreachable;
    expect_eq(cache.capacity(), 3 as usize);
    expect_eq(cache.len(), 0 as usize);

    expect_eq(cache.put(10, 100), true); // freq=1, touch=1
    expect_eq(cache.put(20, 200), true); // freq=1, touch=2
    expect_eq(cache.put(30, 300), true); // freq=1, touch=3
    expect_eq(cache.len(), 3 as usize);

    // Boost key 10 to freq=3, key 20 to freq=2; key 30 stays freq=1
    expect_eq(cache.get(10) ?? unreachable, 100 as u64);
    expect_eq(cache.get(10) ?? unreachable, 100 as u64);
    expect_eq(cache.get(20) ?? unreachable, 200 as u64);
    expect_eq(cache.frequencyOf(10) ?? unreachable, 3 as u64);
    expect_eq(cache.frequencyOf(20) ?? unreachable, 2 as u64);
    expect_eq(cache.frequencyOf(30) ?? unreachable, 1 as u64);

    // peek must not bump frequency
    expect_eq(cache.peek(30) ?? unreachable, 300 as u64);
    expect_eq(cache.frequencyOf(30) ?? unreachable, 1 as u64);

    // Insert key 40 -> must evict key 30 (lowest freq=1)
    expect_eq(cache.put(40, 400), true);
    expect_eq(cache.peek(30) == null, true);
    expect_eq(cache.peek(40) ?? unreachable, 400 as u64);

    // Boost key 40 to freq=2 (touch > key 20's touch). Now key 20 (freq=2, older) vs key 40 (freq=2, newer)
    expect_eq(cache.get(40) ?? unreachable, 400 as u64);
    expect_eq(cache.frequencyOf(20) ?? unreachable, 2 as u64);
    expect_eq(cache.frequencyOf(40) ?? unreachable, 2 as u64);

    // Insert key 50 -> must evict key 20 (same freq=2, older last_touch -> LRU tie-break)
    expect_eq(cache.put(50, 500), true);
    expect_eq(cache.peek(20) == null, true);
    expect_eq(cache.peek(10) ?? unreachable, 100 as u64);
    expect_eq(cache.peek(40) ?? unreachable, 400 as u64);
    expect_eq(cache.peek(50) ?? unreachable, 500 as u64);
});

test("2. LfuCache updating existing key does not evict and increments frequency", () => {
    let slots: [2]LfuSlot = std.mem.zeroes([2]LfuSlot);
    let cache = LfuCache.init(slots[0..]) ?? unreachable;

    expect_eq(cache.put(1, 11), true);
    expect_eq(cache.put(2, 22), true);
    expect_eq(cache.len(), 2 as usize);

    // Update key 1 in-place -> freq becomes 2, key 2 must remain intact
    expect_eq(cache.put(1, 111), true);
    expect_eq(cache.len(), 2 as usize);
    expect_eq(cache.peek(1) ?? unreachable, 111 as u64);
    expect_eq(cache.peek(2) ?? unreachable, 22 as u64);
    expect_eq(cache.frequencyOf(1) ?? unreachable, 2 as u64);
    expect_eq(cache.frequencyOf(2) ?? unreachable, 1 as u64);
});

test("3. LfuCache remove & clear release slots for reuse", () => {
    let slots: [2]LfuSlot = std.mem.zeroes([2]LfuSlot);
    let cache = LfuCache.init(slots[0..]) ?? unreachable;

    expect_eq(cache.put(100, 1), true);
    expect_eq(cache.put(200, 2), true);
    expect_eq(cache.remove(999), false);
    expect_eq(cache.remove(100), true);
    expect_eq(cache.len(), 1 as usize);
    expect_eq(cache.peek(100) == null, true);
    expect_eq(cache.frequencyOf(100) == null, true);

    // Reusing freed slot does not evict key 200
    expect_eq(cache.put(300, 3), true);
    expect_eq(cache.len(), 2 as usize);
    expect_eq(cache.peek(200) ?? unreachable, 2 as u64);
    expect_eq(cache.peek(300) ?? unreachable, 3 as u64);

    cache.clear();
    expect_eq(cache.len(), 0 as usize);
    expect_eq(cache.peek(200) == null, true);
    expect_eq(cache.peek(300) == null, true);
});

test("4. TinyLfuSketch hot-key estimation, 4-bit saturation (15), periodic halving & shouldAdmit", () => {
    expect_eq(TINYLFU_ROWS, 4 as usize);
    expect_eq(TINYLFU_MAX_COUNT, 15 as u8);

    // 64 bytes = 128 4-bit counters (32 counters per row)
    let table: [64]u8 = std.mem.zeroes([64]u8);
    let sketch = TinyLfuSketch.init(table[0..], 25) ?? unreachable;

    const hot_key: u64 = 0xCAFEBABE12345678;
    const cold_key: u64 = 0x0000000000000042;
    const mid_key: u64 = 0xDEADBEEF00000007;

    expect_eq(sketch.estimate(hot_key), 0 as u8);
    expect_eq(sketch.estimate(cold_key), 0 as u8);

    // Increment hot_key 20 times -> must saturate at 15 (before threshold 25)
    let i: usize = 0;
    while (i < 20) {
        * = sketch.increment(hot_key);
        i += 1;
    }
    expect_eq(sketch.estimate(hot_key), 15 as u8);

    // Increment mid_key 3 times, cold_key 1 time -> total 24 increments (< 25)
    * = sketch.increment(mid_key);
    * = sketch.increment(mid_key);
    * = sketch.increment(mid_key);
    * = sketch.increment(cold_key);

    expect_eq(sketch.estimate(hot_key), 15 as u8);
    expect_eq(sketch.estimate(mid_key) >= 3, true);
    expect_eq(sketch.shouldAdmit(hot_key, cold_key), true);
    expect_eq(sketch.shouldAdmit(cold_key, hot_key), false);
    expect_eq(sketch.shouldAdmit(mid_key, cold_key), true);

    // 25th increment hits reset_threshold -> triggers automatic halve()!
    // hot_key: 15 >> 1 = 7
    * = sketch.increment(cold_key);
    expect_eq(sketch.estimate(hot_key), 7 as u8);

    // Manual halve: 7 >> 1 = 3
    sketch.halve();
    expect_eq(sketch.estimate(hot_key), 3 as u8);

    // Clear resets everything to 0
    sketch.clear();
    expect_eq(sketch.estimate(hot_key), 0 as u8);
    expect_eq(sketch.estimate(mid_key), 0 as u8);
});

test("5. Invalid initialization rejection (empty slices, non-power-of-two sketch, zero threshold)", () => {
    let empty_slots: [0]LfuSlot = std.mem.zeroes([0]LfuSlot);
    expect_eq(LfuCache.init(empty_slots[0..]) == null, true);

    let too_small: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(TinyLfuSketch.init(too_small[0..], 16) == null, true);

    let non_pow2: [12]u8 = std.mem.zeroes([12]u8);
    expect_eq(TinyLfuSketch.init(non_pow2[0..], 16) == null, true);

    let valid_buf: [16]u8 = std.mem.zeroes([16]u8);
    expect_eq(TinyLfuSketch.init(valid_buf[0..], 0) == null, true);
    expect_eq(TinyLfuSketch.init(valid_buf[0..], 16) != null, true);
});
