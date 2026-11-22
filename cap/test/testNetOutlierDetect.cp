// Origin: Pure-CP Stdlib Test Suite (lib/net/outlierDetect.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Envoy-Style Passive Outlier Detector:
//   1. Consecutive 5xx / gateway_error ejection & capped backoff (`min(base * count, max)`)
//   2. Consecutive local-origin error ejection & success reset of consecutive failure counters
//   3. `max_ejection_percent` cluster-wide panic guard preventing cascading blackout
//   4. Periodic statistical success-rate anomaly ejection (`mean - stdev_factor * stdev`) & window reset
//   5. Automatic time-based reinstatement (`now_ms >= ejected_until_ms`) & healthy-interval backoff decay
//   6. Host registration/removal, invalid IDs, global `stats()` & `cap.net` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    OUTLIER_HOST_ID_MAX_LEN,
    OutlierEjectReason,
    OutlierOutcome,
    OutlierConfig,
    OutlierDetector,
} from "cap:net";

test("OutlierDetector: consecutive 5xx ejection & escalating backoff capped at max_ejection_time_ms") {
    let cfg = OutlierConfig.defaults();
    cfg.consecutive_5xx = 3;
    cfg.base_ejection_time_ms = 10_000;
    cfg.max_ejection_time_ms = 25_000;
    cfg.max_ejection_percent = 50;

    let det = OutlierDetector.init(cfg);
    * = det.registerHost("h1");
    * = det.registerHost("h2");

    // 1st ejection: 2 http_5xx + 1 gateway_error -> ejected for 10_000ms (1 * base)
    expect_eq(det.recordOutcome("h1", OutlierOutcome.http_5xx, 1000), OutlierEjectReason.none);
    expect_eq(det.recordOutcome("h1", OutlierOutcome.gateway_error, 1100), OutlierEjectReason.none);
    expect_eq(det.recordOutcome("h1", OutlierOutcome.http_5xx, 1200), OutlierEjectReason.consecutive_5xx);

    expect_true(!det.isHostHealthy("h1", 1200));
    expect_true(!det.isHostHealthy("h1", 11_199));
    expect_true(det.isHostHealthy("h1", 11_200));

    if (s1 := det.getHost("h1")) {
        expect_true(s1.ejected);
        expect_eq(s1.ejection_count, @as(u32, 1));
        expect_eq(s1.ejected_until_ms, @as(u64, 11_200));
        expect_eq(s1.last_reason, OutlierEjectReason.consecutive_5xx);
    } else {
        expect_true(false);
    }

    // Reinstate at t=12_000, then fail 3x immediately -> 2nd ejection lasts 20_000ms (2 * base)
    const r1 = det.runIntervalCheck(12_000);
    expect_eq(r1.reinstated, @as(usize, 1));
    expect_true(det.isHostHealthy("h1", 12_000));

    * = det.recordOutcome("h1", OutlierOutcome.http_5xx, 12_100);
    * = det.recordOutcome("h1", OutlierOutcome.http_5xx, 12_200);
    expect_eq(det.recordOutcome("h1", OutlierOutcome.http_5xx, 12_300), OutlierEjectReason.consecutive_5xx);
    if (s2 := det.getHost("h1")) {
        expect_eq(s2.ejection_count, @as(u32, 2));
        expect_eq(s2.ejected_until_ms, @as(u64, 32_300)); // 12_300 + 20_000
    } else {
        expect_true(false);
    }

    // Reinstate at t=33_000, fail 3x again -> 3rd ejection would be 30_000ms, capped at 25_000ms!
    * = det.runIntervalCheck(33_000);
    * = det.recordOutcome("h1", OutlierOutcome.http_5xx, 33_100);
    * = det.recordOutcome("h1", OutlierOutcome.http_5xx, 33_200);
    expect_eq(det.recordOutcome("h1", OutlierOutcome.http_5xx, 33_300), OutlierEjectReason.consecutive_5xx);
    if (s3 := det.getHost("h1")) {
        expect_eq(s3.ejection_count, @as(u32, 3));
        expect_eq(s3.ejected_until_ms, @as(u64, 58_300)); // 33_300 + 25_000 (capped)
    } else {
        expect_true(false);
    }
}

