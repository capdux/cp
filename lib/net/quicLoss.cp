// Origin: Pure-CP Stdlib (Unbounded-Slice RFC 9002 QUIC Loss/PTO, RFC 9000 ACK Ranges, RFC 9438 CUBIC & BBRv1) | Created: 2026-11-23
// Reference-hardened against quinn-rs/quinn (MIT OR Apache-2.0) & RFC 9002
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice transport-layer recovery & congestion engine:
//   - RFC 9000 `QuicAckSet`: descending coalesced ACK ranges (`insertPacket`, `insertRange`) with O(log R) binary search,
//     strict `first_pn > last_pn` rejection & `u64::MAX` wraparound-free arithmetic
//   - RFC 9002 `QuicRttState` & `QuicLossDetector`: saturating ack_delay-clamped SRTT/RTTVAR, clamped exponential PTO backoff,
//     packet threshold (`kPacketThreshold = 3`) & time threshold (`9/8 * max(srtt, latest_rtt)`) with 4-lane `@Vector(4, u64)` SIMD
//   - RFC 9438 `CubicState`, BBRv1 `BbrController`, & `QuicPacer`: overflow-safe u128/saturating math & div-by-zero guards

const std = @import("std");

type QuicVec4u64 = @Vector(4, u64);

// ============================================================================
// 1. Constants, Error Set & Saturating / SIMD Helpers
// ============================================================================

/// RFC 9002 §6.1.1 packet reordering threshold (`kPacketThreshold = 3`).
pub const QUIC_PACKET_THRESHOLD: u64 = 3;

/// RFC 9002 §6.2.2 initial RTT estimate in microseconds (`333 ms`).
pub const QUIC_INITIAL_RTT_US: u64 = 333_000;

/// RFC 9002 §6.1.2 timer granularity in microseconds (`1 ms`).
pub const QUIC_GRANULARITY_US: u64 = 1_000;

/// RFC 9000 default maximum UDP payload size in bytes (`1200 B`).
pub const QUIC_MAX_DATAGRAM_DEFAULT: u32 = 1200;

const QUIC_MAX_U64: u64 = ~@as(u64, 0);
const QUIC_MAX_U32: u32 = 0xFFFF_FFFF;
const BBR_BW_WINDOW_LEN: usize = 10;
const BBR_STARTUP_GAIN: f64 = 2.8853900817779268; // 2 / ln(2)
const BBR_DRAIN_GAIN: f64 = 0.34657359027997264;  // ln(2) / 2
const BBR_MIN_RTT_EXPIRY_US: u64 = 10_000_000;    // 10 s
const BBR_PROBE_RTT_DURATION_US: u64 = 200_000;   // 200 ms

/// Error set returned by QUIC ACK set, loss detector, and congestion controller operations.
pub err QuicLossError {
    InvalidRange,
    TableFull,
    BufferTooSmall,
    InvalidConfig,
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > QUIC_MAX_U64 - b) return QUIC_MAX_U64;
    return a + b;
}

fn satSubU64(a: u64, b: u64): u64 {
    return if (a >= b) a - b else 0;
}

fn satMulDivU64(a: u64, b: u64, denom: u64): u64 {
    if (denom == 0) return QUIC_MAX_U64;
    const num: u128 = @as(u128, a) * @as(u128, b);
    const q: u128 = num / @as(u128, denom);
    if (q >= @as(u128, QUIC_MAX_U64)) return QUIC_MAX_U64;
    return @as(u64, @intCast(q));
}

fn clampFloatToU64(x: f64): u64 {
    if (std.math.isNan(x) || x <= 0.0) return 0;
    if (std.math.isInf(x) || x >= 18446744073709549568.0) return QUIC_MAX_U64;
    return @as(u64, @intFromFloat(x));
}

fn cbrtPos(x: f64): f64 {
    if (std.math.isNan(x) || x <= 0.0) return 0.0;
    return std.math.pow(f64, x, 1.0 / 3.0);
}

/// Scalar reference: returns the minimum `u64` in `vals`, or `u64::MAX` if empty.
pub fn scalarQuicMinU64(vals: []const u64): u64 {
    let best: u64 = QUIC_MAX_U64;
    let i: usize = 0;
    while (i < vals.len) : (i += 1) {
        if (vals[i] < best) best = vals[i];
    }
    return best;
}

/// 4-lane `@Vector(4, u64)` SIMD minimum scan over `vals` (0-diff vs `scalarQuicMinU64`).
pub fn simdQuicMinU64(vals: []const u64): u64 {
    if (vals.len == 0) return QUIC_MAX_U64;
    let i: usize = 0;
    let acc: QuicVec4u64 = @splat(QUIC_MAX_U64);
    while (i + 4 <= vals.len) : (i += 4) {
        const chunk: QuicVec4u64 = vals[i..][0..4].*;
        acc = @select(u64, chunk < acc, chunk, acc);
    }
    let best: u64 = acc[0];
    if (acc[1] < best) best = acc[1];
    if (acc[2] < best) best = acc[2];
    if (acc[3] < best) best = acc[3];
    while (i < vals.len) : (i += 1) {
        if (vals[i] < best) best = vals[i];
    }
    return best;
}

/// Scalar reference: counts active in-flight packets satisfying RFC 9002 packet-threshold
/// (`pn + 3 <= largest_acked`) or time-threshold (`sent_time_us <= lost_send_time_us`).
pub fn scalarQuicCountLossCandidates(
    pns: []const u64,
    sent_times: []const u64,
    active_flags: []const u64,
    largest_acked: u64,
    lost_send_time_us: u64,
): usize {
    const n_min1: usize = if (pns.len < sent_times.len) pns.len else sent_times.len;
    const n: usize = if (n_min1 < active_flags.len) n_min1 else active_flags.len;
    const has_pkt_thresh: bool = largest_acked >= QUIC_PACKET_THRESHOLD;
    const pkt_cutoff: u64 = if (has_pkt_thresh) largest_acked - QUIC_PACKET_THRESHOLD else 0;

    let total: usize = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (active_flags[i] == 0) continue;
        const pn: u64 = pns[i];
        if (pn > largest_acked) continue;
        const hit_pkt: bool = has_pkt_thresh && (pn <= pkt_cutoff);
        const hit_time: bool = sent_times[i] <= lost_send_time_us;
        if (hit_pkt || hit_time) {
            total += 1;
        }
    }
    return total;
}

