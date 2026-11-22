// Origin: Pure-CP Stdlib (Zero-Alloc RFC 9002 Congestion Control, SRTT/PTO & Pacing Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 9002 NewReno congestion controller, Jacobson/Karels RTT estimator,
// Probe Timeout (PTO) calculator, and burst-safe byte pacing rate engine for QUIC / HTTP/3 & UDP RPC:
//   - RFC 9002 §5.3 exact min_rtt, ack_delay-adjusted RTT, 7/8-1/8 SRTT & 3/4-1/4 RTTVAR
//   - RFC 9002 §6.2.1 Probe Timeout (PTO = srtt + max(4 * rttvar, kGranularity) + max_ack_delay)
//   - RFC 9002 §7.3 Slow Start exponential growth, Congestion Avoidance linear growth & single-RTT recovery guard
//   - RFC 9002 §7.6 Persistent Congestion collapse to min_cwnd & §7.7 u128 overflow-free pacing rate

// ============================================================================
// 1. Constants & Internal Safe Integer Helpers
// ============================================================================

const U64_MAX: u64 = 0xFFFFFFFFFFFFFFFF;
/// RFC 9002 §6.1.2 timer granularity (kGranularity = 1ms = 1000us).
const K_GRANULARITY_US: u64 = 1000;

fn satAddU64(a: u64, b: u64): u64 {
    if (U64_MAX - a < b) return U64_MAX;
    return a + b;
}

fn satSubU64(a: u64, b: u64): u64 {
    if (b >= a) return 0;
    return a - b;
}

// ============================================================================
// 2. Public Models: Phase, Config, RTT Estimate & Stats
// ============================================================================

/// RFC 9002 congestion control state machine phase.
pub enum CongestionPhase {
    slow_start,
    congestion_avoidance,
    recovery,
}

/// Configuration parameters for `Rfc9002CongestionController`.
pub struct CongestionConfig {
    max_datagram_size: u64 = 1200;
    initial_cwnd_bytes: u64 = 12000;
    min_cwnd_bytes: u64 = 2400;
    initial_rtt_us: u64 = 333000;
    loss_reduction_bps: u16 = 5000;
    pacing_gain_bps: u16 = 12500;

    /// Returns RFC 9002 recommended default parameters (1200B MTU, 10x initial cwnd, 333ms initial RTT, 0.5x loss reduction, 1.25x pacing gain).
    static fn defaults(): CongestionConfig {
        return CongestionConfig{
            max_datagram_size: 1200,
            initial_cwnd_bytes: 12000,
            min_cwnd_bytes: 2400,
            initial_rtt_us: 333000,
            loss_reduction_bps: 5000,
            pacing_gain_bps: 12500,
        };
    }

    /// Validates all congestion control invariants before controller initialization.
    fn isValid(): bool {
        if (_.max_datagram_size == 0) return false;
        if (_.min_cwnd_bytes == 0 || _.min_cwnd_bytes < _.max_datagram_size) return false;
        if (_.initial_cwnd_bytes < _.min_cwnd_bytes) return false;
        if (_.initial_rtt_us == 0) return false;
        if (_.loss_reduction_bps == 0 || _.loss_reduction_bps >= 10000) return false;
        if (_.pacing_gain_bps < 10000) return false;
        return true;
    }
}

/// Jacobson/Karels & RFC 9002 §5.3 RTT estimator state.
pub struct RttEstimate {
    has_sample: bool = false;
    latest_rtt_us: u64 = 0;
    min_rtt_us: u64 = 0;
    smoothed_rtt_us: u64 = 333000;
    rttvar_us: u64 = 166500;

    /// Computes RFC 9002 §6.2.1 Probe Timeout (PTO) in microseconds:
    /// `smoothed_rtt + max(4 * rttvar, kGranularity) + max_ack_delay`.
    fn pto_us(max_ack_delay_us: u64): u64 {
        const four_var: u64 = if (_.rttvar_us > U64_MAX / 4) U64_MAX else _.rttvar_us * 4;
        const var_term: u64 = if (four_var > K_GRANULARITY_US) four_var else K_GRANULARITY_US;
        return satAddU64(satAddU64(_.smoothed_rtt_us, var_term), max_ack_delay_us);
    }
}

/// Operational snapshot of congestion window, RTT, pacing rate, and loss counters.
pub struct CongestionStats {
    phase: CongestionPhase = CongestionPhase.slow_start;
    cwnd_bytes: u64 = 0;
    ssthresh_bytes: u64 = U64_MAX;
    bytes_in_flight: u64 = 0;
    available_cwnd_bytes: u64 = 0;
    pacing_rate_bytes_per_sec: u64 = 0;
    smoothed_rtt_us: u64 = 0;
    min_rtt_us: u64 = 0;
    loss_events: u64 = 0;
    persistent_congestion_events: u64 = 0;
}

