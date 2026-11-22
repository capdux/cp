// Origin: Pure-CP Stdlib Test Suite (lib/ds/heavy_hitter.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Space-Saving Heavy-Hitter Tracker & Conservative CMS:
//   1. Space-Saving Zipf skewed stream: exact Top-K heavy-hitter capture & mathematical error bounds
//   2. Space-Saving deterministic tie-breaking, lookup, guaranteedMinCount & reset
//   3. Conservative-Update CMS: low overestimation on skewed heavy-tailed streams
//   4. Conservative-Update CMS distributed shard mergeFrom & saturating arithmetic
//   5. Sliding-window exponential decayHalf & reset lifecycle
//   6. Comprehensive boundary & invalid input guards (zero weight, empty slots, dim/seed mismatch, undersized buffer)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HeavyHitterEntry,
    HeavyHitterTracker,
    CmsSketchConfig,
    ConservativeCms,
} from "cap:ds";

// ============================================================================
// Test 1: Space-Saving Zipf Skewed Stream & Error Bound Invariants
// ============================================================================

fn testSpaceSavingZipfAndErrorBounds()! {
    let slots: [8]HeavyHitterEntry = [HeavyHitterEntry{}; 8];
    let slots_s: []HeavyHitterEntry = slots[0..];
    let tracker = cap.ds.HeavyHitterTracker.init(slots_s) ?? return error.InitFailed;

    expect_eq(tracker.capacity(), 8);
    expect_eq(tracker.len(), 0);
    expect_eq(tracker.totalWeight(), 0);

    // Feed 3 dominant heavy hitters + 24 low-frequency tail keys (Zipf-like skew)
    const heavy_keys: [3]string = ["hot:gpt4", "hot:embed", "hot:rag"];
    const heavy_freqs: [3]u64 = [600, 350, 180];

    const tail_keys: [24]string = [
        "tail:00", "tail:01", "tail:02", "tail:03", "tail:04", "tail:05",
        "tail:06", "tail:07", "tail:08", "tail:09", "tail:10", "tail:11",
        "tail:12", "tail:13", "tail:14", "tail:15", "tail:16", "tail:17",
        "tail:18", "tail:19", "tail:20", "tail:21", "tail:22", "tail:23",
    ];

    // Interleave heavy and tail observations across rounds to trigger Space-Saving evictions
    let round: usize = 0;
    while (round < 24) : (round += 1) {
        if (round < 12) {
            expect_eq(tracker.observe(heavy_keys[0], 50), true); // 12 * 50 = 600
        }
        if (round < 10) {
            expect_eq(tracker.observe(heavy_keys[1], 35), true); // 10 * 35 = 350
        }
        if (round < 9) {
            expect_eq(tracker.observe(heavy_keys[2], 20), true); // 9 * 20 = 180
        }
        // Each tail key appears with small weight 3 (24 * 3 = 72 total tail weight)
        expect_eq(tracker.observe(tail_keys[round], 3), true);
    }

    const expected_total: u64 = 600 + 350 + 180 + 72;
    expect_eq(tracker.totalWeight(), expected_total);
    expect_eq(tracker.len(), 8);
    expect_eq(tracker.evictionCount() > 0, true);

    // Extract Top-5 and verify all 3 heavy hitters are ranked #1, #2, #3 accurately
    let top: [5]HeavyHitterEntry = [HeavyHitterEntry{}; 5];
    let top_s: []HeavyHitterEntry = top[0..];
    const n_top = tracker.topK(top_s);
    expect_eq(n_top, 5);

    expect_eq(std.mem.eql(u8, top[0].label, "hot:gpt4"), true);
    expect_eq(std.mem.eql(u8, top[1].label, "hot:embed"), true);
    expect_eq(std.mem.eql(u8, top[2].label, "hot:rag"), true);

    // Space-Saving mathematical invariant:
    //   est_count - error_bound <= true_freq <= est_count
    //   error_bound <= floor(total_weight / K)
    const max_theoretical_err: u64 = expected_total / 8;
    let i: usize = 0;
    while (i < 3) : (i += 1) {
        const entry = top[i];
        const true_f = heavy_freqs[i];
        expect_eq(entry.est_count >= true_f, true);
        expect_eq(entry.est_count - entry.error_bound <= true_f, true);
        expect_eq(entry.error_bound <= max_theoretical_err, true);
    }

    // Monotonic descending order of est_count
    let r: usize = 1;
    while (r < n_top) : (r += 1) {
        expect_eq(top[r - 1].est_count >= top[r].est_count, true);
    }
}

// ============================================================================
// Test 2: Space-Saving Tie-Breaking, Lookup, Guaranteed Lower Bound & Reset
// ============================================================================

