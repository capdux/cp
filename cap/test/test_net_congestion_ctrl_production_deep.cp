// Origin: Pure-CP Stdlib Test Suite (lib/net/congestion_ctrl.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc RFC 9002 Congestion Controller & Pacing Engine:
//   1. RFC 9002 §5.3 & §6.2.1 first-sample & multi-sample SRTT, RTTVAR, min_rtt, ack_delay clamp & PTO
//   2. Slow Start exponential growth & app-limited growth suppression
//   3. Loss -> Recovery cwnd halving + single-RTT recovery guard (no double halving in same RTT)
//   4. Recovery exit -> Congestion Avoidance RFC 9002 linear growth
//   5. RFC 9002 §7.6 Persistent Congestion collapse to min_cwnd & recovery state reset
//   6. bytes_in_flight accounting, underflow protection, available_window & can_send gating
//   7. Pacing rate calculation (u128 overflow-free) & invalid CongestionConfig boundary guards

import { test, expect_eq } from "cap:test";
import {
    CongestionPhase,
    CongestionConfig,
    RttEstimate,
    CongestionStats,
    Rfc9002CongestionController,
} from "cap:net";

// ============================================================================
// Test 1: RFC 9002 §5.3 SRTT / RTTVAR / min_rtt / ack_delay & §6.2.1 PTO
// ============================================================================
test("congestion_ctrl: RFC 9002 SRTT, RTTVAR, min_rtt & PTO exact derivation", () => {
    let cc = Rfc9002CongestionController.init(CongestionConfig.defaults()) ?? return;

    // Initial state before any RTT sample: srtt=333000, rttvar=166500
    expect_eq(cc.rtt.has_sample, false);
    expect_eq(cc.rtt.smoothed_rtt_us, 333000);
    expect_eq(cc.rtt.rttvar_us, 166500);
    // Initial PTO with max_ack_delay=25000: 333000 + 4*166500 + 25000 = 1024000
    expect_eq(cc.rtt.pto_us(25000), 1024000);

    // First RTT sample: latest=80000us -> min_rtt=80000, srtt=80000, rttvar=40000
    cc.on_rtt_sample(80000, 10000, 25000);
    expect_eq(cc.rtt.has_sample, true);
    expect_eq(cc.rtt.min_rtt_us, 80000);
    expect_eq(cc.rtt.smoothed_rtt_us, 80000);
    expect_eq(cc.rtt.rttvar_us, 40000);
    // PTO = 80000 + max(160000, 1000) + 25000 = 265000
    expect_eq(cc.rtt.pto_us(25000), 265000);

    // Second sample: latest=100000, ack_delay=30000 (clamped to max_ack_delay=20000).
    // Since 100000 >= 80000 + 20000 -> adjusted_rtt = 80000.
    // abs_diff = |80000 - 80000| = 0 -> rttvar = (3*40000 + 0)/4 = 30000; srtt = (7*80000 + 80000)/8 = 80000.
    cc.on_rtt_sample(100000, 30000, 20000);
    expect_eq(cc.rtt.min_rtt_us, 80000);
    expect_eq(cc.rtt.rttvar_us, 30000);
    expect_eq(cc.rtt.smoothed_rtt_us, 80000);

    // Third sample: lower RTT=60000 -> min_rtt drops to 60000; ack_delay=15000 cannot be subtracted
    // because 60000 < 60000 + 15000 -> adjusted_rtt = 60000.
    // abs_diff = |80000 - 60000| = 20000 -> rttvar = (3*30000 + 20000)/4 = 27500; srtt = (7*80000 + 60000)/8 = 77500.
    cc.on_rtt_sample(60000, 15000, 25000);
    expect_eq(cc.rtt.min_rtt_us, 60000);
    expect_eq(cc.rtt.rttvar_us, 27500);
    expect_eq(cc.rtt.smoothed_rtt_us, 77500);

    // Verify kGranularity (1000us) floor in PTO when rttvar is tiny
    const tiny_rtt = RttEstimate{
        has_sample: true,
        latest_rtt_us: 2000,
        min_rtt_us: 2000,
        smoothed_rtt_us: 2000,
        rttvar_us: 100, // 4 * 100 = 400 < kGranularity(1000)
    };
    expect_eq(tiny_rtt.pto_us(500), 3500); // 2000 + 1000 + 500
});

