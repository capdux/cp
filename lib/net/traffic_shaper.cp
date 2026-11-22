// Origin: Pure-CP Stdlib (RFC 2698 trTCM Policer, Weighted DRR Fair Scheduler & AIMD Congestion Limiter) | Created: 2026-11-22
// Zero-heap-allocation deterministic network & gateway traffic shaping primitives:
//   1. `TrTcmPolicer` (`TrTcmConfig`, `TrafficColor`): RFC 2698 Two-Rate Three-Color Marker (CIR/CBS + PIR/PBS)
//   2. `WeightedDrrScheduler` (`DrrFlowSlot`): O(N) Weighted Deficit Round Robin fair multi-tenant flow scheduler
//   3. `AimdLimiter` (`AimdConfig`): Integer-exact Additive-Increase / Multiplicative-Decrease concurrency window limiter

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return a + b;
}

fn satAddU32(a: u32, b: u32): u32 {
    if (a > 0xFFFFFFFF - b) {
        return 0xFFFFFFFF;
    }
    return a + b;
}

fn refillBucketBytes(
    current: u64,
    rem_ms_bytes: *u64,
    rate_bytes_per_sec: u64,
    burst_cap: u64,
    elapsed_ms: u64,
): u64 {
    if (elapsed_ms == 0 || rate_bytes_per_sec == 0) {
        return current;
    }
    const whole_sec = elapsed_ms / 1000;
    const rem_ms = elapsed_ms % 1000;

    const whole_add: u64 = if (whole_sec > 0 && rate_bytes_per_sec > 0xFFFFFFFFFFFFFFFF / whole_sec)
        0xFFFFFFFFFFFFFFFF
    else
        whole_sec * rate_bytes_per_sec;

    let frac_add: u64 = 0;
    if (rem_ms > 0) {
        if (rate_bytes_per_sec > (0xFFFFFFFFFFFFFFFF - rem_ms_bytes.*) / rem_ms) {
            frac_add = (rate_bytes_per_sec / 1000) * rem_ms;
            rem_ms_bytes.* = 0;
        } else {
            const total_ms_bytes = rem_ms_bytes.* + rate_bytes_per_sec * rem_ms;
            frac_add = total_ms_bytes / 1000;
            rem_ms_bytes.* = total_ms_bytes % 1000;
        }
    }

    const total = satAddU64(current, satAddU64(whole_add, frac_add));
    if (total >= burst_cap) {
        rem_ms_bytes.* = 0;
        return burst_cap;
    }
    return total;
}

// ============================================================================
// 1. RFC 2698 Two-Rate Three-Color Marker (`TrTcmPolicer`)
// ============================================================================

/// QoS conformance color returned by `TrTcmPolicer.police`.
pub enum TrafficColor {
    Green,
    Yellow,
    Red,
}

/// RFC 2698 Two-Rate Three-Color Marker configuration.
/// Requires `cir_bytes_per_sec > 0`, `cbs_bytes > 0`, `pir_bytes_per_sec >= cir_bytes_per_sec`,
/// and `pbs_bytes >= cbs_bytes`.
pub struct TrTcmConfig {
    cir_bytes_per_sec: u64 = 0;
    cbs_bytes: u64 = 0;
    pir_bytes_per_sec: u64 = 0;
    pbs_bytes: u64 = 0;
}

/// Zero-heap-allocation RFC 2698 Two-Rate Three-Color Marker (color-blind mode).
pub struct TrTcmPolicer {
    config: TrTcmConfig;
    tc_bytes: u64 = 0;
    tp_bytes: u64 = 0;
    c_rem_ms_bytes: u64 = 0;
    p_rem_ms_bytes: u64 = 0;
    last_ms: i64 = 0;
    green_count: u64 = 0;
    yellow_count: u64 = 0;
    red_count: u64 = 0;

