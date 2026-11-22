// Origin: Pure-CP Stdlib Test Suite (lib/net/healthProbe.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Envoy-Style Active Health Prober:
//   1. Consecutive pass threshold promotion (`unknown -> warming/healthy`) & fail threshold demotion (`unhealthy`)
//   2. Linear slow-start weight ramp (`effectiveWeightAt`) across `slow_start_window_ms`
//   3. `collectDueProbes` scheduling & accelerated `unhealthy_interval_ms` re-probing
//   4. Envoy Panic Mode (`panic_threshold_pct`) fail-open routing when cluster health drops below threshold
//   5. `draining` strict isolation (never probed, never routed even under Panic Mode)
//   6. `degraded_header` half-weight degradation & consecutive-pass recovery to `healthy`
//   7. Endpoint registration/removal guards & `cap.net` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    HEALTH_PROBE_MAX_ENDPOINTS,
    HEALTH_PROBE_ID_MAX_LEN,
    ProbeEndpointState,
    ProbeOutcome,
    HealthProbeConfig,
    ActiveHealthProber,
} from "cap:net";

test("HealthProbe: consecutive pass promotion & consecutive fail demotion") {
    let cfg = HealthProbeConfig.defaults();
    cfg.healthy_threshold = 2;
    cfg.unhealthy_threshold = 3;
    cfg.slow_start_window_ms = 0; // direct promotion to healthy
    cfg.panic_threshold_pct = 0;  // disable panic mode for single-node test
    let prober = ActiveHealthProber.init(cfg);

    expect_true(prober.registerEndpoint("10.0.0.1:8080", 100, 1000) != null);
    expect_true(!prober.isRoutable("10.0.0.1:8080", 1000));

    // 1st pass: still unknown
    const s1 = prober.recordProbeResult("10.0.0.1:8080", ProbeOutcome.pass, 1000) ?? ProbeEndpointState.draining;
    expect_true(s1 == ProbeEndpointState.unknown);
    // 2nd consecutive pass: promoted to healthy
    const s2 = prober.recordProbeResult("10.0.0.1:8080", ProbeOutcome.pass, 2000) ?? ProbeEndpointState.draining;
    expect_true(s2 == ProbeEndpointState.healthy);
    expect_true(prober.isRoutable("10.0.0.1:8080", 2000));

    // 2 failures: still healthy; 3rd consecutive failure: demoted to unhealthy
    const f1 = prober.recordProbeResult("10.0.0.1:8080", ProbeOutcome.fail_timeout, 7000) ?? ProbeEndpointState.draining;
    expect_true(f1 == ProbeEndpointState.healthy);
    const f2 = prober.recordProbeResult("10.0.0.1:8080", ProbeOutcome.fail_status, 8000) ?? ProbeEndpointState.draining;
    expect_true(f2 == ProbeEndpointState.healthy);
    const f3 = prober.recordProbeResult("10.0.0.1:8080", ProbeOutcome.fail_connect, 9000) ?? ProbeEndpointState.draining;
    expect_true(f3 == ProbeEndpointState.unhealthy);
    expect_true(!prober.isRoutable("10.0.0.1:8080", 9000));
}

test("HealthProbe: linear slow-start weight ramp inside slow_start_window_ms") {
    let cfg = HealthProbeConfig.defaults();
    cfg.healthy_threshold = 1;
    cfg.slow_start_window_ms = 20000;
    cfg.slow_start_min_weight_bps = 1000; // starts at 10%
    let prober = ActiveHealthProber.init(cfg);

    * = prober.registerEndpoint("10.0.1.1:8080", 1000, 10000);
    const st = prober.recordProbeResult("10.0.1.1:8080", ProbeOutcome.pass, 10000) ?? ProbeEndpointState.unknown;
    expect_true(st == ProbeEndpointState.warming);

    if (slot := prober.getEndpoint("10.0.1.1:8080")) {
        // t = 10000 (0% elapsed) -> 10% of 1000 = 100
        expect_eq(slot.effectiveWeightAt(10000, cfg), @as(u32, 100));
        // t = 20000 (50% elapsed) -> 10% + 45% = 55% of 1000 = 550
        expect_eq(slot.effectiveWeightAt(20000, cfg), @as(u32, 550));
        // t = 30000 (100% elapsed) -> full base_weight = 1000
        expect_eq(slot.effectiveWeightAt(30000, cfg), @as(u32, 1000));
    } else {
        expect_true(false);
    }

    // Passing probe after slow-start window promotes warming -> healthy
    const promoted = prober.recordProbeResult("10.0.1.1:8080", ProbeOutcome.pass, 30000) ?? ProbeEndpointState.unknown;
    expect_true(promoted == ProbeEndpointState.healthy);
}

