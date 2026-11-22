// Origin: Pure-CP Stdlib Test Suite (lib/db/lsm_compactor.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc LSM-Tree Leveled Compaction Planner:
//   1. Byte-order closed-interval key range overlap boundaries (disjoint, edge-touching, nested, prefix)
//   2. L0 file-count trigger with fixed-point L0 overlap cluster expansion & L1 fanout selection
//   3. L1 -> L2 size overflow picking largest SSTable & all overlapping L2 files
//   4. Zero-write-amplification trivial_move fast path when Ln+1 has zero key overlap
//   5. High-tombstone reclamation (tombstone_reclaim) when all levels are under size triggers
//   6. Concurrency conflict avoidance via being_compacted (skipping conflicting files/targets)
//   7. Invalid config, min>max keys, duplicate IDs, 128-slot capacity & bottom-level immobility guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_LSM_SST_FILES,
    compareKeyBytesLex,
    LsmCompactionReason,
    LsmCompactionConfig,
    SstFileMeta,
    CompactionPlan,
    LsmLevelStats,
    LsmCompactionPlanner,
} from "cap:db";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: Key-Range Lexicographical Overlap Boundaries
// ============================================================================

fn testKeyRangeOverlapBoundaries()! {
    expect_eq(compareKeyBytesLex("a", "b") < 0, true);
    expect_eq(compareKeyBytesLex("ab", "ab"), 0);
    expect_eq(compareKeyBytesLex("ab", "a") > 0, true);

    const f = SstFileMeta.init(1, 1, 100, "d", "k", 0) ?? return error.BadMeta;
    expect_eq(std.mem.eql(u8, f.minKeySlice(), "d"), true);
    expect_eq(std.mem.eql(u8, f.maxKeySlice(), "k"), true);

    // Disjoint left & right
    expect_eq(f.overlapsRange("a", "c"), false);
    expect_eq(f.overlapsRange("l", "z"), false);
    // Exact boundary touch (closed interval -> overlaps!)
    expect_eq(f.overlapsRange("a", "d"), true);
    expect_eq(f.overlapsRange("k", "z"), true);
    // Partial & nested overlap
    expect_eq(f.overlapsRange("e", "g"), true);
    expect_eq(f.overlapsRange("a", "z"), true);
}

// ============================================================================
// Test 2: L0 File-Count Trigger & Overlap Cluster Expansion + L1 Fanout
// ============================================================================

fn testL0ClusterExpansionAndL1Fanout()! {
    const cfg = LsmCompactionConfig{
        max_levels: 4,
        l0_compaction_trigger: 3,
        base_level_bytes: 10000,
        level_multiplier: 10,
        tombstone_trigger_bps: 4000,
    };
    let planner = cap.db.LsmCompactionPlanner.init(cfg) ?? return error.InitFailed;

    // L0 cluster: f1 [a..f] overlaps f2 [e..k] -> combined [a..k]; f3 [t..z] is disjoint!
    expect_eq(planner.add_sst(SstFileMeta.init(1, 0, 200, "a", "f", 100) ?? return error.BadMeta), true);
    expect_eq(planner.add_sst(SstFileMeta.init(2, 0, 250, "e", "k", 100) ?? return error.BadMeta), true);
    expect_eq(planner.add_sst(SstFileMeta.init(3, 0, 150, "t", "z", 100) ?? return error.BadMeta), true);

    // L1 files: f10 [b..g] and f11 [j..m] overlap [a..k]; f12 [n..r] does NOT overlap!
    expect_eq(planner.add_sst(SstFileMeta.init(10, 1, 500, "b", "g", 0) ?? return error.BadMeta), true);
    expect_eq(planner.add_sst(SstFileMeta.init(11, 1, 600, "j", "m", 0) ?? return error.BadMeta), true);
    expect_eq(planner.add_sst(SstFileMeta.init(12, 1, 700, "n", "r", 0) ?? return error.BadMeta), true);

    const st0: LsmLevelStats = planner.level_stats(0);
    expect_eq(st0.file_count, 3);
    expect_eq(approxEqF32(st0.score_f32, 1.0, 0.001), true);

    const plan: CompactionPlan = planner.plan_next_compaction();
    expect_eq(plan.valid, true);
    expect_eq(plan.reason == LsmCompactionReason.l0_file_count, true);
    expect_eq(plan.source_level, 0);
    expect_eq(plan.target_level, 1);
    expect_eq(plan.is_trivial_move, false);
    expect_eq(plan.source_count, 2); // f1 + f2 (f3 excluded)
    expect_eq(plan.target_count, 2); // f10 + f11 (f12 excluded)
    expect_eq(plan.estimated_read_bytes, 200 + 250 + 500 + 600);
}

