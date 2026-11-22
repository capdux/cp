// Origin: Pure-CP Stdlib Test Suite (lib/obs/canaryGate.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Kayenta/Flagger-Style Canary Statistical Gate Judge:
//   1. Two-sample Welch t-test + relative regression joint criterion (lower_is_better & higher_is_better)
//   2. Dual-gate false-alarm suppression: noisy high-variance non-significant shift & tiny sub-budget shift
//   3. Zero-variance deterministic regression (0% -> 2% errors), NaN/Inf filtering & insufficient_data guard
//   4. Weighted multi-KPI scoring verdicts: pass_promote, marginal_hold & score-based fail_rollback
//   5. Critical metric veto (`fail_rollback` despite high score) & critical insufficient_data holdback
//   6. Zero-alloc JSON summary formatting, 16-metric capacity cap & `cap.obs` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CANARY_MAX_METRICS,
    CANARY_BPS_SCALE,
    CanaryDirection,
    CanaryMetricStatus,
    CanaryVerdict,
    CanaryMetricSpec,
    CanaryGateConfig,
    CanaryJudgeReport,
    evaluateMetricSample,
    formatCanaryVerdict,
    formatCanaryReportJson,
    CanaryGateJudge,
} from "cap:obs";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("CanaryGate: Welch t-test + relative regression for lower_is_better & higher_is_better") {
    // 1. P99 latency (lower_is_better): baseline ~40ms, canary ~50ms (+25% > 5% budget, t >> 1.96) -> regressed
    const lat_spec = CanaryMetricSpec.make("p99_latency", CanaryDirection.lower_is_better, 5, 500, 1.96, true, 5);
    const b_lat: [6]f64 = [39.5, 40.0, 40.2, 40.5, 39.8, 40.0];
    const c_lat_bad: [6]f64 = [49.5, 50.1, 50.3, 49.8, 50.2, 50.1];
    const r_lat = evaluateMetricSample(lat_spec, b_lat[0..], c_lat_bad[0..]);
    expect_eq(r_lat.status, CanaryMetricStatus.regressed);
    expect_true(r_lat.statistically_significant);
    expect_true(r_lat.exceeds_regression_budget);
    expect_true(r_lat.welch_t > 10.0);
    expect_true(r_lat.regression_bps >= 2400 and r_lat.regression_bps <= 2600);

    // Improvement (canary ~32ms < baseline ~40ms) -> passed with negative regression_bps
    const c_lat_good: [6]f64 = [31.8, 32.1, 32.0, 31.9, 32.2, 32.0];
    const r_good = evaluateMetricSample(lat_spec, b_lat[0..], c_lat_good[0..]);
    expect_eq(r_good.status, CanaryMetricStatus.passed);
    expect_true(r_good.regression_bps < 0);

    // 2. Throughput QPS (higher_is_better): baseline ~1000, canary ~850 (-15% > 5% budget) -> regressed
    const qps_spec = CanaryMetricSpec.make("qps", CanaryDirection.higher_is_better, 3, 500, 1.96, false, 5);
    const b_qps: [6]f64 = [995.0, 1002.0, 1005.0, 998.0, 1000.0, 1000.0];
    const c_qps_bad: [6]f64 = [848.0, 852.0, 850.0, 849.0, 851.0, 850.0];
    const r_qps = evaluateMetricSample(qps_spec, b_qps[0..], c_qps_bad[0..]);
    expect_eq(r_qps.status, CanaryMetricStatus.regressed);
    expect_true(r_qps.regression_bps >= 1400 and r_qps.regression_bps <= 1600);
}

