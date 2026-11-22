// Origin: Pure-CP Stdlib (Envoy-Style Passive Outlier Detection & Exponential Ejection Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation service-mesh passive outlier detector:
//   - Fixed 64-host table (`OUTLIER_MAX_HOSTS`) with inline host IDs (`OUTLIER_HOST_ID_MAX_LEN = 48`)
//   - Real-time consecutive 5xx & consecutive local-origin connection error ejection with
//     linear/capped backoff `min(base_ejection_time_ms * ejection_count, max_ejection_time_ms)`
//   - Periodic statistical success-rate outlier evaluation (`mean - stdev_factor * stdev`)
//     ejecting worst anomalies first while resetting rolling windows
//   - Strict `max_ejection_percent` cluster-wide panic cap preventing cascading blackout
//   - Automatic time-based reinstatement (`now_ms >= ejected_until_ms`) and healthy-interval decay

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed maximum number of upstream hosts tracked by `OutlierDetector`.
pub const OUTLIER_MAX_HOSTS: usize = 64;

/// Maximum UTF-8 byte length of a host identifier stored inline.
pub const OUTLIER_HOST_ID_MAX_LEN: usize = 48;

/// Reason why an upstream host was ejected from the healthy load-balancing pool.
pub enum OutlierEjectReason {
    none,
    consecutive_5xx,
    consecutive_local_origin,
    success_rate_anomaly,
}

/// Request outcome reported to `OutlierDetector.recordOutcome`.
pub enum OutlierOutcome {
    success,
    http_5xx,
    local_origin_error,
    gateway_error,
}

/// Configuration parameters for Envoy-style passive outlier detection.
pub struct OutlierConfig {
    consecutive_5xx: u32 = 5;
    consecutive_local_origin: u32 = 5;
    base_ejection_time_ms: u64 = 30_000;
    max_ejection_time_ms: u64 = 300_000;
    max_ejection_percent: u8 = 50;
    min_hosts_for_success_rate: usize = 5;
    success_rate_min_volume: u32 = 10;
    success_rate_stdev_factor: f64 = 1.9;

    /// Return Envoy-aligned production default configuration.
    static fn defaults(): OutlierConfig {
        return OutlierConfig{};
    }
}

/// Per-host health, consecutive failure, and rolling success-rate telemetry slot.
pub struct OutlierHostSlot {
    active: bool = false;
    host_id_buf: [OUTLIER_HOST_ID_MAX_LEN]u8 = [0; OUTLIER_HOST_ID_MAX_LEN];
    host_id_len: u16 = 0;
    ejected: bool = false;
    ejection_count: u32 = 0;
    ejected_until_ms: u64 = 0;
    last_reason: OutlierEjectReason = OutlierEjectReason.none;
    window_requests: u32 = 0;
    window_successes: u32 = 0;
    consecutive_5xx: u32 = 0;
    consecutive_local_errors: u32 = 0;

    /// Return the host identifier slice.
    fn host_id(): []const u8 {
        return host_id_buf[0..@as(usize, host_id_len)];
    }

    /// Return the current window success rate in `[0.0, 1.0]` (returns `1.0` if no requests).
    fn successRate(): f64 {
        if (window_requests == 0) return 1.0;
        return @as(f64, @floatFromInt(window_successes)) / @as(f64, @floatFromInt(window_requests));
    }
}

/// Summary report returned by `OutlierDetector.runIntervalCheck`.
pub struct OutlierEvalReport {
    evaluated_hosts: usize = 0;
    newly_ejected: usize = 0;
    reinstated: usize = 0;
    capped_by_max_percent: usize = 0;
    mean_success_rate: f64 = 0.0;
    stdev_success_rate: f64 = 0.0;
    ejection_threshold: f64 = 0.0;
}

/// Global telemetry snapshot returned by `OutlierDetector.stats`.
pub struct OutlierDetectorStats {
    total_hosts: usize = 0;
    currently_ejected: usize = 0;
    total_ejections: u64 = 0;
    total_reinstatements: u64 = 0;
    capped_ejections: u64 = 0;
}