/// 4-lane `@Vector(4, u64)` SIMD batch scan counting RFC 9002 loss candidates
/// (0-diff vs `scalarQuicCountLossCandidates`).
pub fn simdQuicCountLossCandidates(
    pns: []const u64,
    sent_times: []const u64,
    active_flags: []const u64,
    largest_acked: u64,
    lost_send_time_us: u64,
): usize {
    const n_min1: usize = if (pns.len < sent_times.len) pns.len else sent_times.len;
    const n: usize = if (n_min1 < active_flags.len) n_min1 else active_flags.len;
    const has_pkt_thresh: bool = largest_acked >= QUIC_PACKET_THRESHOLD;
    const pkt_cutoff: u64 = if (has_pkt_thresh) largest_acked - QUIC_PACKET_THRESHOLD else 0;

    const v_zero: QuicVec4u64 = @splat(0);
    const v_largest: QuicVec4u64 = @splat(largest_acked);
    const v_cutoff: QuicVec4u64 = @splat(pkt_cutoff);
    const v_lost_t: QuicVec4u64 = @splat(lost_send_time_us);

    let total: usize = 0;
    let i: usize = 0;
    while (i + 4 <= n) : (i += 4) {
        const vp: QuicVec4u64 = pns[i..][0..4].*;
        const vt: QuicVec4u64 = sent_times[i..][0..4].*;
        const va: QuicVec4u64 = active_flags[i..][0..4].*;

        const m_act: u4 = @bitCast(va != v_zero);
        const m_le: u4 = @bitCast(vp <= v_largest);
        const m_pkt: u4 = if (has_pkt_thresh) @bitCast(vp <= v_cutoff) else 0;
        const m_time: u4 = @bitCast(vt <= v_lost_t);
        const m_hit: u4 = m_act & m_le & (m_pkt | m_time);
        total += @as(usize, @popCount(m_hit));
    }

    while (i < n) : (i += 1) {
        if (active_flags[i] == 0) continue;
        const pn: u64 = pns[i];
        if (pn > largest_acked) continue;
        const hit_pkt: bool = has_pkt_thresh && (pn <= pkt_cutoff);
        const hit_time: bool = sent_times[i] <= lost_send_time_us;
        if (hit_pkt || hit_time) {
            total += 1;
        }
    }
    return total;
}

// ============================================================================
// 2. RFC 9000 ACK Range Coalescing & Binary-Search Set
// ============================================================================

/// Closed packet number interval `[first_pn, last_pn]` (`first_pn <= last_pn`).
pub struct QuicAckRange {
    first_pn: u64 = 0;
    last_pn: u64 = 0;
}

/// Descending, non-overlapping, automatically coalesced ACK range set backed by caller's slice.
pub struct QuicAckSet {
    ranges: []QuicAckRange = [],
    count: usize = 0;

    /// Wraps a caller-provided slice of `QuicAckRange` with zero dynamic allocation.
    static fn init(buf: *[]QuicAckRange): QuicAckSet ! QuicLossError {
        if (buf.len == 0) {
            return QuicLossError.BufferTooSmall;
        }
        return QuicAckSet{
            ranges: buf,
            count: 0,
        };
    }

    /// Resets the ACK set to empty.
    fn clear(*_) {
        count = 0;
    }

    /// Returns the number of coalesced ranges currently stored.
    fn rangeCount(): usize {
        return count;
    }

    /// Returns the number of unacknowledged gaps between stored ranges.
    fn gapCount(): usize {
        return if (count <= 1) 0 else count - 1;
    }

    /// Returns the largest acknowledged packet number, or `null` if empty.
    fn largestAcked(): u64? {
        if (count == 0) return null;
        return ranges[0].last_pn;
    }

    /// Returns a read-only view of all active coalesced ranges in strictly descending order.
    fn activeRanges(): []const QuicAckRange {
        return ranges[0..count];
    }

