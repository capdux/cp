// Origin: Pure-CP Stdlib Test Suite (AI Agent Cluster Liveness Heartbeat Monitor & Restart Storm Quarantine) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/liveness.cp
// Covers:
//   1. Regular heartbeat lease renewal keeping Agents Healthy & backward clock-skew defense
//   2. Two-stage timeout progression Healthy -> Suspect -> Unreachable & recovery on new heartbeat
//   3. Restart storm exceeding max_restarts_in_window triggering Quarantined & automatic expiry release
//   4. Sliding restart window expiration resetting restart counter without false quarantine
//   5. Multi-agent batch sweep truncation safety, stop/unregister, full pool & invalid config guards

import { test, expect_eq } from "cap:test";
import {
    AgentHealthState,
    LivenessConfig,
    AgentLivenessSlot,
    LivenessSummary,
    AgentLivenessMonitor,
} from "../../lib/agent/index.cp";

fn testHeartbeatRenewalAndClockSkewDefense()! {
    let slots_buf: [4]AgentLivenessSlot = [AgentLivenessSlot{}; 4];
    let slots: []AgentLivenessSlot = slots_buf[0..];
    const cfg = LivenessConfig{
        heartbeat_timeout_ms: 5000,
        suspect_grace_ms: 3000,
        max_restarts_in_window: 3,
        restart_window_ms: 60000,
        quarantine_duration_ms: 30000,
    };
    let mon = AgentLivenessMonitor.init(slots, cfg) ?? return;

    expect_eq(mon.registerAgent(101, 1000), 0);
    expect_eq(mon.heartbeat(101, 4000), true);
    expect_eq(mon.heartbeat(101, 8000), true);
    // Backward clock skew (timestamp 6000 < 8000) must not roll back last_heartbeat_ms
    expect_eq(mon.heartbeat(101, 6000), true);

    let unreach_buf: [4]u32 = [0; 4];
    let unreach: []u32 = unreach_buf[0..];
    expect_eq(mon.sweepTimeouts(12000, unreach), 0);
    expect_eq(mon.getState(101) == AgentHealthState.Healthy, true);
    expect_eq((mon.getSlot(101) ?? AgentLivenessSlot{}).last_heartbeat_ms, 8000);
}

fn testTwoStageTimeoutProgressionAndRecovery()! {
    let slots_buf: [4]AgentLivenessSlot = [AgentLivenessSlot{}; 4];
    let slots: []AgentLivenessSlot = slots_buf[0..];
    const cfg = LivenessConfig{
        heartbeat_timeout_ms: 5000,
        suspect_grace_ms: 3000,
        max_restarts_in_window: 3,
        restart_window_ms: 60000,
        quarantine_duration_ms: 30000,
    };
    let mon = AgentLivenessMonitor.init(slots, cfg) ?? return;
    * = mon.registerAgent(1, 1000);

    let unreach_buf: [4]u32 = [0; 4];
    let unreach: []u32 = unreach_buf[0..];

    // t=5999 (elapsed 4999 < 5000): still Healthy
    expect_eq(mon.sweepTimeouts(5999, unreach), 0);
    expect_eq(mon.getState(1) == AgentHealthState.Healthy, true);

    // t=6000 (elapsed 5000 >= 5000, < 8000): transitions to Suspect
    expect_eq(mon.sweepTimeouts(6000, unreach), 0);
    expect_eq(mon.getState(1) == AgentHealthState.Suspect, true);

    // t=9000 (elapsed 8000 >= 5000 + 3000): transitions to Unreachable and reported in out slice
    expect_eq(mon.sweepTimeouts(9000, unreach), 1);
    expect_eq(unreach[0], 1);
    expect_eq(mon.getState(1) == AgentHealthState.Unreachable, true);

    // Late heartbeat recovers Agent back to Healthy
    expect_eq(mon.heartbeat(1, 9500), true);
    expect_eq(mon.getState(1) == AgentHealthState.Healthy, true);
}

fn testRestartStormQuarantineAndExpiryRelease()! {
    let slots_buf: [4]AgentLivenessSlot = [AgentLivenessSlot{}; 4];
    let slots: []AgentLivenessSlot = slots_buf[0..];
    const cfg = LivenessConfig{
        heartbeat_timeout_ms: 5000,
        suspect_grace_ms: 2000,
        max_restarts_in_window: 2,
        restart_window_ms: 60000,
        quarantine_duration_ms: 20000,
    };
    let mon = AgentLivenessMonitor.init(slots, cfg) ?? return;
    * = mon.registerAgent(7, 1000);

    // Restarts 1 and 2 within window are tolerated -> Healthy
    expect_eq(mon.recordRestart(7, 2000) == AgentHealthState.Healthy, true);
    expect_eq(mon.recordRestart(7, 3000) == AgentHealthState.Healthy, true);

    // Restart 3 exceeds max_restarts_in_window (2) -> Quarantined until 4000 + 20000 = 24000
    expect_eq(mon.recordRestart(7, 4000) == AgentHealthState.Quarantined, true);
    expect_eq(mon.getState(7) == AgentHealthState.Quarantined, true);

    // Heartbeat during active quarantine is rejected
    expect_eq(mon.heartbeat(7, 15000), false);
    expect_eq(mon.getState(7) == AgentHealthState.Quarantined, true);

    // Sweep before expiry keeps Quarantined
    let unreach_buf: [4]u32 = [0; 4];
    let unreach: []u32 = unreach_buf[0..];
    expect_eq(mon.sweepTimeouts(23999, unreach), 0);
    expect_eq(mon.getState(7) == AgentHealthState.Quarantined, true);

    // Sweep at t=24000 releases quarantine back to Healthy with fresh window
    expect_eq(mon.sweepTimeouts(24000, unreach), 0);
    expect_eq(mon.getState(7) == AgentHealthState.Healthy, true);
    const rec = mon.getSlot(7) ?? AgentLivenessSlot{};
    expect_eq(rec.restarts_in_window, 0);
    expect_eq(rec.total_restarts, 3);
}