test("CanaryGate: dual-gate false-alarm suppression (high-variance noise & sub-budget micro-shift)") {
    const spec = CanaryMetricSpec.make("cpu_pct", CanaryDirection.lower_is_better, 2, 500, 1.96, false, 5);

    // Case A: High variance noise where mean is +10% higher (exceeds 5% budget) but t < 1.96 -> passed!
    const noisy_base: [5]f64 = [10.0, 90.0, 20.0, 80.0, 50.0];   // mean = 50, huge variance
    const noisy_canary: [5]f64 = [15.0, 95.0, 25.0, 85.0, 55.0]; // mean = 55 (+10%), t ~= 0.22
    const r_noise = evaluateMetricSample(spec, noisy_base[0..], noisy_canary[0..]);
    expect_true(r_noise.exceeds_regression_budget);
    expect_true(!r_noise.statistically_significant);
    expect_eq(r_noise.status, CanaryMetricStatus.passed);

    // Case B: Statistically significant (+1%, zero variance -> t=100) but within 5% budget -> passed!
    const flat_base: [5]f64 = [100.0, 100.0, 100.0, 100.0, 100.0];
    const flat_canary: [5]f64 = [101.0, 101.0, 101.0, 101.0, 101.0]; // +100 bps <= 500 bps
    const r_small = evaluateMetricSample(spec, flat_base[0..], flat_canary[0..]);
    expect_true(r_small.statistically_significant);
    expect_true(!r_small.exceeds_regression_budget);
    expect_eq(r_small.status, CanaryMetricStatus.passed);
}

test("CanaryGate: zero-variance 0->nonzero regression, NaN/Inf filtering & insufficient_data") {
    const zero: f64 = 0.0;
    const nan_val: f64 = zero / zero;
    const inf_val: f64 = 1.0 / zero;

    const err_spec = CanaryMetricSpec.make("error_rate", CanaryDirection.lower_is_better, 5, 100, 1.96, true, 4);

    // Zero-variance baseline 0.0 vs constant canary 0.02 -> deterministic regression detected
    const z_base: [4]f64 = [0.0, 0.0, 0.0, 0.0];
    const z_bad: [4]f64 = [0.02, 0.02, 0.02, 0.02];
    const r_zero = evaluateMetricSample(err_spec, z_base[0..], z_bad[0..]);
    expect_eq(r_zero.status, CanaryMetricStatus.regressed);
    expect_true(r_zero.welch_t >= 99.0);

    // NaN/Inf samples are filtered out; if remaining finite count < min_samples -> insufficient_data
    const dirty: [5]f64 = [0.0, nan_val, inf_val, 0.0, 0.0]; // only 3 finite < min_samples(4)
    const r_insuff = evaluateMetricSample(err_spec, dirty[0..], z_base[0..]);
    expect_eq(r_insuff.status, CanaryMetricStatus.insufficient_data);
    expect_eq(r_insuff.baseline_count, @as(usize, 3));
}

test("CanaryGate: weighted multi-KPI scoring (pass_promote, marginal_hold, fail_rollback)") {
    let judge = CanaryGateJudge.init();
    const m0 = judge.addMetric(CanaryMetricSpec.make("m0", CanaryDirection.lower_is_better, 7, 500, 1.96, false, 4)) ?? return;
    const m1 = judge.addMetric(CanaryMetricSpec.make("m1", CanaryDirection.lower_is_better, 2, 500, 1.96, false, 4)) ?? return;
    const m2 = judge.addMetric(CanaryMetricSpec.make("m2", CanaryDirection.lower_is_better, 1, 500, 1.96, false, 4)) ?? return;

    const ok_b: [4]f64 = [10.0, 10.1, 9.9, 10.0];
    const ok_c: [4]f64 = [10.0, 10.0, 10.1, 9.9];
    const bad_c: [4]f64 = [15.0, 15.1, 14.9, 15.0];

    const cfg = CanaryGateConfig.defaults(); // pass>=8500, marginal>=6500

    // All pass -> score 10000 -> pass_promote
    * = judge.recordMetricSamples(m0, ok_b[0..], ok_c[0..]);
    * = judge.recordMetricSamples(m1, ok_b[0..], ok_c[0..]);
    * = judge.recordMetricSamples(m2, ok_b[0..], ok_c[0..]);
    const r_pass = judge.evaluateCanary(cfg);
    expect_eq(r_pass.verdict, CanaryVerdict.pass_promote);
    expect_eq(r_pass.score_bps, CANARY_BPS_SCALE);

    // m1 (weight 2/10) regresses -> score 8000 in [6500, 8500) -> marginal_hold
    * = judge.recordMetricSamples(m1, ok_b[0..], bad_c[0..]);
    const r_marg = judge.evaluateCanary(cfg);
    expect_eq(r_marg.verdict, CanaryVerdict.marginal_hold);
    expect_eq(r_marg.score_bps, @as(u32, 8000));

    // m0 (weight 7/10) regresses -> score 1000 < 6500 -> fail_rollback
    * = judge.recordMetricSamples(m0, ok_b[0..], bad_c[0..]);
    const r_fail = judge.evaluateCanary(cfg);
    expect_eq(r_fail.verdict, CanaryVerdict.fail_rollback);
    expect_eq(r_fail.score_bps, @as(u32, 1000));
}