// ============================================================================
// 2. Pure Helper Functions
// ============================================================================

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn sqrtF64(x: f64): f64 {
    if (x <= 0.0) return 0.0;
    let g: f64 = if (x > 1.0) x * 0.5 else 1.0;
    let i: usize = 0;
    while (i < 24) : (i += 1) {
        g = 0.5 * (g + x / g);
    }
    return g;
}

// ============================================================================
// 3. OutlierDetector — Zero-Heap Passive Outlier Detection Engine
// ============================================================================

/// Zero-heap-allocation Envoy-style passive outlier detector with exponential ejection
/// backoff, statistical success-rate anomaly detection, and `max_ejection_percent` guard.
pub struct OutlierDetector {
    cfg: OutlierConfig = OutlierConfig{},
    hosts: [OUTLIER_MAX_HOSTS]OutlierHostSlot = [OutlierHostSlot{}; OUTLIER_MAX_HOSTS];
    host_count: usize = 0;
    total_ejections: u64 = 0;
    total_reinstatements: u64 = 0;
    capped_ejections: u64 = 0;

    /// Initialize an `OutlierDetector` with the given configuration.
    static fn init(cfg: OutlierConfig): OutlierDetector {
        let det = OutlierDetector{};
        det.cfg = cfg;
        return det;
    }

    fn findHostIndex(host_id: []const u8): usize? {
        if (host_id.len == 0 || host_id.len > OUTLIER_HOST_ID_MAX_LEN) return null;
        let i: usize = 0;
        while (i < OUTLIER_MAX_HOSTS) : (i += 1) {
            if (!hosts[i].active) continue;
            const cur: []const u8 = hosts[i].host_id_buf[0..@as(usize, hosts[i].host_id_len)];
            if (bytesEqual(cur, host_id)) return i;
        }
        return null;
    }

    /// Register a new upstream host. Returns its slot index (or existing index if already registered),
    /// or `null` if `host_id` is empty / exceeds `OUTLIER_HOST_ID_MAX_LEN` / pool is full.
    fn registerHost(*_, host_id: []const u8): usize? {
        if (host_id.len == 0 || host_id.len > OUTLIER_HOST_ID_MAX_LEN) return null;
        if (existing := findHostIndex(host_id)) {
            return existing;
        }
        let i: usize = 0;
        while (i < OUTLIER_MAX_HOSTS) : (i += 1) {
            if (!hosts[i].active) {
                let slot = OutlierHostSlot{};
                slot.active = true;
                let k: usize = 0;
                while (k < host_id.len) : (k += 1) {
                    slot.host_id_buf[k] = host_id[k];
                }
                slot.host_id_len = @as(u16, @intCast(host_id.len));
                hosts[i] = slot;
                host_count += 1;
                return i;
            }
        }
        return null;
    }

    /// Remove a registered host by `host_id`. Returns `true` if removed.
    fn removeHost(*_, host_id: []const u8): bool {
        if (idx := findHostIndex(host_id)) {
            hosts[idx] = OutlierHostSlot{};
            if (host_count > 0) host_count -= 1;
            return true;
        }
        return false;
    }

    /// Return true if `host_id` is registered and currently healthy at `now_ms`.
    fn isHostHealthy(host_id: []const u8, now_ms: u64): bool {
        const idx = findHostIndex(host_id) ?? return false;
        if (!hosts[idx].ejected) return true;
        return now_ms >= hosts[idx].ejected_until_ms;
    }

    /// Look up a host slot snapshot by `host_id`.
    fn getHost(host_id: []const u8): OutlierHostSlot? {
        const idx = findHostIndex(host_id) ?? return null;
        return hosts[idx];
    }

