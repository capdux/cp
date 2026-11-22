// Origin: Pure-CP Stdlib Test Suite (lib/net/zoneRouter.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Multi-AZ Envoy Locality Spillover Load Balancer:
//   1. Healthy local zone 100% locality affinity & weighted deterministic selection
//   2. Degraded local zone (`< panic_threshold_bps`) smooth proportional spillover to healthy remote AZs
//   3. Zero-healthy local zone 100% remote spillover & sole-healthy-zone local retention
//   4. Global total panic: `fail_open_on_total_panic = true` (`panic_fallback`) vs `false` (`no_endpoint`)
//   5. Real-time `zoneSummary`, `active_conns` tracking & dynamic health/weight updates
//   6. 64-endpoint capacity, invalid zone/weight/ID/config guards & `removeEndpoint`
//   7. `cap.net` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    ZONE_ROUTER_MAX_ENDPOINTS,
    ZONE_ROUTER_MAX_ZONES,
    ZonePickMode,
    ZoneRouterOpStatus,
    ZoneRouterConfig,
    ZoneLocalityRouter,
} from "cap:net";

test("ZoneRouter: healthy local zone achieves 100% locality affinity with weighted selection") {
    let lb = ZoneLocalityRouter.initDefault();
    expect_eq(lb.addEndpoint(1, 0, 300, true), ZoneRouterOpStatus.ok);
    expect_eq(lb.addEndpoint(2, 0, 100, true), ZoneRouterOpStatus.ok);
    expect_eq(lb.addEndpoint(3, 1, 200, true), ZoneRouterOpStatus.ok);

    let hits_ep1: usize = 0;
    let hits_ep2: usize = 0;
    let i: u64 = 0;
    while (i < 200) : (i += 1) {
        const d = lb.pickEndpoint(0, i);
        expect_eq(d.mode, ZonePickMode.local_hit);
        expect_eq(d.selected_zone, 0 as u8);
        expect_eq(d.local_health_bps, 10000 as u16);
        expect_eq(d.local_retain_bps, 10000 as u16);
        if (d.endpoint_id == 1) hits_ep1 += 1;
        if (d.endpoint_id == 2) hits_ep2 += 1;
    }
    expect_eq(hits_ep1 + hits_ep2, 200 as usize);
    // 3:1 weight ratio -> ep1 (~150) significantly exceeds ep2 (~50)
    expect_true(hits_ep1 > 110 && hits_ep2 > 20);
    expect_eq(lb.stats().remote_spillovers, 0 as u64);
}

test("ZoneRouter: degraded local zone smoothly spills proportional traffic to healthy remote AZs") {
    // panic_threshold_bps = 5000 (50%); AZ 0 has 1/4 healthy (2500 bps) -> retain 50% locally, spill 50% remotely!
    let lb = ZoneLocalityRouter.init(ZoneRouterConfig{
        panic_threshold_bps: 5000,
        fail_open_on_total_panic: true,
    }) ?? return;

    * = lb.addEndpoint(10, 0, 100, true);
    * = lb.addEndpoint(11, 0, 100, false);
    * = lb.addEndpoint(12, 0, 100, false);
    * = lb.addEndpoint(13, 0, 100, false);

    * = lb.addEndpoint(20, 1, 100, true);
    * = lb.addEndpoint(21, 1, 100, false); // unhealthy remote must never be picked
    * = lb.addEndpoint(30, 2, 100, true);

    let local_n: usize = 0;
    let remote_n: usize = 0;
    let i: u64 = 0;
    while (i < 1000) : (i += 1) {
        const d = lb.pickEndpoint(0, i * 17 + 3);
        expect_eq(d.local_health_bps, 2500 as u16);
        expect_eq(d.local_retain_bps, 5000 as u16);
        if (d.mode == ZonePickMode.local_hit) {
            expect_eq(d.endpoint_id, 10 as u64);
            local_n += 1;
        } else {
            expect_eq(d.mode, ZonePickMode.remote_spillover);
            expect_true(d.endpoint_id == 20 || d.endpoint_id == 30);
            remote_n += 1;
        }
    }

    // Both local retention and remote spillover sit tightly around 500 / 1000
    expect_true(local_n >= 420 && local_n <= 580);
    expect_true(remote_n >= 420 && remote_n <= 580);
    expect_eq(lb.stats().local_hits, local_n as u64);
    expect_eq(lb.stats().remote_spillovers, remote_n as u64);
}

test("ZoneRouter: zero-healthy local spills 100% remotely; sole healthy zone retains 100% locally") {
    let lb = ZoneLocalityRouter.initDefault();
    * = lb.addEndpoint(1, 0, 100, false);
    * = lb.addEndpoint(2, 0, 100, false);
    * = lb.addEndpoint(3, 1, 100, true);

    // AZ 0 completely unhealthy -> 100% remote_spillover to AZ 1
    let i: u64 = 0;
    while (i < 20) : (i += 1) {
        const d = lb.pickEndpoint(0, i);
        expect_eq(d.mode, ZonePickMode.remote_spillover);
        expect_eq(d.endpoint_id, 3 as u64);
        expect_eq(d.selected_zone, 1 as u8);
        expect_eq(d.local_retain_bps, 0 as u16);
    }

    // Flip: AZ 0 has 1/2 degraded (below 60% threshold), AZ 1 has 0 healthy -> must retain 100% locally!
    let lb2 = ZoneLocalityRouter.init(ZoneRouterConfig{
        panic_threshold_bps: 6000,
        fail_open_on_total_panic: true,
    }) ?? return;
    * = lb2.addEndpoint(1, 0, 100, true);
    * = lb2.addEndpoint(2, 0, 100, false);
    * = lb2.addEndpoint(3, 1, 100, false);

    const d2 = lb2.pickEndpoint(0, 99);
    expect_eq(d2.mode, ZonePickMode.local_hit);
    expect_eq(d2.endpoint_id, 1 as u64);
    expect_eq(d2.local_retain_bps, 10000 as u16);
}

