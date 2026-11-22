// Origin: Pure-CP Stdlib Test Suite (lib/a2a/router.cp) | Created: 2026-11-22
// Production Deep Verification Suite for A2A zero-allocation capability discovery,
// 64-bit skill bitmask routing, heartbeat health failover, load balancing, and HandoffTrace cycle guard.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PeerHealth,
    AgentPeer,
    AgentRouter,
    HandoffStatus,
    HandoffTrace,
} from "cap:a2a";

const SKILL_CODE: u64 = 1;
const SKILL_SQL: u64 = 2;
const SKILL_SEARCH: u64 = 4;
const SKILL_VISION: u64 = 1 << 40;

fn testSkillBitmaskMatchingAndListCapable()! {
    let pool: [4]AgentPeer = undefined;
    let pool_slice: []AgentPeer = pool[0..];
    let router = AgentRouter.init(pool_slice, 5000);

    expect_eq(router.registerPeer(10, "coder", SKILL_CODE | SKILL_SQL, 5, 4, 1000) != null, true);
    expect_eq(router.registerPeer(20, "researcher", SKILL_SEARCH | SKILL_VISION, 5, 4, 1000) != null, true);
    expect_eq(router.registerPeer(30, "fullstack", SKILL_CODE | SKILL_SQL | SKILL_SEARCH | SKILL_VISION, 3, 4, 1000) != null, true);

    // 1. Exact composite skill match (high bit 1<<40 + SKILL_SEARCH)
    let out_arr: [4]u32 = undefined;
    let out_slice: []u32 = out_arr[0..];
    const n_vis = router.listCapablePeers(SKILL_SEARCH | SKILL_VISION, 2000, out_slice);
    expect_eq(n_vis, 2);
    expect_eq(out_slice[0], 20);
    expect_eq(out_slice[1], 30);

    // 2. Route composite skill (CODE | SQL): both 10 and 30 have 0 load, 10 has higher priority (5 > 3)
    if (picked := router.routeTask(SKILL_CODE | SKILL_SQL, 2000, 0)) {
        expect_eq(picked, 10);
    } else {
        expect_eq(true, false);
    }

    // 3. No peer has unoffered high bit skill
    const SKILL_UNKNOWN: u64 = 1 << 60;
    expect_eq(router.routeTask(SKILL_UNKNOWN, 2000, 0) == null, true);
    expect_eq(router.listCapablePeers(SKILL_UNKNOWN, 2000, out_slice), 0);
}

fn testHeartbeatTimeoutRecoveryAndDegradedFallback()! {
    let pool: [3]AgentPeer = undefined;
    let pool_slice: []AgentPeer = pool[0..];
    let router = AgentRouter.init(pool_slice, 5000);

    * = router.registerPeer(1, "primary", SKILL_CODE, 10, 2, 1000);
    * = router.registerPeer(2, "backup", SKILL_CODE, 5, 2, 1000);
    * = router.registerPeer(3, "draining_node", SKILL_CODE, 20, 2, 1000);

    // Put peer 2 into Degraded, peer 3 into Draining
    expect_eq(router.heartbeat(2, 4000, PeerHealth.Degraded), true);
    expect_eq(router.heartbeat(3, 4000, PeerHealth.Draining), true);
    expect_eq(router.heartbeat(999, 4000, PeerHealth.Healthy), false);

    // At t=4000: peer 1 is Healthy & alive -> selected over Degraded peer 2 and Draining peer 3
    if (p := router.routeTask(SKILL_CODE, 4000, 0)) {
        expect_eq(p, 1);
    } else {
        expect_eq(true, false);
    }
    expect_eq(router.releaseTask(1), true);

    // At t=7000: peer 1 timed out (7000 - 1000 = 6000 > 5000), peer 2 is alive (7000 - 4000 = 3000 <= 5000)
    // Degraded peer 2 seamlessly acts as fallback! Draining peer 3 is never selected.
    if (p := router.routeTask(SKILL_CODE, 7000, 0)) {
        expect_eq(p, 2);
    } else {
        expect_eq(true, false);
    }
    expect_eq(router.releaseTask(2), true);

    // Peer 1 recovers via fresh heartbeat -> immediately preferred over Degraded peer 2 again
    expect_eq(router.heartbeat(1, 7500, PeerHealth.Healthy), true);
    if (p := router.routeTask(SKILL_CODE, 7600, 0)) {
        expect_eq(p, 1);
    } else {
        expect_eq(true, false);
    }

    // Peer 1 marked Unhealthy -> falls back to Degraded peer 2
    expect_eq(router.heartbeat(1, 7700, PeerHealth.Unhealthy), true);
    if (p := router.routeTask(SKILL_CODE, 7700, 0)) {
        expect_eq(p, 2);
    } else {
        expect_eq(true, false);
    }
}

