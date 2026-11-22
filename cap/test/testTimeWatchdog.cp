// Origin: Pure-CP Stdlib Test Suite (lib/time/watchdog.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Distributed Heartbeat Watchdog & Phi-Accrual Failure Detector:
//   1. Deterministic fixed_timeout mode (`healthy -> suspect -> dead -> recovered`) & grace_ms window
//   2. Phi-Accrual adaptive detection adapting to network jitter vs steady heartbeats (no false positives)
//   3. `pause` / `resume` maintenance window exemption & clock reset on resume
//   4. 16-sample circular heartbeat ring wrap-around & monotonic clock rollback safety
//   5. `pollTransitions` multi-node batch event collection & truncation safety
//   6. 64-node capacity saturation, invalid config/ID guards, stats & `cap.time` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    WATCHDOG_MAX_NODES,
    WATCHDOG_SAMPLE_WINDOW,
    WatchdogMode,
    WatchdogState,
    WatchdogEventKind,
    WatchdogNodeConfig,
    WatchdogNodeState,
    WatchdogTransitionEvent,
    computePhiAccrual,
    HeartbeatWatchdog,
} from "cap:time";

test("Watchdog: deterministic fixed_timeout FSM (healthy -> suspect -> dead -> recovered)") {
    let wd = HeartbeatWatchdog.init();
    let cfg = WatchdogNodeConfig.fixedDefaults();
    cfg.grace_ms = 500;
    cfg.suspect_timeout_ms = 2000;
    cfg.dead_timeout_ms = 5000;

    expect_true(wd.registerNode(1, cfg, 1000) != null);

    let evs: [8]WatchdogTransitionEvent = [WatchdogTransitionEvent{}; 8];
    let evs_s: []WatchdogTransitionEvent = evs[0..];

    // t=3499: silent=2499, eff=1999 < 2000 -> still healthy
    expect_eq(wd.pollTransitions(3499, *evs_s), 0 as usize);
    expect_eq((wd.getNode(1) ?? WatchdogNodeState{}).state, WatchdogState.healthy);

    // t=3500: silent=2500, eff=2000 >= 2000 -> became_suspect
    expect_eq(wd.pollTransitions(3500, *evs_s), 1 as usize);
    expect_eq(evs[0].node_id, 1 as u64);
    expect_eq(evs[0].kind, WatchdogEventKind.became_suspect);
    expect_eq(evs[0].new_state, WatchdogState.suspect);

    // t=6500: silent=5500, eff=5000 >= 5000 -> became_dead
    expect_eq(wd.pollTransitions(6500, *evs_s), 1 as usize);
    expect_eq(evs[0].kind, WatchdogEventKind.became_dead);
    expect_eq(evs[0].prev_state, WatchdogState.suspect);
    expect_eq(evs[0].new_state, WatchdogState.dead);

    // Heartbeat arrives at t=7000 -> immediately healthy, emits recovered on next poll
    expect_true(wd.heartbeat(1, 7000));
    expect_eq((wd.getNode(1) ?? WatchdogNodeState{}).state, WatchdogState.healthy);
    expect_eq(wd.pollTransitions(7100, *evs_s), 1 as usize);
    expect_eq(evs[0].kind, WatchdogEventKind.recovered);
    expect_eq(evs[0].prev_state, WatchdogState.dead);
    expect_eq(evs[0].new_state, WatchdogState.healthy);
}

test("Watchdog: Phi-Accrual adaptive detection tolerates jitter & flags real outages") {
    // Pure function monotonicity & asymptotic stability
    const p_low = computePhiAccrual(500.0, 1000.0, 100.0);
    const p_mean = computePhiAccrual(1000.0, 1000.0, 100.0);
    const p_high = computePhiAccrual(1800.0, 1000.0, 100.0);
    const p_extreme = computePhiAccrual(50000.0, 1000.0, 100.0);
    expect_true(p_low < p_mean);
    expect_true(p_mean > 0.25 && p_mean < 0.35); // ~log10(2) == 0.301
    expect_true(p_high > 8.0);
    expect_true(p_extreme > 50.0 && p_extreme < 100000.0);

    let wd = HeartbeatWatchdog.init();
    const cfg = WatchdogNodeConfig.phiDefaults();
    // Node 10: steady 1000ms heartbeats (low variance)
    // Node 20: jittery heartbeats alternating 600ms / 1400ms (std_dev ~ 400ms)
    * = wd.registerNode(10, cfg, 0);
    * = wd.registerNode(20, cfg, 0);

    let t10: u64 = 0;
    let t20: u64 = 0;
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        t10 += 1000;
        * = wd.heartbeat(10, t10);
        t20 += if ((i & 1) == 0) 600 else 1400;
        * = wd.heartbeat(20, t20);
    }

    // Both nodes last beat at t=10000. At t=11800 (1800ms silence):
    // Steady node 10 has very high phi (dead), while jittery node 20 still has low phi (healthy)!
    const phi10 = wd.phiOf(10, 11800) ?? 0.0;
    const phi20 = wd.phiOf(20, 11800) ?? 0.0;
    expect_true(phi10 > 8.0);
    expect_true(phi20 < 3.0);

    let evs: [4]WatchdogTransitionEvent = [WatchdogTransitionEvent{}; 4];
    let evs_s: []WatchdogTransitionEvent = evs[0..];
    const n = wd.pollTransitions(11800, *evs_s);
    expect_eq(n, 1 as usize);
    expect_eq(evs[0].node_id, 10 as u64);
    expect_eq(evs[0].kind, WatchdogEventKind.became_dead);
    expect_eq((wd.getNode(20) ?? WatchdogNodeState{}).state, WatchdogState.healthy);
}

