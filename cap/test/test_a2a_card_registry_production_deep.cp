// Origin: Pure-CP Stdlib Test Suite (lib/a2a/card_registry.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc A2A AgentCard Discovery Registry:
//   1. AgentCard registration & same-ID upsert_card in-place metadata refresh
//   2. Exact bitmask filtering across skills, input/output modalities & protocol features
//   3. Health & latency weighted ranking (health / (1 + lat/200)) & agent_id ascending tie-break
//   4. Hard SLA filters: min_health_bps & max_latency_ms (including 0 = unlimited)
//   5. heartbeat TTL renewal, discover_topk expired skip & prune_expired reclamation
//   6. deregister slot reclamation & immediate reuse
//   7. Invalid agent_id/health/TTL/empty-name guards & full 64-card capacity guard

import { test, expect_eq } from "cap:test";
import {
    A2A_REGISTRY_MAX_CARDS,
    A2A_CARD_NAME_MAX_LEN,
    A2A_CARD_URL_MAX_LEN,
    A2A_MOD_TEXT,
    A2A_MOD_JSON,
    A2A_MOD_IMAGE,
    A2A_MOD_AUDIO,
    A2A_MOD_FILE,
    A2A_FEAT_STREAMING,
    A2A_FEAT_PUSH_NOTIFY,
    A2A_FEAT_STATE_HISTORY,
    A2aCardEntry,
    A2aDiscoveryQuery,
    A2aDiscoveryHit,
    A2aCardRegistryStats,
    A2aCardRegistry,
} from "cap:a2a";

// ============================================================================
// Test 1: Registration & Same-ID upsert_card In-Place Refresh
// ============================================================================
test("card_registry: register and same-ID upsert_card in-place refresh", () => {
    expect_eq(A2A_REGISTRY_MAX_CARDS, 64);
    expect_eq(A2A_CARD_NAME_MAX_LEN, 48);
    expect_eq(A2A_CARD_URL_MAX_LEN, 128);

    let reg = A2aCardRegistry.init();

    const c1 = A2aCardEntry.create(
        101, "coder-v1", "https://coder1.local/a2a",
        0b01, A2A_MOD_TEXT, A2A_MOD_JSON, A2A_FEAT_STREAMING,
        8000, 400, 1_000, 10_000,
    ) ?? return;
    expect_eq(reg.upsert_card(c1), true);
    expect_eq(reg.stats().active_cards, 1);
    expect_eq(reg.stats().total_registrations, 1);

    // Upsert same agent_id 101 with upgraded skills, higher health & lower latency
    const c1_v2 = A2aCardEntry.create(
        101, "coder-v2", "https://coder2.local/a2a",
        0b11, A2A_MOD_TEXT | A2A_MOD_FILE, A2A_MOD_JSON, A2A_FEAT_STREAMING | A2A_FEAT_STATE_HISTORY,
        9800, 50, 2_000, 10_000,
    ) ?? return;
    expect_eq(reg.upsert_card(c1_v2), true);

    const st: A2aCardRegistryStats = reg.stats();
    expect_eq(st.active_cards, 1); // no duplicate slot allocated
    expect_eq(st.total_registrations, 2);

    let hits: [2]A2aDiscoveryHit = [A2aDiscoveryHit{}; 2];
    let hits_s: []A2aDiscoveryHit = hits[0..];
    const n = reg.discover_topk(
        A2aDiscoveryQuery{ required_skills: 0b11, required_in_mods: A2A_MOD_FILE },
        2_500, 2, hits_s,
    );
    expect_eq(n, 1);
    expect_eq(hits[0].agent_id, 101);
    expect_eq(hits[0].health_score_bps, 9800);
    expect_eq(hits[0].avg_latency_ms, 50);
});

