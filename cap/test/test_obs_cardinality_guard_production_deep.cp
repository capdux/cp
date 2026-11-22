// Origin: Pure-CP Stdlib Test Suite (lib/obs/cardinality_guard.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Prometheus / OpenTelemetry High-Cardinality Guard:
//   1. Admission fast-path for existing series + automatic __overflow__ bucket rollup + label length guard
//   2. Drop vs warn_only overflow policies
//   3. 64-bucket HyperLogLog (@clz) true-cardinality estimation after overflow cap engagement
//   4. TTL cold-series eviction (evictStaleSeries) + tombstone slot reuse + quota recovery
//   5. Per-metric window reset (resetMetricWindow) isolation across multiple metrics
//   6. Global series table exhaustion (CARD_MAX_SERIES = 1024) guard & recovery
//   7. Boundary guards: invalid quota, duplicate metric_id, full metric table (32), unknown metric
//   8. FNV-1a fingerprint determinism, cross-metric isolation & CardinalityGuardStats accounting invariant

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CARD_MAX_METRICS,
    CARD_MAX_SERIES,
    CARD_HLL_REGISTERS,
    CARD_OVERFLOW_LABEL,
    CardinalityOverflowPolicy,
    CardinalityDecision,
    MetricQuotaConfig,
    MetricCardinalitySnapshot,
    CardinalityGuardStats,
    fnv1aHash64,
    computeSeriesFingerprint,
    labelValueOrOverflow,
    CardinalityGuard,
} from "cap:obs";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Existing Series Fast-Path + __overflow__ Rollup + Label Length Guard
// ============================================================================

