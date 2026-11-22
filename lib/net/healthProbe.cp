// Origin: Pure-CP Stdlib (Envoy-Style Active Health Prober, Slow-Start Weight Ramp & Cluster Panic Mode) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation active health probing engine complementing `lib/net/outlierDetect.cp`:
//   - Fixed-capacity 64-endpoint active probe registry with due-probe scheduling (`collectDueProbes`)
//   - Consecutive pass/fail threshold state machine (`unknown -> warming -> healthy`, `degraded`, `unhealthy`, `draining`)
//   - Linear slow-start weight ramp (`effectiveWeightAt`) protecting newly recovered instances from cold-start overload
//   - Envoy Panic Mode (`panic_threshold_pct`): fails open across non-draining endpoints when healthy ratio drops
//     below threshold to prevent cascading overload, while strictly keeping `draining` endpoints isolated

// ============================================================================
// 1. Constants & Enums
// ============================================================================

/// Fixed maximum number of upstream endpoints tracked in `ActiveHealthProber`.
pub const HEALTH_PROBE_MAX_ENDPOINTS: usize = 64;

/// Maximum inline byte length for an endpoint identifier (e.g. `"10.0.1.5:8080"`).
pub const HEALTH_PROBE_ID_MAX_LEN: usize = 48;

/// Lifecycle health state of an actively probed upstream endpoint.
pub enum ProbeEndpointState {
    unknown,
    warming,
    healthy,
    degraded,
    unhealthy,
    draining,
}

/// Outcome of a single active health check probe.
pub enum ProbeOutcome {
    pass,
    fail_timeout,
    fail_status,
    fail_connect,
    degraded_header,
}

// ============================================================================
// 2. Configuration, Endpoint Slot & Cluster Summary Models
// ============================================================================

/// Configuration parameters for `ActiveHealthProber`.
pub struct HealthProbeConfig {
    /// Normal active probe interval in milliseconds for healthy/degraded endpoints.
    interval_ms: u64 = 5000;
    /// Accelerated re-probe interval in milliseconds for unhealthy/unknown endpoints.
    unhealthy_interval_ms: u64 = 1000;
    /// Consecutive `pass` probes required to promote an endpoint to `warming`/`healthy`.
    healthy_threshold: u32 = 2;
    /// Consecutive failed probes required to demote an endpoint to `unhealthy`.
    unhealthy_threshold: u32 = 3;
    /// Linear slow-start ramp duration in milliseconds after becoming healthy (`0` = disabled).
    slow_start_window_ms: u64 = 30000;
    /// Starting weight fraction in basis points (`0..=10000`) at the beginning of slow-start.
    slow_start_min_weight_bps: u16 = 1000;
    /// Envoy Panic Mode threshold (`0..=100`%). When non-draining healthy percentage falls strictly
    /// below this value, cluster enters panic mode and routes fail-open to all non-draining nodes.
    panic_threshold_pct: u8 = 50;

    /// Returns production Envoy-style defaults.
    static fn defaults(): HealthProbeConfig {
        return HealthProbeConfig{};
    }
}

/// Per-endpoint active health state and scheduling metadata.
pub struct ProbeEndpointSlot {
    active: bool = false;
    id_buf: [HEALTH_PROBE_ID_MAX_LEN]u8 = [0; HEALTH_PROBE_ID_MAX_LEN];
    id_len: usize = 0;
    state: ProbeEndpointState = ProbeEndpointState.unknown;
    base_weight: u32 = 100;
    consecutive_pass: u32 = 0;
    consecutive_fail: u32 = 0;
    last_probe_ms: u64 = 0;
    next_probe_ms: u64 = 0;
    healthy_since_ms: u64 = 0;

    /// Returns the endpoint identifier slice.
    fn endpoint_id(): []const u8 {
        return id_buf[0..id_len];
    }

