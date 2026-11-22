// Origin: Pure-CP Stdlib (Envoy-Style Zero-Alloc Upstream Outlier Ejection & Circuit-Breaker Pool) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation upstream cluster outlier detector, circuit breaker, and
// weighted endpoint router:
//   - Fixed 64-slot upstream endpoint pool with per-node weight, state (`healthy`, `ejected_open`,
//     `half_open_probe`), consecutive failure counter, 8-bucket sliding-window `error_rate_bps`,
//     ejection counter, and cooldown expiry `eject_until_ms`.
//   - Envoy max-ejection-percent cap (`max_ejection_percent_bps`) to prevent total cluster blackhole.
//   - Escalating ejection duration `min(base_ejection_ms * ejection_count, max_ejection_ms)` with
//     automatic expiry transition into single-flight `half_open_probe` and recovery to `healthy`.
//   - Weighted healthy endpoint selection (`pickHealthyEndpoint`) with automatic panic-mode fallback
//     when all endpoints are unavailable.
//   - Monotonic clock rollback protection, duplicate/unknown endpoint guards, and full-pool protection.

// ============================================================================
// Constants
// ============================================================================

pub const CIRCUIT_POOL_MAX_ENDPOINTS: usize = 64;
pub const CIRCUIT_POOL_BPS_DENOM: u32 = 10_000;
pub const CIRCUIT_POOL_WINDOW_BUCKETS: usize = 8;

// ============================================================================
// Internal Helpers
// ============================================================================