    /// O(log R) binary search checking whether `pn` is covered by any stored ACK range.
    fn contains(pn: u64): bool {
        if (count == 0) return false;
        let lo: usize = 0;
        let hi: usize = count;
        while (lo < hi) {
            const mid: usize = lo + ((hi - lo) >> 1);
            const rg = ranges[mid];
            if (pn >= rg.first_pn && pn <= rg.last_pn) {
                return true;
            }
            // Ranges are stored in strictly descending order.
            if (pn > rg.last_pn) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return false;
    }

    /// Inserts closed range `[first_pn, last_pn]` (`first_pn <= last_pn`), rejecting invalid
    /// inverted ranges (`first_pn > last_pn`), coalescing overlapping/adjacent ranges, and guarding `u64::MAX`.
    fn insertRange(*_, first_pn: u64, last_pn: u64): void ! QuicLossError {
        if (first_pn > last_pn) {
            return QuicLossError.InvalidRange;
        }

        // Find first range `lo` that could overlap or be adjacent on the high side:
        // ranges are strictly descending, so we skip ranges strictly above `last_pn + 1`.
        let lo: usize = 0;
        while (lo < count) : (lo += 1) {
            const rg = ranges[lo];
            const strictly_above: bool = (last_pn < QUIC_MAX_U64) && (rg.first_pn > last_pn + 1);
            if (!strictly_above) break;
        }

        // Find end `hi` of ranges that overlap or are adjacent to `[first_pn, last_pn]`.
        let merged_first: u64 = first_pn;
        let merged_last: u64 = last_pn;
        let hi: usize = lo;
        while (hi < count) : (hi += 1) {
            const rg = ranges[hi];
            const strictly_below: bool = (rg.last_pn < QUIC_MAX_U64) && (rg.last_pn + 1 < merged_first);
            if (strictly_below) break;
            if (rg.first_pn < merged_first) merged_first = rg.first_pn;
            if (rg.last_pn > merged_last) merged_last = rg.last_pn;
        }

        const span: usize = hi - lo;
        if (span == 0) {
            if (count >= ranges.len) {
                return QuicLossError.TableFull;
            }
            let k: usize = count;
            while (k > lo) {
                ranges[k] = ranges[k - 1];
                k -= 1;
            }
            ranges[lo] = QuicAckRange{ first_pn: merged_first, last_pn: merged_last };
            count += 1;
            return;
        }

        ranges[lo] = QuicAckRange{ first_pn: merged_first, last_pn: merged_last };
        if (span > 1) {
            const shift: usize = span - 1;
            let s: usize = hi;
            while (s < count) : (s += 1) {
                ranges[s - shift] = ranges[s];
            }
            count -= shift;
        }
    }

    /// Inserts packet number `pn`, automatically coalescing adjacent or overlapping ranges
    /// while preserving strict descending order (`ranges[i].first_pn > ranges[i+1].last_pn + 1`).
    fn insertPacket(*_, pn: u64): void ! QuicLossError {
        return insertRange(pn, pn);
    }
}

fn pnInAckRanges(pn: u64, ack_ranges: []const QuicAckRange): bool {
    let i: usize = 0;
    while (i < ack_ranges.len) : (i += 1) {
        if (pn >= ack_ranges[i].first_pn && pn <= ack_ranges[i].last_pn) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// 3. RFC 9002 RTT Estimator & Loss / PTO Detection Engine
// ============================================================================

/// RFC 9002 §5 RTT estimator (`min_rtt`, `smoothed_rtt`, `rttvar`) and PTO calculator.
pub struct QuicRttState {
    latest_rtt_us: u64 = 0;
    min_rtt_us: u64 = 0;
    smoothed_rtt_us: u64 = QUIC_INITIAL_RTT_US;
    rttvar_us: u64 = QUIC_INITIAL_RTT_US / 2;
    first_sample: bool = true;

    /// Returns a freshly initialized RFC 9002 RTT state (`srtt = 333ms, rttvar = 166.5ms`).
    static fn init(): QuicRttState {
        return QuicRttState{};
    }

    /// Updates RTT statistics from a new `latest_rtt` sample per RFC 9002 §5.3, clamping
    /// `ack_delay_us` to `max_ack_delay_us` and using saturating u128 arithmetic to prevent overflow.
    fn update(*_, latest_rtt: u64, ack_delay_us: u64, max_ack_delay_us: u64) {
        latest_rtt_us = latest_rtt;
        if (first_sample) {
            min_rtt_us = latest_rtt;
            smoothed_rtt_us = latest_rtt;
            rttvar_us = latest_rtt / 2;
            first_sample = false;
            return;
        }

        if (latest_rtt < min_rtt_us) {
            min_rtt_us = latest_rtt;
        }

        const clamped_delay: u64 = if (ack_delay_us > max_ack_delay_us) max_ack_delay_us else ack_delay_us;
        let adjusted_rtt: u64 = latest_rtt;
        if (latest_rtt >= satAddU64(min_rtt_us, clamped_delay)) {
            adjusted_rtt = latest_rtt - clamped_delay;
        }

        const diff: u64 = if (smoothed_rtt_us >= adjusted_rtt)
            smoothed_rtt_us - adjusted_rtt
        else
            adjusted_rtt - smoothed_rtt_us;
        const next_var: u128 = (@as(u128, rttvar_us) * 3 + @as(u128, diff)) / 4;
        const next_srtt: u128 = (@as(u128, smoothed_rtt_us) * 7 + @as(u128, adjusted_rtt)) / 8;
        rttvar_us = if (next_var >= @as(u128, QUIC_MAX_U64)) QUIC_MAX_U64 else @as(u64, @intCast(next_var));
        smoothed_rtt_us = if (next_srtt >= @as(u128, QUIC_MAX_U64)) QUIC_MAX_U64 else @as(u64, @intCast(next_srtt));
    }

    /// Computes Probe Timeout (PTO) duration with clamped exponential backoff `2^pto_count` per RFC 9002 §6.2.1.
    fn ptoDurationUs(max_ack_delay_us: u64, pto_count: u32): u64 {
        const four_var: u64 = satMulDivU64(rttvar_us, 4, 1);
        const var_term: u64 = if (four_var > QUIC_GRANULARITY_US) four_var else QUIC_GRANULARITY_US;
        const base: u64 = satAddU64(satAddU64(smoothed_rtt_us, var_term), max_ack_delay_us);
        const shift: u6 = if (pto_count > 16) 16 else @as(u6, @intCast(pto_count));
        const max_before_shift: u64 = QUIC_MAX_U64 >> shift;
        if (base >= max_before_shift) {
            return QUIC_MAX_U64;
        }
        return base << shift;
    }
}

/// Tracking record for a single sent QUIC packet stored in caller's external slice.
pub struct QuicSentPacket {
    pn: u64 = 0;
    sent_time_us: u64 = 0;
    bytes: u32 = 0;
    ack_eliciting: bool = true;
    in_flight: bool = false;
    acked: bool = false;
    lost: bool = false;
}

/// Summary returned by `onAckReceived` and `onLossDetectionTimeout`.
pub struct QuicAckOutcome {
    newly_acked_packets: usize = 0;
    newly_acked_bytes: u64 = 0;
    newly_lost_packets: usize = 0;
    newly_lost_bytes: u64 = 0;
    largest_newly_acked_pn: u64 = 0;
    latest_rtt_us: u64 = 0;
    loss_time_us: u64 = 0;
}

/// RFC 9002 loss detection and PTO timer engine backed by an arbitrary-length external packet slice.
pub struct QuicLossDetector {
    sent_packets: []QuicSentPacket = [],
    count: usize = 0;
    bytes_in_flight: u64 = 0;
    ack_eliciting_in_flight: usize = 0;
    largest_acked_pn: u64 = 0;
    has_largest_acked: bool = false;
    time_of_last_ack_eliciting_us: u64 = 0;
    loss_time_us: u64 = 0;
    pto_count: u32 = 0;
    max_ack_delay_us: u64 = 25_000;
    rtt: QuicRttState = QuicRttState{};

    /// Initializes a `QuicLossDetector` over caller's `sent_packets` buffer.
    static fn init(
        sent_packets: *[]QuicSentPacket,
        max_ack_delay_us: u64,
    ): QuicLossDetector ! QuicLossError {
        if (sent_packets.len == 0) {
            return QuicLossError.BufferTooSmall;
        }
        return QuicLossDetector{
            sent_packets: sent_packets,
            count: 0,
            max_ack_delay_us: max_ack_delay_us,
            rtt: QuicRttState.init(),
        };
    }

    fn compactCompleted(*_) {
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < count) : (read_idx += 1) {
            const p = sent_packets[read_idx];
            if (!p.acked && !p.lost) {
                if (write_idx != read_idx) {
                    sent_packets[write_idx] = p;
                }
                write_idx += 1;
            }
        }
        count = write_idx;
    }

    /// Records a newly transmitted packet. Automatically compacts completed entries when full.
    fn onPacketSent(
        *_,
        pn: u64,
        sent_time_us: u64,
        bytes: u32,
        ack_eliciting: bool,
    ): void ! QuicLossError {
        if (count >= sent_packets.len) {
            compactCompleted();
        }
        if (count >= sent_packets.len) {
            return QuicLossError.TableFull;
        }
        const in_fl: bool = bytes > 0;
        sent_packets[count] = QuicSentPacket{
            pn: pn,
            sent_time_us: sent_time_us,
            bytes: bytes,
            ack_eliciting: ack_eliciting,
            in_flight: in_fl,
            acked: false,
            lost: false,
        };
        count += 1;
        if (in_fl) {
            bytes_in_flight = satAddU64(bytes_in_flight, @as(u64, bytes));
        }
        if (ack_eliciting) {
            ack_eliciting_in_flight += 1;
            time_of_last_ack_eliciting_us = sent_time_us;
        }
    }

    fn detectLostPackets(*_, now_us: u64, outcome: *QuicAckOutcome) {
        loss_time_us = 0;
        if (!has_largest_acked) {
            return;
        }

        const max_rtt: u64 = if (rtt.latest_rtt_us > rtt.smoothed_rtt_us)
            rtt.latest_rtt_us
        else
            rtt.smoothed_rtt_us;
        // RFC 9002 §6.1.2: loss_delay = max(9/8 * max(latest_rtt, smoothed_rtt), kGranularity)
        const scaled_rtt: u64 = satMulDivU64(max_rtt, 9, 8);
        const loss_delay: u64 = if (scaled_rtt > QUIC_GRANULARITY_US) scaled_rtt else QUIC_GRANULARITY_US;
        const lost_send_time: u64 = satSubU64(now_us, loss_delay);
        const has_pkt_thresh: bool = largest_acked_pn >= QUIC_PACKET_THRESHOLD;
        const pkt_cutoff: u64 = if (has_pkt_thresh) largest_acked_pn - QUIC_PACKET_THRESHOLD else 0;

        // 4-lane SIMD batch pre-scan & min candidate-time reduction on chunks of 4 packets
        let chunk_base: usize = 0;
        let min_cand_time: u64 = QUIC_MAX_U64;
        while (chunk_base + 4 <= count) : (chunk_base += 4) {
            let pns4: [4]u64 = [0; 4];
            let ts4: [4]u64 = [0; 4];
            let act4: [4]u64 = [0; 4];
            let cand4: [4]u64 = [QUIC_MAX_U64; 4];
            let lane: usize = 0;
            while (lane < 4) : (lane += 1) {
                const pkt = sent_packets[chunk_base + lane];
                pns4[lane] = pkt.pn;
                ts4[lane] = pkt.sent_time_us;
                act4[lane] = if (!pkt.acked && !pkt.lost) 1 else 0;
            }

            const loss_cnt: usize = simdQuicCountLossCandidates(
                pns4[0..],
                ts4[0..],
                act4[0..],
                largest_acked_pn,
                lost_send_time,
            );

            lane = 0;
            while (lane < 4) : (lane += 1) {
                const idx = chunk_base + lane;
                if (act4[lane] == 0) continue;
                const pn: u64 = pns4[lane];
                if (pn > largest_acked_pn) continue;

                const hit: bool = loss_cnt > 0 && ((has_pkt_thresh && pn <= pkt_cutoff) || (ts4[lane] <= lost_send_time));
                if (hit) {
                    sent_packets[idx].lost = true;
                    if (sent_packets[idx].in_flight) {
                        sent_packets[idx].in_flight = false;
                        const b: u64 = @as(u64, sent_packets[idx].bytes);
                        bytes_in_flight = satSubU64(bytes_in_flight, b);
                        outcome.newly_lost_bytes = satAddU64(outcome.newly_lost_bytes, b);
                    }
                    if (sent_packets[idx].ack_eliciting && ack_eliciting_in_flight > 0) {
                        ack_eliciting_in_flight -= 1;
                    }
                    outcome.newly_lost_packets += 1;
                } else {
                    cand4[lane] = satAddU64(ts4[lane], loss_delay);
                }
            }
            const chunk_min: u64 = simdQuicMinU64(cand4[0..]);
            if (chunk_min < min_cand_time) {
                min_cand_time = chunk_min;
            }
        }

        // Scalar tail for remaining < 4 packets
        let i: usize = chunk_base;
        while (i < count) : (i += 1) {
            if (sent_packets[i].acked || sent_packets[i].lost) continue;
            const pn: u64 = sent_packets[i].pn;
            if (pn > largest_acked_pn) continue;

            const pkt_threshold_hit: bool = has_pkt_thresh && (pn <= pkt_cutoff);
            const time_threshold_hit: bool = sent_packets[i].sent_time_us <= lost_send_time;

            if (pkt_threshold_hit || time_threshold_hit) {
                sent_packets[i].lost = true;
                if (sent_packets[i].in_flight) {
                    sent_packets[i].in_flight = false;
                    const b: u64 = @as(u64, sent_packets[i].bytes);
                    bytes_in_flight = satSubU64(bytes_in_flight, b);
                    outcome.newly_lost_bytes = satAddU64(outcome.newly_lost_bytes, b);
                }
                if (sent_packets[i].ack_eliciting && ack_eliciting_in_flight > 0) {
                    ack_eliciting_in_flight -= 1;
                }
                outcome.newly_lost_packets += 1;
            } else {
                const candidate_time: u64 = satAddU64(sent_packets[i].sent_time_us, loss_delay);
                if (candidate_time < min_cand_time) {
                    min_cand_time = candidate_time;
                }
            }
        }

        loss_time_us = if (min_cand_time == QUIC_MAX_U64) 0 else min_cand_time;
        outcome.loss_time_us = loss_time_us;
    }

    /// Processes an incoming ACK frame's ranges, updates RTT, and declares packet/time-threshold losses.
    fn onAckReceived(
        *_,
        ack_ranges: []const QuicAckRange,
        ack_delay_us: u64,
        now_us: u64,
    ): QuicAckOutcome {
        let outcome = QuicAckOutcome{};
        if (ack_ranges.len == 0) {
            return outcome;
        }

        let newly_acked_largest: bool = false;
        let largest_new_pn: u64 = 0;
        let largest_new_sent_us: u64 = 0;
        let any_ack_eliciting: bool = false;

        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (sent_packets[i].acked || sent_packets[i].lost) continue;
            if (!pnInAckRanges(sent_packets[i].pn, ack_ranges)) continue;

            sent_packets[i].acked = true;
            if (sent_packets[i].in_flight) {
                sent_packets[i].in_flight = false;
                const b: u64 = @as(u64, sent_packets[i].bytes);
                bytes_in_flight = if (bytes_in_flight >= b) bytes_in_flight - b else 0;
                outcome.newly_acked_bytes += b;
            }
            if (sent_packets[i].ack_eliciting) {
                any_ack_eliciting = true;
                if (ack_eliciting_in_flight > 0) {
                    ack_eliciting_in_flight -= 1;
                }
            }
            outcome.newly_acked_packets += 1;

            if (!newly_acked_largest || sent_packets[i].pn > largest_new_pn) {
                newly_acked_largest = true;
                largest_new_pn = sent_packets[i].pn;
                largest_new_sent_us = sent_packets[i].sent_time_us;
            }
        }

        if (!newly_acked_largest) {
            return outcome;
        }

        outcome.largest_newly_acked_pn = largest_new_pn;
        const updated_largest: bool = !has_largest_acked || largest_new_pn > largest_acked_pn;
        if (updated_largest) {
            largest_acked_pn = largest_new_pn;
            has_largest_acked = true;
        }

        // Update RTT sample only if the largest acknowledged packet is newly acked and ack-eliciting.
        if (updated_largest && any_ack_eliciting && now_us >= largest_new_sent_us) {
            const sample_rtt: u64 = now_us - largest_new_sent_us;
            rtt.update(sample_rtt, ack_delay_us, max_ack_delay_us);
            outcome.latest_rtt_us = sample_rtt;
        }

        pto_count = 0;
        detectLostPackets(now_us, *outcome);
        return outcome;
    }

    /// Returns the next loss-detection or PTO timer deadline in microseconds (`0` = no timer armed).
    fn nextTimerDeadlineUs(now_us: u64): u64 {
        if (loss_time_us > 0) {
            return loss_time_us;
        }
        if (ack_eliciting_in_flight == 0) {
            return 0;
        }
        const pto: u64 = rtt.ptoDurationUs(max_ack_delay_us, pto_count);
        const deadline: u64 = satAddU64(time_of_last_ack_eliciting_us, pto);
        return if (deadline < now_us) now_us else deadline;
    }

    /// Handles timer expiration at `now_us`: triggers time-threshold loss detection or advances `pto_count`.
    fn onLossDetectionTimeout(*_, now_us: u64): QuicAckOutcome {
        let outcome = QuicAckOutcome{};
        if (loss_time_us > 0 && now_us >= loss_time_us) {
            detectLostPackets(now_us, *outcome);
            return outcome;
        }
        if (pto_count < QUIC_MAX_U32) {
            pto_count += 1;
        }
        return outcome;
    }
}

// ============================================================================
// 4. RFC 9438 CUBIC Congestion Controller
// ============================================================================

/// RFC 9438 CUBIC congestion control state machine.
pub struct CubicState {
    cwnd_bytes: u64 = 12_000;
    ssthresh_bytes: u64 = QUIC_MAX_U64;
    w_max_bytes: f64 = 0.0;
    k_sec: f64 = 0.0;
    epoch_start_us: u64 = 0;
    recovery_start_us: u64 = 0;
    mss: u64 = 1200;
    beta: f64 = 0.7;
    c_factor: f64 = 0.4;

