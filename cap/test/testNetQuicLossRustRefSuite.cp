// Origin: Wave 35 R10 Reference-Hardening Verification Suite (Lane 2F)
// Reference-hardened against quinn-rs/quinn (MIT OR Apache-2.0) & RFC 9002 / RFC 9000 / RFC 9438
// Covers:
//   1. RFC 9002 RTT estimator (§5.3) & clamped exponential PTO backoff (no overflow at extreme u64 / pto_count)
//   2. RFC 9000 QuicAckSet: inverted range rejection, out-of-order/overlap coalescing, capacity cap, u64::MAX wraparound guard
//   3. RFC 9002 QuicLossDetector: kPacketThreshold=3, 9/8 time threshold, PTO backoff & recovery
//   4. RFC 9438 CubicState, BBRv1 BbrController & QuicPacer: zero-div guards, min_cwnd floor, overflow-safe pacing & burst cap
//   5. 4-lane @Vector(4, u64) SIMD vs scalar 0-diff parity (simdQuicCountLossCandidates & simdQuicMinU64)
//   6. >= 1,600-step (1,800 steps) deterministic PRNG transport state-machine fuzz (0 panic / 0 div-zero / 0 overflow)

import { test, expect_eq } from "cap:test";
import {
    QUIC_PACKET_THRESHOLD,
    QUIC_INITIAL_RTT_US,
    QUIC_GRANULARITY_US,
    QuicLossError,
    scalarQuicMinU64,
    simdQuicMinU64,
    scalarQuicCountLossCandidates,
    simdQuicCountLossCandidates,
    QuicAckRange,
    QuicAckSet,
    QuicRttState,
    QuicSentPacket,
    QuicLossDetector,
    CubicState,
    BbrMode,
    BbrController,
    QuicPacer,
} from "../../lib/net/quicLoss.cp";

fn nextRand(s: u64): u64 {
    let x: u64 = s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x;
}

fn randOut(s: u64): u64 {
    return s *% @as(u64, 0x2545F4914F6CDD1D);
}

// ============================================================================
// 1. RFC 9002 RTT & Clamped PTO Backoff
// ============================================================================

fn checkRttAndPto(): u32 {
    let rtt = QuicRttState.init();
    if (rtt.smoothed_rtt_us != QUIC_INITIAL_RTT_US) return 1;
    if (rtt.rttvar_us != QUIC_INITIAL_RTT_US / 2) return 2;

    // First sample: min = srtt = latest, rttvar = latest / 2
    rtt.update(100_000, 10_000, 25_000);
    if (rtt.min_rtt_us != 100_000 || rtt.smoothed_rtt_us != 100_000 || rtt.rttvar_us != 50_000) return 3;

    // Second sample: latest = 140ms, ack_delay = 50ms clamped to max_ack_delay = 25ms -> adjusted = 115ms
    // diff = 15ms -> rttvar = (50*3 + 15)/4 = 41_250, srtt = (100*7 + 115)/8 = 101_875
    rtt.update(140_000, 50_000, 25_000);
    if (rtt.min_rtt_us != 100_000) return 4;
    if (rtt.rttvar_us != 41_250) return 5;
    if (rtt.smoothed_rtt_us != 101_875) return 6;

    // Exponential PTO backoff & shift-clamp safety (pto_count = 64 / 1000 must NOT overflow!)
    const p0 = rtt.ptoDurationUs(25_000, 0);
    const p1 = rtt.ptoDurationUs(25_000, 1);
    const p16 = rtt.ptoDurationUs(25_000, 16);
    const p64 = rtt.ptoDurationUs(25_000, 64);
    const p999 = rtt.ptoDurationUs(25_000, 999);
    if (p1 != p0 * 2) return 7;
    if (p64 != p16 || p999 != p16) return 8;

    // Extreme near-u64::MAX RTT saturates cleanly without panic
    let rtt_big = QuicRttState.init();
    const max_u64: u64 = ~@as(u64, 0);
    const huge: u64 = max_u64 >> 1;
    rtt_big.update(huge, huge, huge);
    rtt_big.update(huge, huge, huge);
    const p_sat = rtt_big.ptoDurationUs(huge, 100);
    if (p_sat != max_u64) return 9;
    return 0;
}

