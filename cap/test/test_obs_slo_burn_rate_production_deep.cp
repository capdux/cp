// Origin: Pure-CP Stdlib Test Suite (lib/obs/slo_burn_rate.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Google SRE Multi-Window Multi-Burn-Rate SLO Monitor:
//   1. Healthy traffic -> ok, 0 burn rate, 0 budget consumed & reset() reuse
//   2. Severe incident (1h + 5m >= 14.4x) -> page_critical_burn & exact hours_to_budget_exhaustion
//   3. Dual-window fast recovery: 5 clean minutes immediately clear page_critical_burn despite 1h tail
//   4. Sustained moderate fault -> page_high_burn (6h+30m >= 6x) & slow degradation -> ticket_slow_burn (>= 1x)
//   5. Low-traffic cold-start guard (min_samples_short_win) prevents false pages on tiny samples
//   6. Time jump > 360m zeroes stale ring buckets & rejects stale events older than 360 minutes
//   7. Invalid config guards (target <= 0 / >= 1, NaN, non-monotonic burn thresholds)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SLO_RING_MINUTES,
    SloAlertSeverity,
    SloBurnConfig,
    SloWindowMetrics,
    SloBurnEvaluation,
    SloBurnMonitor,
} from "cap:obs";

fn approxEq(a: f64, b: f64, tol: f64): bool {
    const d: f64 = if (a >= b) (a - b) else (b - a);
    return d <= tol;
}

// ============================================================================
// Test 1: Healthy Traffic -> ok, 0 Burn Rate & reset() Reuse
// ============================================================================

fn testHealthyTrafficAndReset()! {
    let mon = cap.obs.SloBurnMonitor.init(SloBurnConfig.defaultsThreeNines()) ?? return error.InitFailed;

    let m: u64 = 1000;
    while (m < 1060) : (m += 1) {
        expect_eq(mon.record_events(m, 1000, 0), true);
    }

    const ev: SloBurnEvaluation = mon.evaluate(1059);
    expect_eq(ev.severity == SloAlertSeverity.ok, true);
    expect_eq(ev.win_5m.total_events, 5000);
    expect_eq(ev.win_1h.total_events, 60000);
    expect_eq(approxEq(ev.win_1h.burn_rate_f64, 0.0, 1.0e-12), true);
    expect_eq(approxEq(ev.error_budget_consumed_ratio, 0.0, 1.0e-12), true);
    expect_eq(ev.hours_to_budget_exhaustion > 720.0, true);

    mon.reset();
    const after_reset = mon.evaluate(1059);
    expect_eq(after_reset.win_1h.total_events, 0);
    expect_eq(after_reset.severity == SloAlertSeverity.ok, true);
}

// ============================================================================
// Test 2: Severe Outage (1h + 5m >= 14.4x) -> page_critical_burn
// ============================================================================

fn testSevereOutageTriggersCriticalPage()! {
    let mon = SloBurnMonitor.init(SloBurnConfig.defaultsThreeNines()) ?? return error.InitFailed;

    // 99.9% SLO -> error budget = 0.001. 2.0% error rate => 20.0x burn rate (> 14.4x).
    let m: u64 = 2000;
    while (m < 2010) : (m += 1) {
        expect_eq(mon.record_events(m, 980, 20), true);
    }

    const ev = mon.evaluate(2009);
    expect_eq(ev.severity == SloAlertSeverity.page_critical_burn, true);
    expect_eq(approxEq(ev.win_5m.burn_rate_f64, 20.0, 1.0e-6), true);
    expect_eq(approxEq(ev.win_1h.burn_rate_f64, 20.0, 1.0e-6), true);
    // 720h / 20x = 36.0 hours until 30-day budget exhaustion
    expect_eq(approxEq(ev.hours_to_budget_exhaustion, 36.0, 1.0e-6), true);
}

// ============================================================================
// Test 3: Dual-Window Fast Recovery (5 Clean Minutes Immediately Clear Page)
// ============================================================================

fn testDualWindowFastRecovery()! {
    let mon = SloBurnMonitor.init(SloBurnConfig.defaultsThreeNines()) ?? return error.InitFailed;

    // Minutes 3000..3009: severe 3.0% error spike (30x burn)
    let m: u64 = 3000;
    while (m < 3010) : (m += 1) {
        expect_eq(mon.record_events(m, 970, 30), true);
    }
    expect_eq(mon.evaluate(3009).severity == SloAlertSeverity.page_critical_burn, true);

    // Minutes 3010..3014: incident mitigated -> 5 consecutive clean minutes (0 errors)
    m = 3010;
    while (m < 3015) : (m += 1) {
        expect_eq(mon.record_events(m, 1000, 0), true);
    }

    const ev = mon.evaluate(3014);
    // 1h window still has 20x burn (300/15000 = 2%), BUT 5m window is 0x -> no longer page_critical_burn!
    expect_eq(ev.win_1h.burn_rate_f64 > 14.4, true);
    expect_eq(approxEq(ev.win_5m.burn_rate_f64, 0.0, 1.0e-12), true);
    expect_eq(ev.severity != SloAlertSeverity.page_critical_burn, true);
}

// ============================================================================
// Test 4: High Burn (page_high_burn) & Slow Degradation (ticket_slow_burn)
// ============================================================================

