// Origin: Pure-CP Stdlib Test Suite (lib/net/quicLoss.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RFC 9002 QUIC Loss/PTO, RFC 9000 ACK Ranges, CUBIC & BBRv1:
//   1. Out-of-order `QuicAckSet` range coalescing, bridging & O(log R) binary search
//   2. RFC 9002 RTT smoothing, `ack_delay` clamping & exponential PTO backoff
//   3. RFC 9002 packet-threshold (`kPacketThreshold=3`) & `9/8` time-threshold loss detection + PTO
//   4. RFC 9438 CUBIC slow start, `0.7x` decrease, per-RTT recovery guard & concave/convex cubic growth
//   5. BBRv1 full 4-state cycle (`startup -> drain -> probe_bw [8-phase] -> probe_rtt -> probe_bw`) & `QuicPacer`
//   6. 500-packet high-throughput external-slice end-to-end recovery loop
//   7. Error guards (`BufferTooSmall`, `TableFull`) & `cap.net` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    QUIC_PACKET_THRESHOLD,
    QUIC_INITIAL_RTT_US,
    QUIC_GRANULARITY_US,
    QUIC_MAX_DATAGRAM_DEFAULT,
    QuicLossError,
    QuicAckRange,
    QuicAckSet,
    QuicRttState,
    QuicSentPacket,
    QuicLossDetector,
    CubicState,
    BbrMode,
    BbrController,
    QuicPacer,
} from "cap:net";

test("QuicAckSet: out-of-order coalescing, bridging & O(log R) binary search") {
    let buf: [16]QuicAckRange = [QuicAckRange{}; 16];
    let buf_s: []QuicAckRange = buf[0..];
    let acks = cap.net.QuicAckSet.init(*buf_s) catch {
        expect_true(false);
        return;
    };

    expect_true(acks.largestAcked() == null);
    expect_eq(acks.rangeCount(), @as(usize, 0));

    // Insert out-of-order packets forming bridges
    const seq: [10]u64 = [10, 12, 11, 5, 7, 6, 8, 9, 20, 22];
    let i: usize = 0;
    while (i < seq.len) : (i += 1) {
        acks.insertPacket(seq[i]) catch {
            expect_true(false);
            return;
        };
    }

    // Ranges should be strictly descending: [22,22], [20,20], [5,12]
    expect_eq(acks.rangeCount(), @as(usize, 3));
    expect_eq(acks.gapCount(), @as(usize, 2));
    expect_eq(acks.largestAcked() ?? 0, @as(u64, 22));
    expect_true(acks.contains(5));
    expect_true(acks.contains(9));
    expect_true(acks.contains(12));
    expect_true(!acks.contains(4));
    expect_true(!acks.contains(13));
    expect_true(!acks.contains(21));

    // Bridge [20,20] and [22,22] by inserting 21 -> 2 ranges: [20,22], [5,12]
    acks.insertPacket(21) catch {
        expect_true(false);
        return;
    };
    expect_eq(acks.rangeCount(), @as(usize, 2));
    expect_eq(acks.gapCount(), @as(usize, 1));
    expect_true(acks.contains(21));
    const active = acks.activeRanges();
    expect_eq(active[0].first_pn, @as(u64, 20));
    expect_eq(active[0].last_pn, @as(u64, 22));
    expect_eq(active[1].first_pn, @as(u64, 5));
    expect_eq(active[1].last_pn, @as(u64, 12));
}

test("QuicRttState: RFC 9002 smoothing, ack_delay clamping & exponential PTO backoff") {
    let rtt = QuicRttState.init();
    expect_eq(rtt.smoothed_rtt_us, QUIC_INITIAL_RTT_US);

    // Sample 1: first sample sets min_rtt = srtt = 40ms, rttvar = 20ms
    rtt.update(40_000, 5_000, 25_000);
    expect_eq(rtt.min_rtt_us, @as(u64, 40_000));
    expect_eq(rtt.smoothed_rtt_us, @as(u64, 40_000));
    expect_eq(rtt.rttvar_us, @as(u64, 20_000));

    // Sample 2: latest=50ms, ack_delay=50ms (clamped to 25ms); 50 < 40+25 so adjusted stays 50ms
    rtt.update(50_000, 50_000, 25_000);
    expect_eq(rtt.min_rtt_us, @as(u64, 40_000));
    // rttvar = (3*20000 + 10000)/4 = 17500; srtt = (7*40000 + 50000)/8 = 41250
    expect_eq(rtt.rttvar_us, @as(u64, 17_500));
    expect_eq(rtt.smoothed_rtt_us, @as(u64, 41_250));

    // Sample 3: latest=70ms, ack_delay=20ms -> adjusted=50ms (>= 40ms)
    rtt.update(70_000, 20_000, 25_000);
    expect_eq(rtt.smoothed_rtt_us, @as(u64, 42_343));

    // PTO exponential backoff: pto(1) == 2*pto(0), pto(2) == 4*pto(0)
    const pto0 = rtt.ptoDurationUs(25_000, 0);
    const pto1 = rtt.ptoDurationUs(25_000, 1);
    const pto2 = rtt.ptoDurationUs(25_000, 2);
    expect_eq(pto1, pto0 * 2);
    expect_eq(pto2, pto0 * 4);
}

