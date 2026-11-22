// Source: cp/cap/test/test_cusum_production_deep.cp (Production deep tests for cap:cusum)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ShiftDirection,
    ChangePoint,
    CusumDetector,
    PageHinkleyDetector,
    EwmaControlChart,
    StreamingZScore,
    scanCusum,
} from "cap:math";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

test("1. Stationary zero-mean series triggers zero false alarms across all detectors", () => {
    let cusum = CusumDetector.init(0.0, 0.5, 4.0) ?? unreachable;
    let ph = PageHinkleyDetector.init(0.1, 5.0, 10) ?? unreachable;
    let ewma = EwmaControlChart.init(0.0, 1.0, 0.2, 3.0) ?? unreachable;
    let zs = StreamingZScore.init(10, 3.5) ?? unreachable;

    let series: [60]f64 = std.mem.zeroes([60]f64);
    let i: usize = 0;
    while (i < 60) {
        const val: f64 = if ((i & 1) == 0) 0.2 else -0.2;
        series[i] = val;
        expect_eq(cusum.update(val) == ShiftDirection.None, true);
        expect_eq(ph.update(val) == ShiftDirection.None, true);
        expect_eq(ewma.update(val) == ShiftDirection.None, true);
        expect_eq(zs.update(val) == ShiftDirection.None, true);
        i += 1;
    }
    expect_eq(cusum.alarm_count, 0);

    let cps: [8]ChangePoint = std.mem.zeroes([8]ChangePoint);
    let cps_s: []ChangePoint = cps[0..];
    const found = scanCusum(series[0..], 0.0, 0.5, 4.0, &cps_s) ?? unreachable;
    expect_eq(found, 0);
});

test("2. Positive and negative mean step shifts detected within a few samples by CusumDetector & scanCusum", () => {
    let series: [60]f64 = std.mem.zeroes([60]f64);
    let i: usize = 0;
    while (i < 60) {
        if (i >= 20 && i < 26) {
            series[i] = 3.0;
        } else if (i >= 45 && i < 51) {
            series[i] = -3.0;
        } else {
            series[i] = 0.0;
        }
        i += 1;
    }

    let det = CusumDetector.init(0.0, 0.5, 4.0) ?? unreachable;
    let first_pos_idx: usize = 999;
    let first_neg_idx: usize = 999;
    let j: usize = 0;
    while (j < 60) {
        const d = det.update(series[j]);
        if (d == ShiftDirection.Positive && first_pos_idx == 999) {
            first_pos_idx = j;
        }
        if (d == ShiftDirection.Negative && first_neg_idx == 999) {
            first_neg_idx = j;
        }
        j += 1;
    }
    // Each +3.0 sample adds (3.0 - 0.5) = 2.5 -> crosses 4.0 on the 2nd shifted sample (index 21)
    expect_eq(first_pos_idx == 21, true);
    expect_eq(first_neg_idx == 46, true);
    expect_eq(det.alarm_count >= 2, true);

    det.reset();
    expect_eq(det.count, 0);
    expect_eq(det.positiveStat() == 0.0, true);
    expect_eq(det.negativeStat() == 0.0, true);

    // Batch scanCusum matches streaming detection and records pre-reset statistic
    let cps: [16]ChangePoint = std.mem.zeroes([16]ChangePoint);
    let cps_s: []ChangePoint = cps[0..];
    const n_cp = scanCusum(series[0..], 0.0, 0.5, 4.0, &cps_s) ?? unreachable;
    expect_eq(n_cp >= 2, true);
    expect_eq(cps[0].index, 21);
    expect_eq(cps[0].direction == ShiftDirection.Positive, true);
    expect_eq(cps[0].stat > 4.0, true);
});

test("3. PageHinkleyDetector adapts without prior mean and detects upward & downward drift", () => {
    // Upward drift on unknown baseline 100.0 -> 103.0
    let ph_up = PageHinkleyDetector.init(0.05, 8.0, 15) ?? unreachable;
    let up_idx: usize = 999;
    let i: usize = 0;
    while (i < 60) {
        const base: f64 = if (i < 30) 100.0 else 103.0;
        const noise: f64 = if ((i & 1) == 0) 0.05 else -0.05;
        const d = ph_up.update(base + noise);
        if (d == ShiftDirection.Positive && up_idx == 999) {
            up_idx = i;
        }
        i += 1;
    }
    expect_eq(up_idx >= 30 && up_idx < 45, true);

    // Downward drift on unknown baseline -50.0 -> -53.0
    let ph_dn = PageHinkleyDetector.init(0.05, 8.0, 15) ?? unreachable;
    let dn_idx: usize = 999;
    let j: usize = 0;
    while (j < 60) {
        const base: f64 = if (j < 30) -50.0 else -53.0;
        const noise: f64 = if ((j & 1) == 0) 0.05 else -0.05;
        const d = ph_dn.update(base + noise);
        if (d == ShiftDirection.Negative && dn_idx == 999) {
            dn_idx = j;
        }
        j += 1;
    }
    expect_eq(dn_idx >= 30 && dn_idx < 45, true);
});