    /// Initializes a CUBIC controller with maximum segment size `mss` and initial window `init_cwnd`.
    static fn init(mss_in: u64, init_cwnd: u64): CubicState {
        const eff_mss: u64 = if (mss_in == 0) @as(u64, QUIC_MAX_DATAGRAM_DEFAULT) else mss_in;
        const min_cwnd: u64 = eff_mss * 2;
        const eff_cwnd: u64 = if (init_cwnd < min_cwnd) min_cwnd else init_cwnd;
        return CubicState{
            cwnd_bytes: eff_cwnd,
            ssthresh_bytes: QUIC_MAX_U64,
            mss: eff_mss,
        };
    }

    /// Records a congestion/loss event for a packet sent at `sent_time_us`, applying RFC 9438
    /// `beta = 0.7` multiplicative decrease at most once per RTT recovery epoch.
    fn onCongestionEvent(*_, sent_time_us: u64, now_us: u64) {
        if (recovery_start_us > 0 && sent_time_us <= recovery_start_us) {
            return; // Ignore additional losses within the same recovery epoch.
        }
        recovery_start_us = now_us;
        epoch_start_us = 0;
        w_max_bytes = @as(f64, @floatFromInt(cwnd_bytes));

        const eff_mss: u64 = if (mss == 0) @as(u64, QUIC_MAX_DATAGRAM_DEFAULT) else mss;
        const reduced: u64 = clampFloatToU64(w_max_bytes * beta);
        const min_cwnd: u64 = satMulDivU64(eff_mss, 2, 1);
        ssthresh_bytes = if (reduced > min_cwnd) reduced else min_cwnd;
        cwnd_bytes = ssthresh_bytes;
    }