// ============================================================================
// 2. RFC 9000 QuicAckSet: Range Rejection, Coalescing, Capacity & u64::MAX
// ============================================================================

fn checkAckSet(): u32 {
    let buf: [4]QuicAckRange = [QuicAckRange{}; 4];
    let empty_s: []QuicAckRange = buf[0..0];
    let got_small = false;
    * = QuicAckSet.init(empty_s) catch |e| {
        got_small = (e == QuicLossError.BufferTooSmall);
    };
    if (!got_small) return 1;

    let buf_s: []QuicAckRange = buf[0..];
    let acks = QuicAckSet.init(buf_s) catch return 2;

    // Reject inverted range start > end
    let got_inv = false;
    acks.insertRange(10, 5) catch |e| {
        got_inv = (e == QuicLossError.InvalidRange);
    };
    if (!got_inv) return 3;

    // Out-of-order + overlapping + adjacent coalescing
    acks.insertRange(10, 15) catch return 4;
    acks.insertRange(1, 3) catch return 5;
    acks.insertRange(5, 8) catch return 6;
    if (acks.rangeCount() != 3 || acks.gapCount() != 2) return 7;
    // Bridge [5..8] and [10..15] via overlapping [7..11] -> [5..15]
    acks.insertRange(7, 11) catch return 8;
    if (acks.rangeCount() != 2) return 9;
    if (!acks.contains(9) || !acks.contains(15) || acks.contains(4)) return 10;

    // u64::MAX boundary wraparound guard (no overflow on pn + 1!)
    const max_u64: u64 = ~@as(u64, 0);
    acks.insertPacket(max_u64) catch return 11;
    acks.insertPacket(max_u64 - 1) catch return 12;
    if (!acks.contains(max_u64) || !acks.contains(max_u64 - 1)) return 13;
    const lg = acks.largestAcked() ?? return 14;
    if (lg != max_u64) return 15;

    // Capacity cap (4 slots: currently 3 ranges -> 4th ok, 5th TableFull)
    acks.insertPacket(100) catch return 16;
    let got_full = false;
    acks.insertPacket(200) catch |e| {
        got_full = (e == QuicLossError.TableFull);
    };
    if (!got_full) return 17;
    return 0;
}

// ============================================================================
// 3. RFC 9002 Loss Detector, CUBIC, BBRv1 & Pacer Boundaries
// ============================================================================

