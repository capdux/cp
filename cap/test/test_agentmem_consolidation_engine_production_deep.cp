// Production Deep Test Suite for cap:agentmem Consolidation Engine (lib/agentmem/consolidation_engine.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    MEM_CONSOL_MAX_EPISODIC,
    MEM_CONSOL_MAX_DIM,
    MemConsolStatus,
    MemConsolPassConfig,
    computeEntryRetentionScore,
    AgentMemoryConsolidator,
} from "cap:agentmem";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const d: f32 = if (a >= b) (a - b) else (b - a);
    return d <= eps;
}

// ============================================================================
// Test 1: Ebbinghaus retention decay, access reinforcement & pinned immunity
// ============================================================================
test("consolidation_engine: Ebbinghaus retention decay, access reinforcement & pinned immunity", fn() {
    let eng = AgentMemoryConsolidator.init(4) ?? return;
    const v: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    expect_true(eng.addEpisodic(1, 0, 1, 0.40, 1000, false, v[0..]));
    expect_true(eng.addEpisodic(2, 0, 1, 0.40, 1000, false, v[0..]));
    expect_true(eng.addEpisodic(3, 0, 1, 0.05, 1000, true, v[0..])); // pinned

    // Reinforce memory 2 multiple times -> higher access_count strengthens stability
    let k: usize = 0;
    while (k < 6) : (k += 1) {
        expect_true(eng.touchEpisodic(2, 1000));
    }

    const e1 = eng.getEpisodic(1) ?? return;
    const e2 = eng.getEpisodic(2) ?? return;
    const e3 = eng.getEpisodic(3) ?? return;

    const half_life: u64 = 10_000;
    const s1_fresh = computeEntryRetentionScore(e1, 1000, half_life);
    const s1_decayed = computeEntryRetentionScore(e1, 31_000, half_life); // 3 half-lives later
    const s2_decayed = computeEntryRetentionScore(e2, 31_000, half_life);
    const s3_pinned = computeEntryRetentionScore(e3, 999_999_999, half_life);

    expect_true(s1_fresh > s1_decayed);
    expect_true(s2_decayed > s1_decayed); // reinforced memory decays slower
    expect_true(approxEqF32(s3_pinned, 1.0, 1.0e-6)); // pinned always 1.0
});

// ============================================================================
// Test 2: Near-duplicate episodic clustering, @fma merge & semantic promotion
// ============================================================================
test("consolidation_engine: near-duplicate episodic clustering, FMA merge & semantic promotion", fn() {
    let eng = AgentMemoryConsolidator.init(4) ?? return;

    // Cluster A: 2 near-duplicate episodic memories near X-axis (cos > 0.98)
    const va1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const va2: [4]f32 = [0.98, 0.20, 0.0, 0.0];
    // Isolated episodic memory along Z-axis (should NOT merge into Cluster A)
    const vz: [4]f32 = [0.0, 0.0, 1.0, 0.0];

    expect_true(eng.addEpisodic(10, 0, 7, 0.60, 1000, false, va1[0..]));
    expect_true(eng.addEpisodic(11, 0, 7, 0.80, 2000, false, va2[0..]));
    expect_true(eng.addEpisodic(12, 0, 9, 0.80, 2000, false, vz[0..]));

    const rep = eng.runConsolidationPass(3000, MemConsolPassConfig.defaults());
    expect_eq(rep.clusters_merged, 1 as usize);
    expect_eq(rep.episodic_merged_count, 2 as usize);
    expect_eq(rep.semantic_promoted_count, 1 as usize);

    // Check episodic 10 & 11 marked merged and linked to the same promoted semantic ID
    const e10 = eng.getEpisodic(10) ?? return;
    const e11 = eng.getEpisodic(11) ?? return;
    const e12 = eng.getEpisodic(12) ?? return;
    expect_eq(e10.status, MemConsolStatus.merged);
    expect_eq(e11.status, MemConsolStatus.merged);
    expect_eq(e10.merged_into_semantic_id, e11.merged_into_semantic_id);
    expect_eq(e12.status, MemConsolStatus.active);

    // Verify promoted semantic memory has boosted importance (0.80 + 0.10 = 0.90), unit norm, and cluster_members = 2
    const sem = eng.getSemantic(e10.merged_into_semantic_id) ?? return;
    expect_eq(sem.cluster_members, 2 as u32);
    expect_true(approxEqF32(sem.importance, 0.90, 1.0e-5));
    const sem_v: []const f32 = sem.vec[0..4];
    expect_true(approxEqF32(@dot(sem_v, sem_v), 1.0, 1.0e-5));
    expect_true(sem.vec[1] > 0.05); // shifted toward va2 by @fma weighted centroid
});