    /// Updates congestion window on receipt of `acked_bytes` at timestamp `now_us`.
    fn onAck(*_, acked_bytes: u64, rtt_us: u64, now_us: u64) {
        if (acked_bytes == 0) return;

        const eff_mss: u64 = if (mss == 0) @as(u64, QUIC_MAX_DATAGRAM_DEFAULT) else mss;
        const min_cwnd: u64 = satMulDivU64(eff_mss, 2, 1);
        if (cwnd_bytes < min_cwnd) {
            cwnd_bytes = min_cwnd;
        }

        // Slow Start: exponential growth until ssthresh.
        if (cwnd_bytes < ssthresh_bytes) {
            cwnd_bytes = satAddU64(cwnd_bytes, acked_bytes);
            return;
        }

        // Congestion Avoidance: RFC 9438 cubic curve + TCP-friendly region.
        const mss_f: f64 = @as(f64, @floatFromInt(eff_mss));
        const cwnd_pkts: f64 = @as(f64, @floatFromInt(cwnd_bytes)) / mss_f;
        if (w_max_bytes < @as(f64, @floatFromInt(cwnd_bytes))) {
            w_max_bytes = @as(f64, @floatFromInt(cwnd_bytes));
        }
        const w_max_pkts: f64 = w_max_bytes / mss_f;

        if (epoch_start_us == 0) {
            epoch_start_us = now_us;
            if (cwnd_pkts < w_max_pkts) {
                k_sec = cbrtPos(w_max_pkts * (1.0 - beta) / c_factor);
            } else {
                k_sec = 0.0;
            }
        }

        const rtt_eff_us: u64 = if (rtt_us == 0) QUIC_GRANULARITY_US else rtt_us;
        const rtt_sec: f64 = @as(f64, @floatFromInt(rtt_eff_us)) / 1.0e6;
        const elapsed_us: u64 = satSubU64(now_us, epoch_start_us);
        const t_sec: f64 = @as(f64, @floatFromInt(elapsed_us)) / 1.0e6 + rtt_sec;

        const dt: f64 = t_sec - k_sec;
        const w_cubic_pkts: f64 = c_factor * dt * dt * dt + w_max_pkts;

        // RFC 9438 §4.3 TCP-friendly window estimate
        const alpha_cubic: f64 = 3.0 * (1.0 - beta) / (1.0 + beta);
        const w_est_pkts: f64 = w_max_pkts * beta + alpha_cubic * (t_sec / rtt_sec);

        const target_pkts: f64 = if (w_cubic_pkts > w_est_pkts) w_cubic_pkts else w_est_pkts;
        if (target_pkts > cwnd_pkts && cwnd_pkts > 0.0) {
            const delta_pkts: f64 = target_pkts - cwnd_pkts;
            const inc_bytes_f: f64 = (delta_pkts / cwnd_pkts) * @as(f64, @floatFromInt(acked_bytes));
            const inc_bytes: u64 = clampFloatToU64(inc_bytes_f);
            cwnd_bytes = satAddU64(cwnd_bytes, if (inc_bytes > 0) inc_bytes else 1);
        } else {
            // Minimum additive progress when already near target (u128 overflow-safe).
            const denom: u128 = @as(u128, cwnd_bytes) * 100;
            const min_inc_128: u128 = if (denom > 0) (@as(u128, acked_bytes) * @as(u128, eff_mss)) / denom else 1;
            const min_inc: u64 = if (min_inc_128 >= @as(u128, QUIC_MAX_U64)) QUIC_MAX_U64 else @as(u64, @intCast(min_inc_128));
            cwnd_bytes = satAddU64(cwnd_bytes, if (min_inc > 0) min_inc else 1);
        }
    }