// ============================================================================
// Test 2: Exact Bitmask Filtering (Skills, In/Out Modalities, Protocol Features)
// ============================================================================
test("card_registry: exact bitmask intersection filtering across capabilities", () => {
    let reg = A2aCardRegistry.init();
    const SKILL_VISION: u64 = 1 << 0;
    const SKILL_SQL: u64 = 1 << 1;

    // Agent 1: has VISION + IMAGE in + JSON out + STREAMING (full match)
    const a1 = A2aCardEntry.create(
        1, "vision-full", "https://a1.local",
        SKILL_VISION | SKILL_SQL,
        A2A_MOD_TEXT | A2A_MOD_IMAGE,
        A2A_MOD_JSON | A2A_MOD_TEXT,
        A2A_FEAT_STREAMING | A2A_FEAT_PUSH_NOTIFY,
        9500, 100, 1_000, 30_000,
    ) ?? return;

    // Agent 2: missing IMAGE input modality
    const a2 = A2aCardEntry.create(
        2, "no-image", "https://a2.local",
        SKILL_VISION, A2A_MOD_TEXT, A2A_MOD_JSON, A2A_FEAT_STREAMING,
        10000, 20, 1_000, 30_000,
    ) ?? return;

    // Agent 3: missing STREAMING feature
    const a3 = A2aCardEntry.create(
        3, "no-stream", "https://a3.local",
        SKILL_VISION, A2A_MOD_IMAGE, A2A_MOD_JSON, A2A_FEAT_STATE_HISTORY,
        10000, 20, 1_000, 30_000,
    ) ?? return;

    // Agent 4: missing JSON output modality
    const a4 = A2aCardEntry.create(
        4, "audio-out", "https://a4.local",
        SKILL_VISION, A2A_MOD_IMAGE, A2A_MOD_AUDIO, A2A_FEAT_STREAMING,
        10000, 20, 1_000, 30_000,
    ) ?? return;

    expect_eq(reg.upsert_card(a1), true);
    expect_eq(reg.upsert_card(a2), true);
    expect_eq(reg.upsert_card(a3), true);
    expect_eq(reg.upsert_card(a4), true);

    let hits: [4]A2aDiscoveryHit = [A2aDiscoveryHit{}; 4];
    let hits_s: []A2aDiscoveryHit = hits[0..];
    const n = reg.discover_topk(
        A2aDiscoveryQuery{
            required_skills: SKILL_VISION,
            required_in_mods: A2A_MOD_IMAGE,
            required_out_mods: A2A_MOD_JSON,
            required_features: A2A_FEAT_STREAMING,
        },
        2_000, 4, hits_s,
    );
    expect_eq(n, 1);
    expect_eq(hits[0].agent_id, 1);
});

// ============================================================================
// Test 3: Health & Latency Weighted Ranking & agent_id Ascending Tie-Break
// ============================================================================
test("card_registry: health/latency weighted ranking & agent_id tie-break", () => {
    let reg = A2aCardRegistry.init();

    // Agent 30: health 9000, lat 200ms -> 0.9 / (1 + 1.0) = 0.45
    const a30 = A2aCardEntry.create(30, "a30", "https://30", 1, 1, 1, 1, 9000, 200, 1_000, 30_000) ?? return;
    // Agent 10: health 9000, lat 200ms -> 0.45 (tied with 30, smaller ID wins!)
    const a10 = A2aCardEntry.create(10, "a10", "https://10", 1, 1, 1, 1, 9000, 200, 1_000, 30_000) ?? return;
    // Agent 20: health 9500, lat 50ms -> 0.95 / 1.25 = 0.76 (Rank #1!)
    const a20 = A2aCardEntry.create(20, "a20", "https://20", 1, 1, 1, 1, 9500, 50, 1_000, 30_000) ?? return;
    // Agent 40: health 10000, lat 800ms -> 1.0 / 5.0 = 0.20 (Rank #4)
    const a40 = A2aCardEntry.create(40, "a40", "https://40", 1, 1, 1, 1, 10000, 800, 1_000, 30_000) ?? return;

    expect_eq(reg.upsert_card(a30), true);
    expect_eq(reg.upsert_card(a10), true);
    expect_eq(reg.upsert_card(a20), true);
    expect_eq(reg.upsert_card(a40), true);

    let hits: [3]A2aDiscoveryHit = [A2aDiscoveryHit{}; 3];
    let hits_s: []A2aDiscoveryHit = hits[0..];
    const n = reg.discover_topk(A2aDiscoveryQuery{}, 2_000, 3, hits_s);
    expect_eq(n, 3);
    expect_eq(hits[0].agent_id, 20);
    expect_eq(hits[1].agent_id, 10);
    expect_eq(hits[2].agent_id, 30);
    expect_eq(hits[0].rank_score_f32 > hits[1].rank_score_f32, true);
});