test("OutlierDetector: consecutive local-origin errors & success resetting failure streaks") {
    let cfg = OutlierConfig.defaults();
    cfg.consecutive_5xx = 3;
    cfg.consecutive_local_origin = 2;
    cfg.base_ejection_time_ms = 5_000;

    let det = OutlierDetector.init(cfg);
    * = det.registerHost("node-a");
    * = det.registerHost("node-b");

    // 2 http_5xx followed by 1 success resets consecutive_5xx -> next 2 http_5xx do NOT eject
    expect_eq(det.recordOutcome("node-a", OutlierOutcome.http_5xx, 100), OutlierEjectReason.none);
    expect_eq(det.recordOutcome("node-a", OutlierOutcome.http_5xx, 200), OutlierEjectReason.none);
    expect_eq(det.recordOutcome("node-a", OutlierOutcome.success, 300), OutlierEjectReason.none);
    expect_eq(det.recordOutcome("node-a", OutlierOutcome.http_5xx, 400), OutlierEjectReason.none);
    expect_eq(det.recordOutcome("node-a", OutlierOutcome.http_5xx, 500), OutlierEjectReason.none);
    expect_true(det.isHostHealthy("node-a", 500));

    // 2 consecutive local_origin_error ejects immediately with consecutive_local_origin
    expect_eq(det.recordOutcome("node-a", OutlierOutcome.local_origin_error, 600), OutlierEjectReason.none);
    expect_eq(det.recordOutcome("node-a", OutlierOutcome.local_origin_error, 700), OutlierEjectReason.consecutive_local_origin);
    expect_true(!det.isHostHealthy("node-a", 700));
    if (sa := det.getHost("node-a")) {
        expect_eq(sa.last_reason, OutlierEjectReason.consecutive_local_origin);
    } else {
        expect_true(false);
    }
}

test("OutlierDetector: max_ejection_percent caps ejections and prevents cluster blackout") {
    let cfg = OutlierConfig.defaults();
    cfg.consecutive_5xx = 2;
    cfg.max_ejection_percent = 50; // 4 hosts -> max 2 ejected

    let det = OutlierDetector.init(cfg);
    * = det.registerHost("h1");
    * = det.registerHost("h2");
    * = det.registerHost("h3");
    * = det.registerHost("h4");

    // Eject h1 (1/4 = 25% <= 50%)
    * = det.recordOutcome("h1", OutlierOutcome.http_5xx, 1000);
    expect_eq(det.recordOutcome("h1", OutlierOutcome.http_5xx, 1100), OutlierEjectReason.consecutive_5xx);

    // Eject h2 (2/4 = 50% <= 50%)
    * = det.recordOutcome("h2", OutlierOutcome.http_5xx, 1200);
    expect_eq(det.recordOutcome("h2", OutlierOutcome.http_5xx, 1300), OutlierEjectReason.consecutive_5xx);

    // Attempt to eject h3 (would be 3/4 = 75% > 50%) -> capped! h3 stays healthy
    * = det.recordOutcome("h3", OutlierOutcome.http_5xx, 1400);
    expect_eq(det.recordOutcome("h3", OutlierOutcome.http_5xx, 1500), OutlierEjectReason.none);
    expect_true(det.isHostHealthy("h3", 1500));

    const st = det.stats();
    expect_eq(st.total_hosts, @as(usize, 4));
    expect_eq(st.currently_ejected, @as(usize, 2));
    expect_eq(st.total_ejections, @as(u64, 2));
    expect_eq(st.capped_ejections, @as(u64, 1));
}