    /// Computes recommended pacing rate in bytes per second (overflow-safe u128 arithmetic).
    fn pacingRateBps(rtt_us: u64): u64 {
        const eff_rtt: u64 = if (rtt_us == 0) QUIC_INITIAL_RTT_US else rtt_us;
        const gain_num: u64 = if (cwnd_bytes < ssthresh_bytes) 200 else 125;
        const num: u128 = @as(u128, cwnd_bytes) * @as(u128, 10_000) * @as(u128, gain_num);
        const q: u128 = num / @as(u128, eff_rtt);
        if (q >= @as(u128, QUIC_MAX_U64)) return QUIC_MAX_U64;
        return @as(u64, @intCast(q));
    }
}

// ============================================================================
// 5. BBRv1 Bottleneck Bandwidth & Round-Trip Propagation Time Controller
// ============================================================================

/// BBRv1 4-state operating mode.
pub enum BbrMode {
    startup,
    drain,
    probe_bw,
    probe_rtt,
}

/// BBRv1 congestion controller with 10-round windowed `max_bw_bps`, `min_rtt_us`,
/// 8-phase `ProbeBW` gain cycling, and periodic `ProbeRTT` queue draining.
pub struct BbrController {
    cur_mode: BbrMode = BbrMode.startup;
    mss: u64 = 1200;