// ============================================================================
// Test 3: L1 -> L2 Size Overflow & Overlapping Fanout Selection
// ============================================================================

fn testL1ToL2SizeOverflowSelection()! {
    const cfg = LsmCompactionConfig{
        max_levels: 4,
        l0_compaction_trigger: 4,
        base_level_bytes: 1000, // L1 target = 1000, L2 target = 10000
        level_multiplier: 10,
        tombstone_trigger_bps: 5000,
    };
    expect_eq(cfg.levelTargetBytes(0), 0);
    expect_eq(cfg.levelTargetBytes(1), 1000);
    expect_eq(cfg.levelTargetBytes(2), 10000);
    expect_eq(cfg.levelTargetBytes(3), 100000);

    let planner = LsmCompactionPlanner.init(cfg) ?? return error.InitFailed;
    // L1 total = 1400 > 1000 (score = 1.4). Largest file is f20 (900 bytes, [e..m])
    * = planner.add_sst(SstFileMeta.init(19, 1, 500, "a", "d", 0) ?? return error.BadMeta);
    * = planner.add_sst(SstFileMeta.init(20, 1, 900, "e", "m", 0) ?? return error.BadMeta);

    // L2 files: f30 [c..f] & f31 [k..p] overlap [e..m]; f32 [r..z] does not
    * = planner.add_sst(SstFileMeta.init(30, 2, 1000, "c", "f", 0) ?? return error.BadMeta);
    * = planner.add_sst(SstFileMeta.init(31, 2, 1200, "k", "p", 0) ?? return error.BadMeta);
    * = planner.add_sst(SstFileMeta.init(32, 2, 1500, "r", "z", 0) ?? return error.BadMeta);

    const plan = planner.plan_next_compaction();
    expect_eq(plan.valid, true);
    expect_eq(plan.reason == LsmCompactionReason.level_size_overflow, true);
    expect_eq(plan.source_level, 1);
    expect_eq(plan.target_level, 2);
    expect_eq(plan.is_trivial_move, false);
    expect_eq(plan.source_count, 1);
    expect_eq(plan.source_file_ids[0], 20);
    expect_eq(plan.target_count, 2);
    expect_eq(plan.estimated_read_bytes, 900 + 1000 + 1200);
    expect_eq(approxEqF32(plan.priority_score_f32, 1.4, 0.01), true);
}

// ============================================================================
// Test 4: Zero-Write-Amplification Trivial Move Fast Path
// ============================================================================

fn testTrivialMoveFastPath()! {
    const cfg = LsmCompactionConfig{
        max_levels: 4,
        l0_compaction_trigger: 4,
        base_level_bytes: 1000,
        level_multiplier: 10,
        tombstone_trigger_bps: 5000,
    };
    let planner = LsmCompactionPlanner.init(cfg) ?? return error.InitFailed;

    // L1 overflows (1200 > 1000) with range [a..d], while L2 only has [m..z] -> zero overlap!
    * = planner.add_sst(SstFileMeta.init(101, 1, 1200, "a", "d", 100) ?? return error.BadMeta);
    * = planner.add_sst(SstFileMeta.init(201, 2, 2000, "m", "z", 0) ?? return error.BadMeta);

    const plan = planner.plan_next_compaction();
    expect_eq(plan.valid, true);
    expect_eq(plan.reason == LsmCompactionReason.trivial_move, true);
    expect_eq(plan.is_trivial_move, true);
    expect_eq(plan.source_level, 1);
    expect_eq(plan.target_level, 2);
    expect_eq(plan.source_count, 1);
    expect_eq(plan.source_file_ids[0], 101);
    expect_eq(plan.target_count, 0);
    expect_eq(plan.estimated_read_bytes, 0);
}