test("Watchdog: pause / resume maintenance window exemption & clock reset") {
    let wd = HeartbeatWatchdog.init();
    let cfg = WatchdogNodeConfig.fixedDefaults();
    cfg.suspect_timeout_ms = 1000;
    cfg.dead_timeout_ms = 2000;

    * = wd.registerNode(1, cfg, 1000);
    expect_true(wd.pauseNode(1, 1500));
    expect_eq((wd.getNode(1) ?? WatchdogNodeState{}).state, WatchdogState.paused);
    expect_eq(wd.phiOf(1, 50000) ?? 99.0, 0.0 as f64);

    // Long silence during maintenance produces ZERO failure transitions
    let evs: [4]WatchdogTransitionEvent = [WatchdogTransitionEvent{}; 4];
    let evs_s: []WatchdogTransitionEvent = evs[0..];
    expect_eq(wd.pollTransitions(50000, *evs_s), 0 as usize);

    // Resume at t=50000 resets heartbeat clock -> stays healthy at t=50500
    expect_true(wd.resumeNode(1, 50000));
    expect_eq(wd.pollTransitions(50500, *evs_s), 0 as usize);
    expect_eq((wd.getNode(1) ?? WatchdogNodeState{}).state, WatchdogState.healthy);
    // Resuming an already-healthy node returns false
    expect_eq(wd.resumeNode(1, 50600), false);
}

test("Watchdog: 16-sample circular ring wrap-around & monotonic clock rollback safety") {
    let wd = HeartbeatWatchdog.init();
    * = wd.registerNode(1, WatchdogNodeConfig.phiDefaults(), 0);

    // Feed 24 heartbeats (first 8 at 2000ms, next 16 at 500ms) -> ring retains only last 16 (mean == 500ms)
    let now: u64 = 0;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        now += 2000;
        * = wd.heartbeat(1, now);
    }
    let j: usize = 0;
    while (j < WATCHDOG_SAMPLE_WINDOW) : (j += 1) {
        now += 500;
        * = wd.heartbeat(1, now);
    }

    const node = wd.getNode(1) ?? WatchdogNodeState{};
    expect_eq(node.sample_count, WATCHDOG_SAMPLE_WINDOW);
    expect_true(node.meanIntervalMs() > 499.0 && node.meanIntervalMs() < 501.0);

    // Clock rollback (now_ms < last_heartbeat_ms) produces phi == 0.0 safely
    expect_eq(wd.phiOf(1, now - 1000) ?? 99.0, 0.0 as f64);
    expect_true(wd.heartbeat(1, now - 500));
}

test("Watchdog: multi-node batch pollTransitions & output buffer truncation safety") {
    let wd = HeartbeatWatchdog.init();
    let cfg = WatchdogNodeConfig.fixedDefaults();
    cfg.suspect_timeout_ms = 1000;
    cfg.dead_timeout_ms = 3000;

    * = wd.registerNode(1, cfg, 1000);
    * = wd.registerNode(2, cfg, 1000);
    * = wd.registerNode(3, cfg, 1000);

    // Only 2-slot output buffer while 3 nodes simultaneously become suspect -> truncation safe
    let evs: [2]WatchdogTransitionEvent = [WatchdogTransitionEvent{}; 2];
    let evs_s: []WatchdogTransitionEvent = evs[0..];
    const written = wd.pollTransitions(2500, *evs_s);
    expect_eq(written, 2 as usize);

    const st = wd.stats();
    expect_eq(st.suspect_count, 3 as usize);
    expect_eq(st.total_suspect_events, 3 as u64);
}

test("Watchdog: 64-node capacity saturation, invalid guards & cap.time facade parity") {
    let bad_cfg = WatchdogNodeConfig.fixedDefaults();
    bad_cfg.suspect_timeout_ms = 5000;
    bad_cfg.dead_timeout_ms = 2000; // dead < suspect invalid
    expect_eq(bad_cfg.isValid(), false);

    let wd = cap.time.HeartbeatWatchdog.init();
    expect_true(wd.registerNode(0, WatchdogNodeConfig.fixedDefaults(), 100) == null);
    expect_true(wd.registerNode(1, bad_cfg, 100) == null);

    let id: u64 = 1;
    while (id <= (WATCHDOG_MAX_NODES as u64)) : (id += 1) {
        expect_true(wd.registerNode(id, WatchdogNodeConfig.fixedDefaults(), 100) != null);
    }
    expect_eq(wd.stats().node_count, WATCHDOG_MAX_NODES);
    expect_true(wd.registerNode(1, WatchdogNodeConfig.fixedDefaults(), 100) == null);  // duplicate
    expect_true(wd.registerNode(99, WatchdogNodeConfig.fixedDefaults(), 100) == null); // full

    // Unknown node operations return false / null
    expect_eq(wd.heartbeat(999, 200), false);
    expect_eq(wd.pauseNode(999, 200), false);
    expect_true(wd.phiOf(999, 200) == null);

    // Unregister and reuse slot
    expect_true(wd.unregisterNode(10));
    expect_eq(wd.stats().node_count, WATCHDOG_MAX_NODES - 1);
    expect_true(wd.registerNode(100, WatchdogNodeConfig.phiDefaults(), 200) != null);
}