// ============================================================================
// Test 3: Fusing new episodic cluster into an existing matching semantic memory
// ============================================================================
test("consolidation_engine: fusing episodic cluster into existing matching semantic memory", fn() {
    let eng = AgentMemoryConsolidator.init(4) ?? return;

    const base_sem: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    expect_true(eng.addSemantic(500, 0, 1, 0.70, 500, false, base_sem[0..]));

    const ep1: [4]f32 = [0.99, 0.14, 0.0, 0.0];
    const ep2: [4]f32 = [0.97, 0.24, 0.0, 0.0];
    expect_true(eng.addEpisodic(1, 0, 1, 0.75, 1000, false, ep1[0..]));
    expect_true(eng.addEpisodic(2, 0, 1, 0.75, 1500, false, ep2[0..]));

    const rep = eng.runConsolidationPass(2000, MemConsolPassConfig.defaults());
    expect_eq(rep.clusters_merged, 1 as usize);
    // Should fuse into existing semantic 500 rather than creating a duplicate semantic slot
    expect_eq(eng.stats().active_semantic, 1 as usize);
    const sem = eng.getSemantic(500) ?? return;
    expect_eq(sem.cluster_members, 3 as u32); // 1 original + 2 merged episodic
    expect_true(sem.importance >= 0.84);
});

// ============================================================================
// Test 4: Entity conflict detection & superseding older episodic/semantic facts
// ============================================================================
test("consolidation_engine: entity_key_hash conflict detection & superseding older facts", fn() {
    let eng = AgentMemoryConsolidator.init(4) ?? return;
    const ENTITY_USER_CITY: u64 = 0xCAFE1234;

    const old_fact: [4]f32 = [1.0, 0.0, 0.0, 0.0]; // "lives in Paris"
    const new_fact: [4]f32 = [0.0, 1.0, 0.0, 0.0]; // "moved to Tokyo" (cos = 0.0 < 0.35)

    // Both an older semantic fact and an older episodic fact share ENTITY_USER_CITY
    expect_true(eng.addSemantic(100, ENTITY_USER_CITY, 1, 0.8, 1000, false, old_fact[0..]));
    expect_true(eng.addEpisodic(101, ENTITY_USER_CITY, 1, 0.7, 2000, false, old_fact[0..]));
    // Newer conflicting episodic fact arrives at t=5000
    expect_true(eng.addEpisodic(102, ENTITY_USER_CITY, 1, 0.9, 5000, false, new_fact[0..]));

    const rep = eng.runConsolidationPass(6000, MemConsolPassConfig.defaults());
    expect_eq(rep.conflicts_superseded, 2 as usize);

    const old_s = eng.getSemantic(100) ?? return;
    const old_e = eng.getEpisodic(101) ?? return;
    const cur_e = eng.getEpisodic(102) ?? return;
    expect_eq(old_s.status, MemConsolStatus.superseded);
    expect_eq(old_s.superseded_by_id, 102 as u64);
    expect_eq(old_e.status, MemConsolStatus.superseded);
    expect_eq(old_e.superseded_by_id, 102 as u64);
    expect_eq(cur_e.status, MemConsolStatus.active);
});