fn checkLossAndCongestion(): u32 {
    // 1. Packet threshold (kPacketThreshold = 3) + 9/8 time threshold
    let sent_buf: [32]QuicSentPacket = [QuicSentPacket{}; 32];
    let sent_s: []QuicSentPacket = sent_buf[0..];
    let det = QuicLossDetector.init(sent_s, 25_000) catch return 1;

    // Send 8 packets (triggers 4-lane SIMD batch path)
    let p: u64 = 0;
    while (p < 8) : (p += 1) {
        det.onPacketSent(p, 1000 + p * 100, 1200, true) catch return 2;
    }

    // ACK packets [5..7] -> packets 0..=4 have pn + 3 <= 7 -> 5 packets lost by packet threshold!
    const ack_r: [1]QuicAckRange = [QuicAckRange{ first_pn: 5, last_pn: 7 }];
    const out1 = det.onAckReceived(ack_r[0..], 0, 50_000);
    if (out1.newly_acked_packets != 3) return 3;
    if (out1.newly_lost_packets != 5) return 4;
    if (det.bytes_in_flight != 0) return 5;

    // Extreme pn near u64::MAX (no pn + 3 overflow!)
    let sent_buf2: [8]QuicSentPacket = [QuicSentPacket{}; 8];
    let sent_s2: []QuicSentPacket = sent_buf2[0..];
    let det2 = QuicLossDetector.init(sent_s2, 25_000) catch return 6;
    const max_u64: u64 = ~@as(u64, 0);
    det2.onPacketSent(max_u64 - 4, 1000, 1200, true) catch return 7;
    det2.onPacketSent(max_u64 - 1, 1100, 1200, true) catch return 8;
    det2.onPacketSent(max_u64, 1200, 1200, true) catch return 9;
    const ack_hi: [1]QuicAckRange = [QuicAckRange{ first_pn: max_u64, last_pn: max_u64 }];
    const out_hi = det2.onAckReceived(ack_hi[0..], 0, 50_000);
    if (out_hi.newly_acked_packets != 1 || out_hi.newly_lost_packets != 1) return 10;

    // 2. CUBIC zero-div & min_cwnd floor guards
    let cubic = CubicState.init(0, 0); // mss=0 -> 1200, min_cwnd=2400
    if (cubic.mss != 1200 || cubic.cwnd_bytes != 2400) return 11;
    cubic.onAck(1200, 0, 10_000); // rtt=0 must not div-by-zero
    cubic.onCongestionEvent(5_000, 20_000);
    if (cubic.cwnd_bytes < 2400) return 12;
    cubic.onAck(1200, 0, 30_000);
    if (cubic.pacingRateBps(0) == 0) return 13;

    // 3. BBRv1 zero-div & state progression guards
    let bbr = BbrController.init(0, 1000);
    bbr.onDeliverySample(0, 0, 0, 2000); // zero interval/rtt must not div-by-zero
    if (bbr.cwndBytes() < 4800) return 14;
    if (bbr.pacingRateBps() == 0) return 15;

    // 4. QuicPacer rate=0 & burst cap guards
    let pacer = QuicPacer.init(0, 0, 1000);
    if (!pacer.canSendNow(1200, 1000)) return 16;
    if (pacer.nextSendTimeUs(1200, 1000) != 1000) return 17;
    pacer.updateRate(1_000_000, 2000);
    pacer.onPacketSent(4800, 2000);
    if (pacer.canSendNow(1200, 2000)) return 18;
    const next_t = pacer.nextSendTimeUs(1200, 2000);
    if (next_t <= 2000) return 19;
    return 0;
}

// ============================================================================
// 4. 4-Lane @Vector(4, u64) SIMD vs Scalar 0-Diff Parity
// ============================================================================

fn checkSimdParity(): u32 {
    let seed: u64 = 0x1234_5678_9ABC_DEF1;
    let pns: [65]u64 = [0; 65];
    let times: [65]u64 = [0; 65];
    let flags: [65]u64 = [0; 65];

    const max_u64: u64 = ~@as(u64, 0);
    // Sweep lengths 0..=65 (covering 0, 1, 2, 3, 4, 5, ..., 64, 65)
    let len: usize = 0;
    while (len <= 65) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            seed = nextRand(seed);
            const r1 = randOut(seed);
            seed = nextRand(seed);
            const r2 = randOut(seed);
            // Mix small values, normal values, and near-u64::MAX values
            if ((i & 7) == 0) {
                pns[i] = (max_u64 - 15) + (r1 & 15);
            } else {
                pns[i] = r1 % 200;
            }
            times[i] = r2 % 100_000;
            flags[i] = if ((r1 & 3) == 0) 0 else 1;
        }

        const largest_cases: [4]u64 = [0, 2, 100, max_u64];
        let lc: usize = 0;
        while (lc < 4) : (lc += 1) {
            const simd_c = simdQuicCountLossCandidates(
                pns[0..len],
                times[0..len],
                flags[0..len],
                largest_cases[lc],
                50_000,
            );
            const scal_c = scalarQuicCountLossCandidates(
                pns[0..len],
                times[0..len],
                flags[0..len],
                largest_cases[lc],
                50_000,
            );
            if (simd_c != scal_c) return 1;
        }

        if (simdQuicMinU64(times[0..len]) != scalarQuicMinU64(times[0..len])) return 2;
        if (simdQuicMinU64(pns[0..len]) != scalarQuicMinU64(pns[0..len])) return 3;
    }
    return 0;
}

// ============================================================================
// 5. 1,800-Step Deterministic PRNG Transport State-Machine Fuzz
// ============================================================================