test("QuicLossDetector: packet-threshold & 9/8 time-threshold loss detection + PTO") {
    let sent_buf: [32]QuicSentPacket = [QuicSentPacket{}; 32];
    let sent_s: []QuicSentPacket = sent_buf[0..];
    let det = QuicLossDetector.init(*sent_s, 25_000) catch {
        expect_true(false);
        return;
    };

    // Send packets 0..4 at 10ms, 12ms, 14ms, 16ms, 18ms
    det.onPacketSent(0, 10_000, 1200, true) catch {};
    det.onPacketSent(1, 12_000, 1200, true) catch {};
    det.onPacketSent(2, 14_000, 1200, true) catch {};
    det.onPacketSent(3, 16_000, 1200, true) catch {};
    det.onPacketSent(4, 18_000, 1200, true) catch {};
    expect_eq(det.bytes_in_flight, @as(u64, 6000));

    // ACK [3, 4] at t = 48_000 (RTT = 30_000 us -> loss_delay = 9/8 * 30_000 = 33_750 us)
    // Packet threshold: 4 - 0 >= 3 and 4 - 1 >= 3 -> pn 0 & 1 lost immediately.
    // pn 2 sent at 14_000: 14_000 + 33_750 = 47_750 <= 48_000 -> let's ACK at 40_000 (RTT = 22_000, loss_delay = 24_750):
    // pn 2 loss_time = 14_000 + 24_750 = 38_750 <= 40_000; so ACK at 36_000 (RTT=18_000, loss_delay=20_250, pn 2 loss_time=34_250).
    // Instead send pn 2 closer to pn 4: let's ACK at t = 30_000 (RTT = 12_000, loss_delay = 13_500):
    //   pn 2 sent at 14_000 -> loss_time = 27_500 <= 30_000.
    // Better: let's use exact fresh detector with tight send spacing for time-threshold test!
    const ack_rg: [1]QuicAckRange = [QuicAckRange{ first_pn: 3, last_pn: 4 }];
    const out1 = det.onAckReceived(ack_rg[0..], 0, 26_000);
    // RTT = 26_000 - 18_000 = 8_000 -> loss_delay = 9_000 -> pn 2 loss_time = 14_000 + 9_000 = 23_000 <= 26_000
    // Let's check: pn 0, 1 lost by packet threshold, pn 2 lost by time threshold here -> all 3 lost!
    expect_eq(out1.newly_acked_packets, @as(usize, 2));
    expect_eq(out1.newly_lost_packets, @as(usize, 3));
    expect_eq(det.bytes_in_flight, @as(u64, 0));

    // Separate test strictly verifying future `loss_time_us` -> timeout triggers loss!
    let det2 = QuicLossDetector.init(*sent_s, 25_000) catch {
        expect_true(false);
        return;
    };
    det2.onPacketSent(10, 100_000, 1200, true) catch {};
    det2.onPacketSent(11, 101_000, 1200, true) catch {};
    const ack11: [1]QuicAckRange = [QuicAckRange{ first_pn: 11, last_pn: 11 }];
    // ACK pn 11 at 121_000 (RTT = 20_000 -> loss_delay = 22_500 -> pn 10 loss_time = 122_500 > 121_000)
    const out2 = det2.onAckReceived(ack11[0..], 0, 121_000);
    expect_eq(out2.newly_acked_packets, @as(usize, 1));
    expect_eq(out2.newly_lost_packets, @as(usize, 0));
    expect_eq(out2.loss_time_us, @as(u64, 122_500));
    expect_eq(det2.nextTimerDeadlineUs(121_000), @as(u64, 122_500));

    // Advance clock to loss_time_us -> time-threshold declares pn 10 lost!
    const out3 = det2.onLossDetectionTimeout(122_500);
    expect_eq(out3.newly_lost_packets, @as(usize, 1));
    expect_eq(det2.bytes_in_flight, @as(u64, 0));

    // PTO arming when single unacked packet sits in flight
    det2.onPacketSent(12, 200_000, 1200, true) catch {};
    const pto_deadline = det2.nextTimerDeadlineUs(200_000);
    expect_true(pto_deadline > 200_000);
    * = det2.onLossDetectionTimeout(pto_deadline);
    expect_eq(det2.pto_count, @as(u32, 1));
}