// ============================================================================
// Test 2: Slow Start Exponential Growth & App-Limited Growth Suppression
// ============================================================================
test("congestion_ctrl: Slow Start exponential growth & app-limited suppression", () => {
    let cc = Rfc9002CongestionController.init(CongestionConfig.defaults()) ?? return;
    expect_eq(cc.stats().phase == CongestionPhase.slow_start, true);
    expect_eq(cc.stats().cwnd_bytes, 12000);

    // Send & ack 3600B not app-limited -> cwnd grows exponentially: 12000 + 3600 = 15600
    cc.on_packet_sent(3600);
    cc.on_packets_acked(3600, 1000, false);
    expect_eq(cc.stats().cwnd_bytes, 15600);
    expect_eq(cc.stats().bytes_in_flight, 0);
    expect_eq(cc.stats().phase == CongestionPhase.slow_start, true);

    // Send & ack 2400B while app-limited -> bytes_in_flight drops, cwnd MUST NOT grow
    cc.on_packet_sent(2400);
    cc.on_packets_acked(2400, 2000, true);
    expect_eq(cc.stats().cwnd_bytes, 15600);
    expect_eq(cc.stats().bytes_in_flight, 0);
});

// ============================================================================
// Test 3: Loss -> Recovery Halving & Single-RTT Guard Against Double Halving
// ============================================================================
test("congestion_ctrl: recovery halving & single-RTT double-halving guard", () => {
    let cc = Rfc9002CongestionController.init(CongestionConfig.defaults()) ?? return;
    // Grow cwnd to 20000
    cc.on_packet_sent(8000);
    cc.on_packets_acked(8000, 1000, false);
    expect_eq(cc.stats().cwnd_bytes, 20000);

    // Send 3 packets at t=2000, 2100, 2200
    cc.on_packet_sent(3600);

    // First loss detected at now=5000 for packet sent at t=2000 -> enters recovery, cwnd = ssthresh = 10000
    cc.on_congestion_event(1200, 2000, 5000);
    const s1: CongestionStats = cc.stats();
    expect_eq(s1.phase == CongestionPhase.recovery, true);
    expect_eq(s1.cwnd_bytes, 10000);
    expect_eq(s1.ssthresh_bytes, 10000);
    expect_eq(s1.loss_events, 1);
    expect_eq(s1.bytes_in_flight, 2400);

    // Second loss in same RTT (sent at t=2200 <= recovery_start_us=5000) -> MUST NOT halve again!
    cc.on_congestion_event(1200, 2200, 5100);
    const s2: CongestionStats = cc.stats();
    expect_eq(s2.phase == CongestionPhase.recovery, true);
    expect_eq(s2.cwnd_bytes, 10000);
    expect_eq(s2.loss_events, 1);
    expect_eq(s2.bytes_in_flight, 1200);

    // Ack for packet sent at t=2100 (<= recovery_start_us=5000) -> stays in recovery, no cwnd growth
    cc.on_packets_acked(1200, 2100, false);
    expect_eq(cc.stats().phase == CongestionPhase.recovery, true);
    expect_eq(cc.stats().cwnd_bytes, 10000);
    expect_eq(cc.stats().bytes_in_flight, 0);
});

// ============================================================================
// Test 4: Exit Recovery -> Congestion Avoidance Linear Growth
// ============================================================================
test("congestion_ctrl: exit recovery into Congestion Avoidance linear growth", () => {
    let cc = Rfc9002CongestionController.init(CongestionConfig.defaults()) ?? return;
    // Trigger recovery from initial 12000 -> ssthresh=6000, cwnd=6000 at now=5000
    cc.on_packet_sent(1200);
    cc.on_congestion_event(1200, 1000, 5000);
    expect_eq(cc.stats().phase == CongestionPhase.recovery, true);
    expect_eq(cc.stats().cwnd_bytes, 6000);

    // Packet sent strictly after recovery_start_us (sent=5500 > 5000) gets acked -> exits recovery!
    // Linear growth: inc = max_datagram_size(1200) * acked(6000) / cwnd(6000) = 1200 -> cwnd = 7200
    cc.on_packet_sent(6000);
    cc.on_packets_acked(6000, 5500, false);
    const s = cc.stats();
    expect_eq(s.phase == CongestionPhase.congestion_avoidance, true);
    expect_eq(s.cwnd_bytes, 7200);
    expect_eq(s.ssthresh_bytes, 6000);

    // Another ack of 3600B -> inc = 1200 * 3600 / 7200 = 600 -> cwnd = 7800
    cc.on_packet_sent(3600);
    cc.on_packets_acked(3600, 6000, false);
    expect_eq(cc.stats().cwnd_bytes, 7800);
    expect_eq(cc.stats().phase == CongestionPhase.congestion_avoidance, true);
});

