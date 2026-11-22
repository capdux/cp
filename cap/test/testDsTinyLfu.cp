// Origin: Pure-CP Stdlib Test Suite (lib/ds/tinyLfu.cp) | Created: 2026-11-22
// Production Deep Verification Suite for W-TinyLFU Frequency Sketch & SLRU Cache Engine:
//   1. TinyLfuSketch 4-bit saturation (max 15), SWAR resetHalve(), & automatic periodic aging
//   2. Three-segment SLRU lifecycle: Window -> Probation -> Protected promotion & demotion
//   3. Anti-scan pollution defense: 100 one-hit cold keys rejected while hot working set survives 100%
//   4. Temporal aging & hotspot turnover: newly frequent key defeats aged victim into Main
//   5. Overwrite semantics (updated_existing) & side-effect-free contains()
//   6. remove() across all three segments (window / probation / protected) & slot reuse
//   7. Invalid config guards (0 window/probation, >256 cap, 0 threshold) & protected_cap=0 mode
//   8. hit_rate_bps telemetry, clear() full reset, & 500-op Zipf-like mixed workload invariants

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TINYLFU_MAX_COUNT,
    TINYLFU_MAX_CAPACITY,
    TinyLfuSketch,
    TinyLfuSegment,
    TinyLfuPutOutcome,
    TinyLfuConfig,
    TinyLfuStats,
    WTinyLfuCache,
} from "cap:ds";

test("tinyLfu: 4-bit Count-Min Sketch saturation, SWAR resetHalve & automatic periodic decay", () => {
    expect_eq(TinyLfuSketch.init(0) == null, true);

    let sk = TinyLfuSketch.init(1000) ?? unreachable;
    expect_eq(sk.estimate(12345), 0 as u8);

    // Increment 25 times -> saturates cleanly at 15 (4-bit max) without wrapping
    let i: usize = 0;
    while (i < 25) : (i += 1) {
        sk.increment(12345);
    }
    expect_eq(sk.estimate(12345), TINYLFU_MAX_COUNT);

    // Manual SWAR halving: 15 -> 7 -> 3 -> 1 -> 0
    sk.resetHalve();
    expect_eq(sk.estimate(12345), 7 as u8);
    sk.resetHalve();
    expect_eq(sk.estimate(12345), 3 as u8);
    sk.resetHalve();
    expect_eq(sk.estimate(12345), 1 as u8);
    sk.resetHalve();
    expect_eq(sk.estimate(12345), 0 as u8);
    expect_eq(sk.halve_count, 4 as u64);

    // Automatic halving when sample_size hits reset_threshold=10
    let auto_sk = TinyLfuSketch.init(10) ?? unreachable;
    let k: u64 = 0;
    while (k < 10) : (k += 1) {
        auto_sk.increment(999);
    }
    expect_eq(auto_sk.halve_count, 1 as u64);
    expect_eq(auto_sk.estimate(999), 5 as u8); // 10 >> 1 == 5
});

test("tinyLfu: three-segment SLRU Window -> Probation -> Protected promotion & demotion", () => {
    const cfg = TinyLfuConfig {
        window_cap: 2,
        probation_cap: 2,
        protected_cap: 2,
        sketch_reset_threshold: 500,
    };
    let cache = WTinyLfuCache.init(cfg) ?? unreachable;

    // Insert 2 keys -> fill Window
    expect_eq(cache.put(1, 100), TinyLfuPutOutcome.inserted_window);
    expect_eq(cache.put(2, 200), TinyLfuPutOutcome.inserted_window);
    expect_eq(cache.segmentOf(1), TinyLfuSegment.window);
    expect_eq(cache.segmentOf(2), TinyLfuSegment.window);

    // Insert key 3 -> key 1 overflows from Window into Probation
    expect_eq(cache.put(3, 300), TinyLfuPutOutcome.admitted_main);
    expect_eq(cache.segmentOf(1), TinyLfuSegment.probation);
    expect_eq(cache.segmentOf(3), TinyLfuSegment.window);

    // Insert key 4 -> key 2 overflows into Probation; insert key 5 -> key 3 overflows into Probation
    expect_eq(cache.put(4, 400), TinyLfuPutOutcome.admitted_main);
    expect_eq(cache.put(5, 500), TinyLfuPutOutcome.admitted_main);
    expect_eq(cache.segmentOf(2), TinyLfuSegment.probation);
    expect_eq(cache.segmentOf(3), TinyLfuSegment.probation);

    // Access key 1 & key 2 in Probation -> promote both to Protected (Protected now full: cap=2)
    expect_eq(cache.get(1) ?? 0, 100 as u64);
    expect_eq(cache.get(2) ?? 0, 200 as u64);
    expect_eq(cache.segmentOf(1), TinyLfuSegment.protected);
    expect_eq(cache.segmentOf(2), TinyLfuSegment.protected);
    expect_eq(cache.stats().promotions, 2 as u64);
    expect_eq(cache.stats().demotions, 0 as u64);

    // Access key 3 in Probation -> promotes to Protected, demoting LRU key 1 back to Probation
    expect_eq(cache.get(3) ?? 0, 300 as u64);
    expect_eq(cache.segmentOf(3), TinyLfuSegment.protected);
    expect_eq(cache.segmentOf(2), TinyLfuSegment.protected);
    expect_eq(cache.segmentOf(1), TinyLfuSegment.probation);
    expect_eq(cache.stats().promotions, 3 as u64);
    expect_eq(cache.stats().demotions, 1 as u64);
});