    /// Initializes a trTCM policer with full C and P buckets at `now_ms`.
    /// Returns `null` if RFC 2698 invariants (`cir > 0`, `cbs > 0`, `pir >= cir`, `pbs >= cbs`) fail.
    static fn init(config: TrTcmConfig, now_ms: i64): TrTcmPolicer? {
        if (config.cir_bytes_per_sec == 0 || config.cbs_bytes == 0) return null;
        if (config.pir_bytes_per_sec < config.cir_bytes_per_sec) return null;
        if (config.pbs_bytes < config.cbs_bytes) return null;

        return TrTcmPolicer{
            config: config,
            tc_bytes: config.cbs_bytes,
            tp_bytes: config.pbs_bytes,
            c_rem_ms_bytes: 0,
            p_rem_ms_bytes: 0,
            last_ms: now_ms,
            green_count: 0,
            yellow_count: 0,
            red_count: 0,
        };
    }

    /// Refills C and P token buckets up to `now_ms`. Backwards timestamps are safely ignored.
    fn refill(*_, now_ms: i64) {
        if (now_ms <= _.last_ms) return;
        const elapsed_ms: u64 = (now_ms - _.last_ms) as u64;
        _.last_ms = now_ms;

        _.tc_bytes = refillBucketBytes(
            _.tc_bytes,
            &_.c_rem_ms_bytes,
            _.config.cir_bytes_per_sec,
            _.config.cbs_bytes,
            elapsed_ms,
        );
        _.tp_bytes = refillBucketBytes(
            _.tp_bytes,
            &_.p_rem_ms_bytes,
            _.config.pir_bytes_per_sec,
            _.config.pbs_bytes,
            elapsed_ms,
        );
    }

    /// Meters a packet of `packet_bytes` at `now_ms` according to RFC 2698:
    ///   - `Red` if `packet_bytes > tp_bytes` (neither bucket decremented)
    ///   - `Yellow` if `packet_bytes > tc_bytes` (only P-bucket decremented)
    ///   - `Green` otherwise (both P-bucket and C-bucket decremented)
    fn police(*_, packet_bytes: u64, now_ms: i64): TrafficColor {
        _.refill(now_ms);

        if (packet_bytes > _.tp_bytes) {
            _.red_count = satAddU64(_.red_count, 1);
            return TrafficColor.Red;
        }
        if (packet_bytes > _.tc_bytes) {
            _.tp_bytes -= packet_bytes;
            _.yellow_count = satAddU64(_.yellow_count, 1);
            return TrafficColor.Yellow;
        }
        _.tp_bytes -= packet_bytes;
        _.tc_bytes -= packet_bytes;
        _.green_count = satAddU64(_.green_count, 1);
        return TrafficColor.Green;
    }

    /// Returns current committed-bucket tokens in bytes.
    fn committedTokens(): u64 {
        return _.tc_bytes;
    }

    /// Returns current peak-bucket tokens in bytes.
    fn peakTokens(): u64 {
        return _.tp_bytes;
    }

    /// Returns cumulative count of Green packets.
    fn greenCount(): u64 {
        return _.green_count;
    }

    /// Returns cumulative count of Yellow packets.
    fn yellowCount(): u64 {
        return _.yellow_count;
    }

    /// Returns cumulative count of Red packets.
    fn redCount(): u64 {
        return _.red_count;
    }

    /// Resets buckets to full burst capacity and clears counters at `now_ms`.
    fn reset(*_, now_ms: i64) {
        _.tc_bytes = _.config.cbs_bytes;
        _.tp_bytes = _.config.pbs_bytes;
        _.c_rem_ms_bytes = 0;
        _.p_rem_ms_bytes = 0;
        _.last_ms = now_ms;
        _.green_count = 0;
        _.yellow_count = 0;
        _.red_count = 0;
    }
}

// ============================================================================
// 2. Weighted Deficit Round Robin Fair Scheduler (`WeightedDrrScheduler`)
// ============================================================================

/// Per-flow scheduling slot stored in the caller-owned WDRR flow table.
pub struct DrrFlowSlot {
    flow_id: u32 = 0;
    quantum: u32 = 0;
    deficit: u64 = 0;
    queued_bytes: u64 = 0;
    queued_packets: u32 = 0;
    head_packet_bytes: u32 = 0;
    dispatched_bytes: u64 = 0;
    dispatched_packets: u32 = 0;
    active: bool = false;
    occupied: bool = false;
}

