// Origin: Pure-CP Stdlib Test Suite (lib/prompt/version_registry.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Prompt Version Registry & Canary A/B Traffic Router:
//   1. Registration, duplicate-ID guard, field length limits & capacity (16 slots) boundary
//   2. Deterministic sticky routing, monotonic canary expansion & 25% traffic split accuracy
//   3. Canary promotion & one-click atomic rollback (both during canary and post-promotion)
//   4. Shadow traffic mirroring & safe fallback degradation when no active version exists
//   5. Automated rollout guard rollback on error-rate breach (respecting min_samples)
//   6. Automated rollout guard rollback on latency breach & invalid guard config guards
//   7. Per-variant telemetry & zero-alloc summary_stats aggregation

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_PROMPT_VERSIONS,
    BPS_SCALE,
    PromptVariantStatus,
    RouteDecision,
    RolloutGuardConfig,
    RegistrySummaryStats,
    PromptVersionRegistry,
} from "cap:prompt";

fn formatTenantKeyLen(buf: *[]u8, idx: usize): usize {
    // Writes "tenant-" followed by decimal idx and returns byte length
    const prefix = "tenant-";
    let pos: usize = 0;
    while (pos < prefix.len) : (pos += 1) {
        buf[pos] = prefix[pos];
    }
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    let v: usize = idx;
    if (v == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = (48 + (v % 10)) as u8;
            v /= 10;
            n += 1;
        }
    }
    let k: usize = 0;
    while (k < n) : (k += 1) {
        buf[pos + k] = tmp[n - 1 - k];
    }
    return pos + n;
}

// ============================================================================
// Test 1: Registration, Duplicate ID & Capacity Guards
// ============================================================================

fn testRegistrationDuplicateAndCapacityGuards()! {
    let reg = cap.prompt.PromptVersionRegistry.init();

    // Reject empty ID and negative created_ms
    expect_eq(reg.register_version("", 1, "gpt-4o", "summary", 1, 0, 0, 100) == null, true);
    expect_eq(reg.register_version("v1", 1, "gpt-4o", "summary", 1, 0, 0, -1) == null, true);

    // Reject oversized ID (> 48 bytes)
    const long_id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"; // 49 bytes
    expect_eq(reg.register_version(long_id, 1, "gpt-4o", "summary", 1, 0, 0, 100) == null, true);

    const s0 = reg.register_version("v1.0.0", 0xABCD, "claude-3.5", "Base system prompt", 1, 0, 0, 1000) ?? return error.RegFailed;
    expect_eq(s0, 0);
    expect_eq(reg.entries[s0].status == PromptVariantStatus.draft, true);
    expect_eq(std.mem.eql(u8, reg.entries[s0].idSlice(), "v1.0.0"), true);
    expect_eq(std.mem.eql(u8, reg.entries[s0].modelHintSlice(), "claude-3.5"), true);
    expect_eq(std.mem.eql(u8, reg.entries[s0].summarySlice(), "Base system prompt"), true);

    // Duplicate ID must be rejected
    expect_eq(reg.register_version("v1.0.0", 0x9999, "gpt-4o", "dup", 1, 0, 1, 1001) == null, true);

    // Fill remaining 15 slots up to MAX_PROMPT_VERSIONS (16)
    let id_buf: [32]u8 = [0; 32];
    let i: usize = 1;
    while (i < MAX_PROMPT_VERSIONS) : (i += 1) {
        let id_s: []u8 = id_buf[0..];
        const klen = formatTenantKeyLen(id_s, i);
        expect_eq(reg.register_version(id_buf[0..klen], i as u64, "model", "prompt", 1, i as u16, 0, 1000 + (i as i64)) != null, true);
    }

    // 17th version must be rejected cleanly
    expect_eq(reg.register_version("overflow-v17", 99, "model", "prompt", 2, 0, 0, 2000) == null, true);
    expect_eq(reg.count, MAX_PROMPT_VERSIONS);
}

// ============================================================================
// Test 2: Deterministic Sticky Routing, Monotonicity & 25% Canary Split
// ============================================================================