    bw_window: [BBR_BW_WINDOW_LEN]u64 = [0; BBR_BW_WINDOW_LEN];
    bw_head: usize = 0;
    max_bw_bps: u64 = 0;

    min_rtt_us: u64 = 0;
    min_rtt_stamp_us: u64 = 0;
    probe_rtt_start_us: u64 = 0;

    pacing_gain: f64 = BBR_STARTUP_GAIN;
    cwnd_gain: f64 = 2.0;

    full_bw_bps: u64 = 0;
    full_bw_rounds: u32 = 0;
    filled_pipe: bool = false;

    cycle_index: usize = 0;
    cycle_stamp_us: u64 = 0;
    bytes_in_flight: u64 = 0;

    /// Initializes a BBRv1 controller in `startup` mode.
    static fn init(mss_in: u64, now_us: u64): BbrController {
        const eff_mss: u64 = if (mss_in == 0) @as(u64, QUIC_MAX_DATAGRAM_DEFAULT) else mss_in;
        return BbrController{
            cur_mode: BbrMode.startup,
            mss: eff_mss,
            min_rtt_stamp_us: now_us,
            cycle_stamp_us: now_us,
        };
    }

    /// Returns the current BBR state machine mode.
    fn mode(): BbrMode {
        return cur_mode;
    }

    /// Updates tracked in-flight bytes (used by `drain` exit condition).
    fn onInFlightUpdate(*_, inflight_bytes: u64) {
        bytes_in_flight = inflight_bytes;
        if (cur_mode == BbrMode.drain && bytes_in_flight <= bdpBytes()) {
            enterProbeBw(cycle_stamp_us);
        }
    }

    fn probeBwGainAt(idx: usize): f64 {
        if (idx == 0) return 1.25;
        if (idx == 1) return 0.75;
        return 1.0;
    }

    fn enterProbeBw(*_, now_us: u64) {
        cur_mode = BbrMode.probe_bw;
        cycle_index = 0;
        cycle_stamp_us = now_us;
        pacing_gain = probeBwGainAt(0);
        cwnd_gain = 2.0;
    }

    fn recordBandwidthSample(*_, bw_bps: u64) {
        bw_window[bw_head] = bw_bps;
        bw_head = (bw_head + 1) % BBR_BW_WINDOW_LEN;

        let best: u64 = 0;
        let i: usize = 0;
        while (i < BBR_BW_WINDOW_LEN) : (i += 1) {
            if (bw_window[i] > best) {
                best = bw_window[i];
            }
        }
        max_bw_bps = best;
    }

    /// Processes a new delivery rate and RTT sample at `now_us` and drives the BBRv1 state machine.
    fn onDeliverySample(
        *_,
        delivered_bytes: u64,
        interval_us: u64,
        rtt_us: u64,
        now_us: u64,
    ) {
        if (interval_us > 0 && delivered_bytes > 0) {
            const bw_bps: u64 = satMulDivU64(delivered_bytes, 1_000_000, interval_us);
            recordBandwidthSample(bw_bps);
        }

        // Update min_rtt_us and detect 10-second expiration for ProbeRTT.
        let min_rtt_expired: bool = false;
        if (rtt_us > 0) {
            if (min_rtt_us == 0 || rtt_us <= min_rtt_us) {
                min_rtt_us = rtt_us;
                min_rtt_stamp_us = now_us;
            } else if (now_us >= satAddU64(min_rtt_stamp_us, BBR_MIN_RTT_EXPIRY_US)) {
                min_rtt_expired = true;
            }
        }

        // Handle ProbeRTT entry & exit.
        if (cur_mode == BbrMode.probe_rtt) {
            if (now_us >= satAddU64(probe_rtt_start_us, BBR_PROBE_RTT_DURATION_US)) {
                min_rtt_stamp_us = now_us;
                if (filled_pipe) {
                    enterProbeBw(now_us);
                } else {
                    cur_mode = BbrMode.startup;
                    pacing_gain = BBR_STARTUP_GAIN;
                    cwnd_gain = 2.0;
                }
            }
            return;
        }

        if (min_rtt_expired && cur_mode == BbrMode.probe_bw) {
            cur_mode = BbrMode.probe_rtt;
            probe_rtt_start_us = now_us;
            pacing_gain = 1.0;
            cwnd_gain = 1.0;
            return;
        }

        // State transitions: Startup -> Drain -> ProbeBW
        if (cur_mode == BbrMode.startup) {
            // Check full pipe: bandwidth grew by >= 25%?
            const threshold: u64 = satMulDivU64(full_bw_bps, 125, 100);
            if (max_bw_bps >= threshold && max_bw_bps > full_bw_bps) {
                full_bw_bps = max_bw_bps;
                full_bw_rounds = 0;
            } else if (max_bw_bps > 0) {
                if (full_bw_rounds < QUIC_MAX_U32) {
                    full_bw_rounds += 1;
                }
                if (full_bw_rounds >= 3) {
                    filled_pipe = true;
                    cur_mode = BbrMode.drain;
                    pacing_gain = BBR_DRAIN_GAIN;
                    cwnd_gain = 2.0;
                    cycle_stamp_us = now_us;
                }
            }
        } else if (cur_mode == BbrMode.drain) {
            if (bytes_in_flight <= bdpBytes()) {
                enterProbeBw(now_us);
            }
        } else if (cur_mode == BbrMode.probe_bw) {
            const eff_rtt: u64 = if (min_rtt_us > 0) min_rtt_us else QUIC_INITIAL_RTT_US;
            if (now_us >= satAddU64(cycle_stamp_us, eff_rtt)) {
                cycle_index = (cycle_index + 1) % 8;
                cycle_stamp_us = now_us;
                pacing_gain = probeBwGainAt(cycle_index);
            }
        }
    }