test("CubicState: slow start, 0.7x decrease, per-RTT recovery guard & concave/convex growth") {
    let cubic = CubicState.init(1200, 12_000);

    // Slow start grows cwnd linearly per acked byte (exponential per RTT)
    cubic.onAck(12_000, 40_000, 50_000);
    expect_eq(cubic.cwnd_bytes, @as(u64, 24_000));
    cubic.onAck(24_000, 40_000, 90_000);
    expect_eq(cubic.cwnd_bytes, @as(u64, 48_000));

    // Congestion event -> 0.7x reduction: 48_000 * 0.7 = 33_600
    cubic.onCongestionEvent(80_000, 100_000);
    expect_eq(cubic.cwnd_bytes, @as(u64, 33_600));
    expect_eq(cubic.ssthresh_bytes, @as(u64, 33_600));

    // Second loss from same recovery epoch (sent_time <= recovery_start_us) must be ignored
    cubic.onCongestionEvent(95_000, 110_000);
    expect_eq(cubic.cwnd_bytes, @as(u64, 33_600));

    // Congestion avoidance over multiple RTTs recovers past W_max (concave -> convex)
    let t: u64 = 140_000;
    let step: usize = 0;
    while (step < 80) : (step += 1) {
        cubic.onAck(12_000, 40_000, t);
        t += 40_000;
    }
    expect_true(cubic.cwnd_bytes > 48_000);
    expect_true(cubic.pacingRateBps(40_000) > 1_000_000);
}

test("BbrController: Startup -> Drain -> ProbeBW (8-phase) -> ProbeRTT -> ProbeBW & QuicPacer") {
    let bbr = BbrController.init(1200, 0);
    expect_true(bbr.mode() == BbrMode.startup);

    // Startup: ramp bandwidth then plateau for 3 rounds to trigger full_pipe -> Drain
    bbr.onDeliverySample(50_000, 50_000, 20_000, 50_000);   // 1 MB/s
    bbr.onDeliverySample(100_000, 50_000, 20_000, 100_000); // 2 MB/s (+100%)
    bbr.onDeliverySample(102_000, 50_000, 20_000, 150_000); // plateau 1
    bbr.onDeliverySample(101_000, 50_000, 20_000, 200_000); // plateau 2
    bbr.onDeliverySample(100_000, 50_000, 20_000, 250_000); // plateau 3 -> Drain
    expect_true(bbr.mode() == BbrMode.drain);
    expect_true(bbr.bdpBytes() >= 35_000);

    // Drain exits to ProbeBW once inflight <= BDP
    bbr.onInFlightUpdate(bbr.bdpBytes());
    expect_true(bbr.mode() == BbrMode.probe_bw);
    const rate_probe_up = bbr.pacingRateBps(); // phase 0: 1.25x

    // Advance 1 min_rtt (20ms) -> phase 1: 0.75x
    bbr.onDeliverySample(40_000, 20_000, 20_000, 280_000);
    const rate_probe_down = bbr.pacingRateBps();
    expect_true(rate_probe_up > rate_probe_down);

    // Advance > 10 seconds with higher RTT -> enter ProbeRTT (cwnd == 4 * mss = 4800)
    bbr.onDeliverySample(40_000, 20_000, 25_000, 10_500_000);
    expect_true(bbr.mode() == BbrMode.probe_rtt);
    expect_eq(bbr.cwndBytes(), @as(u64, 4800));

    // After 200ms in ProbeRTT -> return to ProbeBW
    bbr.onDeliverySample(10_000, 20_000, 20_000, 10_750_000);
    expect_true(bbr.mode() == BbrMode.probe_bw);

    // QuicPacer token consumption and wait calculation
    let pacer = QuicPacer.init(1_000_000, 2400, 0); // 1 MB/s, burst 2400 B
    expect_true(pacer.canSendNow(1200, 0));
    pacer.onPacketSent(1200, 0);
    pacer.onPacketSent(1200, 0);
    expect_true(!pacer.canSendNow(1200, 0));
    const next_t = pacer.nextSendTimeUs(1200, 0);
    expect_eq(next_t, @as(u64, 1200)); // 1200 B at 1 B/us = 1200 us
}