fn testDeterministicStickyAndCanarySplitDistribution()! {
    let reg = PromptVersionRegistry.init();
    const v1 = reg.register_version("prod-v1", 0x101, "gpt-4o", "Stable", 1, 0, 0, 1000) ?? return error.RegFailed;
    const v2 = reg.register_version("prod-v2", 0x202, "gpt-4o", "Canary", 1, 1, 0, 2000) ?? return error.RegFailed;

    expect_eq(reg.set_active(v1), true);
    expect_eq(reg.entries[v1].weight_bps, BPS_SCALE);

    // Start 25% (2500 bps) canary on v2 -> v1 normalized to 7500 bps
    expect_eq(reg.start_canary(v2, 2500), true);
    expect_eq(reg.entries[v2].weight_bps, 2500);
    expect_eq(reg.entries[v1].weight_bps, 7500);

    // Verify 100% stickiness across repeated lookups for the same tenant_key
    const d_first: RouteDecision = reg.route_request("tenant-sticky-42", 777);
    const d_second: RouteDecision = reg.route_request("tenant-sticky-42", 777);
    expect_eq(d_first.found, true);
    expect_eq(d_first.slot_idx, d_second.slot_idx);
    expect_eq(d_first.bucket_bps, d_second.bucket_bps);
    expect_eq(d_first.is_canary, d_second.is_canary);
    expect_eq(d_first.is_fallback, false);

    // Statistical distribution across 2000 distinct tenants at 25.00% canary weight
    let key_buf: [32]u8 = [0; 32];
    let canary_hits: usize = 0;
    let t: usize = 0;
    while (t < 2000) : (t += 1) {
        let key_s: []u8 = key_buf[0..];
        const klen = formatTenantKeyLen(key_s, t);
        const route = reg.route_request(key_buf[0..klen], 12345);
        expect_eq(route.found, true);
        expect_eq(route.bucket_bps < BPS_SCALE, true);
        if (route.is_canary) {
            expect_eq(route.slot_idx, v2);
            expect_eq(route.bucket_bps < 2500, true);
            canary_hits += 1;
        } else {
            expect_eq(route.slot_idx, v1);
            expect_eq(route.bucket_bps >= 2500, true);
        }
    }

    // Expected ~500 out of 2000 (25%); verify tight tolerance [430, 570] (25% +/- 3.5%)
    expect_eq(canary_hits >= 430 && canary_hits <= 570, true);

    // Monotonicity check: expanding canary from 2500 bps -> 5000 bps keeps all prior canary tenants on canary
    expect_eq(reg.start_canary(v2, 5000), true);
    t = 0;
    while (t < 200) : (t += 1) {
        let key_s: []u8 = key_buf[0..];
        const klen = formatTenantKeyLen(key_s, t);
        const route = reg.route_request(key_buf[0..klen], 12345);
        if (route.bucket_bps < 2500) {
            expect_eq(route.is_canary, true);
        }
    }
}

// ============================================================================
// Test 3: Canary Promotion & One-Click Atomic Rollback
// ============================================================================

fn testCanaryPromotionAndAtomicRollback()! {
    let reg = PromptVersionRegistry.init();
    const v1 = reg.register_version("v1", 1, "m1", "s1", 1, 0, 0, 100) ?? return error.RegFailed;
    const v2 = reg.register_version("v2", 2, "m1", "s2", 1, 1, 0, 200) ?? return error.RegFailed;
    const v3 = reg.register_version("v3", 3, "m1", "s3", 1, 2, 0, 300) ?? return error.RegFailed;

    expect_eq(reg.set_active(v1), true);
    expect_eq(reg.start_canary(v2, 1000), true);

    // Rollback while canary is running: v2 -> rolled_back (0 bps), v1 -> active (10000 bps)
    expect_eq(reg.rollback(), true);
    expect_eq(reg.entries[v2].status == PromptVariantStatus.rolled_back, true);
    expect_eq(reg.entries[v2].weight_bps, 0);
    expect_eq(reg.entries[v1].status == PromptVariantStatus.active, true);
    expect_eq(reg.entries[v1].weight_bps, BPS_SCALE);

    // Now canary v3 and promote it to active
    expect_eq(reg.start_canary(v3, 3000), true);
    expect_eq(reg.promote_canary(), true);
    expect_eq(reg.entries[v3].status == PromptVariantStatus.active, true);
    expect_eq(reg.entries[v3].weight_bps, BPS_SCALE);
    expect_eq(reg.entries[v1].status == PromptVariantStatus.deprecated, true);
    expect_eq(reg.entries[v1].weight_bps, 0);

    // Post-promotion rollback restores v1 -> active and marks v3 -> rolled_back
    expect_eq(reg.rollback(), true);
    expect_eq(reg.entries[v3].status == PromptVariantStatus.rolled_back, true);
    expect_eq(reg.entries[v1].status == PromptVariantStatus.active, true);
    expect_eq(reg.entries[v1].weight_bps, BPS_SCALE);

    // Second rollback with no canary and no prev_active returns false
    expect_eq(reg.rollback(), false);
}