fn testSpaceSavingTieBreakLookupAndReset()! {
    let slots: [4]HeavyHitterEntry = [HeavyHitterEntry{}; 4];
    let slots_s: []HeavyHitterEntry = slots[0..];
    let tracker = HeavyHitterTracker.init(slots_s) ?? return error.InitFailed;

    expect_eq(tracker.observe("alpha", 100), true);
    expect_eq(tracker.observe("beta", 100), true);
    expect_eq(tracker.observe("gamma", 100), true);

    let out: [4]HeavyHitterEntry = [HeavyHitterEntry{}; 4];
    let out_s: []HeavyHitterEntry = out[0..];
    const n = tracker.topK(out_s);
    expect_eq(n, 3);
    // Equal est_count must tie-break by ascending key_hash
    expect_eq(out[0].est_count, 100);
    expect_eq(out[1].est_count, 100);
    expect_eq(out[2].est_count, 100);
    expect_eq(out[0].key_hash <= out[1].key_hash, true);
    expect_eq(out[1].key_hash <= out[2].key_hash, true);

    // Lookup and guaranteedMinCount (error_bound == 0 before eviction)
    const alpha_e = tracker.lookup("alpha") ?? return error.MissingKey;
    expect_eq(alpha_e.est_count, 100);
    expect_eq(alpha_e.error_bound, 0);
    expect_eq(tracker.guaranteedMinCount("alpha"), 100);
    expect_eq(tracker.lookup("nonexistent") == null, true);
    expect_eq(tracker.guaranteedMinCount("nonexistent"), 0);

    // Reset clears everything cleanly
    tracker.reset();
    expect_eq(tracker.len(), 0);
    expect_eq(tracker.totalWeight(), 0);
    expect_eq(tracker.evictionCount(), 0);
    expect_eq(tracker.topK(out_s), 0);
}

// ============================================================================
// Test 3: Conservative-Update CMS Accuracy on Skewed Stream
// ============================================================================

fn testConservativeCmsAccuracy()! {
    const cfg = CmsSketchConfig.init(4, 128, 0x123456789ABCDEF0) ?? return error.BadCfg;
    let buf: [512]u32 = [0; 512];
    let buf_s: []u32 = buf[0..];
    let cms = ConservativeCms.init(buf_s, cfg) ?? return error.InitFailed;

    expect_eq(cms.rowCount(), 4);
    expect_eq(cms.colCount(), 128);
    expect_eq(cms.totalCount(), 0);

    // Heavy keys
    expect_eq(cms.add("user:vip_1", 1000), true);
    expect_eq(cms.add("user:vip_2", 500), true);
    expect_eq(cms.add("user:vip_1", 250), true); // total 1250

    // Background noise (30 distinct keys with small count 1..2)
    const noise: [20]string = [
        "n00", "n01", "n02", "n03", "n04", "n05", "n06", "n07", "n08", "n09",
        "n10", "n11", "n12", "n13", "n14", "n15", "n16", "n17", "n18", "n19",
    ];
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        expect_eq(cms.add(noise[i], 2), true);
    }

    expect_eq(cms.totalCount(), 1250 + 500 + 40);

    // Point estimates must never underestimate true count, and overestimation must stay tiny
    const est_vip1 = cms.estimate("user:vip_1");
    const est_vip2 = cms.estimate("user:vip_2");
    expect_eq(est_vip1 >= 1250, true);
    expect_eq(est_vip1 <= 1260, true);
    expect_eq(est_vip2 >= 500, true);
    expect_eq(est_vip2 <= 510, true);

    // Unseen key should have very small (typically 0) estimate
    const est_unseen = cms.estimate("never_seen_key_xyz");
    expect_eq(est_unseen <= 10, true);
}

// ============================================================================
// Test 4: Conservative CMS Distributed Shard Merging (mergeFrom)
// ============================================================================

fn testConservativeCmsShardMerge()! {
    const cfg = CmsSketchConfig.init(4, 256, 0xCAFEBABE) ?? return error.BadCfg;
    let buf_a: [1024]u32 = [0; 1024];
    let buf_b: [1024]u32 = [0; 1024];
    let buf_a_s: []u32 = buf_a[0..];
    let buf_b_s: []u32 = buf_b[0..];

    let shard_a = ConservativeCms.init(buf_a_s, cfg) ?? return error.InitFailed;
    let shard_b = ConservativeCms.init(buf_b_s, cfg) ?? return error.InitFailed;

    expect_eq(shard_a.add("model:llama3", 120), true);
    expect_eq(shard_a.add("model:qwen2", 45), true);

    expect_eq(shard_b.add("model:llama3", 80), true);
    expect_eq(shard_b.add("model:deepseek", 90), true);

    // Merge shard_b into shard_a
    expect_eq(shard_a.mergeFrom(shard_b), true);
    expect_eq(shard_a.totalCount(), 120 + 45 + 80 + 90);

    // Merged estimates must reflect combined frequencies
    expect_eq(shard_a.estimate("model:llama3") >= 200, true);
    expect_eq(shard_a.estimate("model:llama3") <= 205, true);
    expect_eq(shard_a.estimate("model:qwen2") >= 45, true);
    expect_eq(shard_a.estimate("model:deepseek") >= 90, true);
}