test("HealthProbe: collectDueProbes scheduling & accelerated unhealthy_interval_ms") {
    let cfg = HealthProbeConfig.defaults();
    cfg.interval_ms = 5000;
    cfg.unhealthy_interval_ms = 1000;
    cfg.healthy_threshold = 1;
    cfg.unhealthy_threshold = 1;
    cfg.slow_start_window_ms = 0;
    let prober = ActiveHealthProber.init(cfg);

    const idx_a = prober.registerEndpoint("svc-a:80", 100, 1000) ?? 99;
    const idx_b = prober.registerEndpoint("svc-b:80", 100, 1000) ?? 99;

    // Both due immediately at t=1000
    let due: [4]usize = [0; 4];
    let due_s: []usize = due[0..];
    expect_eq(prober.collectDueProbes(1000, *due_s), @as(usize, 2));

    // svc-a passes -> next_probe_ms = 1000 + 5000 = 6000
    // svc-b fails  -> next_probe_ms = 1000 + 1000 = 2000
    * = prober.recordProbeResult("svc-a:80", ProbeOutcome.pass, 1000);
    * = prober.recordProbeResult("svc-b:80", ProbeOutcome.fail_timeout, 1000);

    // At t=2000 only unhealthy svc-b is due for accelerated re-probe
    const n2 = prober.collectDueProbes(2000, *due_s);
    expect_eq(n2, @as(usize, 1));
    expect_eq(due[0], idx_b);

    // At t=6000 both are due again
    expect_eq(prober.collectDueProbes(6000, *due_s), @as(usize, 2));
    * = idx_a;
}

test("HealthProbe: Envoy Panic Mode fail-open protection below panic_threshold_pct") {
    let cfg = HealthProbeConfig.defaults();
    cfg.healthy_threshold = 1;
    cfg.unhealthy_threshold = 1;
    cfg.slow_start_window_ms = 0;
    cfg.panic_threshold_pct = 50; // < 50% triggers panic mode
    let prober = ActiveHealthProber.init(cfg);

    * = prober.registerEndpoint("n1:80", 100, 1000);
    * = prober.registerEndpoint("n2:80", 100, 1000);
    * = prober.registerEndpoint("n3:80", 100, 1000);
    * = prober.registerEndpoint("n4:80", 100, 1000);

    // 2/4 healthy = 50% -> NOT in panic mode (n3/n4 remain unroutable)
    * = prober.recordProbeResult("n1:80", ProbeOutcome.pass, 1000);
    * = prober.recordProbeResult("n2:80", ProbeOutcome.pass, 1000);
    * = prober.recordProbeResult("n3:80", ProbeOutcome.fail_status, 1000);
    * = prober.recordProbeResult("n4:80", ProbeOutcome.fail_status, 1000);

    const s_ok = prober.clusterSummary(1000);
    expect_eq(s_ok.healthy_pct, @as(u8, 50));
    expect_true(!s_ok.in_panic_mode);
    expect_true(prober.isRoutable("n1:80", 1000));
    expect_true(!prober.isRoutable("n3:80", 1000));

    // n2 fails -> 1/4 healthy = 25% < 50% -> Envoy Panic Mode activates, fail-open to n3/n4!
    * = prober.recordProbeResult("n2:80", ProbeOutcome.fail_connect, 2000);
    const s_panic = prober.clusterSummary(2000);
    expect_eq(s_panic.healthy_pct, @as(u8, 25));
    expect_true(s_panic.in_panic_mode);
    expect_true(prober.isRoutable("n1:80", 2000));
    expect_true(prober.isRoutable("n2:80", 2000));
    expect_true(prober.isRoutable("n3:80", 2000));
}