    fn countActiveEjected(now_ms: u64): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < OUTLIER_MAX_HOSTS) : (i += 1) {
            if (hosts[i].active && hosts[i].ejected && now_ms < hosts[i].ejected_until_ms) {
                count += 1;
            }
        }
        return count;
    }

    fn canEjectMore(now_ms: u64): bool {
        if (host_count == 0) return false;
        const pct: usize = if (cfg.max_ejection_percent > 100) 100 else @as(usize, cfg.max_ejection_percent);
        const max_allowed: usize = (host_count * pct) / 100;
        return countActiveEjected(now_ms) < max_allowed;
    }

    fn tryEjectSlot(*_, idx: usize, reason: OutlierEjectReason, now_ms: u64): bool {
        if (hosts[idx].ejected && now_ms < hosts[idx].ejected_until_ms) return false;
        if (!canEjectMore(now_ms)) {
            capped_ejections += 1;
            return false;
        }

        hosts[idx].ejection_count += 1;
        const mult: u64 = @as(u64, hosts[idx].ejection_count);
        let dur: u64 = cfg.base_ejection_time_ms * mult;
        if (dur > cfg.max_ejection_time_ms) {
            dur = cfg.max_ejection_time_ms;
        }

        hosts[idx].ejected = true;
        hosts[idx].ejected_until_ms = now_ms + dur;
        hosts[idx].last_reason = reason;
        hosts[idx].consecutive_5xx = 0;
        hosts[idx].consecutive_local_errors = 0;
        total_ejections += 1;
        return true;
    }

    /// Record a request outcome for `host_id` at `now_ms`.
    /// Immediately ejects the host if consecutive 5xx or local-origin error thresholds are reached
    /// and `max_ejection_percent` permits. Returns the triggered `OutlierEjectReason` (or `.none`).
    fn recordOutcome(*_, host_id: []const u8, outcome: OutlierOutcome, now_ms: u64): OutlierEjectReason {
        const idx = findHostIndex(host_id) ?? return OutlierEjectReason.none;
        hosts[idx].window_requests += 1;

        if (outcome == OutlierOutcome.success) {
            hosts[idx].window_successes += 1;
            hosts[idx].consecutive_5xx = 0;
            hosts[idx].consecutive_local_errors = 0;
            return OutlierEjectReason.none;
        }

        if (outcome == OutlierOutcome.http_5xx || outcome == OutlierOutcome.gateway_error) {
            hosts[idx].consecutive_5xx += 1;
            hosts[idx].consecutive_local_errors = 0;
            if (cfg.consecutive_5xx > 0 && hosts[idx].consecutive_5xx >= cfg.consecutive_5xx) {
                if (tryEjectSlot(idx, OutlierEjectReason.consecutive_5xx, now_ms)) {
                    return OutlierEjectReason.consecutive_5xx;
                }
            }
            return OutlierEjectReason.none;
        }

        // local_origin_error
        hosts[idx].consecutive_local_errors += 1;
        hosts[idx].consecutive_5xx = 0;
        if (cfg.consecutive_local_origin > 0 && hosts[idx].consecutive_local_errors >= cfg.consecutive_local_origin) {
            if (tryEjectSlot(idx, OutlierEjectReason.consecutive_local_origin, now_ms)) {
                return OutlierEjectReason.consecutive_local_origin;
            }
        }
        return OutlierEjectReason.none;
    }

    /// Run a periodic outlier evaluation sweep at `now_ms`:
    ///   1. Automatically reinstate hosts with `now_ms >= ejected_until_ms` and decay `ejection_count`
    ///      on hosts that remained healthy through the interval.
    ///   2. Evaluate success-rate mean & standard deviation across active non-ejected hosts meeting
    ///      `success_rate_min_volume`; eject outliers below `mean - stdev_factor * stdev` (worst first)
    ///      while enforcing `max_ejection_percent`.
    ///   3. Reset per-host rolling window counters (`window_requests`, `window_successes`).
    fn runIntervalCheck(*_, now_ms: u64): OutlierEvalReport {
        let rep = OutlierEvalReport{};

        // 1. Reinstatement of expired ejections & healthy-interval backoff decay.
        let i: usize = 0;
        while (i < OUTLIER_MAX_HOSTS) : (i += 1) {
            if (!hosts[i].active) continue;
            if (hosts[i].ejected) {
                if (now_ms >= hosts[i].ejected_until_ms) {
                    hosts[i].ejected = false;
                    hosts[i].ejected_until_ms = 0;
                    hosts[i].consecutive_5xx = 0;
                    hosts[i].consecutive_local_errors = 0;
                    rep.reinstated += 1;
                    total_reinstatements += 1;
                }
            } else if (hosts[i].ejection_count > 0) {
                hosts[i].ejection_count -= 1;
            }
        }

        // 2. Collect eligible non-ejected hosts meeting `success_rate_min_volume`.
        let elig_idx: [OUTLIER_MAX_HOSTS]usize = [0; OUTLIER_MAX_HOSTS];
        let elig_rate: [OUTLIER_MAX_HOSTS]f64 = [0.0; OUTLIER_MAX_HOSTS];
        let elig_count: usize = 0;
        let sum_rate: f64 = 0.0;

        let j: usize = 0;
        while (j < OUTLIER_MAX_HOSTS) : (j += 1) {
            if (!hosts[j].active || hosts[j].ejected) continue;
            if (hosts[j].window_requests < cfg.success_rate_min_volume) continue;
            const r = hosts[j].successRate();
            elig_idx[elig_count] = j;
            elig_rate[elig_count] = r;
            sum_rate += r;
            elig_count += 1;
        }
        rep.evaluated_hosts = elig_count;

        if (elig_count >= cfg.min_hosts_for_success_rate && elig_count > 0) {
            const mean: f64 = sum_rate / @as(f64, @floatFromInt(elig_count));
            let var_sum: f64 = 0.0;
            let k: usize = 0;
            while (k < elig_count) : (k += 1) {
                const diff = elig_rate[k] - mean;
                var_sum += diff * diff;
            }
            const stdev: f64 = sqrtF64(var_sum / @as(f64, @floatFromInt(elig_count)));
            const threshold: f64 = mean - cfg.success_rate_stdev_factor * stdev;

            rep.mean_success_rate = mean;
            rep.stdev_success_rate = stdev;
            rep.ejection_threshold = threshold;

            if (stdev > 0.0) {
                // Sort eligible hosts by ascending success rate so the worst outlier is ejected first.
                let a: usize = 0;
                while (a < elig_count) : (a += 1) {
                    let b: usize = a + 1;
                    while (b < elig_count) : (b += 1) {
                        if (elig_rate[b] < elig_rate[a]) {
                            const tr = elig_rate[a];
                            elig_rate[a] = elig_rate[b];
                            elig_rate[b] = tr;
                            const ti = elig_idx[a];
                            elig_idx[a] = elig_idx[b];
                            elig_idx[b] = ti;
                        }
                    }
                }

                let m: usize = 0;
                while (m < elig_count) : (m += 1) {
                    if (elig_rate[m] < threshold) {
                        const prev_capped = capped_ejections;
                        if (tryEjectSlot(elig_idx[m], OutlierEjectReason.success_rate_anomaly, now_ms)) {
                            rep.newly_ejected += 1;
                        } else if (capped_ejections > prev_capped) {
                            rep.capped_by_max_percent += 1;
                        }
                    }
                }
            }
        }

        // 3. Reset per-host rolling window counters for the next evaluation interval.
        let r_idx: usize = 0;
        while (r_idx < OUTLIER_MAX_HOSTS) : (r_idx += 1) {
            if (!hosts[r_idx].active) continue;
            hosts[r_idx].window_requests = 0;
            hosts[r_idx].window_successes = 0;
        }

        return rep;
    }

    /// Return global telemetry statistics for this `OutlierDetector`.
    fn stats(): OutlierDetectorStats {
        let ejected_now: usize = 0;
        let i: usize = 0;
        while (i < OUTLIER_MAX_HOSTS) : (i += 1) {
            if (hosts[i].active && hosts[i].ejected) {
                ejected_now += 1;
            }
        }
        return OutlierDetectorStats{
            total_hosts: host_count,
            currently_ejected: ejected_now,
            total_ejections: total_ejections,
            total_reinstatements: total_reinstatements,
            capped_ejections: capped_ejections,
        };
    }
}