// ============================================================================
// Test 5: High-Tombstone Reclamation Trigger
// ============================================================================

fn testTombstoneReclaimTrigger()! {
    const cfg = LsmCompactionConfig{
        max_levels: 4,
        l0_compaction_trigger: 4,
        base_level_bytes: 10000,
        level_multiplier: 10,
        tombstone_trigger_bps: 3000, // 30% tombstones triggers reclaim
    };
    let planner = LsmCompactionPlanner.init(cfg) ?? return error.InitFailed;

    // All levels well under size/count thresholds, but f50 has 45% tombstones (4500 bps)
    * = planner.add_sst(SstFileMeta.init(49, 1, 400, "a", "c", 1000) ?? return error.BadMeta);
    * = planner.add_sst(SstFileMeta.init(50, 1, 300, "d", "f", 4500) ?? return error.BadMeta);
    * = planner.add_sst(SstFileMeta.init(51, 2, 500, "m", "z", 2000) ?? return error.BadMeta);

    const plan = planner.plan_next_compaction();
    expect_eq(plan.valid, true);
    expect_eq(plan.reason == LsmCompactionReason.tombstone_reclaim, true);
    expect_eq(plan.is_trivial_move, false); // tombstone reclaim must rewrite, never trivial move
    expect_eq(plan.source_file_ids[0], 50);
    expect_eq(plan.source_level, 1);
    expect_eq(plan.target_level, 2);
    expect_eq(plan.estimated_read_bytes, 300);
}

// ============================================================================
// Test 6: Concurrency Conflict Avoidance (being_compacted)
// ============================================================================

fn testBeingCompactedConflictAvoidance()! {
    const cfg = LsmCompactionConfig{
        max_levels: 4,
        l0_compaction_trigger: 4,
        base_level_bytes: 1000,
        level_multiplier: 10,
        tombstone_trigger_bps: 5000,
    };
    let planner = LsmCompactionPlanner.init(cfg) ?? return error.InitFailed;

    // L1 has two files: f1 (800B, [a..f]) and f2 (600B, [p..z]); total 1400 > 1000
    * = planner.add_sst(SstFileMeta.init(1, 1, 800, "a", "f", 0) ?? return error.BadMeta);
    * = planner.add_sst(SstFileMeta.init(2, 1, 600, "p", "z", 0) ?? return error.BadMeta);
    // L2 file f10 ([c..g]) overlaps f1 and is currently being_compacted!
    * = planner.add_sst(SstFileMeta.init(10, 2, 1000, "c", "g", 0) ?? return error.BadMeta);
    expect_eq(planner.mark_compacting(10, true), true);

    // Planner must skip f1 (due to overlapping L2 f10 being compacted) and pick f2 instead!
    const plan1 = planner.plan_next_compaction();
    expect_eq(plan1.valid, true);
    expect_eq(plan1.source_file_ids[0], 2);

    // Mark f2 as compacting too -> now no conflict-free candidate remains
    expect_eq(planner.mark_compacting(2, true), true);
    const plan2 = planner.plan_next_compaction();
    expect_eq(plan2.valid, false);
}

// ============================================================================
// Test 7: Invalid Config, Bottom-Level Immobility & 128-Slot Capacity Guards
// ============================================================================