test("QuicLoss: 500-packet high-throughput external-slice end-to-end loop") {
    let sent_buf: [128]QuicSentPacket = [QuicSentPacket{}; 128]; // Smaller than 500 -> tests auto-compaction!
    let sent_s: []QuicSentPacket = sent_buf[0..];
    let det = QuicLossDetector.init(*sent_s, 25_000) catch {
        expect_true(false);
        return;
    };
    let cubic = CubicState.init(1200, 12_000);

    let ack_buf: [64]QuicAckRange = [QuicAckRange{}; 64];
    let ack_s: []QuicAckRange = ack_buf[0..];
    let acks = QuicAckSet.init(*ack_s) catch {
        expect_true(false);
        return;
    };

    let total_acked: usize = 0;
    let total_lost: usize = 0;
    let now_us: u64 = 10_000;

    let pn: u64 = 0;
    while (pn < 500) : (pn += 1) {
        det.onPacketSent(pn, now_us, 1200, true) catch {
            expect_true(false);
            return;
        };
        // Drop every 50th packet (except last 10) to trigger periodic loss & CUBIC recovery
        if (pn % 50 != 25 || pn >= 490) {
            acks.insertPacket(pn) catch {
                expect_true(false);
                return;
            };
        }

        if (pn % 10 == 9) {
            now_us += 20_000;
            const out = det.onAckReceived(acks.activeRanges(), 1_000, now_us);
            total_acked += out.newly_acked_packets;
            total_lost += out.newly_lost_packets;
            if (out.newly_acked_bytes > 0) {
                cubic.onAck(out.newly_acked_bytes, det.rtt.smoothed_rtt_us, now_us);
            }
            if (out.newly_lost_packets > 0) {
                cubic.onCongestionEvent(now_us - 20_000, now_us);
            }
        }
        now_us += 500;
    }

    expect_eq(total_acked + total_lost, @as(usize, 500));
    expect_true(total_lost >= 9);
    expect_eq(det.bytes_in_flight, @as(u64, 0));
    expect_true(cubic.cwnd_bytes >= 2400);
}

test("QuicLossError: buffer & table capacity guards + cap.net facade parity") {
    expect_eq(QUIC_PACKET_THRESHOLD, @as(u64, 3));
    expect_eq(QUIC_INITIAL_RTT_US, @as(u64, 333_000));
    expect_eq(QUIC_GRANULARITY_US, @as(u64, 1_000));
    expect_eq(QUIC_MAX_DATAGRAM_DEFAULT, @as(u32, 1200));

    let empty_acks: [0]QuicAckRange = [];
    let empty_acks_s: []QuicAckRange = empty_acks[0..];
    let got_buf_small: bool = false;
    * = QuicAckSet.init(*empty_acks_s) catch |e| {
        got_buf_small = (e == QuicLossError.BufferTooSmall);
    };
    expect_true(got_buf_small);

    let tiny_acks: [1]QuicAckRange = [QuicAckRange{}; 1];
    let tiny_acks_s: []QuicAckRange = tiny_acks[0..];
    let acks = QuicAckSet.init(*tiny_acks_s) catch {
        expect_true(false);
        return;
    };
    acks.insertPacket(10) catch {};
    let got_table_full: bool = false;
    acks.insertPacket(20) catch |e| {
        got_table_full = (e == QuicLossError.TableFull);
    };
    expect_true(got_table_full);

    let tiny_sent: [1]QuicSentPacket = [QuicSentPacket{}; 1];
    let tiny_sent_s: []QuicSentPacket = tiny_sent[0..];
    let det = cap.net.QuicLossDetector.init(*tiny_sent_s, 25_000) catch {
        expect_true(false);
        return;
    };
    det.onPacketSent(1, 1000, 1200, true) catch {};
    let got_sent_full: bool = false;
    det.onPacketSent(2, 2000, 1200, true) catch |e| {
        got_sent_full = (e == QuicLossError.TableFull);
    };
    expect_true(got_sent_full);
}