fn testRollupOverflowAndExistingFastPath()! {
    let guard = cap.obs.CardinalityGuard.init();
    const cfg = MetricQuotaConfig{
        metric_id: 101,
        max_series: 4,
        max_label_value_len: 16,
        overflow_policy: CardinalityOverflowPolicy.rollup_overflow_label,
    };
    expect_eq(guard.registerMetric(cfg), true);

    // Admit 4 distinct series -> all allow_new
    let i: u64 = 1;
    while (i <= 4) : (i += 1) {
        const decision = guard.admitSeries(101, i * 1000, 1000 + i);
        expect_eq(decision == CardinalityDecision.allow_new, true);
    }

    // 5th and 6th new series exceed quota -> rewritten to __overflow__
    expect_eq(guard.admitSeries(101, 5000, 2000) == CardinalityDecision.rewrite_to_overflow, true);
    expect_eq(guard.admitSeries(101, 6000, 2001) == CardinalityDecision.rewrite_to_overflow, true);

    // Existing 4 series remain completely unthrottled (allow_existing)
    let j: u64 = 1;
    while (j <= 4) : (j += 1) {
        expect_eq(guard.admitSeries(101, j * 1000, 3000 + j) == CardinalityDecision.allow_existing, true);
    }

    if (snap := guard.getMetricSnapshot(101)) {
        expect_eq(snap.active_series, 4);
        expect_eq(snap.overflow_active, true);
        expect_eq(snap.overflow_hits, 2);
        expect_eq(snap.dropped_hits, 0);
        expect_eq(snap.total_admits, 10);
    } else {
        expect_eq(true, false);
    }

    // Label length guard rewrites oversized value to "__overflow__"
    if (ok_lbl := guard.guardLabelValue(101, "region=us-east")) {
        expect_eq(bytesEq(ok_lbl, "region=us-east"), true);
    } else {
        expect_eq(true, false);
    }
    if (ov_lbl := guard.guardLabelValue(101, "user_id=extremely_long_high_cardinality_uuid_12345")) {
        expect_eq(bytesEq(ov_lbl, CARD_OVERFLOW_LABEL), true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(bytesEq(labelValueOrOverflow("short", 8), "short"), true);
    expect_eq(bytesEq(labelValueOrOverflow("too_long_label", 8), "__overflow__"), true);
}

// ============================================================================
// Test 2: Drop Policy vs Warn-Only Policy
// ============================================================================

fn testDropAndWarnOnlyPolicies()! {
    let guard = CardinalityGuard.init();
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultDrop(10, 3)), true);
    expect_eq(guard.registerMetric(MetricQuotaConfig{
        metric_id: 20,
        max_series: 2,
        max_label_value_len: 64,
        overflow_policy: CardinalityOverflowPolicy.warn_only,
    }), true);

    // Drop policy: 3 allowed, 4th dropped, existing still allowed
    expect_eq(guard.admitSeries(10, 1, 100) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(10, 2, 100) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(10, 3, 100) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(10, 4, 100) == CardinalityDecision.drop_over_limit, true);
    expect_eq(guard.admitSeries(10, 2, 105) == CardinalityDecision.allow_existing, true);

    if (s10 := guard.getMetricSnapshot(10)) {
        expect_eq(s10.active_series, 3);
        expect_eq(s10.dropped_hits, 1);
        expect_eq(s10.overflow_hits, 0);
    } else {
        expect_eq(true, false);
    }

    // Warn-only policy: 4 distinct series all admitted, last 2 increment warn_hits
    expect_eq(guard.admitSeries(20, 11, 100) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(20, 12, 100) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(20, 13, 100) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(20, 14, 100) == CardinalityDecision.allow_new, true);

    if (s20 := guard.getMetricSnapshot(20)) {
        expect_eq(s20.active_series, 4);
        expect_eq(s20.warn_hits, 2);
        expect_eq(s20.dropped_hits, 0);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 3: 64-Bucket HyperLogLog (@clz) True-Cardinality Estimation Under Explosion
// ============================================================================

fn testHyperLogLogTrueCardinalityUnderExplosion()! {
    let guard = CardinalityGuard.init();
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultRollup(200, 16)), true);

    // Flood 500 distinct series into a metric capped at max_series = 16
    let i: u64 = 1;
    while (i <= 500) : (i += 1) {
        const decision = guard.admitSeries(200, i * 2654435761, 1000 + i);
        if (i <= 16) {
            expect_eq(decision == CardinalityDecision.allow_new, true);
        } else {
            expect_eq(decision == CardinalityDecision.rewrite_to_overflow, true);
        }
    }

    const est1: f64 = guard.estimateMetricCardinality(200) ?? 0.0;
    // With m = 64 registers (SE ~13%), 500 true uniques should estimate cleanly within [340, 700]
    expect_eq(est1 >= 340.0 && est1 <= 700.0, true);

    // Replay first 100 fingerprints: HLL estimate must remain idempotent (exact same value)
    let r: u64 = 1;
    while (r <= 100) : (r += 1) {
        * = guard.admitSeries(200, r * 2654435761, 5000 + r);
    }
    const est2: f64 = guard.estimateMetricCardinality(200) ?? 0.0;
    expect_eq(est2 == est1, true);

    if (snap := guard.getMetricSnapshot(200)) {
        expect_eq(snap.active_series, 16);
        expect_eq(snap.overflow_active, true);
        expect_eq(snap.overflow_hits, 484 + 84); // 484 initial + 84 replayed overflow series
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 4: TTL Cold-Series Eviction & Tombstone Reuse
// ============================================================================

fn testTtlEvictionAndTombstoneReuse()! {
    let guard = CardinalityGuard.init();
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultDrop(300, 3)), true);

    expect_eq(guard.admitSeries(300, 10, 1000) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(300, 20, 1500) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(300, 30, 2000) == CardinalityDecision.allow_new, true);
    // Quota full
    expect_eq(guard.admitSeries(300, 40, 2500) == CardinalityDecision.drop_over_limit, true);

    // Refresh series 20 so it stays hot at t = 5000
    expect_eq(guard.admitSeries(300, 20, 5000) == CardinalityDecision.allow_existing, true);

    // Evict series with last_seen_ms < 3000 -> evicts 10 and 30, preserves 20
    const evicted = guard.evictStaleSeries(3000);
    expect_eq(evicted, 2);

    const st_after_evict = guard.stats();
    expect_eq(st_after_evict.active_series_total, 1);
    expect_eq(st_after_evict.tombstone_slots, 2);
    expect_eq(st_after_evict.evicted_series_total, 2);

    // Now 2 new series can be admitted again, reusing tombstones
    expect_eq(guard.admitSeries(300, 40, 5100) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(300, 50, 5200) == CardinalityDecision.allow_new, true);
    // Series 20 is still existing
    expect_eq(guard.admitSeries(300, 20, 5300) == CardinalityDecision.allow_existing, true);
    // Quota full again
    expect_eq(guard.admitSeries(300, 60, 5400) == CardinalityDecision.drop_over_limit, true);
}