fn splitMix64(x: u64): u64 {
    let z: u64 = x +% 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn saturatingAddU64(a: u64, b: u64): u64 {
    const max_u64: u64 = 0xFFFFFFFFFFFFFFFF;
    if (a > max_u64 - b) {
        return max_u64;
    }
    return a + b;
}

fn saturatingMulU64(a: u64, b: u64): u64 {
    if (a == 0 || b == 0) {
        return 0;
    }
    const max_u64: u64 = 0xFFFFFFFFFFFFFFFF;
    if (a > max_u64 / b) {
        return max_u64;
    }
    return a * b;
}

// ============================================================================
// 1. Enums & Data Models
// ============================================================================

/// Circuit-breaker health state of an upstream endpoint.
pub enum CircuitEndpointState {
    healthy,
    ejected_open,
    half_open_probe,
}

/// Request outcome class reported back to the outlier detector.
pub enum CircuitOutcomeKind {
    success,
    server_error_5xx,
    timeout,
    connect_failure,
    client_error_4xx,
}

/// Routing mode chosen by `pickHealthyEndpoint`.
pub enum CircuitPickMode {
    normal_healthy,
    half_open_probe,
    panic_fallback,
    empty_pool,
}

/// Configuration for `CircuitBreakerPool`.
pub struct CircuitPoolConfig {
    consecutive_5xx_threshold: u32 = 5;
    error_rate_threshold_bps: u32 = 5_000;
    min_request_volume: u32 = 10;
    window_ms: u64 = 10_000;
    base_ejection_ms: u64 = 30_000;
    max_ejection_ms: u64 = 300_000;
    max_ejection_percent_bps: u32 = 5_000;
    half_open_success_threshold: u32 = 1;

    static fn defaults(): CircuitPoolConfig {
        return CircuitPoolConfig{};
    }

    fn isValid(): bool {
        if (_.consecutive_5xx_threshold == 0) {
            return false;
        }
        if (_.error_rate_threshold_bps > CIRCUIT_POOL_BPS_DENOM) {
            return false;
        }
        if (_.max_ejection_percent_bps > CIRCUIT_POOL_BPS_DENOM) {
            return false;
        }
        if (_.window_ms < (CIRCUIT_POOL_WINDOW_BUCKETS as u64)) {
            return false;
        }
        if (_.base_ejection_ms == 0 || _.max_ejection_ms < _.base_ejection_ms) {
            return false;
        }
        if (_.half_open_success_threshold == 0) {
            return false;
        }
        return true;
    }
}

/// Internal fixed-size slot for one upstream endpoint.
pub struct CircuitEndpointSlot {
    occupied: bool = false;
    endpoint_id: u64 = 0;
    weight: u32 = 1;
    state: CircuitEndpointState = CircuitEndpointState.healthy;
    consecutive_failures: u32 = 0;
    half_open_successes: u32 = 0;
    probe_in_flight: bool = false;
    ejection_count: u32 = 0;
    eject_until_ms: u64 = 0;
    error_rate_bps: u32 = 0;
    window_total: u32 = 0;
    window_errors: u32 = 0;
    bucket_total: [8]u32 = [0; 8];
    bucket_errors: [8]u32 = [0; 8];
    last_bucket_idx: u64 = 0;
    window_initialized: bool = false;
}

/// Read-only snapshot of an upstream endpoint's circuit-breaker status.
pub struct CircuitEndpointView {
    endpoint_id: u64 = 0;
    weight: u32 = 1;
    state: CircuitEndpointState = CircuitEndpointState.healthy;
    consecutive_failures: u32 = 0;
    error_rate_bps: u32 = 0;
    window_total: u32 = 0;
    window_errors: u32 = 0;
    ejection_count: u32 = 0;
    eject_until_ms: u64 = 0;
    probe_in_flight: bool = false;
}

/// Result returned by `pickHealthyEndpoint`.
pub struct CircuitPickResult {
    mode: CircuitPickMode = CircuitPickMode.empty_pool;
    endpoint_id: u64 = 0;
    weight: u32 = 0;
    state: CircuitEndpointState = CircuitEndpointState.healthy;
}

/// Aggregate cluster-level circuit-breaker and routing telemetry.
pub struct CircuitPoolStats {
    total_endpoints: usize = 0;
    healthy_count: usize = 0;
    ejected_count: usize = 0;
    half_open_count: usize = 0;
    max_ejectable_now: usize = 0;
    total_ejections: u64 = 0;
    ejection_suppressed_by_cap: u64 = 0;
    panic_picks: u64 = 0;
    probe_picks: u64 = 0;
    healthy_picks: u64 = 0;
}

// ============================================================================
// 2. Upstream Outlier Ejection & Circuit-Breaker Pool Engine
// ============================================================================

/// Zero-heap-allocation Envoy-style upstream cluster circuit-breaker & outlier-ejection pool.
pub struct CircuitBreakerPool {
    config: CircuitPoolConfig = CircuitPoolConfig{};
    slots: [64]CircuitEndpointSlot = [CircuitEndpointSlot{}; 64];
    endpoint_count: usize = 0;
    last_now_ms: u64 = 0;
    total_ejections: u64 = 0;
    ejection_suppressed_by_cap: u64 = 0;
    panic_picks: u64 = 0;
    probe_picks: u64 = 0;
    healthy_picks: u64 = 0;

    /// Constructs a validated `CircuitBreakerPool`. Returns `null` if `config` is invalid.
    static fn init(config: CircuitPoolConfig): CircuitBreakerPool? {
        if (!config.isValid()) {
            return null;
        }
        return CircuitBreakerPool{
            config: config,
        };
    }

    /// Registers an upstream endpoint with positive `weight`.
    /// Returns `false` on zero weight, duplicate `endpoint_id`, or full pool (64 endpoints).
    fn addEndpoint(*_, endpoint_id: u64, weight: u32): bool {
        if (weight == 0 || _.endpoint_count >= CIRCUIT_POOL_MAX_ENDPOINTS) {
            return false;
        }
        let free_idx: usize = CIRCUIT_POOL_MAX_ENDPOINTS;
        let i: usize = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].endpoint_id == endpoint_id) {
                    return false;
                }
            } else if (free_idx == CIRCUIT_POOL_MAX_ENDPOINTS) {
                free_idx = i;
            }
        }
        if (free_idx >= CIRCUIT_POOL_MAX_ENDPOINTS) {
            return false;
        }
        _.slots[free_idx] = CircuitEndpointSlot{
            occupied: true,
            endpoint_id: endpoint_id,
            weight: weight,
            state: CircuitEndpointState.healthy,
        };
        _.endpoint_count += 1;
        return true;
    }

    /// Removes an upstream endpoint by `endpoint_id`. Returns `false` if not found.
    fn removeEndpoint(*_, endpoint_id: u64): bool {
        let i: usize = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].endpoint_id == endpoint_id) {
                _.slots[i] = CircuitEndpointSlot{};
                _.endpoint_count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Updates an existing endpoint's positive routing weight. Returns `false` if not found or `new_weight == 0`.
    fn updateWeight(*_, endpoint_id: u64, new_weight: u32): bool {
        if (new_weight == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].endpoint_id == endpoint_id) {
                _.slots[i].weight = new_weight;
                return true;
            }
        }
        return false;
    }

    /// Maximum number of endpoints permitted to be simultaneously ejected under `max_ejection_percent_bps`.
    fn maxEjectableCount(): usize {
        if (_.endpoint_count == 0 || _.config.max_ejection_percent_bps == 0) {
            return 0;
        }
        return (_.endpoint_count * (_.config.max_ejection_percent_bps as usize)) / (CIRCUIT_POOL_BPS_DENOM as usize);
    }

    /// Advances the monotonic pool clock (ignoring backward timestamps) and transitions any
    /// expired `ejected_open` endpoints into `half_open_probe`. Returns count transitioned.
    fn advanceClock(*_, now_ms: u64): usize {
        const effective_now = _.clampNow(now_ms);
        let transitioned: usize = 0;
        let i: usize = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == CircuitEndpointState.ejected_open) {
                if (effective_now >= _.slots[i].eject_until_ms) {
                    _.slots[i].state = CircuitEndpointState.half_open_probe;
                    _.slots[i].probe_in_flight = false;
                    _.slots[i].half_open_successes = 0;
                    transitioned += 1;
                }
            }
        }
        return transitioned;
    }

    /// Records a request outcome for `endpoint_id` at `now_ms`, updating the 8-bucket sliding
    /// window error rate, consecutive failure streak, and outlier ejection / half-open state.
    /// Returns `false` if `endpoint_id` is not registered.
    fn recordOutcome(*_, endpoint_id: u64, outcome: CircuitOutcomeKind, now_ms: u64): bool {
        const effective_now = _.clampNow(now_ms);
        * = _.advanceClock(effective_now);

        const idx = _.findSlotIndex(endpoint_id);
        if (idx >= CIRCUIT_POOL_MAX_ENDPOINTS) {
            return false;
        }

        const is_outlier_failure =
            (outcome == CircuitOutcomeKind.server_error_5xx) ||
            (outcome == CircuitOutcomeKind.timeout) ||
            (outcome == CircuitOutcomeKind.connect_failure);

        _.recordWindowSample(idx, effective_now, is_outlier_failure);

        if (outcome == CircuitOutcomeKind.success) {
            _.slots[idx].consecutive_failures = 0;
            if (_.slots[idx].state == CircuitEndpointState.half_open_probe) {
                _.slots[idx].probe_in_flight = false;
                _.slots[idx].half_open_successes += 1;
                if (_.slots[idx].half_open_successes >= _.config.half_open_success_threshold) {
                    _.slots[idx].state = CircuitEndpointState.healthy;
                    _.slots[idx].half_open_successes = 0;
                    _.slots[idx].eject_until_ms = 0;
                    _.resetSlotWindow(idx);
                }
            }
            return true;
        }

        if (outcome == CircuitOutcomeKind.client_error_4xx) {
            // 4xx client errors do not trigger upstream outlier ejection.
            if (_.slots[idx].state == CircuitEndpointState.half_open_probe) {
                _.slots[idx].probe_in_flight = false;
            }
            return true;
        }

        // Upstream outlier failure (5xx / timeout / connect_failure)
        _.slots[idx].consecutive_failures += 1;

        if (_.slots[idx].state == CircuitEndpointState.half_open_probe) {
            // Probe failed: immediately re-eject with escalated cooldown (subject to cap).
            _.slots[idx].probe_in_flight = false;
            _.slots[idx].half_open_successes = 0;
            _.tryEjectSlot(idx, effective_now);
            return true;
        }

        if (_.slots[idx].state == CircuitEndpointState.healthy) {
            const tripped_consecutive = _.slots[idx].consecutive_failures >= _.config.consecutive_5xx_threshold;
            const tripped_rate =
                _.config.error_rate_threshold_bps > 0 &&
                _.slots[idx].window_total >= _.config.min_request_volume &&
                _.slots[idx].error_rate_bps >= _.config.error_rate_threshold_bps;
            if (tripped_consecutive || tripped_rate) {
                _.tryEjectSlot(idx, effective_now);
            }
        }
        return true;
    }

    /// Selects an upstream endpoint using weighted selection:
    ///   1. If an expired endpoint is in `half_open_probe` without a probe in flight, routes a
    ///      single probe request to it (`CircuitPickMode.half_open_probe`).
    ///   2. Otherwise performs weighted selection across all `healthy` endpoints
    ///      (`CircuitPickMode.normal_healthy`).
    ///   3. If no healthy or ready-to-probe endpoint exists (all ejected / probing), triggers
    ///      Envoy-style panic-mode weighted routing across all active endpoints
    ///      (`CircuitPickMode.panic_fallback`).
    fn pickHealthyEndpoint(*_, rng_seed: u64, now_ms: u64): CircuitPickResult {
        if (_.endpoint_count == 0) {
            return CircuitPickResult{ mode: CircuitPickMode.empty_pool };
        }
        * = _.advanceClock(now_ms);

        // 1. Admit single-flight probe for ready half_open_probe endpoints
        let probe_total_weight: u64 = 0;
        let i: usize = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied &&
                _.slots[i].state == CircuitEndpointState.half_open_probe &&
                !_.slots[i].probe_in_flight)
            {
                probe_total_weight += _.slots[i].weight as u64;
            }
        }
        if (probe_total_weight > 0) {
            const target = splitMix64(rng_seed) % probe_total_weight;
            let acc: u64 = 0;
            i = 0;
            while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
                if (_.slots[i].occupied &&
                    _.slots[i].state == CircuitEndpointState.half_open_probe &&
                    !_.slots[i].probe_in_flight)
                {
                    acc += _.slots[i].weight as u64;
                    if (target < acc) {
                        _.slots[i].probe_in_flight = true;
                        _.probe_picks += 1;
                        return CircuitPickResult{
                            mode: CircuitPickMode.half_open_probe,
                            endpoint_id: _.slots[i].endpoint_id,
                            weight: _.slots[i].weight,
                            state: _.slots[i].state,
                        };
                    }
                }
            }
        }

        // 2. Weighted selection across healthy endpoints
        let healthy_total_weight: u64 = 0;
        i = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == CircuitEndpointState.healthy) {
                healthy_total_weight += _.slots[i].weight as u64;
            }
        }
        if (healthy_total_weight > 0) {
            const target = splitMix64(rng_seed) % healthy_total_weight;
            let acc: u64 = 0;
            i = 0;
            while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
                if (_.slots[i].occupied && _.slots[i].state == CircuitEndpointState.healthy) {
                    acc += _.slots[i].weight as u64;
                    if (target < acc) {
                        _.healthy_picks += 1;
                        return CircuitPickResult{
                            mode: CircuitPickMode.normal_healthy,
                            endpoint_id: _.slots[i].endpoint_id,
                            weight: _.slots[i].weight,
                            state: _.slots[i].state,
                        };
                    }
                }
            }
        }

        // 3. Panic-mode fallback across all occupied endpoints
        let all_total_weight: u64 = 0;
        i = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied) {
                all_total_weight += _.slots[i].weight as u64;
            }
        }
        if (all_total_weight == 0) {
            return CircuitPickResult{ mode: CircuitPickMode.empty_pool };
        }
        const target = splitMix64(rng_seed) % all_total_weight;
        let acc: u64 = 0;
        i = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied) {
                acc += _.slots[i].weight as u64;
                if (target < acc) {
                    _.panic_picks += 1;
                    return CircuitPickResult{
                        mode: CircuitPickMode.panic_fallback,
                        endpoint_id: _.slots[i].endpoint_id,
                        weight: _.slots[i].weight,
                        state: _.slots[i].state,
                    };
                }
            }
        }
        return CircuitPickResult{ mode: CircuitPickMode.empty_pool };
    }

    /// Manually restores an endpoint to `healthy` and clears its error window.
    fn forceRecover(*_, endpoint_id: u64): bool {
        const idx = _.findSlotIndex(endpoint_id);
        if (idx >= CIRCUIT_POOL_MAX_ENDPOINTS) {
            return false;
        }
        _.slots[idx].state = CircuitEndpointState.healthy;
        _.slots[idx].consecutive_failures = 0;
        _.slots[idx].half_open_successes = 0;
        _.slots[idx].probe_in_flight = false;
        _.slots[idx].eject_until_ms = 0;
        _.resetSlotWindow(idx);
        return true;
    }

    /// Returns a read-only snapshot of an endpoint's status, or `null` if unknown.
    fn getEndpoint(endpoint_id: u64): CircuitEndpointView? {
        const idx = _.findSlotIndex(endpoint_id);
        if (idx >= CIRCUIT_POOL_MAX_ENDPOINTS) {
            return null;
        }
        const s = _.slots[idx];
        return CircuitEndpointView{
            endpoint_id: s.endpoint_id,
            weight: s.weight,
            state: s.state,
            consecutive_failures: s.consecutive_failures,
            error_rate_bps: s.error_rate_bps,
            window_total: s.window_total,
            window_errors: s.window_errors,
            ejection_count: s.ejection_count,
            eject_until_ms: s.eject_until_ms,
            probe_in_flight: s.probe_in_flight,
        };
    }

    /// Returns current cluster-level telemetry snapshot.
    fn stats(): CircuitPoolStats {
        let st = CircuitPoolStats{
            total_endpoints: _.endpoint_count,
            max_ejectable_now: _.maxEjectableCount(),
            total_ejections: _.total_ejections,
            ejection_suppressed_by_cap: _.ejection_suppressed_by_cap,
            panic_picks: _.panic_picks,
            probe_picks: _.probe_picks,
            healthy_picks: _.healthy_picks,
        };
        let i: usize = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].state == CircuitEndpointState.healthy) {
                    st.healthy_count += 1;
                } else if (_.slots[i].state == CircuitEndpointState.ejected_open) {
                    st.ejected_count += 1;
                } else {
                    st.half_open_count += 1;
                }
            }
        }
        return st;
    }

    fn endpointCount(): usize {
        return _.endpoint_count;
    }

    // ========================================================================
    // Internal Helpers
    // ========================================================================

    fn clampNow(*_, now_ms: u64): u64 {
        if (now_ms >= _.last_now_ms) {
            _.last_now_ms = now_ms;
            return now_ms;
        }
        return _.last_now_ms;
    }

    fn findSlotIndex(endpoint_id: u64): usize {
        let i: usize = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].endpoint_id == endpoint_id) {
                return i;
            }
        }
        return CIRCUIT_POOL_MAX_ENDPOINTS;
    }

    fn currentEjectedCount(): usize {
        let cnt: usize = 0;
        let i: usize = 0;
        while (i < CIRCUIT_POOL_MAX_ENDPOINTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == CircuitEndpointState.ejected_open) {
                cnt += 1;
            }
        }
        return cnt;
    }

    fn tryEjectSlot(*_, idx: usize, effective_now: u64) {
        if (_.currentEjectedCount() >= _.maxEjectableCount()) {
            _.ejection_suppressed_by_cap += 1;
            return;
        }
        const next_count: u32 = if (_.slots[idx].ejection_count < 0xFFFFFFFF) (_.slots[idx].ejection_count + 1) else 0xFFFFFFFF;
        _.slots[idx].ejection_count = next_count;

        const raw_duration = saturatingMulU64(_.config.base_ejection_ms, next_count as u64);
        const duration = if (raw_duration > _.config.max_ejection_ms) _.config.max_ejection_ms else raw_duration;

        _.slots[idx].state = CircuitEndpointState.ejected_open;
        _.slots[idx].eject_until_ms = saturatingAddU64(effective_now, duration);
        _.slots[idx].consecutive_failures = 0;
        _.slots[idx].half_open_successes = 0;
        _.slots[idx].probe_in_flight = false;
        _.total_ejections += 1;
    }

    fn resetSlotWindow(*_, idx: usize) {
        let b: usize = 0;
        while (b < CIRCUIT_POOL_WINDOW_BUCKETS) : (b += 1) {
            _.slots[idx].bucket_total[b] = 0;
            _.slots[idx].bucket_errors[b] = 0;
        }
        _.slots[idx].window_total = 0;
        _.slots[idx].window_errors = 0;
        _.slots[idx].error_rate_bps = 0;
        _.slots[idx].window_initialized = false;
    }

    fn recordWindowSample(*_, idx: usize, effective_now: u64, is_error: bool) {
        const bucket_ms: u64 = _.config.window_ms / (CIRCUIT_POOL_WINDOW_BUCKETS as u64);
        const cur_bucket: u64 = effective_now / bucket_ms;

        if (!_.slots[idx].window_initialized) {
            _.resetSlotWindow(idx);
            _.slots[idx].last_bucket_idx = cur_bucket;
            _.slots[idx].window_initialized = true;
        } else if (cur_bucket > _.slots[idx].last_bucket_idx) {
            const diff = cur_bucket - _.slots[idx].last_bucket_idx;
            if (diff >= (CIRCUIT_POOL_WINDOW_BUCKETS as u64)) {
                let b: usize = 0;
                while (b < CIRCUIT_POOL_WINDOW_BUCKETS) : (b += 1) {
                    _.slots[idx].bucket_total[b] = 0;
                    _.slots[idx].bucket_errors[b] = 0;
                }
            } else {
                let step: u64 = 1;
                while (step <= diff) : (step += 1) {
                    const ring_i: usize = ((_.slots[idx].last_bucket_idx + step) % (CIRCUIT_POOL_WINDOW_BUCKETS as u64)) as usize;
                    _.slots[idx].bucket_total[ring_i] = 0;
                    _.slots[idx].bucket_errors[ring_i] = 0;
                }
            }
            _.slots[idx].last_bucket_idx = cur_bucket;
        }

        const cur_ring: usize = (cur_bucket % (CIRCUIT_POOL_WINDOW_BUCKETS as u64)) as usize;
        _.slots[idx].bucket_total[cur_ring] += 1;
        if (is_error) {
            _.slots[idx].bucket_errors[cur_ring] += 1;
        }

        let total: u32 = 0;
        let errs: u32 = 0;
        let b: usize = 0;
        while (b < CIRCUIT_POOL_WINDOW_BUCKETS) : (b += 1) {
            total += _.slots[idx].bucket_total[b];
            errs += _.slots[idx].bucket_errors[b];
        }
        _.slots[idx].window_total = total;
        _.slots[idx].window_errors = errs;
        if (total > 0) {
            _.slots[idx].error_rate_bps = (((errs as u64) * (CIRCUIT_POOL_BPS_DENOM as u64)) / (total as u64)) as u32;
        } else {
            _.slots[idx].error_rate_bps = 0;
        }
    }
}