test("tinyLfu: anti-scan pollution defense protects hot working set from 100 one-hit cold keys", () => {
    const cfg = TinyLfuConfig {
        window_cap: 2,
        probation_cap: 4,
        protected_cap: 4,
        sketch_reset_threshold: 2000,
    };
    let cache = WTinyLfuCache.init(cfg) ?? unreachable;

    // Seed 8 hot keys (keys 1..8) and warm them heavily in sketch & Main segments
    let h: u64 = 1;
    while (h <= 10) : (h += 1) {
        * = cache.put(h, h * 10);
    }
    let round: usize = 0;
    while (round < 8) : (round += 1) {
        let k: u64 = 1;
        while (k <= 8) : (k += 1) {
            expect_eq(cache.get(k) ?? 0, k * 10);
        }
    }

    // Stream 100 sequential one-hit-wonder cold keys (keys 1000..1099)
    let cold: u64 = 1000;
    while (cold < 1100) : (cold += 1) {
        * = cache.put(cold, cold);
    }

    // All 8 hot keys must remain intact in Main despite 100-key scan!
    let k: u64 = 1;
    while (k <= 8) : (k += 1) {
        expect_eq(cache.contains(k), true);
    }
    const st = cache.stats();
    expect_eq(st.len, 10 as usize);
    expect_eq(st.rejected_count >= 90, true);
});

test("tinyLfu: temporal aging allows newly frequent key to overtake stale victim", () => {
    const cfg = TinyLfuConfig {
        window_cap: 1,
        probation_cap: 2,
        protected_cap: 0,
        sketch_reset_threshold: 20,
    };
    let cache = WTinyLfuCache.init(cfg) ?? unreachable;

    // Fill cache with keys 1, 2, 3 and boost key 1 initially
    * = cache.put(1, 10);
    * = cache.put(2, 20);
    * = cache.put(3, 30);
    * = cache.get(1);
    * = cache.get(1);
    * = cache.get(1);

    // Pre-warm a new key 99 via misses/reads across multiple halving cycles so old key 1 ages down
    let t: usize = 0;
    while (t < 24) : (t += 1) {
        * = cache.get(99);
    }
    expect_eq(cache.stats().sketch_halves >= 1, true);

    // Now insert key 99 into Window, then push another key 100 so 99 enters the duel against Probation victim
    * = cache.put(99, 990);
    const out = cache.put(100, 1000);
    expect_eq(out, TinyLfuPutOutcome.admitted_main);
    expect_eq(cache.contains(99), true);
    expect_eq(cache.segmentOf(99), TinyLfuSegment.probation);
});

test("tinyLfu: overwrite semantics (updated_existing) & side-effect-free contains()", () => {
    const cfg = TinyLfuConfig {
        window_cap: 2,
        probation_cap: 2,
        protected_cap: 2,
        sketch_reset_threshold: 500,
    };
    let cache = WTinyLfuCache.init(cfg) ?? unreachable;

    expect_eq(cache.put(42, 111), TinyLfuPutOutcome.inserted_window);
    expect_eq(cache.len(), 1 as usize);

    // contains() must NOT alter frequency sketch or hit/miss stats
    const freq_before: u8 = cache.estimateFreq(42);
    expect_eq(cache.contains(42), true);
    expect_eq(cache.contains(999), false);
    expect_eq(cache.estimateFreq(42), freq_before);
    expect_eq(cache.stats().hits, 0 as u64);
    expect_eq(cache.stats().misses, 0 as u64);

    // Overwrite existing key updates value in-place without increasing len
    expect_eq(cache.put(42, 222), TinyLfuPutOutcome.updated_existing);
    expect_eq(cache.len(), 1 as usize);
    expect_eq(cache.get(42) ?? 0, 222 as u64);
});