test("HealthProbe: draining endpoint isolated from probing and Panic Mode routing") {
    let cfg = HealthProbeConfig.defaults();
    cfg.healthy_threshold = 1;
    cfg.unhealthy_threshold = 1;
    cfg.slow_start_window_ms = 0;
    cfg.panic_threshold_pct = 50;
    let prober = ActiveHealthProber.init(cfg);

    * = prober.registerEndpoint("live-1:80", 100, 1000);
    * = prober.registerEndpoint("live-2:80", 100, 1000);
    * = prober.registerEndpoint("drain-1:80", 100, 1000);

    expect_true(prober.setDraining("drain-1:80", true));
    // Force panic mode on non-draining nodes (0/2 healthy = 0% < 50%)
    * = prober.recordProbeResult("live-1:80", ProbeOutcome.fail_timeout, 1000);
    * = prober.recordProbeResult("live-2:80", ProbeOutcome.fail_timeout, 1000);

    const sum = prober.clusterSummary(1000);
    expect_eq(sum.draining_count, @as(usize, 1));
    expect_true(sum.in_panic_mode);
    // live-1 fails open, but drain-1 MUST remain unroutable and excluded from due probes
    expect_true(prober.isRoutable("live-1:80", 1000));
    expect_true(!prober.isRoutable("drain-1:80", 1000));

    let due: [4]usize = [0; 4];
    let due_s: []usize = due[0..];
    expect_eq(prober.collectDueProbes(5000, *due_s), @as(usize, 2)); // only live-1 & live-2
}

test("HealthProbe: degraded_header half-weight & recovery via consecutive pass") {
    let cfg = HealthProbeConfig.defaults();
    cfg.healthy_threshold = 2;
    cfg.slow_start_window_ms = 0;
    let prober = ActiveHealthProber.init(cfg);

    * = prober.registerEndpoint("deg-1:80", 200, 1000);
    const st_deg = prober.recordProbeResult("deg-1:80", ProbeOutcome.degraded_header, 1000) ?? ProbeEndpointState.unknown;
    expect_true(st_deg == ProbeEndpointState.degraded);
    expect_true(prober.isRoutable("deg-1:80", 1000));

    if (slot := prober.getEndpoint("deg-1:80")) {
        expect_eq(slot.effectiveWeightAt(1000, cfg), @as(u32, 100)); // 50% weight while degraded
    } else {
        expect_true(false);
    }

    // 2 consecutive passes recover degraded -> healthy (full 200 weight)
    * = prober.recordProbeResult("deg-1:80", ProbeOutcome.pass, 6000);
    const st_rec = prober.recordProbeResult("deg-1:80", ProbeOutcome.pass, 11000) ?? ProbeEndpointState.unknown;
    expect_true(st_rec == ProbeEndpointState.healthy);
    if (slot2 := prober.getEndpoint("deg-1:80")) {
        expect_eq(slot2.effectiveWeightAt(11000, cfg), @as(u32, 200));
    } else {
        expect_true(false);
    }
}

test("HealthProbe: duplicate/empty/remove guards & cap.net facade parity") {
    let prober = ActiveHealthProber.init(HealthProbeConfig.defaults());
    expect_true(prober.registerEndpoint("", 100, 0) == null);
    expect_true(prober.registerEndpoint("ep-1", 100, 0) != null);
    expect_true(prober.registerEndpoint("ep-1", 100, 0) == null); // duplicate rejected
    expect_true(prober.removeEndpoint("ep-1"));
    expect_true(!prober.removeEndpoint("ep-1"));

    // `cap.net` facade parity
    expect_eq(cap.net.HEALTH_PROBE_MAX_ENDPOINTS, @as(usize, 64));
    expect_eq(cap.net.HEALTH_PROBE_ID_MAX_LEN, @as(usize, 48));
    expect_eq(HEALTH_PROBE_MAX_ENDPOINTS, @as(usize, 64));
    expect_eq(HEALTH_PROBE_ID_MAX_LEN, @as(usize, 48));
}