fn testHighBurnAndSlowBurnTiers()! {
    // 4a. 0.8% error rate -> 8.0x burn (between 6.0x and 14.4x) -> page_high_burn
    let mon_high = SloBurnMonitor.init(SloBurnConfig.defaultsThreeNines()) ?? return error.InitFailed;
    let m: u64 = 4000;
    while (m < 4040) : (m += 1) {
        expect_eq(mon_high.record_events(m, 992, 8), true);
    }
    const ev_high = mon_high.evaluate(4039);
    expect_eq(ev_high.severity == SloAlertSeverity.page_high_burn, true);
    expect_eq(approxEq(ev_high.win_30m.burn_rate_f64, 8.0, 1.0e-6), true);
    expect_eq(approxEq(ev_high.win_6h.burn_rate_f64, 8.0, 1.0e-6), true);
    // 6h at 8x burn consumes 8 * 6 / 720 = 1/15 ≈ 0.066667 of 30d budget
    expect_eq(approxEq(ev_high.error_budget_consumed_ratio, 8.0 / 120.0, 1.0e-6), true);

    // 4b. 0.2% error rate -> 2.0x burn (between 1.0x and 6.0x) -> ticket_slow_burn
    let mon_slow = SloBurnMonitor.init(SloBurnConfig.defaultsThreeNines()) ?? return error.InitFailed;
    m = 5000;
    while (m < 5040) : (m += 1) {
        expect_eq(mon_slow.record_events(m, 998, 2), true);
    }
    const ev_slow = mon_slow.evaluate(5039);
    expect_eq(ev_slow.severity == SloAlertSeverity.ticket_slow_burn, true);
    expect_eq(approxEq(ev_slow.win_6h.burn_rate_f64, 2.0, 1.0e-6), true);
}

// ============================================================================
// Test 5: Cold-Start Low-Sample Guard (min_samples_short_win)
// ============================================================================

fn testColdStartLowSampleGuard()! {
    let cfg = SloBurnConfig.defaultsThreeNines();
    cfg.min_samples_short_win = 50;
    let mon = SloBurnMonitor.init(cfg) ?? return error.InitFailed;

    // 10 requests with 100% error rate (1000x burn), but total 10 < 50 -> suppressed to ok!
    expect_eq(mon.record_events(6000, 0, 10), true);
    const ev_low = mon.evaluate(6000);
    expect_eq(ev_low.win_5m.total_events, 10);
    expect_eq(ev_low.severity == SloAlertSeverity.ok, true);

    // Adding 40 more failing events reaches 50 -> immediately fires page_critical_burn
    expect_eq(mon.record_events(6001, 0, 40), true);
    const ev_fire = mon.evaluate(6001);
    expect_eq(ev_fire.win_5m.total_events, 50);
    expect_eq(ev_fire.severity == SloAlertSeverity.page_critical_burn, true);
}

// ============================================================================
// Test 6: Time Jump > 360 Minutes Clears Stale Ring & Rejects Stale Events
// ============================================================================

fn testTimeJumpRingResetAndStaleRejection()! {
    let mon = SloBurnMonitor.init(SloBurnConfig.defaultsThreeNines()) ?? return error.InitFailed;

    expect_eq(mon.record_events(1000, 900, 100), true);

    // Jump forward by 400 minutes (> SLO_RING_MINUTES = 360) -> old bucket completely cleared
    expect_eq(mon.record_events(1400, 1000, 0), true);
    const w6h: SloWindowMetrics = mon.window_metrics(1400, SLO_RING_MINUTES);
    expect_eq(w6h.good_events, 1000);
    expect_eq(w6h.bad_events, 0);

    // Event at minute 1040 is 360 minutes old (>= 360) -> safely rejected
    expect_eq(mon.record_events(1040, 100, 100), false);
    // Event at minute 1041 is 359 minutes old (< 360) -> accepted inside window
    expect_eq(mon.record_events(1041, 50, 0), true);
    expect_eq(mon.window_metrics(1400, SLO_RING_MINUTES).good_events, 1050);
}

// ============================================================================
// Test 7: Invalid Config Boundary Guards
// ============================================================================

fn testInvalidConfigGuards()! {
    const nan_f: f64 = std.math.nan(f64);
    expect_eq(SloBurnConfig.defaultsThreeNines().isValid(), true);

    expect_eq(SloBurnMonitor.init(SloBurnConfig{ target_availability_f64: 1.0 }) == null, true);
    expect_eq(SloBurnMonitor.init(SloBurnConfig{ target_availability_f64: 0.0 }) == null, true);
    expect_eq(SloBurnMonitor.init(SloBurnConfig{ target_availability_f64: -0.5 }) == null, true);
    expect_eq(SloBurnMonitor.init(SloBurnConfig{ target_availability_f64: nan_f }) == null, true);
    expect_eq(SloBurnMonitor.init(SloBurnConfig{ slow_burn_rate: 0.0 }) == null, true);
    expect_eq(SloBurnMonitor.init(SloBurnConfig{ slow_burn_rate: 5.0, high_burn_rate: 3.0 }) == null, true);
    expect_eq(SloBurnMonitor.init(SloBurnConfig{ high_burn_rate: 10.0, critical_burn_rate: 8.0 }) == null, true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("obs slo_burn_rate: healthy traffic returns ok with 0 burn rate & reset clears ring", testHealthyTrafficAndReset);
test("obs slo_burn_rate: severe outage (1h+5m >= 14.4x) triggers page_critical_burn", testSevereOutageTriggersCriticalPage);
test("obs slo_burn_rate: dual-window clears page_critical_burn immediately after 5 clean minutes", testDualWindowFastRecovery);
test("obs slo_burn_rate: sustained 8x triggers page_high_burn & 2x triggers ticket_slow_burn", testHighBurnAndSlowBurnTiers);
test("obs slo_burn_rate: min_samples_short_win suppresses cold-start low-traffic false alerts", testColdStartLowSampleGuard);
test("obs slo_burn_rate: >360m time jump zeroes expired buckets & rejects stale events", testTimeJumpRingResetAndStaleRejection);
test("obs slo_burn_rate: rejects invalid target_availability and non-monotonic burn thresholds", testInvalidConfigGuards);