/// Zero-heap-allocation Weighted Deficit Round Robin (WDRR) fair scheduler.
/// Guarantees weighted fairness across flows with variable packet sizes and prevents starvation.
pub struct WeightedDrrScheduler {
    flows: []DrrFlowSlot;
    cursor: usize = 0;
    in_visit: bool = false;

    /// Initializes a WDRR scheduler backed by caller-owned `flows`.
    /// Returns `null` if `flows.len == 0`.
    static fn init(flows: *[]DrrFlowSlot): WeightedDrrScheduler? {
        if (flows.len == 0) return null;

        let i: usize = 0;
        while (i < flows.len) : (i += 1) {
            flows[i] = DrrFlowSlot{};
        }

        return WeightedDrrScheduler{
            flows: flows,
            cursor: 0,
            in_visit: false,
        };
    }

    fn findFlow(flow_id: u32): usize? {
        let i: usize = 0;
        while (i < _.flows.len) : (i += 1) {
            if (_.flows[i].occupied && _.flows[i].flow_id == flow_id) {
                return i;
            }
        }
        return null;
    }

    /// Registers a flow with non-zero `flow_id` and per-round `quantum` (bytes/weight).
    /// Returns `false` on invalid arguments, duplicate `flow_id`, or full table.
    fn registerFlow(*_, flow_id: u32, quantum: u32): bool {
        if (flow_id == 0 || quantum == 0) return false;
        if (_.findFlow(flow_id) != null) return false;

        let i: usize = 0;
        while (i < _.flows.len) : (i += 1) {
            if (!_.flows[i].occupied) {
                _.flows[i] = DrrFlowSlot{
                    flow_id: flow_id,
                    quantum: quantum,
                    occupied: true,
                };
                return true;
            }
        }
        return false;
    }

    /// Enqueues a packet of `packet_bytes` onto `flow_id`.
    /// Returns `false` if `packet_bytes == 0` or `flow_id` is unregistered.
    fn enqueue(*_, flow_id: u32, packet_bytes: u32): bool {
        if (packet_bytes == 0) return false;
        const idx = _.findFlow(flow_id) ?? return false;

        _.flows[idx].queued_bytes = satAddU64(_.flows[idx].queued_bytes, packet_bytes as u64);
        _.flows[idx].queued_packets = satAddU32(_.flows[idx].queued_packets, 1);
        if (!_.flows[idx].active || _.flows[idx].head_packet_bytes == 0) {
            _.flows[idx].head_packet_bytes = packet_bytes;
        }
        _.flows[idx].active = true;
        return true;
    }

    /// Updates the head-of-line packet size for `flow_id` (for variable-size caller-owned queues).
    fn setHeadPacketBytes(*_, flow_id: u32, head_bytes: u32): bool {
        if (head_bytes == 0) return false;
        const idx = _.findFlow(flow_id) ?? return false;
        if (!_.flows[idx].active) return false;
        _.flows[idx].head_packet_bytes = head_bytes;
        return true;
    }

    fn effectiveHeadBytes(idx: usize): u64 {
        const qb = _.flows[idx].queued_bytes;
        if (qb == 0) return 0;
        const hb = _.flows[idx].head_packet_bytes as u64;
        if (hb == 0 || hb > qb) return qb;
        return hb;
    }

    /// Selects and accounts for the next eligible flow in O(N) worst-case time.
    /// Returns the dispatched `flow_id`, or `null` if no flow has pending work.
    fn scheduleNext(*_): u32? {
        const n = _.flows.len;
        let has_active = false;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (_.flows[i].occupied && _.flows[i].active && _.flows[i].queued_bytes > 0) {
                has_active = true;
                break;
            }
        }
        if (!has_active) {
            _.in_visit = false;
            return null;
        }