    /// Computes the effective routing weight at `now_ms`, applying linear slow-start weight ramping
    /// while inside `cfg.slow_start_window_ms`, half-weight for `degraded`, and `0` for
    /// `unknown` / `unhealthy` / `draining`.
    fn effectiveWeightAt(now_ms: u64, cfg: HealthProbeConfig): u32 {
        if (!active || base_weight == 0) return 0;
        if (state == ProbeEndpointState.draining || state == ProbeEndpointState.unhealthy || state == ProbeEndpointState.unknown) {
            return 0;
        }
        if (state == ProbeEndpointState.degraded) {
            const half: u32 = base_weight / 2;
            return if (half > 0) half else 1;
        }

        // `warming` or `healthy`: apply linear slow-start ramp if still inside window
        if (cfg.slow_start_window_ms > 0) {
            const elapsed: u64 = if (now_ms > healthy_since_ms) (now_ms - healthy_since_ms) else 0;
            if (elapsed < cfg.slow_start_window_ms) {
                const min_bps: u64 = if (cfg.slow_start_min_weight_bps > 10000) 10000 else @as(u64, cfg.slow_start_min_weight_bps);
                const ramp_bps: u64 = min_bps + ((10000 - min_bps) * elapsed) / cfg.slow_start_window_ms;
                const w: u64 = (@as(u64, base_weight) * ramp_bps) / 10000;
                return if (w > 0) @as(u32, @intCast(w)) else 1;
            }
        }
        return base_weight;
    }
}

/// Cluster-wide health and Envoy Panic Mode summary snapshot.
pub struct ClusterHealthSummary {
    total_endpoints: usize = 0;
    healthy_count: usize = 0;
    warming_count: usize = 0;
    unhealthy_count: usize = 0;
    draining_count: usize = 0;
    healthy_pct: u8 = 0;
    in_panic_mode: bool = false;
    total_effective_weight: u64 = 0;
}

// ============================================================================
// 3. Internal Helpers
// ============================================================================

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isFailureOutcome(outcome: ProbeOutcome): bool {
    return outcome == ProbeOutcome.fail_timeout ||
        outcome == ProbeOutcome.fail_status ||
        outcome == ProbeOutcome.fail_connect;
}

// ============================================================================
// 4. Core Engine: `ActiveHealthProber`
// ============================================================================

/// Zero-heap-allocation Envoy-style active health prober with slow-start & panic mode protection.
pub struct ActiveHealthProber {
    cfg: HealthProbeConfig = HealthProbeConfig.defaults();
    slots: [HEALTH_PROBE_MAX_ENDPOINTS]ProbeEndpointSlot = [ProbeEndpointSlot{}; HEALTH_PROBE_MAX_ENDPOINTS];

    /// Initializes an `ActiveHealthProber` with sanitized thresholds and intervals.
    static fn init(cfg: HealthProbeConfig): ActiveHealthProber {
        const safe_cfg = HealthProbeConfig{
            interval_ms: if (cfg.interval_ms > 0) cfg.interval_ms else 1,
            unhealthy_interval_ms: if (cfg.unhealthy_interval_ms > 0) cfg.unhealthy_interval_ms else 1,
            healthy_threshold: if (cfg.healthy_threshold > 0) cfg.healthy_threshold else 1,
            unhealthy_threshold: if (cfg.unhealthy_threshold > 0) cfg.unhealthy_threshold else 1,
            slow_start_window_ms: cfg.slow_start_window_ms,
            slow_start_min_weight_bps: if (cfg.slow_start_min_weight_bps > 10000) 10000 else cfg.slow_start_min_weight_bps,
            panic_threshold_pct: if (cfg.panic_threshold_pct > 100) 100 else cfg.panic_threshold_pct,
        };
        return ActiveHealthProber{ cfg: safe_cfg };
    }

