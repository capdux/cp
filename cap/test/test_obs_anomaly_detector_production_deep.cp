// Origin: Pure-CP Stdlib Test Suite (lib/obs/anomaly_detector.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Online Welford/EWMA Anomaly Detector & Cooldown Controller:
//   1. Warmup silence & exact Welford mean/stddev + EWMA tracking
//   2. Single-point glitch debouncing & outlier-resistant Welford baseline
//   3. Consecutive breaches -> AlertFired -> SuppressedCooldown -> post-cooldown re-alert
//   4. Constant zero-variance series protection (no NaN/Inf on 0/0 or diff/0)
//   5. Two-sided downward drop detection & clean reset()
//   6. NaN/Inf sample rejection & invalid AnomalyConfig defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AnomalyConfig,
    AnomalySeverity,
    AnomalyEval,
    AnomalyDetectorStats,
    OnlineAnomalyDetector,
} from "cap:obs";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

// ============================================================================
// Test 1: Warmup Silence & Exact Welford Mean / Stddev
// ============================================================================

fn testWarmupAndWelfordAccuracy()! {
    let det = cap.obs.OnlineAnomalyDetector.init(AnomalyConfig{
        warmup_samples: 4,
        z_threshold: 3.0,
        ewma_alpha: 0.5,
        consecutive_breaches: 2,
        cooldown_ms: 5000,
    }) ?? return error.InitFailed;

    // Samples: [10.0, 12.0, 14.0, 16.0] => mean = 13.0, sample variance = (9+1+1+9)/3 = 20/3, stddev = sqrt(20/3) ~= 2.5819889
    const vals: [4]f64 = [10.0, 12.0, 14.0, 16.0];
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        const ev = det.observe(vals[i], (i as i64) * 1000);
        expect_eq(ev.severity == AnomalySeverity.Warmup, true);
        expect_eq(ev.is_valid_sample, true);
    }

    expect_eq(absF64(det.baselineMean() - 13.0) < 0.0001, true);
    expect_eq(absF64(det.baselineStddev() - @sqrt(20.0 / 3.0)) < 0.0001, true);
    // EWMA (alpha=0.5): 10 -> 11 -> 12.5 -> 14.25
    expect_eq(absF64(det.ewma() - 14.25) < 0.0001, true);
    expect_eq(det.stats().alerts_fired, 0);
}

// ============================================================================
// Test 2: Single-Point Glitch Debouncing & Outlier-Resistant Baseline
// ============================================================================

fn testSingleGlitchDebounceAndBaselineProtection()! {
    let det = OnlineAnomalyDetector.init(AnomalyConfig{
        warmup_samples: 5,
        z_threshold: 3.0,
        ewma_alpha: 0.2,
        consecutive_breaches: 3,
        cooldown_ms: 10000,
    }) ?? return error.InitFailed;

    const base: [5]f64 = [100.0, 102.0, 98.0, 101.0, 99.0];
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        * = det.observe(base[i], (i as i64) * 100);
    }

    const mean_before = det.baselineMean();
    const std_before = det.baselineStddev();

    // Inject a massive single-point glitch (5000.0) -> Warning only, must NOT pollute Welford baseline!
    const glitch = det.observe(5000.0, 500);
    expect_eq(glitch.severity == AnomalySeverity.Warning, true);
    expect_eq(glitch.consecutive_count, 1);
    expect_eq(absF64(det.baselineMean() - mean_before) < 0.000001, true);
    expect_eq(absF64(det.baselineStddev() - std_before) < 0.000001, true);

    // Next normal sample resets breach streak to 0
    const rec = det.observe(100.0, 600);
    expect_eq(rec.severity == AnomalySeverity.Normal, true);
    expect_eq(rec.consecutive_count, 0);
    expect_eq(det.stats().alerts_fired, 0);
    expect_eq(det.stats().warning_count, 1);
}

// ============================================================================
// Test 3: Consecutive Breaches -> AlertFired -> Cooldown -> Re-Alert
// ============================================================================

fn testAlertEscalationCooldownAndReAlert()! {
    let det = OnlineAnomalyDetector.init(AnomalyConfig{
        warmup_samples: 4,
        z_threshold: 3.0,
        ewma_alpha: 0.2,
        consecutive_breaches: 3,
        cooldown_ms: 5000,
    }) ?? return error.InitFailed;

    const base: [4]f64 = [10.0, 11.0, 9.0, 10.0];
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        * = det.observe(base[i], (i as i64) * 1000);
    }

    // 1st & 2nd spikes -> Warning
    expect_eq(det.observe(50.0, 4000).severity == AnomalySeverity.Warning, true);
    expect_eq(det.observe(55.0, 5000).severity == AnomalySeverity.Warning, true);

    // 3rd consecutive spike -> AlertFired at t=6000!
    const a1 = det.observe(60.0, 6000);
    expect_eq(a1.severity == AnomalySeverity.AlertFired, true);
    expect_eq(a1.consecutive_count, 3);

    // 4th spike at t=9000 (3000ms < 5000ms cooldown) -> SuppressedCooldown!
    const sup = det.observe(65.0, 9000);
    expect_eq(sup.severity == AnomalySeverity.SuppressedCooldown, true);

    // 5th spike at t=11000 (5000ms >= 5000ms cooldown) -> AlertFired again!
    const a2 = det.observe(70.0, 11000);
    expect_eq(a2.severity == AnomalySeverity.AlertFired, true);

    const st = det.stats();
    expect_eq(st.warning_count, 2);
    expect_eq(st.alerts_fired, 2);
    expect_eq(st.alerts_suppressed, 1);
}