test("OutlierDetector: periodic statistical success-rate anomaly detection & window reset") {
    let cfg = OutlierConfig.defaults();
    cfg.consecutive_5xx = 10; // high so consecutive rule does not trigger
    cfg.min_hosts_for_success_rate = 5;
    cfg.success_rate_min_volume = 10;
    cfg.success_rate_stdev_factor = 1.9;
    cfg.max_ejection_percent = 50;
    cfg.base_ejection_time_ms = 20_000;

    let det = OutlierDetector.init(cfg);
    * = det.registerHost("ok-1");
    * = det.registerHost("ok-2");
    * = det.registerHost("ok-3");
    * = det.registerHost("ok-4");
    * = det.registerHost("sick-5");

    // 4 healthy hosts: 10/10 successes (1.0)
    let r: u32 = 0;
    while (r < 10) : (r += 1) {
        * = det.recordOutcome("ok-1", OutlierOutcome.success, 1000);
        * = det.recordOutcome("ok-2", OutlierOutcome.success, 1000);
        * = det.recordOutcome("ok-3", OutlierOutcome.success, 1000);
        * = det.recordOutcome("ok-4", OutlierOutcome.success, 1000);
    }

    // 1 degraded host: 2 successes, 8 http_5xx interleaved (rate = 0.20, max streak = 4 < 10)
    * = det.recordOutcome("sick-5", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.success, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.http_5xx, 1000);
    * = det.recordOutcome("sick-5", OutlierOutcome.success, 1000);

    // mean = 0.84, stdev = 0.32, threshold = 0.84 - 1.9 * 0.32 = 0.232 -> sick-5 (0.20) ejected!
    const rep = det.runIntervalCheck(2000);
    expect_eq(rep.evaluated_hosts, @as(usize, 5));
    expect_eq(rep.newly_ejected, @as(usize, 1));
    expect_true(rep.mean_success_rate > 0.83 && rep.mean_success_rate < 0.85);
    expect_true(rep.stdev_success_rate > 0.31 && rep.stdev_success_rate < 0.33);
    expect_true(rep.ejection_threshold > 0.23 && rep.ejection_threshold < 0.24);

    expect_true(!det.isHostHealthy("sick-5", 2000));
    expect_true(det.isHostHealthy("ok-1", 2000));
    if (s5 := det.getHost("sick-5")) {
        expect_eq(s5.last_reason, OutlierEjectReason.success_rate_anomaly);
        // Rolling window must be cleared after interval check
        expect_eq(s5.window_requests, @as(u32, 0));
        expect_eq(s5.window_successes, @as(u32, 0));
    } else {
        expect_true(false);
    }
}

test("OutlierDetector: automatic reinstatement & healthy-interval ejection_count decay") {
    let cfg = OutlierConfig.defaults();
    cfg.consecutive_5xx = 2;
    cfg.base_ejection_time_ms = 10_000;

    let det = OutlierDetector.init(cfg);
    * = det.registerHost("a");
    * = det.registerHost("b");

    // Eject "a" at t=1000 (ejected_until_ms = 11_000, ejection_count = 1)
    * = det.recordOutcome("a", OutlierOutcome.http_5xx, 1000);
    expect_eq(det.recordOutcome("a", OutlierOutcome.http_5xx, 1000), OutlierEjectReason.consecutive_5xx);

    // Check before expiry (t=5000): still ejected
    const r_early = det.runIntervalCheck(5000);
    expect_eq(r_early.reinstated, @as(usize, 0));
    expect_true(!det.isHostHealthy("a", 5000));

    // Check at expiry (t=11_000): reinstated!
    const r_exp = det.runIntervalCheck(11_000);
    expect_eq(r_exp.reinstated, @as(usize, 1));
    expect_true(det.isHostHealthy("a", 11_000));
    expect_eq(det.stats().total_reinstatements, @as(u64, 1));

    // Next healthy interval (t=21_000): ejection_count decays from 1 -> 0
    * = det.runIntervalCheck(21_000);
    if (sa := det.getHost("a")) {
        expect_eq(sa.ejection_count, @as(u32, 0));
    } else {
        expect_true(false);
    }
}

test("OutlierDetector: registration/removal guards, unknown host & cap.net facade parity") {
    let det = cap.net.OutlierDetector.init(cap.net.OutlierConfig.defaults());

    // Empty and oversized host IDs rejected
    expect_true(det.registerHost("") == null);
    let too_long: [OUTLIER_HOST_ID_MAX_LEN + 4]u8 = ['H'; OUTLIER_HOST_ID_MAX_LEN + 4];
    expect_true(det.registerHost(too_long[0..]) == null);

    const idx1 = det.registerHost("svc-1") ?? 999;
    const idx1_dup = det.registerHost("svc-1") ?? 998;
    expect_eq(idx1, idx1_dup);
    expect_eq(det.stats().total_hosts, @as(usize, 1));

    // Unknown host queries return safe defaults
    expect_true(!det.isHostHealthy("ghost", 100));
    expect_eq(det.recordOutcome("ghost", OutlierOutcome.http_5xx, 100), OutlierEjectReason.none);
    expect_true(det.getHost("ghost") == null);

    // Remove host
    expect_true(det.removeHost("svc-1"));
    expect_true(!det.removeHost("svc-1"));
    expect_eq(det.stats().total_hosts, @as(usize, 0));
}