    fn findSlot(endpoint_id: []const u8): usize? {
        let i: usize = 0;
        while (i < HEALTH_PROBE_MAX_ENDPOINTS) : (i += 1) {
            if (slots[i].active && bytesEqual(slots[i].endpoint_id(), endpoint_id)) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new endpoint in `unknown` state, due for immediate probing at `now_ms`.
    /// Returns its slot index, or `null` if `endpoint_id` is empty/duplicate or capacity is full.
    fn registerEndpoint(
        *_,
        endpoint_id: []const u8,
        base_weight: u32,
        now_ms: u64,
    ): usize? {
        if (endpoint_id.len == 0) return null;
        if (findSlot(endpoint_id) != null) return null;

        let free_idx: usize = HEALTH_PROBE_MAX_ENDPOINTS;
        let i: usize = 0;
        while (i < HEALTH_PROBE_MAX_ENDPOINTS) : (i += 1) {
            if (!slots[i].active) {
                free_idx = i;
                break;
            }
        }
        if (free_idx == HEALTH_PROBE_MAX_ENDPOINTS) return null;

        let slot = ProbeEndpointSlot{
            active: true,
            state: ProbeEndpointState.unknown,
            base_weight: if (base_weight > 0) base_weight else 1,
            next_probe_ms: now_ms,
        };
        const copy_len: usize = if (endpoint_id.len < HEALTH_PROBE_ID_MAX_LEN) endpoint_id.len else HEALTH_PROBE_ID_MAX_LEN;
        let ci: usize = 0;
        while (ci < copy_len) : (ci += 1) {
            slot.id_buf[ci] = endpoint_id[ci];
        }
        slot.id_len = copy_len;
        slots[free_idx] = slot;
        return free_idx;
    }

    /// Removes `endpoint_id` from the prober. Returns `true` if removed.
    fn removeEndpoint(*_, endpoint_id: []const u8): bool {
        if (idx := findSlot(endpoint_id)) {
            slots[idx] = ProbeEndpointSlot{};
            return true;
        }
        return false;
    }

    /// Transitions `endpoint_id` into or out of graceful `draining` state.
    /// Draining endpoints are excluded from probing and are never routed even in Panic Mode.
    fn setDraining(*_, endpoint_id: []const u8, draining: bool): bool {
        if (idx := findSlot(endpoint_id)) {
            if (draining) {
                slots[idx].state = ProbeEndpointState.draining;
            } else if (slots[idx].state == ProbeEndpointState.draining) {
                slots[idx].state = ProbeEndpointState.unknown;
                slots[idx].consecutive_pass = 0;
                slots[idx].consecutive_fail = 0;
            }
            return true;
        }
        return false;
    }

    /// Returns a copy of the endpoint slot for `endpoint_id`, or `null` if absent.
    fn getEndpoint(endpoint_id: []const u8): ProbeEndpointSlot? {
        if (idx := findSlot(endpoint_id)) {
            return slots[idx];
        }
        return null;
    }

    /// Collects slot indices of all active, non-draining endpoints whose `next_probe_ms <= now_ms`.
    fn collectDueProbes(now_ms: u64, out_indices: *[]usize): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < HEALTH_PROBE_MAX_ENDPOINTS && written < out_indices.len) : (i += 1) {
            if (!slots[i].active) continue;
            if (slots[i].state == ProbeEndpointState.draining) continue;
            if (now_ms >= slots[i].next_probe_ms) {
                out_indices[written] = i;
                written += 1;
            }
        }
        return written;
    }

    /// Applies an active probe outcome to `endpoint_id` at `now_ms`, advancing the health FSM
    /// and scheduling `next_probe_ms`. Returns the resulting state, or `null` if not found.
    fn recordProbeResult(
        *_,
        endpoint_id: []const u8,
        outcome: ProbeOutcome,
        now_ms: u64,
    ): ProbeEndpointState? {
        const idx: usize = findSlot(endpoint_id) ?? return null;
        if (slots[idx].state == ProbeEndpointState.draining) {
            return ProbeEndpointState.draining;
        }

        slots[idx].last_probe_ms = now_ms;

        if (outcome == ProbeOutcome.pass) {
            slots[idx].consecutive_pass += 1;
            slots[idx].consecutive_fail = 0;

            if (slots[idx].state == ProbeEndpointState.unknown || slots[idx].state == ProbeEndpointState.unhealthy) {
                if (slots[idx].consecutive_pass >= cfg.healthy_threshold) {
                    slots[idx].healthy_since_ms = now_ms;
                    if (cfg.slow_start_window_ms > 0) {
                        slots[idx].state = ProbeEndpointState.warming;
                    } else {
                        slots[idx].state = ProbeEndpointState.healthy;
                    }
                }
            } else if (slots[idx].state == ProbeEndpointState.warming) {
                if (cfg.slow_start_window_ms == 0 || (now_ms >= slots[idx].healthy_since_ms && (now_ms - slots[idx].healthy_since_ms) >= cfg.slow_start_window_ms)) {
                    slots[idx].state = ProbeEndpointState.healthy;
                }
            } else if (slots[idx].state == ProbeEndpointState.degraded) {
                if (slots[idx].consecutive_pass >= cfg.healthy_threshold) {
                    slots[idx].state = ProbeEndpointState.healthy;
                }
            }

            const still_unhealthy: bool = slots[idx].state == ProbeEndpointState.unknown || slots[idx].state == ProbeEndpointState.unhealthy;
            slots[idx].next_probe_ms = now_ms + (if (still_unhealthy) cfg.unhealthy_interval_ms else cfg.interval_ms);
            return slots[idx].state;
        }

        if (outcome == ProbeOutcome.degraded_header) {
            slots[idx].consecutive_pass = 0;
            slots[idx].consecutive_fail = 0;
            if (slots[idx].state == ProbeEndpointState.unknown || slots[idx].state == ProbeEndpointState.unhealthy) {
                slots[idx].healthy_since_ms = now_ms;
            }
            slots[idx].state = ProbeEndpointState.degraded;
            slots[idx].next_probe_ms = now_ms + cfg.interval_ms;
            return slots[idx].state;
        }

        // Failure outcomes: fail_timeout / fail_status / fail_connect
        if (isFailureOutcome(outcome)) {
            slots[idx].consecutive_fail += 1;
            slots[idx].consecutive_pass = 0;
            if (slots[idx].consecutive_fail >= cfg.unhealthy_threshold) {
                slots[idx].state = ProbeEndpointState.unhealthy;
            }
            slots[idx].next_probe_ms = now_ms + cfg.unhealthy_interval_ms;
        }
        return slots[idx].state;
    }