test("4. EwmaControlChart time-varying limits converge asymptotically and catch small persistent shifts", () => {
    const lam: f64 = 0.2;
    const sig: f64 = 1.0;
    const l_fac: f64 = 2.8;
    let chart = EwmaControlChart.init(0.0, sig, lam, l_fac) ?? unreachable;

    * = chart.update(0.0);
    const ucl1 = chart.upperControlLimit();
    const lcl1 = chart.lowerControlLimit();
    expect_eq(absF64(ucl1 + lcl1) < 1.0e-12, true);

    let i: usize = 0;
    while (i < 4) {
        * = chart.update(0.0);
        i += 1;
    }
    const ucl5 = chart.upperControlLimit();
    while (i < 50) {
        * = chart.update(0.0);
        i += 1;
    }
    const ucl50 = chart.upperControlLimit();
    const ucl_asymp = l_fac * sig * std.math.sqrt(lam / (2.0 - lam));

    expect_eq(ucl1 < ucl5 && ucl5 < ucl50, true);
    expect_eq(absF64(ucl50 - ucl_asymp) < 1.0e-6, true);

    // Persistent +1.2*sigma shift (well below 3*sigma Shewhart) caught quickly by EWMA
    chart.reset();
    let hit_step: usize = 999;
    let j: usize = 0;
    while (j < 30) {
        const d = chart.update(1.2);
        if (d == ShiftDirection.Positive && hit_step == 999) {
            hit_step = j;
        }
        j += 1;
    }
    expect_eq(hit_step < 15, true);
    expect_eq(chart.value() > chart.upperControlLimit(), true);
});

test("5. StreamingZScore detects spikes and preserves Welford numerical stability at 1e9 offset", () => {
    let zs = StreamingZScore.init(20, 4.0) ?? unreachable;
    const offset: f64 = 1.0e9;

    // 40 alternating samples: offset - 1.0 and offset + 1.0 -> mean = offset, sample std = sqrt(40/39) ~= 1.012739
    let i: usize = 0;
    while (i < 40) {
        const x = if ((i & 1) == 0) (offset - 1.0) else (offset + 1.0);
        expect_eq(zs.update(x) == ShiftDirection.None, true);
        i += 1;
    }
    expect_eq(absF64(zs.currentMean() - offset) < 1.0e-6, true);
    const expected_std = std.math.sqrt(40.0 / 39.0);
    expect_eq(absF64(zs.currentStd() - expected_std) < 1.0e-6, true);

    // Inject positive and negative spikes
    expect_eq(zs.update(offset + 10.0) == ShiftDirection.Positive, true);
    expect_eq(zs.update(offset - 12.0) == ShiftDirection.Negative, true);
});

test("6. Defensive guards: invalid thresholds/lambdas and NaN resilience", () => {
    const zero_f: f64 = 0.0;
    const nan_f: f64 = zero_f / zero_f;

    expect_eq(CusumDetector.init(0.0, 0.5, 0.0) == null, true);
    expect_eq(CusumDetector.init(0.0, -0.1, 4.0) == null, true);
    expect_eq(CusumDetector.init(nan_f, 0.5, 4.0) == null, true);

    expect_eq(PageHinkleyDetector.init(0.1, 0.0, 5) == null, true);
    expect_eq(PageHinkleyDetector.init(-0.1, 5.0, 5) == null, true);
    expect_eq(PageHinkleyDetector.init(0.1, nan_f, 5) == null, true);

    expect_eq(EwmaControlChart.init(0.0, 0.0, 0.2, 3.0) == null, true);
    expect_eq(EwmaControlChart.init(0.0, 1.0, 0.0, 3.0) == null, true);
    expect_eq(EwmaControlChart.init(0.0, 1.0, 1.2, 3.0) == null, true);
    expect_eq(EwmaControlChart.init(0.0, 1.0, 0.2, 0.0) == null, true);
    expect_eq(EwmaControlChart.init(0.0, 1.0, nan_f, 3.0) == null, true);

    expect_eq(StreamingZScore.init(1, 3.0) == null, true);
    expect_eq(StreamingZScore.init(5, 0.0) == null, true);
    expect_eq(StreamingZScore.init(5, nan_f) == null, true);

    // Online NaN ignored without corrupting state
    let det = CusumDetector.init(0.0, 0.5, 4.0) ?? unreachable;
    expect_eq(det.update(nan_f) == ShiftDirection.None, true);
    expect_eq(det.count, 0);

    // scanCusum rejects NaN in series
    const bad_series = [0.0, nan_f, 1.0];
    let cps: [4]ChangePoint = std.mem.zeroes([4]ChangePoint);
    let cps_s: []ChangePoint = cps[0..];
    expect_eq(scanCusum(bad_series[0..], 0.0, 0.5, 4.0, &cps_s) == null, true);
});