// ============================================================================
// Test 4: Shadow Traffic Mirroring & Safe Fallback Degradation
// ============================================================================

fn testShadowMirrorAndSafeFallback()! {
    let reg = PromptVersionRegistry.init();

    // Empty registry returns found=false
    const empty_dec = reg.route_request("t1", 0);
    expect_eq(empty_dec.found, false);

    // Only a draft version exists -> safe fallback to draft with is_fallback=true
    const v1 = reg.register_version("draft-v1", 1, "m1", "s1", 0, 1, 0, 100) ?? return error.RegFailed;
    const fb_dec = reg.route_request("t1", 0);
    expect_eq(fb_dec.found, true);
    expect_eq(fb_dec.slot_idx, v1);
    expect_eq(fb_dec.is_fallback, true);

    // Activate v1 and attach v2 as shadow mirror
    const v2 = reg.register_version("shadow-v2", 2, "m2", "s2", 0, 2, 0, 200) ?? return error.RegFailed;
    expect_eq(reg.set_active(v1), true);
    expect_eq(reg.set_shadow(v1), false); // cannot shadow active version
    expect_eq(reg.set_shadow(v2), true);

    const live_dec = reg.route_request("t1", 0);
    expect_eq(live_dec.found, true);
    expect_eq(live_dec.slot_idx, v1);
    expect_eq(live_dec.is_fallback, false);
    expect_eq(live_dec.has_shadow, true);
    expect_eq(live_dec.shadow_slot_idx, v2);
}

// ============================================================================
// Test 5: Automated Rollback Guard on Error-Rate Breach
// ============================================================================

fn testAutoRollbackGuardErrorRate()! {
    let reg = PromptVersionRegistry.init();
    const v1 = reg.register_version("stable", 1, "m1", "s1", 1, 0, 0, 100) ?? return error.RegFailed;
    const v2 = reg.register_version("buggy-canary", 2, "m1", "s2", 1, 1, 0, 200) ?? return error.RegFailed;
    expect_eq(reg.set_active(v1), true);
    expect_eq(reg.start_canary(v2, 2000), true);

    // Guard: min_samples=10, max_error_bps=500 (5%), max_avg_latency_us=1_000_000 (1s)
    const guard = RolloutGuardConfig.init(10, 500, 1000000) ?? return error.BadGuard;

    // 9 failing calls (< min_samples=10): guard must NOT trigger prematurely
    let i: usize = 0;
    while (i < 9) : (i += 1) {
        expect_eq(reg.record_outcome(v2, false, 100000, 100), true);
    }
    expect_eq(reg.should_auto_rollback(guard), false);

    // 10th call fails -> 100% error rate >= min_samples -> triggers automatic rollback!
    expect_eq(reg.record_outcome(v2, false, 100000, 100), true);
    expect_eq(reg.should_auto_rollback(guard), true);
    expect_eq(reg.evaluate_and_auto_rollback(guard), true);

    expect_eq(reg.entries[v2].status == PromptVariantStatus.rolled_back, true);
    expect_eq(reg.entries[v1].status == PromptVariantStatus.active, true);
    expect_eq(reg.entries[v1].weight_bps, BPS_SCALE);
}

// ============================================================================
// Test 6: Automated Rollback Guard on Latency Breach & Invalid Guard Config
// ============================================================================