fn testRestartWindowExpiryResetsCounter()! {
    let slots_buf: [2]AgentLivenessSlot = [AgentLivenessSlot{}; 2];
    let slots: []AgentLivenessSlot = slots_buf[0..];
    const cfg = LivenessConfig{
        heartbeat_timeout_ms: 5000,
        suspect_grace_ms: 2000,
        max_restarts_in_window: 2,
        restart_window_ms: 10000,
        quarantine_duration_ms: 20000,
    };
    let mon = AgentLivenessMonitor.init(slots, cfg) ?? return;
    * = mon.registerAgent(9, 1000);

    // 2 restarts inside [1000, 11000)
    expect_eq(mon.recordRestart(9, 2000) == AgentHealthState.Healthy, true);
    expect_eq(mon.recordRestart(9, 5000) == AgentHealthState.Healthy, true);

    // Next restart at t=11500 (elapsed 10500 >= 10000) resets window -> counter becomes 1, stays Healthy!
    expect_eq(mon.recordRestart(9, 11500) == AgentHealthState.Healthy, true);
    const rec = mon.getSlot(9) ?? AgentLivenessSlot{};
    expect_eq(rec.restarts_in_window, 1);
    expect_eq(rec.total_restarts, 3);
}

fn testMultiAgentBatchSweepSummaryAndGuards()! {
    // 1. Invalid config & empty slot guards
    let empty_buf: [0]AgentLivenessSlot = [];
    let empty_slots: []AgentLivenessSlot = empty_buf[0..];
    expect_eq(AgentLivenessMonitor.init(empty_slots, LivenessConfig{}) == null, true);

    let slots_buf: [4]AgentLivenessSlot = [AgentLivenessSlot{}; 4];
    let slots: []AgentLivenessSlot = slots_buf[0..];
    expect_eq(AgentLivenessMonitor.init(slots, LivenessConfig{ heartbeat_timeout_ms: 0 }) == null, true);
    expect_eq(AgentLivenessMonitor.init(slots, LivenessConfig{ max_restarts_in_window: 0 }) == null, true);
    expect_eq(AgentLivenessMonitor.init(slots, LivenessConfig{ restart_window_ms: 0 }) == null, true);
    expect_eq(AgentLivenessMonitor.init(slots, LivenessConfig{ quarantine_duration_ms: 0 }) == null, true);

    const cfg = LivenessConfig{
        heartbeat_timeout_ms: 5000,
        suspect_grace_ms: 3000,
        max_restarts_in_window: 1,
        restart_window_ms: 60000,
        quarantine_duration_ms: 30000,
    };
    let mon = AgentLivenessMonitor.init(slots, cfg) ?? return;

    // Registration guards: id==0, duplicate, full pool
    expect_eq(mon.registerAgent(0, 1000) == null, true);
    expect_eq(mon.registerAgent(1, 1000), 0);
    expect_eq(mon.registerAgent(1, 1000) == null, true);
    expect_eq(mon.registerAgent(2, 1000), 1);
    expect_eq(mon.registerAgent(3, 1000), 2);
    expect_eq(mon.registerAgent(4, 1000), 3);
    expect_eq(mon.registerAgent(5, 1000) == null, true);

    // Agent 1 stays Healthy, Agent 2 & 3 go Unreachable, Agent 4 Stopped
    * = mon.heartbeat(1, 8000);
    expect_eq(mon.stopAgent(4), true);

    // Truncation-safe sweep with 1-element buffer reports 1 out of 2 unreachable agents without OOB
    let tiny_buf: [1]u32 = [0; 1];
    let tiny: []u32 = tiny_buf[0..];
    const written = mon.sweepTimeouts(10000, tiny);
    expect_eq(written, 1);
    expect_eq(tiny[0], 2);

    const sum: LivenessSummary = mon.summary();
    expect_eq(sum.total, 4);
    expect_eq(sum.healthy, 1);
    expect_eq(sum.suspect, 0);
    expect_eq(sum.unreachable_count, 2);
    expect_eq(sum.quarantined, 0);
    expect_eq(sum.stopped, 1);

    // Unregister frees slot for Agent 5
    expect_eq(mon.unregisterAgent(4), true);
    expect_eq(mon.unregisterAgent(4), false);
    expect_eq(mon.registerAgent(5, 10000), 3);
}

test("agent liveness: heartbeat lease renewal and backward clock-skew defense", testHeartbeatRenewalAndClockSkewDefense);
test("agent liveness: two-stage timeout progression Healthy -> Suspect -> Unreachable and recovery", testTwoStageTimeoutProgressionAndRecovery);
test("agent liveness: restart storm triggers Quarantined and auto-releases on expiry", testRestartStormQuarantineAndExpiryRelease);
test("agent liveness: sliding restart window expiration resets counter", testRestartWindowExpiryResetsCounter);
test("agent liveness: multi-agent batch sweep truncation, summary, stop/unregister and guards", testMultiAgentBatchSweepSummaryAndGuards);