test("ZoneRouter: global total panic fail-open vs fail-closed") {
    // Fail-open enabled
    let lb_open = ZoneLocalityRouter.init(ZoneRouterConfig{
        panic_threshold_bps: 5000,
        fail_open_on_total_panic: true,
    }) ?? return;
    * = lb_open.addEndpoint(1, 0, 100, false);
    * = lb_open.addEndpoint(2, 1, 100, false);

    const d_open = lb_open.pickEndpoint(0, 42);
    expect_eq(d_open.mode, ZonePickMode.panic_fallback);
    expect_eq(d_open.endpoint_id, 1 as u64); // prefers local AZ even in fail-open panic
    expect_eq(lb_open.stats().panic_fallbacks, 1 as u64);

    // Fail-closed (fail_open_on_total_panic = false)
    let lb_closed = ZoneLocalityRouter.init(ZoneRouterConfig{
        panic_threshold_bps: 5000,
        fail_open_on_total_panic: false,
    }) ?? return;
    * = lb_closed.addEndpoint(1, 0, 100, false);
    const d_closed = lb_closed.pickEndpoint(0, 42);
    expect_eq(d_closed.mode, ZonePickMode.no_endpoint);
    expect_eq(d_closed.endpoint_id, 0 as u64);
    expect_eq(lb_closed.stats().rejected_no_endpoint, 1 as u64);
}

test("ZoneRouter: zoneSummary, active_conns tracking & dynamic health/weight mutation") {
    let lb = ZoneLocalityRouter.initDefault();
    * = lb.addEndpoint(1, 2, 100, true);
    * = lb.addEndpoint(2, 2, 300, true);
    expect_true(lb.incActiveConns(1));
    expect_true(lb.incActiveConns(1));
    expect_true(lb.incActiveConns(2));
    expect_true(lb.decActiveConns(1));

    // Mark ep 2 unhealthy -> healthy_weight = 100 / 400 = 2500 bps
    expect_eq(lb.setHealth(2, false), ZoneRouterOpStatus.ok);
    const sum = lb.zoneSummary(2);
    expect_eq(sum.total_endpoints, 2 as usize);
    expect_eq(sum.healthy_endpoints, 1 as usize);
    expect_eq(sum.total_weight, 400 as u64);
    expect_eq(sum.healthy_weight, 100 as u64);
    expect_eq(sum.health_bps, 2500 as u16);
    expect_eq(sum.total_active_conns, 2 as u64);

    // Increase ep 1 weight to 300 -> healthy_weight = 300 / 600 = 5000 bps
    expect_eq(lb.setWeight(1, 300), ZoneRouterOpStatus.ok);
    expect_eq(lb.zoneSummary(2).health_bps, 5000 as u16);
}

test("ZoneRouter: 64-endpoint capacity, invalid args & removeEndpoint guards") {
    expect_true(ZoneLocalityRouter.init(ZoneRouterConfig{ panic_threshold_bps: 0 }) == null);
    expect_true(ZoneLocalityRouter.init(ZoneRouterConfig{ panic_threshold_bps: 10001 }) == null);

    let lb = ZoneLocalityRouter.initDefault();
    // Empty pool returns no_endpoint
    expect_eq(lb.pickEndpoint(0, 1).mode, ZonePickMode.no_endpoint);

    // Invalid args
    expect_eq(lb.addEndpoint(0, 0, 100, true), ZoneRouterOpStatus.invalid_arg);
    expect_eq(lb.addEndpoint(1, ZONE_ROUTER_MAX_ZONES as u8, 100, true), ZoneRouterOpStatus.invalid_arg);
    expect_eq(lb.addEndpoint(1, 0, 0, true), ZoneRouterOpStatus.invalid_arg);

    // Fill 64 endpoints
    let id: u64 = 1;
    while (id <= ZONE_ROUTER_MAX_ENDPOINTS as u64) : (id += 1) {
        const z: u8 = (id % (ZONE_ROUTER_MAX_ZONES as u64)) as u8;
        expect_eq(lb.addEndpoint(id, z, 10, true), ZoneRouterOpStatus.ok);
    }
    expect_eq(lb.addEndpoint(1, 0, 10, true), ZoneRouterOpStatus.duplicate_endpoint);
    expect_eq(lb.addEndpoint(999, 0, 10, true), ZoneRouterOpStatus.capacity_full);

    // Remove and re-add succeeds
    expect_eq(lb.removeEndpoint(1), ZoneRouterOpStatus.ok);
    expect_eq(lb.removeEndpoint(1), ZoneRouterOpStatus.not_found);
    expect_eq(lb.addEndpoint(999, 0, 10, true), ZoneRouterOpStatus.ok);
}

test("ZoneRouter: cap.net namespace facade parity") {
    let lb = cap.net.ZoneLocalityRouter.initDefault();
    expect_eq(lb.addEndpoint(101, 0, 50, true), cap.net.ZoneRouterOpStatus.ok);
    expect_eq(lb.addEndpoint(202, 1, 50, true), cap.net.ZoneRouterOpStatus.ok);

    const d1 = lb.pickEndpoint(0, 777);
    expect_eq(d1.mode, cap.net.ZonePickMode.local_hit);
    expect_eq(d1.endpoint_id, 101 as u64);

    * = lb.setHealth(101, false);
    const d2 = lb.pickEndpoint(0, 777);
    expect_eq(d2.mode, cap.net.ZonePickMode.remote_spillover);
    expect_eq(d2.endpoint_id, 202 as u64);
}