test("CanaryGate: critical metric veto & insufficient critical metric holdback") {
    let judge = CanaryGateJudge.init();
    const m_big = judge.addMetric(CanaryMetricSpec.make("throughput", CanaryDirection.higher_is_better, 9, 500, 1.96, false, 4)) ?? return;
    const m_crit = judge.addMetric(CanaryMetricSpec.make("5xx_errors", CanaryDirection.lower_is_better, 1, 100, 1.96, true, 4)) ?? return;

    const ok_b: [4]f64 = [100.0, 100.2, 99.8, 100.0];
    const ok_c: [4]f64 = [100.1, 99.9, 100.0, 100.0];
    const err_b: [4]f64 = [0.001, 0.001, 0.001, 0.001];
    const err_bad: [4]f64 = [0.05, 0.051, 0.049, 0.05];

    // Critical metric missing samples -> insufficient_data despite 90% weight passing
    * = judge.recordMetricSamples(m_big, ok_b[0..], ok_c[0..]);
    const r_ins = judge.evaluateCanary(CanaryGateConfig.defaults());
    expect_eq(r_ins.verdict, CanaryVerdict.insufficient_data);

    // Critical metric regresses -> score is 9000 (>=8500) yet critical veto forces fail_rollback!
    * = judge.recordMetricSamples(m_crit, err_b[0..], err_bad[0..]);
    const r_veto: CanaryJudgeReport = judge.evaluateCanary(CanaryGateConfig.defaults());
    expect_eq(r_veto.verdict, CanaryVerdict.fail_rollback);
    expect_eq(r_veto.score_bps, @as(u32, 9000));
    expect_eq(r_veto.critical_failures, @as(usize, 1));
    expect_true(r_veto.has_first_failed);
    expect_true(bytesEq(r_veto.firstFailedNameSlice(), "5xx_errors"));
}

test("CanaryGate: zero-alloc JSON report, 16-metric cap & cap.obs facade parity") {
    let judge = cap.obs.CanaryGateJudge.init();
    const ok_b: [4]f64 = [10.0, 10.1, 9.9, 10.0];
    const bad_c: [4]f64 = [20.0, 20.1, 19.9, 20.0];

    const m0 = judge.addMetric(
        cap.obs.CanaryMetricSpec.make("latency_p99", CanaryDirection.lower_is_better, 1, 500, 1.96, true, 4),
    ) ?? return;
    * = judge.recordMetricSamples(m0, ok_b[0..], bad_c[0..]);
    const rep = judge.evaluateCanary(cap.obs.CanaryGateConfig.defaults());

    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];
    const n = cap.obs.formatCanaryReportJson(rep, *buf_s) ?? 0;
    const json = buf[0..n];
    expect_true(containsSubstr(json, "\"verdict\":\"fail_rollback\""));
    expect_true(containsSubstr(json, "\"score_bps\":0"));
    expect_true(containsSubstr(json, "\"critical_failures\":1"));
    expect_true(containsSubstr(json, "\"first_failed\":\"latency_p99\""));

    // Tiny buffer returns null cleanly
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatCanaryReportJson(rep, *tiny_s) == null);

    // Fill up to 16 metrics and verify 17th returns null
    let i: usize = 1;
    while (i < CANARY_MAX_METRICS) : (i += 1) {
        expect_true(
            judge.addMetric(CanaryMetricSpec.make("kpi", CanaryDirection.lower_is_better, 1, 500, 1.96, false, 4)) != null,
        );
    }
    expect_true(
        judge.addMetric(CanaryMetricSpec.make("overflow", CanaryDirection.lower_is_better, 1, 500, 1.96, false, 4)) == null,
    );
    expect_true(bytesEq(formatCanaryVerdict(CanaryVerdict.pass_promote), "pass_promote"));
}
