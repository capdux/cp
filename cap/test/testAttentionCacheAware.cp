// Origin: Pure-CP Stdlib Test Suite (lib/attention/cacheAware.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Prefix-Cache-Aware Multi-Replica LLM Inference Router:
//   1. Contiguous longest shared prefix affinity routing & partial-prefix break semantics
//   2. Overload circuit-breaker diversion (`kv_util_bps >= overload_kv_bps`) preventing hot-prefix snowball
//   3. All-overloaded hotspot fallback, full saturation & unhealthy failover
//   4. 64-slot per-replica LRU eviction & refresh behavior
//   5. Closed-loop `completeRequest` underflow protection, `updateReplicaLoad` & telemetry stats
//   6. 16-replica saturation, invalid config/input guards & `cap.attention` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CACHE_ROUTE_MAX_REPLICAS,
    CACHE_ROUTE_LRU_BLOCKS,
    CACHE_ROUTE_BPS_SCALE,
    CacheRouteStatus,
    CacheRouteConfig,
    ReplicaNodeState,
    CacheAwareRouter,
} from "cap:attention";

test("CacheAware: contiguous longest shared prefix affinity routing & partial mismatch") {
    let router = CacheAwareRouter.init();
    * = router.registerReplica(1, 16);
    * = router.registerReplica(2, 16);

    // First prompt lands on replica 1 (lowest ID tie-breaker at equal 0 load)
    const p1: [4]u64 = [101, 102, 103, 104];
    const d1 = router.routePrompt(p1[0..], 1000);
    expect_eq(d1.status, CacheRouteStatus.routed);
    expect_eq(d1.replica_id, 1 as u32);
    expect_eq(d1.matched_blocks, 0 as usize);

    // Second unrelated prompt prefers replica 2 (lighter queue load: 0 vs 1)
    const p2: [3]u64 = [201, 202, 203];
    const d2 = router.routePrompt(p2[0..], 1100);
    expect_eq(d2.replica_id, 2 as u32);

    // Follow-up sharing [101, 102, 103] prefix routes back to replica 1 with 3/5 hit ratio!
    const p3: [5]u64 = [101, 102, 103, 999, 1000];
    const d3 = router.routePrompt(p3[0..], 1200);
    expect_eq(d3.replica_id, 1 as u32);
    expect_eq(d3.matched_blocks, 3 as usize);
    expect_true(d3.hit_ratio > 0.59 && d3.hit_ratio < 0.61);

    // Contiguous prefix check: [101, 888, 103] must only match 1 block (stops at 888!)
    const p4: [3]u64 = [101, 888, 103];
    const rep1 = router.getReplica(1) ?? ReplicaNodeState{};
    expect_eq(rep1.matchedPrefixBlocks(p4[0..]), 1 as usize);
}

test("CacheAware: overload diversion prevents hot-prefix cascade & hotspot fallback") {
    let router = CacheAwareRouter.init();
    * = router.registerReplica(1, 16);
    * = router.registerReplica(2, 16);

    // Warm replica 1 with hot system prompt [10, 20, 30, 40]
    const hot: [4]u64 = [10, 20, 30, 40];
    expect_eq(router.routePrompt(hot[0..], 1000).replica_id, 1 as u32);

    // Push replica 1 into overload (kv_util_bps = 9200 >= 8500), while replica 2 is cool (1000 bps)
    expect_true(router.updateReplicaLoad(1, 8, 9200));
    expect_true(router.updateReplicaLoad(2, 1, 1000));

    // Despite 100% prefix match on replica 1, overload penalty diverts to replica 2!
    const div = router.routePrompt(hot[0..], 1500);
    expect_eq(div.status, CacheRouteStatus.routed);
    expect_eq(div.replica_id, 2 as u32);
    expect_true(div.diverted_by_overload);
    expect_eq(div.hotspot_fallback, false);

    // Now push BOTH replicas into overload -> hotspot_fallback picks relatively better replica
    expect_true(router.updateReplicaLoad(2, 12, 9500));
    const fb = router.routePrompt(hot[0..], 2000);
    expect_eq(fb.status, CacheRouteStatus.routed);
    expect_true(fb.hotspot_fallback);

    const st = router.stats();
    expect_eq(st.overload_diversions, 1 as u64);
    expect_eq(st.hotspot_fallbacks, 1 as u64);
}