fn testBoundaryAndCapacityGuards()! {
    // Invalid configs
    expect_eq(LsmCompactionPlanner.init(LsmCompactionConfig{ max_levels: 1 }) == null, true);
    expect_eq(LsmCompactionPlanner.init(LsmCompactionConfig{ max_levels: 8 }) == null, true);
    expect_eq(LsmCompactionPlanner.init(LsmCompactionConfig{ l0_compaction_trigger: 0 }) == null, true);
    expect_eq(LsmCompactionPlanner.init(LsmCompactionConfig{ base_level_bytes: 0 }) == null, true);
    expect_eq(LsmCompactionPlanner.init(LsmCompactionConfig{ level_multiplier: 1 }) == null, true);
    expect_eq(LsmCompactionPlanner.init(LsmCompactionConfig{ tombstone_trigger_bps: 0 }) == null, true);
    expect_eq(LsmCompactionPlanner.init(LsmCompactionConfig{ tombstone_trigger_bps: 10001 }) == null, true);

    // Invalid SstFileMeta (min > max, zero file_id, empty key)
    expect_eq(SstFileMeta.init(0, 0, 100, "a", "b", 0) == null, true);
    expect_eq(SstFileMeta.init(1, 0, 100, "z", "a", 0) == null, true);
    expect_eq(SstFileMeta.init(1, 0, 100, "", "a", 0) == null, true);

    let planner = LsmCompactionPlanner.init(LsmCompactionConfig{
        max_levels: 3, // levels 0, 1, 2 (2 is bottom level)
        l0_compaction_trigger: 4,
        base_level_bytes: 1000,
        level_multiplier: 10,
        tombstone_trigger_bps: 3000,
    }) ?? return error.InitFailed;

    // Out-of-range level rejected
    expect_eq(planner.add_sst(SstFileMeta.init(1, 3, 100, "a", "b", 0) ?? return error.BadMeta), false);

    // Bottom level (level 2) with huge size & 90% tombstones CANNOT compact downward
    expect_eq(planner.add_sst(SstFileMeta.init(1, 2, 999999, "a", "z", 9000) ?? return error.BadMeta), true);
    expect_eq(planner.plan_next_compaction().valid, false);

    // Duplicate file_id rejected
    expect_eq(planner.add_sst(SstFileMeta.init(1, 1, 100, "a", "b", 0) ?? return error.BadMeta), false);

    // Fill remaining 127 slots up to MAX_LSM_SST_FILES (128)
    let id: u64 = 2;
    while (id <= (MAX_LSM_SST_FILES as u64)) : (id += 1) {
        expect_eq(planner.add_sst(SstFileMeta.init(id, 2, 10, "a", "b", 0) ?? return error.BadMeta), true);
    }
    // 129th file rejected
    expect_eq(planner.add_sst(SstFileMeta.init(999, 2, 10, "a", "b", 0) ?? return error.BadMeta), false);

    // Remove frees slot for reuse
    expect_eq(planner.remove_sst(1), true);
    expect_eq(planner.remove_sst(1), false);
    expect_eq(planner.add_sst(SstFileMeta.init(999, 2, 10, "a", "b", 0) ?? return error.BadMeta), true);
}

test("db lsm_compactor: byte-order closed-interval key range overlap boundaries", testKeyRangeOverlapBoundaries);
test("db lsm_compactor: L0 file-count trigger, overlap cluster expansion & L1 fanout", testL0ClusterExpansionAndL1Fanout);
test("db lsm_compactor: L1 -> L2 size overflow & overlapping fanout selection", testL1ToL2SizeOverflowSelection);
test("db lsm_compactor: zero-write-amplification trivial_move fast path", testTrivialMoveFastPath);
test("db lsm_compactor: high-tombstone reclamation trigger", testTombstoneReclaimTrigger);
test("db lsm_compactor: being_compacted concurrency conflict avoidance", testBeingCompactedConflictAvoidance);
test("db lsm_compactor: invalid config, bottom-level immobility & 128-slot capacity guards", testBoundaryAndCapacityGuards);