        // Up to 2 passes: pass 1 performs normal round-robin; if every active flow's
        // head packet still exceeds its deficit, fast-forward the exact minimum rounds
        // needed for at least one flow to become eligible, then dispatch in pass 2.
        let pass: u8 = 0;
        while (pass < 2) : (pass += 1) {
            let scanned: usize = 0;
            while (scanned < n) : (scanned += 1) {
                const idx = _.cursor;
                if (_.flows[idx].occupied && _.flows[idx].active && _.flows[idx].queued_bytes > 0) {
                    if (!_.in_visit) {
                        _.flows[idx].deficit = satAddU64(_.flows[idx].deficit, _.flows[idx].quantum as u64);
                        _.in_visit = true;
                    }

                    const pkt_bytes = _.effectiveHeadBytes(idx);
                    if (pkt_bytes > 0 && _.flows[idx].deficit >= pkt_bytes) {
                        _.flows[idx].deficit -= pkt_bytes;
                        _.flows[idx].queued_bytes -= pkt_bytes;
                        if (_.flows[idx].queued_packets > 0) {
                            _.flows[idx].queued_packets -= 1;
                        }
                        _.flows[idx].dispatched_bytes = satAddU64(_.flows[idx].dispatched_bytes, pkt_bytes);
                        _.flows[idx].dispatched_packets = satAddU32(_.flows[idx].dispatched_packets, 1);

                        const chosen_id = _.flows[idx].flow_id;
                        if (_.flows[idx].queued_bytes == 0 || _.flows[idx].queued_packets == 0) {
                            _.flows[idx].queued_bytes = 0;
                            _.flows[idx].queued_packets = 0;
                            _.flows[idx].head_packet_bytes = 0;
                            _.flows[idx].deficit = 0;
                            _.flows[idx].active = false;
                            _.cursor = (idx + 1) % n;
                            _.in_visit = false;
                        } else if ((_.flows[idx].head_packet_bytes as u64) > _.flows[idx].queued_bytes) {
                            _.flows[idx].head_packet_bytes = _.flows[idx].queued_bytes as u32;
                        }
                        return chosen_id;
                    }
                }
                _.cursor = (idx + 1) % n;
                _.in_visit = false;
            }

            // Compute minimum additional full rounds needed across all active flows.
            let min_extra_rounds: u64 = 0xFFFFFFFFFFFFFFFF;
            let k: usize = 0;
            while (k < n) : (k += 1) {
                if (_.flows[k].occupied && _.flows[k].active && _.flows[k].queued_bytes > 0) {
                    const pkt_bytes = _.effectiveHeadBytes(k);
                    const q = _.flows[k].quantum as u64;
                    if (pkt_bytes <= _.flows[k].deficit) {
                        min_extra_rounds = 0;
                        break;
                    }
                    const missing = pkt_bytes - _.flows[k].deficit;
                    const rounds = (missing + q - 1) / q;
                    if (rounds < min_extra_rounds) {
                        min_extra_rounds = rounds;
                    }
                }
            }

            if (min_extra_rounds > 1 && min_extra_rounds < 0xFFFFFFFFFFFFFFFF) {
                const bulk = min_extra_rounds - 1;
                let m: usize = 0;
                while (m < n) : (m += 1) {
                    if (_.flows[m].occupied && _.flows[m].active && _.flows[m].queued_bytes > 0) {
                        const q = _.flows[m].quantum as u64;
                        const add = if (q > 0xFFFFFFFFFFFFFFFF / bulk)
                            0xFFFFFFFFFFFFFFFF
                        else
                            bulk * q;
                        _.flows[m].deficit = satAddU64(_.flows[m].deficit, add);
                    }
                }
            }
        }
        return null;
    }

    /// Returns a read-only copy of the flow slot for `flow_id`, or `null` if unregistered.
    fn flowInfo(flow_id: u32): DrrFlowSlot? {
        const idx = _.findFlow(flow_id) ?? return null;
        return _.flows[idx];
    }

    /// Returns the number of currently active (non-empty) flows.
    fn activeFlows(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.flows.len) : (i += 1) {
            if (_.flows[i].occupied && _.flows[i].active) {
                count += 1;
            }
        }
        return count;
    }
}

// ============================================================================
// 3. AIMD Adaptive Concurrency Window Controller (`AimdLimiter`)
// ============================================================================

/// Configuration for `AimdLimiter` (Additive Increase / Multiplicative Decrease).
/// Requires `1 <= min_limit <= initial_limit <= max_limit`, `additive_increase >= 1`,
/// and `1 <= backoff_num < backoff_den`.
pub struct AimdConfig {
    initial_limit: u32 = 16;
    min_limit: u32 = 1;
    max_limit: u32 = 256;
    additive_increase: u32 = 1;
    backoff_num: u32 = 1;
    backoff_den: u32 = 2;
}