fn testAutoRollbackGuardLatencyAndInvalidConfig()! {
    expect_eq(RolloutGuardConfig.init(0, 500, 1000) == null, true);     // min_samples == 0
    expect_eq(RolloutGuardConfig.init(10, 10001, 1000) == null, true);  // max_error_bps > 10000
    expect_eq(RolloutGuardConfig.init(10, 500, 0) == null, true);       // max_avg_latency_us == 0

    let reg = PromptVersionRegistry.init();
    const v1 = reg.register_version("fast", 1, "m1", "s1", 1, 0, 0, 100) ?? return error.RegFailed;
    const v2 = reg.register_version("slow-canary", 2, "m1", "s2", 1, 1, 0, 200) ?? return error.RegFailed;
    expect_eq(reg.set_active(v1), true);
    expect_eq(reg.start_canary(v2, 1500), true);

    const guard = RolloutGuardConfig.init(5, 500, 500000) ?? return error.BadGuard; // 500ms latency budget

    // 5 successful calls (0% error) but 800ms average latency (> 500ms budget)
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        expect_eq(reg.record_outcome(v2, true, 800000, 200), true);
    }
    expect_eq(reg.variant_error_rate_bps(v2) ?? 9999, 0);
    expect_eq(reg.variant_avg_latency_us(v2) ?? 0, 800000);
    expect_eq(reg.evaluate_and_auto_rollback(guard), true);
    expect_eq(reg.entries[v2].status == PromptVariantStatus.rolled_back, true);
}

// ============================================================================
// Test 7: Telemetry Metrics & Summary Stats Aggregation
// ============================================================================

fn testTelemetryAndSummaryStats()! {
    let reg = PromptVersionRegistry.init();
    const v1 = reg.register_version("v1", 1, "m1", "s1", 1, 0, 0, 100) ?? return error.RegFailed;
    const v2 = reg.register_version("v2", 2, "m1", "s2", 1, 1, 0, 200) ?? return error.RegFailed;
    expect_eq(reg.set_active(v1), true);
    expect_eq(reg.start_canary(v2, 1000), true);

    // v1: 3 ok + 1 err = 2500 bps, avg latency = (100+200+300+400)/4 = 250
    expect_eq(reg.record_outcome(v1, true, 100, 50), true);
    expect_eq(reg.record_outcome(v1, true, 200, 60), true);
    expect_eq(reg.record_outcome(v1, true, 300, 70), true);
    expect_eq(reg.record_outcome(v1, false, 400, 80), true);

    // v2: 1 ok, latency = 500, tokens = 140
    expect_eq(reg.record_outcome(v2, true, 500, 140), true);

    expect_eq(reg.variant_error_rate_bps(v1) ?? 0, 2500);
    expect_eq(reg.variant_avg_latency_us(v1) ?? 0, 250);
    expect_eq(reg.variant_error_rate_bps(99) == null, true);
    expect_eq(reg.variant_avg_latency_us(99) == null, true);
    expect_eq(reg.record_outcome(99, true, 10, 10), false);

    const st: RegistrySummaryStats = reg.summary_stats();
    expect_eq(st.total_versions, 2);
    expect_eq(st.has_active, true);
    expect_eq(st.has_canary, true);
    expect_eq(st.total_calls, 5);
    expect_eq(st.total_successes, 4);
    expect_eq(st.total_errors, 1);
    expect_eq(st.total_tokens, 400);
    expect_eq(st.overall_error_rate_bps, 2000); // 1/5 = 20.00%
    expect_eq(st.overall_avg_latency_us, 300);  // 1500/5 = 300
}

test("prompt version_registry: registration, duplicate ID & capacity guards", testRegistrationDuplicateAndCapacityGuards);
test("prompt version_registry: deterministic sticky routing, monotonicity & 25% canary split", testDeterministicStickyAndCanarySplitDistribution);
test("prompt version_registry: canary promotion & one-click atomic rollback", testCanaryPromotionAndAtomicRollback);
test("prompt version_registry: shadow traffic mirroring & safe fallback degradation", testShadowMirrorAndSafeFallback);
test("prompt version_registry: automated rollback guard on error-rate breach", testAutoRollbackGuardErrorRate);
test("prompt version_registry: automated rollback guard on latency breach & invalid config", testAutoRollbackGuardLatencyAndInvalidConfig);
test("prompt version_registry: per-variant telemetry & summary_stats aggregation", testTelemetryAndSummaryStats);