// ============================================================================
// Test 5: Low-retention archival, hard-TTL eviction & strict pinned protection
// ============================================================================
test("consolidation_engine: low-retention archival, hard-TTL eviction & pinned protection", fn() {
    let eng = AgentMemoryConsolidator.init(4) ?? return;
    const v1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const v3: [4]f32 = [0.0, 0.0, 1.0, 0.0];

    // 1: low importance, old enough to drop below prune_threshold (archived)
    expect_true(eng.addEpisodic(1, 0, 1, 0.05, 1000, false, v1[0..]));
    // 2: very old, exceeds hard TTL (evicted)
    expect_true(eng.addEpisodic(2, 0, 1, 0.90, 100, false, v2[0..]));
    // 3: very old & low importance, BUT pinned -> MUST stay active!
    expect_true(eng.addEpisodic(3, 0, 1, 0.01, 100, true, v3[0..]));

    let cfg = MemConsolPassConfig.defaults();
    cfg.half_life_ms = 100;
    cfg.ttl_ms = 10_000;
    cfg.prune_threshold = 0.25;

    // Run pass at now_ms = 10_500:
    // - entry 1: elapsed=9500 <= ttl(10000), decayed score ~ 0.02 < 0.25 -> archived
    // - entry 2: elapsed=10400 > ttl(10000) -> evicted
    // - entry 3: pinned -> protected & active
    const rep = eng.runConsolidationPass(10_500, cfg);
    expect_eq(rep.pruned_archived_count, 1 as usize);
    expect_eq(rep.evicted_count, 1 as usize);
    expect_eq(rep.pinned_protected_count, 1 as usize);

    expect_eq((eng.getEpisodic(1) ?? return).status, MemConsolStatus.archived);
    expect_eq((eng.getEpisodic(2) ?? return).status, MemConsolStatus.evicted);
    expect_eq((eng.getEpisodic(3) ?? return).status, MemConsolStatus.active);
});

// ============================================================================
// Test 6: Full episodic pool capacity guard (64 slots)
// ============================================================================
test("consolidation_engine: full episodic pool capacity guard (64 slots)", fn() {
    let eng = AgentMemoryConsolidator.init(4) ?? return;
    const v: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    let i: u64 = 1;
    while ((i as usize) <= MEM_CONSOL_MAX_EPISODIC) : (i += 1) {
        expect_true(eng.addEpisodic(i, 0, 1, 0.5, i * 10, false, v[0..]));
    }
    // 65th episodic entry rejected cleanly
    expect_true(!eng.addEpisodic(999, 0, 1, 0.5, 9999, false, v[0..]));
    expect_eq(eng.stats().active_episodic, MEM_CONSOL_MAX_EPISODIC);
});

// ============================================================================
// Test 7: Strict boundary guards (invalid dim, zero/NaN vec, bad importance)
// ============================================================================
test("consolidation_engine: strict boundary guards (dim, zero/NaN vec, importance, dup ID)", fn() {
    expect_true(AgentMemoryConsolidator.init(0) == null);
    expect_true(AgentMemoryConsolidator.init(MEM_CONSOL_MAX_DIM + 1) == null);

    let eng = AgentMemoryConsolidator.init(4) ?? return;
    const zero_v: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    const nan_v: [4]f32 = [0.0 / 0.0, 1.0, 0.0, 0.0];
    const bad_dim: [3]f32 = [1.0, 0.0, 0.0];
    const good_v: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    expect_true(!eng.addEpisodic(0, 0, 1, 0.5, 1, false, good_v[0..])); // id == 0
    expect_true(!eng.addEpisodic(1, 0, 1, 0.5, 1, false, zero_v[0..])); // zero vec
    expect_true(!eng.addEpisodic(1, 0, 1, 0.5, 1, false, nan_v[0..]));  // NaN vec
    expect_true(!eng.addEpisodic(1, 0, 1, 0.5, 1, false, bad_dim[0..])); // dim mismatch
    expect_true(!eng.addEpisodic(1, 0, 1, 1.5, 1, false, good_v[0..])); // importance > 1
    expect_true(!eng.addEpisodic(1, 0, 1, -0.1, 1, false, good_v[0..])); // importance < 0

    expect_true(eng.addEpisodic(1, 0, 1, 0.5, 1, false, good_v[0..]));
    expect_true(!eng.addEpisodic(1, 0, 1, 0.5, 2, false, good_v[0..])); // duplicate ID

    // Invalid pass config is safely ignored
    let bad_cfg = MemConsolPassConfig.defaults();
    bad_cfg.half_life_ms = 0;
    expect_true(!bad_cfg.isValid());
    const rep = eng.runConsolidationPass(100, bad_cfg);
    expect_eq(rep.clusters_merged, 0 as usize);
    expect_eq(eng.stats().total_passes, 0 as u64);
});