// ============================================================================
// Test 5: Per-Metric Window Reset Isolation
// ============================================================================

fn testResetMetricWindowIsolation()! {
    let guard = CardinalityGuard.init();
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultRollup(1, 2)), true);
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultRollup(2, 2)), true);

    // Fill both metrics and trigger overflow on both
    * = guard.admitSeries(1, 101, 100);
    * = guard.admitSeries(1, 102, 100);
    expect_eq(guard.admitSeries(1, 103, 100) == CardinalityDecision.rewrite_to_overflow, true);

    * = guard.admitSeries(2, 201, 100);
    * = guard.admitSeries(2, 202, 100);
    expect_eq(guard.admitSeries(2, 203, 100) == CardinalityDecision.rewrite_to_overflow, true);

    // Reset only metric 1
    expect_eq(guard.resetMetricWindow(1), true);
    expect_eq(guard.resetMetricWindow(999), false);

    if (s1 := guard.getMetricSnapshot(1)) {
        expect_eq(s1.active_series, 0);
        expect_eq(s1.overflow_active, false);
        expect_eq(s1.estimated_true_cardinality == 0.0, true);
    } else {
        expect_eq(true, false);
    }

    // Metric 2 must be untouched
    if (s2 := guard.getMetricSnapshot(2)) {
        expect_eq(s2.active_series, 2);
        expect_eq(s2.overflow_active, true);
        expect_eq(s2.estimated_true_cardinality > 1.5, true);
    } else {
        expect_eq(true, false);
    }

    // Metric 1 immediately accepts fresh series again
    expect_eq(guard.admitSeries(1, 103, 200) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(2, 201, 200) == CardinalityDecision.allow_existing, true);
}

// ============================================================================
// Test 6: Global Series Table Exhaustion (CARD_MAX_SERIES = 1024) & Recovery
// ============================================================================

fn testGlobalSeriesTableExhaustionAndRecovery()! {
    let guard = CardinalityGuard.init();
    expect_eq(guard.registerMetric(MetricQuotaConfig{
        metric_id: 777,
        max_series: 2000,
        max_label_value_len: 128,
        overflow_policy: CardinalityOverflowPolicy.warn_only,
    }), true);

    // Fill all 1024 global slots
    let i: usize = 0;
    while (i < CARD_MAX_SERIES) : (i += 1) {
        const decision = guard.admitSeries(777, (i as u64) + 1, 1000 + (i as u64));
        expect_eq(decision == CardinalityDecision.allow_new, true);
    }
    expect_eq(guard.stats().active_series_total, CARD_MAX_SERIES);

    // 1025th distinct series must safely drop (never infinite-loop or corrupt table)
    expect_eq(guard.admitSeries(777, 999999, 5000) == CardinalityDecision.drop_over_limit, true);
    // Existing series still found in full table
    expect_eq(guard.admitSeries(777, 1, 6000) == CardinalityDecision.allow_existing, true);

    // Evict older entries (last_seen_ms < 1010) -> frees slots 1..9 (9 entries since id=1 was refreshed to 6000)
    const freed = guard.evictStaleSeries(1010);
    expect_eq(freed, 9);
    // Re-admit 999999 now succeeds
    expect_eq(guard.admitSeries(777, 999999, 6001) == CardinalityDecision.allow_new, true);
}

// ============================================================================
// Test 7: Boundary Guards (Invalid Quota, Duplicate ID, Full Metric Table, Unknown Metric)
// ============================================================================