    /// Computes cluster-wide health counts, effective weights, healthy percentage, and Envoy Panic Mode status.
    fn clusterSummary(now_ms: u64): ClusterHealthSummary {
        let sum = ClusterHealthSummary{};
        let i: usize = 0;
        while (i < HEALTH_PROBE_MAX_ENDPOINTS) : (i += 1) {
            if (!slots[i].active) continue;
            sum.total_endpoints += 1;
            const st: ProbeEndpointState = slots[i].state;

            if (st == ProbeEndpointState.draining) {
                sum.draining_count += 1;
            } else if (st == ProbeEndpointState.unhealthy || st == ProbeEndpointState.unknown) {
                sum.unhealthy_count += 1;
            } else if (st == ProbeEndpointState.warming) {
                const done_warming: bool = cfg.slow_start_window_ms == 0 ||
                    (now_ms >= slots[i].healthy_since_ms && (now_ms - slots[i].healthy_since_ms) >= cfg.slow_start_window_ms);
                if (done_warming) {
                    sum.healthy_count += 1;
                } else {
                    sum.warming_count += 1;
                }
            } else {
                // healthy or degraded
                sum.healthy_count += 1;
            }

            sum.total_effective_weight += @as(u64, slots[i].effectiveWeightAt(now_ms, cfg));
        }

        const non_draining: usize = sum.total_endpoints - sum.draining_count;
        if (non_draining > 0) {
            const avail: usize = sum.healthy_count + sum.warming_count;
            const pct: usize = (avail * 100) / non_draining;
            sum.healthy_pct = @as(u8, @intCast(pct));
            sum.in_panic_mode = (cfg.panic_threshold_pct > 0) && (sum.healthy_pct < cfg.panic_threshold_pct);
        }
        return sum;
    }

    /// Returns `true` if `endpoint_id` is eligible to receive traffic at `now_ms`.
    /// - `draining` endpoints are NEVER routable (even during Panic Mode).
    /// - Under normal operation, only `healthy`, `warming`, and `degraded` endpoints are routable.
    /// - When cluster `in_panic_mode` is active, all non-draining endpoints fail-open to avoid cascading overload.
    fn isRoutable(endpoint_id: []const u8, now_ms: u64): bool {
        const idx: usize = findSlot(endpoint_id) ?? return false;
        const st: ProbeEndpointState = slots[idx].state;
        if (st == ProbeEndpointState.draining) return false;
        if (st == ProbeEndpointState.healthy || st == ProbeEndpointState.warming || st == ProbeEndpointState.degraded) {
            return true;
        }
        const sum: ClusterHealthSummary = clusterSummary(now_ms);
        return sum.in_panic_mode;
    }
}