test("tinyLfu: remove() across window, probation, protected segments & slot reuse", () => {
    const cfg = TinyLfuConfig {
        window_cap: 2,
        probation_cap: 2,
        protected_cap: 2,
        sketch_reset_threshold: 500,
    };
    let cache = WTinyLfuCache.init(cfg) ?? unreachable;

    // Place keys into all three segments:
    // put 1, 2, 3, 4 -> window=[4,3], probation=[2,1]; get(1) -> protected=[1], probation=[2]
    * = cache.put(1, 10);
    * = cache.put(2, 20);
    * = cache.put(3, 30);
    * = cache.put(4, 40);
    expect_eq(cache.get(1) ?? 0, 10 as u64);

    expect_eq(cache.segmentOf(1), TinyLfuSegment.protected);
    expect_eq(cache.segmentOf(2), TinyLfuSegment.probation);
    expect_eq(cache.segmentOf(4), TinyLfuSegment.window);
    expect_eq(cache.len(), 4 as usize);

    // Remove from each segment
    expect_eq(cache.remove(1), true);
    expect_eq(cache.remove(2), true);
    expect_eq(cache.remove(4), true);
    expect_eq(cache.remove(9999), false);

    expect_eq(cache.len(), 1 as usize);
    expect_eq(cache.contains(3), true);
    expect_eq(cache.segmentOf(1), TinyLfuSegment.free);

    // Freed slots can be reused cleanly up to full capacity 6
    * = cache.put(10, 100);
    * = cache.put(11, 110);
    * = cache.put(12, 120);
    * = cache.put(13, 130);
    * = cache.put(14, 140);
    expect_eq(cache.len(), 6 as usize);
});

test("tinyLfu: invalid config guards & protected_cap=0 degraded mode", () => {
    // window_cap == 0
    expect_eq(WTinyLfuCache.init(TinyLfuConfig { window_cap: 0, probation_cap: 4, protected_cap: 4, sketch_reset_threshold: 100 }) == null, true);
    // probation_cap == 0
    expect_eq(WTinyLfuCache.init(TinyLfuConfig { window_cap: 2, probation_cap: 0, protected_cap: 4, sketch_reset_threshold: 100 }) == null, true);
    // totalCapacity > TINYLFU_MAX_CAPACITY (256)
    expect_eq(WTinyLfuCache.init(TinyLfuConfig { window_cap: 100, probation_cap: 100, protected_cap: 100, sketch_reset_threshold: 100 }) == null, true);
    // sketch_reset_threshold == 0
    expect_eq(WTinyLfuCache.init(TinyLfuConfig { window_cap: 2, probation_cap: 4, protected_cap: 4, sketch_reset_threshold: 0 }) == null, true);

    // protected_cap == 0 is valid (Window + Probation only)
    let cache = WTinyLfuCache.init(TinyLfuConfig {
        window_cap: 2,
        probation_cap: 2,
        protected_cap: 0,
        sketch_reset_threshold: 100,
    }) ?? unreachable;
    * = cache.put(1, 10);
    * = cache.put(2, 20);
    * = cache.put(3, 30);
    expect_eq(cache.get(1) ?? 0, 10 as u64);
    expect_eq(cache.segmentOf(1), TinyLfuSegment.probation);
    expect_eq(cache.stats().protected_len, 0 as usize);
});

test("tinyLfu: hit_rate_bps telemetry, clear() reset & 500-op mixed workload invariants", () => {
    const cfg = TinyLfuConfig {
        window_cap: 4,
        probation_cap: 8,
        protected_cap: 12,
        sketch_reset_threshold: 240,
    };
    let cache = WTinyLfuCache.init(cfg) ?? unreachable;

    // 500-op mixed workload: 70% hot keys (1..16), 30% cold tail keys
    let step: u64 = 0;
    while (step < 500) : (step += 1) {
        const key: u64 = if ((step % 10) < 7) ((step % 16) + 1) else (10000 + step);
        if (cache.get(key) == null) {
            * = cache.put(key, key * 3);
        }
    }

    const st = cache.stats();
    expect_eq(st.len <= cfg.totalCapacity(), true);
    expect_eq(st.window_len + st.probation_len + st.protected_len, st.len);
    expect_eq(st.window_len <= cfg.window_cap, true);
    expect_eq(st.protected_len <= cfg.protected_cap, true);
    expect_eq(st.hits + st.misses, 500 as u64);
    // Hot working set (16 keys) easily fits in cap 24 -> hit rate should exceed 55% (5500 bps)
    expect_eq(st.hit_rate_bps >= 5500, true);

    // clear() resets everything cleanly
    cache.clear();
    const after = cache.stats();
    expect_eq(after.len, 0 as usize);
    expect_eq(after.hits, 0 as u64);
    expect_eq(after.misses, 0 as u64);
    expect_eq(after.hit_rate_bps, 0 as u32);
    expect_eq(after.sketch_halves, 0 as u64);
    expect_eq(cache.estimateFreq(1), 0 as u8);
});