// ============================================================================
// Test 5: Sliding-Window Decay (decayHalf) & Reset
// ============================================================================

fn testConservativeCmsDecayAndReset()! {
    const cfg = CmsSketchConfig.init(4, 64) ?? return error.BadCfg;
    let buf: [256]u32 = [0; 256];
    let buf_s: []u32 = buf[0..];
    let cms = ConservativeCms.init(buf_s, cfg) ?? return error.InitFailed;

    expect_eq(cms.add("burst:key", 400), true);
    expect_eq(cms.estimate("burst:key"), 400);
    expect_eq(cms.totalCount(), 400);

    cms.decayHalf();
    expect_eq(cms.estimate("burst:key"), 200);
    expect_eq(cms.totalCount(), 200);

    cms.decayHalf();
    expect_eq(cms.estimate("burst:key"), 100);
    expect_eq(cms.totalCount(), 100);

    cms.reset();
    expect_eq(cms.estimate("burst:key"), 0);
    expect_eq(cms.totalCount(), 0);
}

// ============================================================================
// Test 6: Boundary & Invalid Input Guards
// ============================================================================

fn testBoundaryAndInvalidInputGuards()! {
    // 1. HeavyHitterTracker guards
    let slots: [2]HeavyHitterEntry = [HeavyHitterEntry{}; 2];
    let empty_slots: []HeavyHitterEntry = slots[0..0];
    let slots_s: []HeavyHitterEntry = slots[0..];
    expect_eq(HeavyHitterTracker.init(empty_slots) == null, true);

    let tracker = HeavyHitterTracker.init(slots_s) ?? return error.InitFailed;
    // Zero weight rejected without mutating state
    expect_eq(tracker.observe("zero_key", 0), false);
    expect_eq(tracker.len(), 0);
    expect_eq(tracker.totalWeight(), 0);

    // Empty output slice for topK returns 0 safely
    let out_buf: [2]HeavyHitterEntry = [HeavyHitterEntry{}; 2];
    let empty_out: []HeavyHitterEntry = out_buf[0..0];
    expect_eq(tracker.observe("k1", 5), true);
    expect_eq(tracker.topK(empty_out), 0);

    // 2. CmsSketchConfig guards
    expect_eq(CmsSketchConfig.init(0, 64) == null, true); // depth == 0
    expect_eq(CmsSketchConfig.init(4, 0) == null, true);  // width == 0

    // 3. ConservativeCms buffer & delta guards
    const cfg4x16 = CmsSketchConfig.init(4, 16, 111) ?? return error.BadCfg;
    let small_buf: [60]u32 = [0; 60]; // needs 64
    let small_s: []u32 = small_buf[0..];
    expect_eq(ConservativeCms.init(small_s, cfg4x16) == null, true);

    let ok_buf: [64]u32 = [0; 64];
    let ok_s: []u32 = ok_buf[0..];
    let cms_a = ConservativeCms.init(ok_s, cfg4x16) ?? return error.InitFailed;
    expect_eq(cms_a.add("k", 0), false); // zero delta rejected
    expect_eq(cms_a.totalCount(), 0);

    // 4. mergeFrom incompatible dimension & seed guards
    const cfg_diff_width = CmsSketchConfig.init(4, 8, 111) ?? return error.BadCfg;
    const cfg_diff_seed = CmsSketchConfig.init(4, 16, 222) ?? return error.BadCfg;
    let buf_w: [32]u32 = [0; 32];
    let buf_seed: [64]u32 = [0; 64];
    let buf_w_s: []u32 = buf_w[0..];
    let buf_seed_s: []u32 = buf_seed[0..];
    const cms_w = ConservativeCms.init(buf_w_s, cfg_diff_width) ?? return error.InitFailed;
    const cms_seed = ConservativeCms.init(buf_seed_s, cfg_diff_seed) ?? return error.InitFailed;

    expect_eq(cms_a.mergeFrom(cms_w), false);    // width mismatch
    expect_eq(cms_a.mergeFrom(cms_seed), false); // seed mismatch
}

test("ds heavy_hitter: Space-Saving Zipf skewed stream & error bound invariants", testSpaceSavingZipfAndErrorBounds);
test("ds heavy_hitter: Space-Saving tie-breaking, lookup, guaranteed lower bound & reset", testSpaceSavingTieBreakLookupAndReset);
test("ds heavy_hitter: Conservative-Update CMS accuracy on skewed stream", testConservativeCmsAccuracy);
test("ds heavy_hitter: Conservative-Update CMS distributed shard mergeFrom", testConservativeCmsShardMerge);
test("ds heavy_hitter: Conservative-Update CMS sliding-window decayHalf & reset", testConservativeCmsDecayAndReset);
test("ds heavy_hitter: boundary & invalid input guards", testBoundaryAndInvalidInputGuards);