// ============================================================================
// 3. Zero-Alloc RFC 9002 Congestion Controller & Pacing Scheduler
// ============================================================================

/// Zero-heap-allocation RFC 9002 NewReno congestion controller with SRTT/PTO & pacing.
pub struct Rfc9002CongestionController {
    cfg: CongestionConfig = CongestionConfig{},
    rtt: RttEstimate = RttEstimate{},
    cwnd_bytes: u64 = 12000,
    ssthresh_bytes: u64 = U64_MAX,
    bytes_in_flight: u64 = 0,
    recovery_start_us: u64 = 0,
    in_recovery: bool = false,
    loss_events: u64 = 0,
    persistent_congestion_events: u64 = 0,

    /// Initializes a validated `Rfc9002CongestionController`. Returns `null` if `cfg.isValid()` is `false`.
    static fn init(cfg: CongestionConfig): Rfc9002CongestionController? {
        if (!cfg.isValid()) return null;
        return Rfc9002CongestionController{
            cfg: cfg,
            rtt: RttEstimate{
                has_sample: false,
                latest_rtt_us: 0,
                min_rtt_us: 0,
                smoothed_rtt_us: cfg.initial_rtt_us,
                rttvar_us: cfg.initial_rtt_us / 2,
            },
            cwnd_bytes: cfg.initial_cwnd_bytes,
            ssthresh_bytes: U64_MAX,
            bytes_in_flight: 0,
            recovery_start_us: 0,
            in_recovery: false,
            loss_events: 0,
            persistent_congestion_events: 0,
        };
    }

    fn currentPhase(): CongestionPhase {
        if (_.in_recovery) return CongestionPhase.recovery;
        if (_.cwnd_bytes < _.ssthresh_bytes) return CongestionPhase.slow_start;
        return CongestionPhase.congestion_avoidance;
    }

    /// Updates `min_rtt`, `smoothed_rtt` (7/8 + 1/8), and `rttvar` (3/4 + 1/4) per RFC 9002 §5.3.
    fn on_rtt_sample(*_, latest_rtt_us: u64, ack_delay_us: u64, max_ack_delay_us: u64) {
        _.rtt.latest_rtt_us = latest_rtt_us;
        if (!_.rtt.has_sample) {
            _.rtt.has_sample = true;
            _.rtt.min_rtt_us = latest_rtt_us;
            _.rtt.smoothed_rtt_us = latest_rtt_us;
            _.rtt.rttvar_us = latest_rtt_us / 2;
            return;
        }

        if (latest_rtt_us < _.rtt.min_rtt_us) {
            _.rtt.min_rtt_us = latest_rtt_us;
        }

        const clamped_delay: u64 = if (ack_delay_us < max_ack_delay_us) ack_delay_us else max_ack_delay_us;
        let adjusted_rtt: u64 = latest_rtt_us;
        if (latest_rtt_us >= satAddU64(_.rtt.min_rtt_us, clamped_delay)) {
            adjusted_rtt = latest_rtt_us - clamped_delay;
        }

        const abs_diff: u64 = if (_.rtt.smoothed_rtt_us >= adjusted_rtt)
            _.rtt.smoothed_rtt_us - adjusted_rtt
        else
            adjusted_rtt - _.rtt.smoothed_rtt_us;

        const rttvar_num: u128 = (3 as u128) * (_.rtt.rttvar_us as u128) + (abs_diff as u128);
        _.rtt.rttvar_us = @intCast(rttvar_num / (4 as u128));

        const srtt_num: u128 = (7 as u128) * (_.rtt.smoothed_rtt_us as u128) + (adjusted_rtt as u128);
        _.rtt.smoothed_rtt_us = @intCast(srtt_num / (8 as u128));
    }

    /// Records `bytes` transmitted onto the wire (saturating addition on `bytes_in_flight`).
    fn on_packet_sent(*_, bytes: u64) {
        _.bytes_in_flight = satAddU64(_.bytes_in_flight, bytes);
    }

    /// Returns currently available congestion window in bytes (`max(0, cwnd - bytes_in_flight)`).
    fn available_window(): u64 {
        return satSubU64(_.cwnd_bytes, _.bytes_in_flight);
    }

    /// Returns `true` iff `bytes` fits within the currently available congestion window.
    fn can_send(bytes: u64): bool {
        return bytes <= _.available_window();
    }