// ============================================================================
// Test 4: Hard SLA Filters (min_health_bps & max_latency_ms)
// ============================================================================
test("card_registry: min_health_bps and max_latency_ms SLA filters", () => {
    let reg = A2aCardRegistry.init();

    const fast_unhealthy = A2aCardEntry.create(1, "unhealthy", "https://1", 1, 1, 1, 1, 6000, 20, 1_000, 30_000) ?? return;
    const slow_healthy = A2aCardEntry.create(2, "slow", "https://2", 1, 1, 1, 1, 9900, 500, 1_000, 30_000) ?? return;
    const balanced = A2aCardEntry.create(3, "ok", "https://3", 1, 1, 1, 1, 9200, 150, 1_000, 30_000) ?? return;

    expect_eq(reg.upsert_card(fast_unhealthy), true);
    expect_eq(reg.upsert_card(slow_healthy), true);
    expect_eq(reg.upsert_card(balanced), true);

    let hits: [4]A2aDiscoveryHit = [A2aDiscoveryHit{}; 4];
    let hits_s: []A2aDiscoveryHit = hits[0..];

    // Strict SLA: min_health 8500, max_latency 200 -> only agent 3
    const n1 = reg.discover_topk(
        A2aDiscoveryQuery{ min_health_bps: 8500, max_latency_ms: 200 },
        2_000, 4, hits_s,
    );
    expect_eq(n1, 1);
    expect_eq(hits[0].agent_id, 3);

    // max_latency_ms == 0 means unlimited -> agents 2 and 3 pass
    const n2 = reg.discover_topk(
        A2aDiscoveryQuery{ min_health_bps: 8500, max_latency_ms: 0 },
        2_000, 4, hits_s,
    );
    expect_eq(n2, 2);
});

// ============================================================================
// Test 5: heartbeat TTL Renewal & prune_expired Stale Node Eviction
// ============================================================================
test("card_registry: heartbeat TTL renewal & prune_expired eviction", () => {
    let reg = A2aCardRegistry.init();

    const a1 = A2aCardEntry.create(1, "renewed", "https://1", 1, 1, 1, 1, 9000, 100, 1_000, 5_000) ?? return;
    const a2 = A2aCardEntry.create(2, "stale", "https://2", 1, 1, 1, 1, 9500, 50, 1_000, 5_000) ?? return;
    expect_eq(reg.upsert_card(a1), true);
    expect_eq(reg.upsert_card(a2), true);

    // At t=4000, agent 1 sends heartbeat -> new expiry at 4000 + 5000 = 9000
    expect_eq(reg.heartbeat(1, 9600, 80, 4_000), true);
    expect_eq(reg.heartbeat(999, 9000, 80, 4_000), false); // unknown ID
    expect_eq(reg.heartbeat(1, 10001, 80, 4_000), false);  // invalid health > 10000

    // At t=6001: agent 2 expired (6001 - 1000 = 5001 > 5000); discover_topk skips it automatically!
    let hits: [2]A2aDiscoveryHit = [A2aDiscoveryHit{}; 2];
    let hits_s: []A2aDiscoveryHit = hits[0..];
    expect_eq(reg.discover_topk(A2aDiscoveryQuery{}, 6_001, 2, hits_s), 1);
    expect_eq(hits[0].agent_id, 1);

    // prune_expired reclaims agent 2's slot
    expect_eq(reg.prune_expired(6_001), 1);
    expect_eq(reg.stats().active_cards, 1);
    expect_eq(reg.stats().expired_pruned, 1);
});