fn checkPrngFuzz(): u32 {
    let seed: u64 = 0x2026_1124_9002_F00D;

    let ack_buf: [32]QuicAckRange = [QuicAckRange{}; 32];
    let ack_s: []QuicAckRange = ack_buf[0..];
    let acks = QuicAckSet.init(ack_s) catch return 1;

    let sent_buf: [64]QuicSentPacket = [QuicSentPacket{}; 64];
    let sent_s: []QuicSentPacket = sent_buf[0..];
    let det = QuicLossDetector.init(sent_s, 25_000) catch return 2;

    let cubic = CubicState.init(1200, 12_000);
    let bbr = BbrController.init(1200, 1000);
    let pacer = QuicPacer.init(500_000, 12_000, 1000);

    let now_us: u64 = 1000;
    let next_pn: u64 = 0;

    let step: usize = 0;
    while (step < 1800) : (step += 1) {
        seed = nextRand(seed);
        const r = randOut(seed);
        const op: u64 = r % 6;
        now_us += (r % 5_000) + 1;

        if (op == 0 || op == 1) {
            // Send packet
            const sz: u32 = @as(u32, @intCast((r % 1400) + 1));
            const ack_el: bool = (r & 7) != 0;
            det.onPacketSent(next_pn, now_us, sz, ack_el) catch {
                // TableFull is valid under heavy fuzz load
            };
            pacer.onPacketSent(@as(u64, sz), now_us);
            next_pn += 1;
        } else if (op == 2 || op == 3) {
            // Random ACK (possibly out-of-order / duplicate / range)
            if (next_pn > 0) {
                const a: u64 = (r >> 8) % next_pn;
                const b: u64 = (r >> 16) % next_pn;
                const lo: u64 = if (a <= b) a else b;
                const hi: u64 = if (a <= b) b else a;
                acks.insertRange(lo, hi) catch {
                    acks.clear();
                };
                const range_arr: [1]QuicAckRange = [QuicAckRange{ first_pn: lo, last_pn: hi }];
                const out = det.onAckReceived(range_arr[0..], r % 40_000, now_us);
                if (out.newly_acked_bytes > 0) {
                    const rtt_sample = if (out.latest_rtt_us > 0) out.latest_rtt_us else det.rtt.smoothed_rtt_us;
                    cubic.onAck(out.newly_acked_bytes, rtt_sample, now_us);
                    bbr.onDeliverySample(out.newly_acked_bytes, (r % 20_000) + 1, rtt_sample, now_us);
                    pacer.updateRate(cubic.pacingRateBps(rtt_sample), now_us);
                }
                if (out.newly_lost_packets > 0) {
                    cubic.onCongestionEvent(now_us, now_us);
                }
            }
        } else if (op == 4) {
            // Timer check & timeout
            const dl = det.nextTimerDeadlineUs(now_us);
            if (dl > 0 && dl <= now_us) {
                const tout = det.onLossDetectionTimeout(now_us);
                if (tout.newly_lost_packets > 0) {
                    cubic.onCongestionEvent(now_us, now_us);
                }
            } else {
                * = det.onLossDetectionTimeout(now_us);
            }
        } else {
            // Update inflight & pacer queries
            bbr.onInFlightUpdate(det.bytes_in_flight);
            * = pacer.canSendNow(1200, now_us);
            * = pacer.nextSendTimeUs(1200, now_us);
            if (cubic.cwnd_bytes < 2400) return 3;
            if (bbr.cwndBytes() < 4800) return 4;
        }
    }

    return 0;
}

fn main() {
    test("1. RFC 9002 RTT estimator (§5.3) & clamped exponential PTO backoff", () => {
        expect_eq(checkRttAndPto(), @as(u32, 0));
    });
    test("2. RFC 9000 QuicAckSet: inverted range, coalescing, capacity & u64::MAX guard", () => {
        expect_eq(checkAckSet(), @as(u32, 0));
    });
    test("3. RFC 9002 LossDetector, CUBIC, BBRv1 & Pacer boundary & div-by-zero guards", () => {
        expect_eq(checkLossAndCongestion(), @as(u32, 0));
    });
    test("4. 4-lane @Vector(4, u64) SIMD vs Scalar 0-diff parity (loss candidates & min u64)", () => {
        expect_eq(checkSimdParity(), @as(u32, 0));
    });
    test("5. PRNG Fuzz: 1,800-step deterministic transport state-machine fuzz", () => {
        expect_eq(checkPrngFuzz(), @as(u32, 0));
    });
}