fn testLoadBalancingAndExcludeSelfPeer()! {
    let pool: [3]AgentPeer = undefined;
    let pool_slice: []AgentPeer = pool[0..];
    let router = AgentRouter.init(pool_slice, 10000);

    // Peer 10: max_concurrency=2, priority=5
    // Peer 20: max_concurrency=4, priority=5
    * = router.registerPeer(10, "worker-a", SKILL_SQL, 5, 2, 1000);
    * = router.registerPeer(20, "worker-b", SKILL_SQL, 5, 4, 1000);

    // 1st task: both 0/2 == 0/4, same priority -> smaller id (10) wins -> load: 10=1/2 (50%), 20=0/4 (0%)
    if (p := router.routeTask(SKILL_SQL, 1500, 0)) {
        expect_eq(p, 10);
    } else {
        expect_eq(true, false);
    }

    // 2nd task: 20 has lower load (0% < 50%) -> 20 wins -> load: 10=1/2 (50%), 20=1/4 (25%)
    if (p := router.routeTask(SKILL_SQL, 1500, 0)) {
        expect_eq(p, 20);
    } else {
        expect_eq(true, false);
    }

    // 3rd task: 20 still has lower load (25% < 50%) -> 20 wins -> load: 10=1/2 (50%), 20=2/4 (50%)
    if (p := router.routeTask(SKILL_SQL, 1500, 0)) {
        expect_eq(p, 20);
    } else {
        expect_eq(true, false);
    }

    // 4th task: equal load 50% == 50%, exclude_peer_id=10 -> must pick 20! -> load: 20=3/4
    if (p := router.routeTask(SKILL_SQL, 1500, 10)) {
        expect_eq(p, 20);
    } else {
        expect_eq(true, false);
    }

    // Release tasks on 10 and verify release underflow guard
    expect_eq(router.releaseTask(10), true);
    expect_eq(router.releaseTask(10), false);
    expect_eq(router.releaseTask(999), false);
}

fn testHandoffTraceCycleMaxHopsAndBufferFull()! {
    // 1. Cycle detection (A -> B -> A)
    let buf1: [8]u32 = undefined;
    let slice1: []u32 = buf1[0..];
    let tr = HandoffTrace.init(slice1, 4);

    expect_eq(tr.hops(), 0);
    expect_eq(tr.hasVisited(101), false);
    expect_eq(tr.recordHop(101) == HandoffStatus.Ok, true);
    expect_eq(tr.recordHop(202) == HandoffStatus.Ok, true);
    expect_eq(tr.hops(), 2);
    expect_eq(tr.hasVisited(101), true);
    expect_eq(tr.hasVisited(202), true);

    // Cycle back to 101 must be rejected without advancing hop_count
    expect_eq(tr.recordHop(101) == HandoffStatus.CycleDetected, true);
    expect_eq(tr.hops(), 2);

    // 2. MaxHopsExceeded guard
    expect_eq(tr.recordHop(303) == HandoffStatus.Ok, true);
    expect_eq(tr.recordHop(404) == HandoffStatus.Ok, true);
    expect_eq(tr.hops(), 4);
    expect_eq(tr.recordHop(505) == HandoffStatus.MaxHopsExceeded, true);
    expect_eq(tr.hops(), 4);

    // 3. BufferFull guard when max_hops > physical buffer length
    let small_buf: [2]u32 = undefined;
    let small_slice: []u32 = small_buf[0..];
    let tr_small = HandoffTrace.init(small_slice, 5);
    expect_eq(tr_small.recordHop(1) == HandoffStatus.Ok, true);
    expect_eq(tr_small.recordHop(2) == HandoffStatus.Ok, true);
    expect_eq(tr_small.recordHop(3) == HandoffStatus.BufferFull, true);
    expect_eq(tr_small.hops(), 2);
}