// ============================================================================
// Test 6: deregister Removal & Inline Slot Reuse
// ============================================================================
test("card_registry: deregister frees slot for immediate reuse", () => {
    let reg = A2aCardRegistry.init();

    const a1 = A2aCardEntry.create(10, "temp", "https://temp", 1, 1, 1, 1, 9000, 50, 1_000, 30_000) ?? return;
    expect_eq(reg.upsert_card(a1), true);
    expect_eq(reg.deregister(10), true);
    expect_eq(reg.deregister(10), false); // already removed
    expect_eq(reg.deregister(0), false);
    expect_eq(reg.stats().active_cards, 0);

    let hits: [2]A2aDiscoveryHit = [A2aDiscoveryHit{}; 2];
    let hits_s: []A2aDiscoveryHit = hits[0..];
    expect_eq(reg.discover_topk(A2aDiscoveryQuery{}, 2_000, 2, hits_s), 0);

    const a2 = A2aCardEntry.create(20, "reused", "https://reused", 1, 1, 1, 1, 9500, 40, 2_000, 30_000) ?? return;
    expect_eq(reg.upsert_card(a2), true);
    expect_eq(reg.stats().active_cards, 1);
});

// ============================================================================
// Test 7: Invalid Entry Guards & Full 64-Card Pool Capacity Guard
// ============================================================================
test("card_registry: invalid entry guards & 64-card full capacity guard", () => {
    expect_eq(A2aCardEntry.create(0, "bad", "https://x", 1, 1, 1, 1, 9000, 10, 1_000, 5_000) == null, true);
    expect_eq(A2aCardEntry.create(1, "", "https://x", 1, 1, 1, 1, 9000, 10, 1_000, 5_000) == null, true);
    expect_eq(A2aCardEntry.create(1, "ok", "", 1, 1, 1, 1, 9000, 10, 1_000, 5_000) == null, true);
    expect_eq(A2aCardEntry.create(1, "ok", "https://x", 1, 1, 1, 1, 10001, 10, 1_000, 5_000) == null, true);
    expect_eq(A2aCardEntry.create(1, "ok", "https://x", 1, 1, 1, 1, 9000, 10, 1_000, 0) == null, true);

    let reg = A2aCardRegistry.init();
    expect_eq(reg.upsert_card(A2aCardEntry{}), false);

    // Fill all 64 slots
    let id: u64 = 1;
    while (id <= A2A_REGISTRY_MAX_CARDS as u64) : (id += 1) {
        const c = A2aCardEntry.create(id, "node", "https://node.local", 1, 1, 1, 1, 9000, 50, 1_000, 30_000) ?? return;
        expect_eq(reg.upsert_card(c), true);
    }
    expect_eq(reg.stats().active_cards, 64);

    // 65th distinct agent_id rejected cleanly
    const overflow = A2aCardEntry.create(999, "overflow", "https://ovf", 1, 1, 1, 1, 9000, 50, 1_000, 30_000) ?? return;
    expect_eq(reg.upsert_card(overflow), false);
    expect_eq(reg.stats().active_cards, 64);

    // Existing agent_id 1 can still upsert in place even when full!
    const refresh1 = A2aCardEntry.create(1, "node-refreshed", "https://node.local", 1, 1, 1, 1, 9999, 10, 2_000, 30_000) ?? return;
    expect_eq(reg.upsert_card(refresh1), true);
    expect_eq(reg.stats().active_cards, 64);
});