/// Zero-heap-allocation AIMD adaptive concurrency limiter for gateway/LLM backpressure.
pub struct AimdLimiter {
    config: AimdConfig;
    limit: u32 = 16;
    in_flight: u32 = 0;
    success_count: u64 = 0;
    congestion_count: u64 = 0;
    rejected_count: u64 = 0;

    /// Initializes an `AimdLimiter`. Returns `null` if `config` invariants are violated.
    static fn init(config: AimdConfig): AimdLimiter? {
        if (config.min_limit == 0 || config.max_limit < config.min_limit) return null;
        if (config.initial_limit < config.min_limit || config.initial_limit > config.max_limit) return null;
        if (config.additive_increase == 0) return null;
        if (config.backoff_den < 2 || config.backoff_num == 0 || config.backoff_num >= config.backoff_den) return null;

        return AimdLimiter{
            config: config,
            limit: config.initial_limit,
            in_flight: 0,
            success_count: 0,
            congestion_count: 0,
            rejected_count: 0,
        };
    }

    /// Attempts to acquire one in-flight concurrency slot.
    /// Returns `true` if admitted, or `false` (incrementing `rejected_count`) when window is full.
    fn tryAcquire(*_): bool {
        if (_.in_flight >= _.limit) {
            _.rejected_count = satAddU64(_.rejected_count, 1);
            return false;
        }
        _.in_flight += 1;
        return true;
    }

    /// Records a healthy completion: releases one in-flight slot and additively increases `limit`
    /// (clamped at `max_limit`). Returns the updated window limit.
    fn onSuccess(*_): u32 {
        if (_.in_flight > 0) {
            _.in_flight -= 1;
        }
        _.success_count = satAddU64(_.success_count, 1);
        const next = satAddU32(_.limit, _.config.additive_increase);
        _.limit = if (next > _.config.max_limit) _.config.max_limit else next;
        return _.limit;
    }

    /// Records a congestion signal (timeout / HTTP 429 / 503): releases one in-flight slot and
    /// multiplicatively decreases `limit` (clamped at `min_limit`). Returns the updated window limit.
    fn onCongestion(*_): u32 {
        if (_.in_flight > 0) {
            _.in_flight -= 1;
        }
        _.congestion_count = satAddU64(_.congestion_count, 1);

        const scaled: u64 = ((_.limit as u64) * (_.config.backoff_num as u64)) / (_.config.backoff_den as u64);
        let next: u32 = scaled as u32;
        if (next >= _.limit && _.limit > _.config.min_limit) {
            next = _.limit - 1;
        }
        if (next < _.config.min_limit) {
            next = _.config.min_limit;
        }
        _.limit = next;
        return _.limit;
    }

    /// Releases an in-flight slot without adjusting the concurrency window (e.g., local cancellation).
    fn releaseWithoutFeedback(*_) {
        if (_.in_flight > 0) {
            _.in_flight -= 1;
        }
    }

    /// Returns the current concurrency window limit.
    fn currentLimit(): u32 {
        return _.limit;
    }

    /// Returns current in-flight request count.
    fn inFlight(): u32 {
        return _.in_flight;
    }

    /// Returns immediately available concurrency slots (`limit - in_flight`, clamped at `0`).
    fn available(): u32 {
        if (_.in_flight >= _.limit) return 0;
        return _.limit - _.in_flight;
    }

    /// Returns cumulative healthy completions recorded.
    fn successCount(): u64 {
        return _.success_count;
    }

    /// Returns cumulative congestion signals recorded.
    fn congestionCount(): u64 {
        return _.congestion_count;
    }

    /// Returns cumulative admissions rejected due to full window.
    fn rejectedCount(): u64 {
        return _.rejected_count;
    }

    /// Resets window to `initial_limit` and clears all counters.
    fn reset(*_) {
        _.limit = _.config.initial_limit;
        _.in_flight = 0;
        _.success_count = 0;
        _.congestion_count = 0;
        _.rejected_count = 0;
    }
}