fn testDefensiveGuardsAndCapNamespace()! {
    // 1. Empty pool
    let empty_arr: [1]AgentPeer = undefined;
    let empty_slice: []AgentPeer = empty_arr[0..0];
    let empty_router = cap.a2a.AgentRouter.init(empty_slice, 5000);
    expect_eq(empty_router.registerPeer(1, "a", SKILL_CODE, 1, 1, 100) == null, true);
    expect_eq(empty_router.routeTask(SKILL_CODE, 100, 0) == null, true);

    // 2. Duplicate ID, empty name, full pool, zero concurrency, and full saturation
    let pool: [2]AgentPeer = undefined;
    let pool_slice: []AgentPeer = pool[0..];
    let router = cap.a2a.AgentRouter.init(pool_slice, 5000);

    // Empty name rejected
    expect_eq(router.registerPeer(1, "", SKILL_CODE, 1, 1, 100) == null, true);
    // Valid registration
    expect_eq(router.registerPeer(1, "zero-cap", SKILL_CODE, 10, 0, 100) != null, true);
    // Duplicate ID rejected
    expect_eq(router.registerPeer(1, "dup", SKILL_CODE, 10, 2, 100) == null, true);
    // Second valid peer (concurrency=1)
    expect_eq(router.registerPeer(2, "cap-one", SKILL_CODE, 1, 1, 100) != null, true);
    // Pool full rejected
    expect_eq(router.registerPeer(3, "overflow", SKILL_CODE, 1, 1, 100) == null, true);

    // Peer 1 has max_concurrency=0 -> skipped; peer 2 picked
    if (p := router.routeTask(SKILL_CODE, 200, 0)) {
        expect_eq(p, 2);
    } else {
        expect_eq(true, false);
    }
    // Now peer 2 is saturated (1/1) -> routeTask returns null
    expect_eq(router.routeTask(SKILL_CODE, 200, 0) == null, true);

    // Verify HandoffTrace via cap.a2a namespace
    let hbuf: [3]u32 = undefined;
    let hslice: []u32 = hbuf[0..];
    let htrace = cap.a2a.HandoffTrace.init(hslice, 3);
    expect_eq(htrace.recordHop(2) == cap.a2a.HandoffStatus.Ok, true);
    expect_eq(htrace.recordHop(2) == cap.a2a.HandoffStatus.CycleDetected, true);
}

fn main() {
    test("1. a2a/router: 64-bit skill bitmask matching & listCapablePeers", () => {
        testSkillBitmaskMatchingAndListCapable()?;
    });

    test("2. a2a/router: heartbeat timeout eviction, recovery & Degraded fallback", () => {
        testHeartbeatTimeoutRecoveryAndDegradedFallback()?;
    });

    test("3. a2a/router: load-ratio balancing, exclude_peer_id & releaseTask guards", () => {
        testLoadBalancingAndExcludeSelfPeer()?;
    });

    test("4. a2a/router: HandoffTrace cycle detection, max_hops & BufferFull guards", () => {
        testHandoffTraceCycleMaxHopsAndBufferFull()?;
    });

    test("5. a2a/router: empty pool, duplicate ID, zero-concurrency & cap.a2a namespace", () => {
        testDefensiveGuardsAndCapNamespace()?;
    });
}