fn testBoundaryGuardsRegistrationAndUnknownMetric()! {
    let guard = CardinalityGuard.init();

    // Invalid configs
    expect_eq(guard.registerMetric(MetricQuotaConfig{
        metric_id: 1,
        max_series: 0,
        max_label_value_len: 64,
        overflow_policy: CardinalityOverflowPolicy.drop,
    }), false);
    expect_eq(guard.registerMetric(MetricQuotaConfig{
        metric_id: 1,
        max_series: 10,
        max_label_value_len: 0,
        overflow_policy: CardinalityOverflowPolicy.drop,
    }), false);

    // Fill all CARD_MAX_METRICS = 32 slots
    let m: usize = 0;
    while (m < CARD_MAX_METRICS) : (m += 1) {
        expect_eq(guard.registerMetric(MetricQuotaConfig.defaultRollup((m as u32) + 1, 8)), true);
    }
    // Duplicate ID rejected
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultRollup(1, 16)), false);
    // 33rd metric rejected (table full)
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultRollup(999, 8)), false);

    // Unknown metric guards
    expect_eq(guard.admitSeries(999, 42, 100) == CardinalityDecision.unknown_metric, true);
    expect_eq(guard.getMetricSnapshot(999) == null, true);
    expect_eq(guard.estimateMetricCardinality(999) == null, true);
    expect_eq(guard.guardLabelValue(999, "foo") == null, true);

    // Constant sanity checks
    expect_eq(CARD_MAX_METRICS, 32);
    expect_eq(CARD_MAX_SERIES, 1024);
    expect_eq(CARD_HLL_REGISTERS, 64);
}

// ============================================================================
// Test 8: FNV-1a Fingerprint Determinism, Cross-Metric Isolation & Stats Invariant
// ============================================================================

fn testFingerprintsIsolationAndStatsAccounting()! {
    const fp_a1 = computeSeriesFingerprint(1, "method=GET,status=200");
    const fp_a2 = computeSeriesFingerprint(1, "method=GET,status=200");
    const fp_b = computeSeriesFingerprint(2, "method=GET,status=200");
    expect_eq(fp_a1 == fp_a2, true);
    expect_eq(fp_a1 != fp_b, true);
    expect_eq(fnv1aHash64("abc") == fnv1aHash64("abc"), true);
    expect_eq(fnv1aHash64("abc") != fnv1aHash64("abd"), true);

    let guard = CardinalityGuard.init();
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultRollup(1, 2)), true);
    expect_eq(guard.registerMetric(MetricQuotaConfig.defaultDrop(2, 1)), true);

    // Same raw fingerprint 42 admitted to metric 1 and metric 2 independently
    expect_eq(guard.admitSeries(1, 42, 10) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(2, 42, 10) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(1, 42, 20) == CardinalityDecision.allow_existing, true);
    expect_eq(guard.admitSeries(1, 43, 20) == CardinalityDecision.allow_new, true);
    expect_eq(guard.admitSeries(1, 44, 20) == CardinalityDecision.rewrite_to_overflow, true);
    expect_eq(guard.admitSeries(2, 43, 20) == CardinalityDecision.drop_over_limit, true);
    expect_eq(guard.admitSeries(99, 42, 20) == CardinalityDecision.unknown_metric, true);

    const st: CardinalityGuardStats = guard.stats();
    expect_eq(st.registered_metrics, 2);
    expect_eq(st.active_series_total, 3);
    expect_eq(st.allow_new_count, 3);
    expect_eq(st.allow_existing_count, 1);
    expect_eq(st.overflow_rollup_count, 1);
    expect_eq(st.dropped_count, 1);
    expect_eq(st.unknown_metric_count, 1);
    expect_eq(
        st.total_admits,
        st.allow_existing_count + st.allow_new_count + st.overflow_rollup_count + st.dropped_count + st.unknown_metric_count,
    );
}

// ============================================================================
// Suite Registration
// ============================================================================

test("obs.cardinality_guard: existing fast-path, __overflow__ rollup & label length guard", testRollupOverflowAndExistingFastPath);
test("obs.cardinality_guard: drop vs warn_only overflow policies", testDropAndWarnOnlyPolicies);
test("obs.cardinality_guard: 64-bucket HyperLogLog (@clz) true-cardinality estimation", testHyperLogLogTrueCardinalityUnderExplosion);
test("obs.cardinality_guard: TTL cold-series eviction & tombstone reuse", testTtlEvictionAndTombstoneReuse);
test("obs.cardinality_guard: per-metric window reset isolation", testResetMetricWindowIsolation);
test("obs.cardinality_guard: global series table exhaustion (1024) & recovery", testGlobalSeriesTableExhaustionAndRecovery);
test("obs.cardinality_guard: invalid quota, duplicate ID, full metric table & unknown metric guards", testBoundaryGuardsRegistrationAndUnknownMetric);
test("obs.cardinality_guard: FNV-1a fingerprints, cross-metric isolation & stats invariant", testFingerprintsIsolationAndStatsAccounting);