    /// Processes acknowledged packets:
    /// - Deducts `acked_bytes` from `bytes_in_flight` (underflow-safe).
    /// - Exits recovery once a packet sent strictly after `recovery_start_us` is acknowledged.
    /// - Suppresses cwnd growth when `is_app_limited` or `sent_time_us <= recovery_start_us`.
    /// - Otherwise grows exponentially in Slow Start (`cwnd += acked_bytes`) or linearly in
    ///   Congestion Avoidance (`cwnd += max_datagram_size * acked_bytes / cwnd`).
    fn on_packets_acked(*_, acked_bytes: u64, sent_time_us: u64, is_app_limited: bool) {
        _.bytes_in_flight = satSubU64(_.bytes_in_flight, acked_bytes);

        const in_recovery_window: bool = (_.in_recovery || _.recovery_start_us > 0) && sent_time_us <= _.recovery_start_us;
        if (_.in_recovery && sent_time_us > _.recovery_start_us) {
            _.in_recovery = false;
        }

        if (acked_bytes == 0 || is_app_limited || in_recovery_window) {
            return;
        }

        if (_.cwnd_bytes < _.ssthresh_bytes) {
            // Slow Start: exponential growth
            _.cwnd_bytes = satAddU64(_.cwnd_bytes, acked_bytes);
        } else {
            // Congestion Avoidance: RFC 9002 §7.3.3 linear growth
            if (_.cwnd_bytes > 0) {
                const num: u128 = (_.cfg.max_datagram_size as u128) * (acked_bytes as u128);
                const inc: u64 = @intCast(num / (_.cwnd_bytes as u128));
                _.cwnd_bytes = satAddU64(_.cwnd_bytes, inc);
            }
        }
    }

    /// Processes a packet loss / ECN congestion event:
    /// - Deducts `lost_bytes` from `bytes_in_flight` (underflow-safe).
    /// - Enforces RFC 9002 §7.3.2 single-RTT recovery guard: losses of packets sent at or before
    ///   `recovery_start_us` do not trigger repeated cwnd halving.
    /// - Otherwise enters `recovery`, sets `ssthresh = max(cwnd * loss_reduction_bps / 10000, min_cwnd)`
    ///   and `cwnd = ssthresh`.
    fn on_congestion_event(*_, lost_bytes: u64, largest_lost_sent_us: u64, now_us: u64) {
        _.bytes_in_flight = satSubU64(_.bytes_in_flight, lost_bytes);

        if ((_.in_recovery || _.recovery_start_us > 0) && largest_lost_sent_us <= _.recovery_start_us) {
            return;
        }

        _.loss_events += 1;
        _.recovery_start_us = now_us;
        _.in_recovery = true;

        const reduced_u128: u128 = ((_.cwnd_bytes as u128) * (_.cfg.loss_reduction_bps as u128)) / (10000 as u128);
        const reduced: u64 = @intCast(reduced_u128);
        _.ssthresh_bytes = if (reduced > _.cfg.min_cwnd_bytes) reduced else _.cfg.min_cwnd_bytes;
        _.cwnd_bytes = _.ssthresh_bytes;
    }

    /// RFC 9002 §7.6 persistent congestion handler: collapses `cwnd` to `min_cwnd` and clears recovery state.
    fn on_persistent_congestion(*_) {
        _.persistent_congestion_events += 1;
        _.cwnd_bytes = _.cfg.min_cwnd_bytes;
        _.recovery_start_us = 0;
        _.in_recovery = false;
    }

    /// Computes burst-safe pacing rate in bytes/sec per RFC 9002 §7.7:
    /// `(cwnd_bytes * pacing_gain_bps / 10000) / (smoothed_rtt_us / 1_000_000)`
    /// `= (cwnd_bytes * pacing_gain_bps * 100) / max(smoothed_rtt_us, 1)`.
    fn pacing_rate_bytes_per_sec(): u64 {
        const srtt: u64 = if (_.rtt.smoothed_rtt_us > 0) _.rtt.smoothed_rtt_us else 1;
        const num: u128 = (_.cwnd_bytes as u128) * (_.cfg.pacing_gain_bps as u128) * (100 as u128);
        const rate: u128 = num / (srtt as u128);
        if (rate > (U64_MAX as u128)) return U64_MAX;
        return @intCast(rate);
    }

    /// Returns a complete operational statistics snapshot.
    fn stats(): CongestionStats {
        return CongestionStats{
            phase: _.currentPhase(),
            cwnd_bytes: _.cwnd_bytes,
            ssthresh_bytes: _.ssthresh_bytes,
            bytes_in_flight: _.bytes_in_flight,
            available_cwnd_bytes: _.available_window(),
            pacing_rate_bytes_per_sec: _.pacing_rate_bytes_per_sec(),
            smoothed_rtt_us: _.rtt.smoothed_rtt_us,
            min_rtt_us: _.rtt.min_rtt_us,
            loss_events: _.loss_events,
            persistent_congestion_events: _.persistent_congestion_events,
        };
    }
}