// ============================================================================
// Test 4: Constant Zero-Variance Series Protection
// ============================================================================

fn testConstantZeroVarianceProtection()! {
    let det = OnlineAnomalyDetector.init(AnomalyConfig{
        warmup_samples: 4,
        z_threshold: 3.0,
        ewma_alpha: 0.3,
        consecutive_breaches: 1,
        cooldown_ms: 1000,
    }) ?? return error.InitFailed;

    let i: usize = 0;
    while (i < 4) : (i += 1) {
        * = det.observe(42.0, (i as i64) * 100);
    }
    expect_eq(det.baselineStddev() == 0.0, true);

    // Same constant value 42.0 -> Normal, z_score == 0.0 (no NaN!)
    const ok = det.observe(42.0, 400);
    expect_eq(ok.severity == AnomalySeverity.Normal, true);
    expect_eq(!std.math.isNan(ok.z_score), true);
    expect_eq(ok.z_score == 0.0, true);

    // Deviation from constant baseline -> finite z_score > z_threshold -> AlertFired!
    const dev = det.observe(43.0, 500);
    expect_eq(dev.severity == AnomalySeverity.AlertFired, true);
    expect_eq(!std.math.isNan(dev.z_score) && !std.math.isInf(dev.z_score), true);
    expect_eq(dev.z_score > 3.0, true);
}

// ============================================================================
// Test 5: Downward Drop Anomaly & Clean Reset
// ============================================================================

fn testDownwardDropAndReset()! {
    let det = OnlineAnomalyDetector.init(AnomalyConfig{
        warmup_samples: 4,
        z_threshold: 3.0,
        ewma_alpha: 0.25,
        consecutive_breaches: 1,
        cooldown_ms: 0,
    }) ?? return error.InitFailed;

    const base: [4]f64 = [1000.0, 1005.0, 995.0, 1000.0];
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        * = det.observe(base[i], (i as i64) * 100);
    }

    // Sudden QPS collapse to 100.0 -> negative z_score, triggers AlertFired!
    const drop = det.observe(100.0, 400);
    expect_eq(drop.severity == AnomalySeverity.AlertFired, true);
    expect_eq(drop.z_score < -3.0, true);

    // Reset restores to fresh Warmup state
    det.reset();
    expect_eq(det.stats().total_samples, 0);
    expect_eq(det.stats().alerts_fired, 0);
    expect_eq(det.observe(50.0, 500).severity == AnomalySeverity.Warmup, true);
}

// ============================================================================
// Test 6: NaN / Inf & Invalid Config Defensive Guards
// ============================================================================

fn testNanInfAndInvalidConfigGuards()! {
    expect_eq(OnlineAnomalyDetector.init(AnomalyConfig{ warmup_samples: 1 }) == null, true);
    expect_eq(OnlineAnomalyDetector.init(AnomalyConfig{ z_threshold: 0.0 }) == null, true);
    expect_eq(OnlineAnomalyDetector.init(AnomalyConfig{ z_threshold: std.math.nan(f64) }) == null, true);
    expect_eq(OnlineAnomalyDetector.init(AnomalyConfig{ ewma_alpha: 0.0 }) == null, true);
    expect_eq(OnlineAnomalyDetector.init(AnomalyConfig{ ewma_alpha: 1.5 }) == null, true);
    expect_eq(OnlineAnomalyDetector.init(AnomalyConfig{ consecutive_breaches: 0 }) == null, true);
    expect_eq(OnlineAnomalyDetector.init(AnomalyConfig{ cooldown_ms: -1 }) == null, true);
    expect_eq(OnlineAnomalyDetector.init(AnomalyConfig{ min_stddev_eps: 0.0 }) == null, true);

    let det = OnlineAnomalyDetector.init(AnomalyConfig{
        warmup_samples: 3,
        z_threshold: 3.0,
        ewma_alpha: 0.5,
        consecutive_breaches: 1,
        cooldown_ms: 1000,
    }) ?? return error.InitFailed;

    * = det.observe(10.0, 0);
    * = det.observe(12.0, 100);
    * = det.observe(11.0, 200);

    // NaN, +Inf, -Inf must be rejected with is_valid_sample == false and leave mean finite
    expect_eq(det.observe(std.math.nan(f64), 300).is_valid_sample, false);
    expect_eq(det.observe(std.math.inf(f64), 400).is_valid_sample, false);
    expect_eq(det.observe(-std.math.inf(f64), 500).is_valid_sample, false);

    const st = det.stats();
    expect_eq(st.invalid_samples, 3);
    expect_eq(st.total_samples, 3);
    expect_eq(!std.math.isNan(det.baselineMean()), true);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("obs/anomaly_detector: warmup silence & Welford mean/stddev accuracy", testWarmupAndWelfordAccuracy);
test("obs/anomaly_detector: single-point glitch debounce & outlier-resistant baseline", testSingleGlitchDebounceAndBaselineProtection);
test("obs/anomaly_detector: consecutive breaches -> AlertFired -> cooldown -> re-alert", testAlertEscalationCooldownAndReAlert);
test("obs/anomaly_detector: constant zero-variance series protection", testConstantZeroVarianceProtection);
test("obs/anomaly_detector: two-sided downward drop detection & clean reset", testDownwardDropAndReset);
test("obs/anomaly_detector: NaN/Inf sample & invalid config defensive guards", testNanInfAndInvalidConfigGuards);