// ============================================================================
// Test 5: RFC 9002 §7.6 Persistent Congestion Collapse to min_cwnd
// ============================================================================
test("congestion_ctrl: persistent congestion collapses cwnd to min_cwnd & resets recovery", () => {
    let cc = Rfc9002CongestionController.init(CongestionConfig.defaults()) ?? return;
    cc.on_packet_sent(12000);
    cc.on_packets_acked(12000, 1000, false); // cwnd = 24000
    cc.on_congestion_event(1200, 2000, 5000); // ssthresh = 12000, cwnd = 12000, recovery_start = 5000
    expect_eq(cc.stats().phase == CongestionPhase.recovery, true);

    // Persistent congestion collapses cwnd to min_cwnd (2400) and resets recovery_start_us = 0
    cc.on_persistent_congestion();
    const s = cc.stats();
    expect_eq(s.cwnd_bytes, 2400);
    expect_eq(s.persistent_congestion_events, 1);
    expect_eq(s.phase == CongestionPhase.slow_start, true); // cwnd(2400) < ssthresh(12000)

    // Subsequent ack grows via slow start immediately (even with timestamp < old recovery_start 5000)
    cc.on_packet_sent(1200);
    cc.on_packets_acked(1200, 3000, false);
    expect_eq(cc.stats().cwnd_bytes, 3600);
});

// ============================================================================
// Test 6: bytes_in_flight Underflow Guard, available_window & can_send Gating
// ============================================================================
test("congestion_ctrl: bytes_in_flight underflow guard & available_window gating", () => {
    let cc = Rfc9002CongestionController.init(CongestionConfig.defaults()) ?? return;
    expect_eq(cc.available_window(), 12000);
    expect_eq(cc.can_send(12000), true);
    expect_eq(cc.can_send(12001), false);

    cc.on_packet_sent(10000);
    expect_eq(cc.available_window(), 2000);
    expect_eq(cc.can_send(2000), true);
    expect_eq(cc.can_send(2001), false);

    // Over-sending beyond cwnd clamps available_window to 0 (no underflow)
    cc.on_packet_sent(5000);
    expect_eq(cc.stats().bytes_in_flight, 15000);
    expect_eq(cc.available_window(), 0);
    expect_eq(cc.can_send(1), false);

    // Over-acking / over-losing more bytes than in-flight clamps bytes_in_flight to 0 safely
    cc.on_packets_acked(50000, 1000, true);
    expect_eq(cc.stats().bytes_in_flight, 0);
    cc.on_congestion_event(99999, 2000, 3000);
    expect_eq(cc.stats().bytes_in_flight, 0);
});

// ============================================================================
// Test 7: Pacing Rate Derivation & Invalid CongestionConfig Boundary Guards
// ============================================================================
test("congestion_ctrl: pacing rate calculation & invalid config guards", () => {
    // Invalid configs must return null
    expect_eq(Rfc9002CongestionController.init(CongestionConfig{ max_datagram_size: 0 }) == null, true);
    expect_eq(Rfc9002CongestionController.init(CongestionConfig{ min_cwnd_bytes: 600, max_datagram_size: 1200 }) == null, true);
    expect_eq(Rfc9002CongestionController.init(CongestionConfig{ initial_cwnd_bytes: 1200, min_cwnd_bytes: 2400 }) == null, true);
    expect_eq(Rfc9002CongestionController.init(CongestionConfig{ initial_rtt_us: 0 }) == null, true);
    expect_eq(Rfc9002CongestionController.init(CongestionConfig{ loss_reduction_bps: 0 }) == null, true);
    expect_eq(Rfc9002CongestionController.init(CongestionConfig{ loss_reduction_bps: 10000 }) == null, true);
    expect_eq(Rfc9002CongestionController.init(CongestionConfig{ pacing_gain_bps: 9999 }) == null, true);

    // Exact pacing rate: cwnd=12000B, gain=1.25x (12500 bps), srtt=10ms (10000us)
    // rate = (12000 * 1.25) / 0.01s = 1,500,000 bytes/sec
    let cc = Rfc9002CongestionController.init(CongestionConfig.defaults()) ?? return;
    cc.on_rtt_sample(10000, 0, 25000);
    expect_eq(cc.pacing_rate_bytes_per_sec(), 1500000);
    expect_eq(cc.stats().pacing_rate_bytes_per_sec, 1500000);

    // Minimum cwnd floor on loss: cwnd=3000 * 0.5 = 1500 < min_cwnd(2400) -> clamped to 2400
    let cc_min = Rfc9002CongestionController.init(CongestionConfig{
        max_datagram_size: 1200,
        initial_cwnd_bytes: 3000,
        min_cwnd_bytes: 2400,
        initial_rtt_us: 50000,
        loss_reduction_bps: 5000,
        pacing_gain_bps: 12500,
    }) ?? return;
    cc_min.on_congestion_event(1200, 100, 200);
    expect_eq(cc_min.stats().cwnd_bytes, 2400);
    expect_eq(cc_min.stats().ssthresh_bytes, 2400);
});