test("CacheAware: health failover, queue saturation & completeRequest underflow protection") {
    let router = CacheAwareRouter.init();
    * = router.registerReplica(1, 2);
    * = router.registerReplica(2, 2);

    const p: [2]u64 = [55, 66];
    expect_eq(router.routePrompt(p[0..], 100).replica_id, 1 as u32);

    // Mark replica 1 unhealthy -> traffic automatically fails over to replica 2
    expect_true(router.setReplicaHealth(1, false));
    expect_eq(router.routePrompt(p[0..], 200).replica_id, 2 as u32);
    expect_eq(router.routePrompt(p[0..], 300).replica_id, 2 as u32);

    // Replica 2 now has active_requests == 2 == max_requests -> all_replicas_saturated
    const sat = router.routePrompt(p[0..], 400);
    expect_eq(sat.status, CacheRouteStatus.all_replicas_saturated);

    // Mark replica 2 unhealthy as well -> no_healthy_replica
    expect_true(router.setReplicaHealth(2, false));
    expect_eq(router.routePrompt(p[0..], 500).status, CacheRouteStatus.no_healthy_replica);

    // Complete requests on replica 2 (including extra completion to verify underflow guard)
    expect_true(router.setReplicaHealth(2, true));
    expect_true(router.completeRequest(2, 3000));
    expect_true(router.completeRequest(2, 1000));
    expect_true(router.completeRequest(2, 0)); // underflow safe: stays 0
    const r2 = router.getReplica(2) ?? ReplicaNodeState{};
    expect_eq(r2.active_requests, 0 as u32);
    expect_eq(r2.kv_util_bps, 0 as u32);
    expect_eq(router.stats().rejected_routes, 2 as u64);
}

test("CacheAware: 64-slot per-replica LRU eviction & touch refresh") {
    let router = CacheAwareRouter.init();
    * = router.registerReplica(1, 128);

    // Insert 64 distinct single-block prompts (block hashes 1..64)
    let b: u64 = 1;
    while (b <= (CACHE_ROUTE_LRU_BLOCKS as u64)) : (b += 1) {
        const blk: [1]u64 = [b];
        expect_eq(router.routePrompt(blk[0..], b * 10).status, CacheRouteStatus.routed);
    }

    // Refresh block 1 so block 2 becomes the oldest LRU victim
    const touch1: [1]u64 = [1];
    expect_eq(router.routePrompt(touch1[0..], 1000).matched_blocks, 1 as usize);

    // Insert 65th distinct block (hash 999) -> MUST evict block 2, keeping block 1!
    const blk999: [1]u64 = [999];
    * = router.routePrompt(blk999[0..], 1100);

    const rep = router.getReplica(1) ?? ReplicaNodeState{};
    expect_eq(rep.lru_count, CACHE_ROUTE_LRU_BLOCKS);
    expect_true(rep.hasBlock(1));
    expect_eq(rep.hasBlock(2), false);
    expect_true(rep.hasBlock(999));
}

test("CacheAware: empty prompt routing & telemetry counters accuracy") {
    let router = CacheAwareRouter.init();
    * = router.registerReplica(10, 8);
    * = router.registerReplica(20, 8);

    const empty_p: [0]u64 = [];
    const d0 = router.routePrompt(empty_p[0..], 100);
    expect_eq(d0.status, CacheRouteStatus.routed);
    expect_eq(d0.matched_blocks, 0 as usize);
    expect_eq(d0.hit_ratio, 0.0 as f32);

    const seq: [3]u64 = [700, 701, 702];
    const d1 = router.routePrompt(seq[0..], 200);
    * = router.completeRequest(d1.replica_id, 500);
    const d2 = router.routePrompt(seq[0..], 300);
    expect_eq(d2.replica_id, d1.replica_id);
    expect_eq(d2.matched_blocks, 3 as usize);

    const st = router.stats();
    expect_eq(st.total_routes, 3 as u64);
    expect_eq(st.cache_hit_routes, 1 as u64);
    expect_eq(st.total_matched_blocks, 3 as u64);
    expect_eq(st.total_prompt_blocks, 6 as u64);
}

test("CacheAware: 16-replica capacity saturation, invalid guards & cap.attention facade parity") {
    let bad_cfg = CacheRouteConfig.defaults();
    bad_cfg.overload_kv_bps = CACHE_ROUTE_BPS_SCALE + 1;
    expect_true(CacheAwareRouter.initWithConfig(bad_cfg) == null);

    let router = cap.attention.CacheAwareRouter.init();
    expect_true(router.registerReplica(0, 16) == null);
    expect_true(router.registerReplica(1, 0) == null);

    let i: u32 = 1;
    while (i <= (CACHE_ROUTE_MAX_REPLICAS as u32)) : (i += 1) {
        expect_true(router.registerReplica(i, 16) != null);
    }
    expect_eq(router.stats().replica_count, CACHE_ROUTE_MAX_REPLICAS);
    expect_true(router.registerReplica(1, 16) == null); // duplicate
    expect_true(router.registerReplica(99, 16) == null); // capacity full

    // Reject out-of-range KV bps (> 10000) and unknown replica IDs
    expect_eq(router.updateReplicaLoad(1, 0, CACHE_ROUTE_BPS_SCALE + 1), false);
    expect_eq(router.completeRequest(999, 100), false);
    expect_eq(router.setReplicaHealth(999, false), false);

    // Remove and reuse slot
    expect_true(router.removeReplica(8));
    expect_eq(router.stats().replica_count, CACHE_ROUTE_MAX_REPLICAS - 1);
    expect_true(router.registerReplica(88, 32) != null);
}