    /// Computes estimated Bandwidth-Delay Product (BDP) in bytes.
    fn bdpBytes(): u64 {
        const eff_mss: u64 = if (mss == 0) @as(u64, QUIC_MAX_DATAGRAM_DEFAULT) else mss;
        const eff_rtt: u64 = if (min_rtt_us > 0) min_rtt_us else QUIC_INITIAL_RTT_US;
        if (max_bw_bps == 0) {
            return satMulDivU64(eff_mss, 10, 1);
        }
        const bdp: u64 = satMulDivU64(max_bw_bps, eff_rtt, 1_000_000);
        const floor_bdp: u64 = satMulDivU64(eff_mss, 4, 1);
        return if (bdp < floor_bdp) floor_bdp else bdp;
    }

    /// Returns current target pacing rate in bytes per second (`max_bw_bps * pacing_gain`).
    fn pacingRateBps(): u64 {
        const eff_mss: u64 = if (mss == 0) @as(u64, QUIC_MAX_DATAGRAM_DEFAULT) else mss;
        if (max_bw_bps == 0) {
            return satMulDivU64(satMulDivU64(eff_mss, 10, 1), 1_000_000, QUIC_INITIAL_RTT_US);
        }
        return clampFloatToU64(@as(f64, @floatFromInt(max_bw_bps)) * pacing_gain);
    }

    /// Returns current congestion window in bytes (`4 * mss` during `probe_rtt`, else `cwnd_gain * BDP`).
    fn cwndBytes(): u64 {
        const eff_mss: u64 = if (mss == 0) @as(u64, QUIC_MAX_DATAGRAM_DEFAULT) else mss;
        const min_cwnd: u64 = satMulDivU64(eff_mss, 4, 1);
        if (cur_mode == BbrMode.probe_rtt) {
            return min_cwnd;
        }
        const target: u64 = clampFloatToU64(@as(f64, @floatFromInt(bdpBytes())) * cwnd_gain);
        return if (target < min_cwnd) min_cwnd else target;
    }
}

// ============================================================================
// 6. RFC 9002 §7.7 Leaky-Bucket Packet Pacer (Zero-Alloc Smooth Transmission)
// ============================================================================

/// Token-bucket / leaky-bucket packet pacer smoothing bursty QUIC transmissions
/// at the rate recommended by `CubicState.pacingRateBps` or `BbrController.pacingRateBps`.
pub struct QuicPacer {
    rate_bps: u64 = 0;
    max_burst_bytes: u64 = 12_000;
    tokens_bytes: f64 = 12000.0;
    last_update_us: u64 = 0;

    /// Initializes a `QuicPacer` with initial `rate_bps` (bytes/sec) and burst cap `max_burst_bytes`.
    static fn init(rate_bps: u64, max_burst_bytes: u64, now_us: u64): QuicPacer {
        const burst: u64 = if (max_burst_bytes == 0)
            @as(u64, QUIC_MAX_DATAGRAM_DEFAULT) * 4
        else
            max_burst_bytes;
        return QuicPacer{
            rate_bps: rate_bps,
            max_burst_bytes: burst,
            tokens_bytes: @as(f64, @floatFromInt(burst)),
            last_update_us: now_us,
        };
    }

    /// Updates the target pacing rate in bytes per second.
    fn updateRate(*_, new_rate_bps: u64, now_us: u64) {
        refill(now_us);
        rate_bps = new_rate_bps;
    }

    fn refill(*_, now_us: u64) {
        if (now_us <= last_update_us) return;
        const dt_us: u64 = now_us - last_update_us;
        last_update_us = now_us;
        if (rate_bps == 0) return;

        const added: f64 = (@as(f64, @floatFromInt(rate_bps)) * @as(f64, @floatFromInt(dt_us))) / 1.0e6;
        const cap_f: f64 = @as(f64, @floatFromInt(max_burst_bytes));
        if (!std.math.isNan(added)) {
            tokens_bytes += added;
        }
        if (std.math.isNan(tokens_bytes) || tokens_bytes > cap_f) {
            tokens_bytes = cap_f;
        }
    }

    /// Returns `true` if a packet of `packet_bytes` can be transmitted immediately at `now_us`.
    fn canSendNow(*_, packet_bytes: u64, now_us: u64): bool {
        refill(now_us);
        if (rate_bps == 0) return true;
        return tokens_bytes >= @as(f64, @floatFromInt(packet_bytes));
    }

    /// Consumes tokens for a transmitted packet of `packet_bytes` at `now_us`.
    fn onPacketSent(*_, packet_bytes: u64, now_us: u64) {
        refill(now_us);
        tokens_bytes -= @as(f64, @floatFromInt(packet_bytes));
        const min_debt: f64 = -@as(f64, @floatFromInt(max_burst_bytes));
        if (std.math.isNan(tokens_bytes) || tokens_bytes < min_debt) {
            tokens_bytes = min_debt;
        }
    }

    /// Computes the earliest timestamp (in microseconds) when `packet_bytes` may be transmitted.
    fn nextSendTimeUs(*_, packet_bytes: u64, now_us: u64): u64 {
        refill(now_us);
        if (rate_bps == 0) return now_us;
        const need: f64 = @as(f64, @floatFromInt(packet_bytes));
        if (tokens_bytes >= need) return now_us;
        const deficit: f64 = need - tokens_bytes;
        const wait_us_f: f64 = (deficit * 1.0e6) / @as(f64, @floatFromInt(rate_bps));
        return satAddU64(now_us, clampFloatToU64(std.math.ceil(wait_us_f)));
    }
}
