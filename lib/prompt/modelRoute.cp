// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Model LLM Gateway Router, SLA Optimizer & Cascading Fallback) | Created: 2026-11-23
// Pure CP zero-heap-allocation multi-model LLM gateway routing & fault-tolerant fallback engine:
//   - Fixed 16-endpoint model registry tracking `model_id`, capability bitmask (`tools | vision | json_mode |
//     reasoning | streaming`), `max_context_tokens`, per-1M input/output micro-USD pricing, `quality_bps`,
//     real-time health, EWMA latency, consecutive failures & exponential-backoff `cooldown_until_ms`
//   - Multi-objective route planning & ordered fallback chain generation (`planRoute`):
//       * Filters out capability mismatches, insufficient context windows, disabled or cooling-down endpoints,
//         and hard latency/cost/quality SLA violations
//       * Supports 4 routing strategies: `lowest_cost`, `lowest_latency`, `highest_quality`, `balanced_sla`
//       * Outputs complete ordered fallback chain (Primary -> Fallback 1 -> Fallback 2 ...)
//   - Closed-loop outcome feedback (`recordOutcome`): EWMA latency updates on `ok`, immediate exponential
//     cooldown circuit trip on 429 `rate_limited`, thresholded cooldown on `server_error` / `timeout`
//   - Zero-alloc request cost estimator (`estimateRequestCostMicros`) & global telemetry snapshot

// ============================================================================
// 1. Constants, Capability Flags, Enums & Data Models
// ============================================================================

/// Maximum number of LLM endpoints registered inline (zero dynamic heap allocation).
pub const MODEL_ROUTE_MAX_ENDPOINTS: usize = 16;

/// Basis points scale (10000 = 100.00%).
pub const MODEL_ROUTE_BPS_SCALE: u32 = 10000;

/// Model capability bitmask flags.
pub const MODEL_CAP_TOOLS: u32 = 1 << 0;
pub const MODEL_CAP_VISION: u32 = 1 << 1;
pub const MODEL_CAP_JSON_MODE: u32 = 1 << 2;
pub const MODEL_CAP_REASONING: u32 = 1 << 3;
pub const MODEL_CAP_STREAMING: u32 = 1 << 4;

/// Default base cooldown duration on circuit trip (5 seconds).
pub const MODEL_ROUTE_DEFAULT_BASE_COOLDOWN_MS: u64 = 5000;

/// Default maximum exponential-backoff cooldown duration (5 minutes).
pub const MODEL_ROUTE_DEFAULT_MAX_COOLDOWN_MS: u64 = 300_000;

/// Default consecutive `server_error`/`timeout` threshold before tripping cooldown circuit.
pub const MODEL_ROUTE_DEFAULT_FAIL_THRESHOLD: u32 = 3;

/// Routing optimization strategy.
pub enum ModelRouteStrategy {
    lowest_cost,
    lowest_latency,
    highest_quality,
    balanced_sla,
}

/// Runtime health state of a registered LLM endpoint.
pub enum ModelEndpointHealth {
    healthy,
    degraded,
    cooldown,
    disabled,
}

/// Outcome classification fed back to `recordOutcome`.
pub enum ModelCallOutcomeKind {
    ok,
    rate_limited,
    server_error,
    timeout,
}

/// Static specification of an LLM endpoint.
pub struct ModelEndpointSpec {
    model_id: u64 = 0,
    cap_mask: u32 = 0,
    max_context_tokens: u32 = 0,
    input_price_micros_per_1m: u64 = 0,
    output_price_micros_per_1m: u64 = 0,
    /// Intrinsic model quality score in basis points (`0..=10000`).
    quality_bps: u16 = 5000,
    /// Initial EWMA latency seed in milliseconds.
    initial_latency_ms: u32 = 500,
}

/// Inline runtime state of a registered LLM endpoint.
pub struct ModelEndpointState {
    occupied: bool = false,
    spec: ModelEndpointSpec = ModelEndpointSpec{},
    enabled: bool = true,
    health: ModelEndpointHealth = ModelEndpointHealth.healthy,
    ewma_latency_ms: u32 = 500,
    consecutive_failures: u32 = 0,
    cooldown_level: u8 = 0,
    cooldown_until_ms: u64 = 0,
    total_calls: u64 = 0,
    total_successes: u64 = 0,
    total_rate_limits: u64 = 0,
    total_errors: u64 = 0,
}

/// Request constraints and routing objective passed to `planRoute`.
pub struct ModelRouteRequest {
    required_caps: u32 = 0,
    estimated_input_tokens: u32 = 0,
    estimated_output_tokens: u32 = 0,
    strategy: ModelRouteStrategy = ModelRouteStrategy.balanced_sla,
    /// Optional hard EWMA latency ceiling in milliseconds (`0` = no limit).
    max_latency_ms: u32 = 0,
    /// Optional hard estimated request cost ceiling in micro-USD (`0` = no limit).
    max_cost_micros: u64 = 0,
    /// Optional minimum quality score in basis points (`0` = no limit).
    min_quality_bps: u16 = 0,
}

/// Ranked candidate entry in the generated primary-to-fallback routing chain.
pub struct ModelRouteCandidate {
    model_id: u64 = 0,
    estimated_cost_micros: u64 = 0,
    ewma_latency_ms: u32 = 0,
    quality_bps: u16 = 0,
    score: u32 = 0,
}

/// Router circuit-breaker & EWMA configuration.
pub struct ModelRouterConfig {
    base_cooldown_ms: u64 = MODEL_ROUTE_DEFAULT_BASE_COOLDOWN_MS,
    max_cooldown_ms: u64 = MODEL_ROUTE_DEFAULT_MAX_COOLDOWN_MS,
    fail_threshold: u32 = MODEL_ROUTE_DEFAULT_FAIL_THRESHOLD,
    /// EWMA new-sample weight in basis points (`1..=10000`, default 2500 = 25%).
    ewma_alpha_bps: u16 = 2500,
}

/// Global gateway telemetry snapshot.
pub struct ModelRouterStats {
    endpoint_count: usize = 0,
    capacity: usize = MODEL_ROUTE_MAX_ENDPOINTS,
    healthy_count: usize = 0,
    degraded_count: usize = 0,
    cooldown_count: usize = 0,
    disabled_count: usize = 0,
    total_routes_planned: u64 = 0,
    total_routes_empty: u64 = 0,
    total_outcomes_ok: u64 = 0,
    total_outcomes_failed: u64 = 0,
    total_circuit_trips: u64 = 0,
}

// ============================================================================
// 2. Cost Estimator & Ordering Helpers
// ============================================================================

/// Estimates single-request cost in micro-USD using overflow-safe 128-bit ceiling division.
/// Returns `0` when both token counts or prices yield zero total.
pub fn estimateRequestCostMicros(
    input_tokens: u32,
    output_tokens: u32,
    input_price_micros_per_1m: u64,
    output_price_micros_per_1m: u64,
): u64 {
    const in_scaled: u128 = (input_tokens as u128) * (input_price_micros_per_1m as u128);
    const out_scaled: u128 = (output_tokens as u128) * (output_price_micros_per_1m as u128);
    const total_scaled: u128 = in_scaled + out_scaled;
    if (total_scaled == 0) return 0;
    const micros: u128 = (total_scaled + 999_999) / 1_000_000;
    if (micros > 0xFFFF_FFFF_FFFF_FFFF as u128) return 0xFFFF_FFFF_FFFF_FFFF;
    return micros as u64;
}

fn candidatePrecedes(
    a: ModelRouteCandidate,
    b: ModelRouteCandidate,
    strategy: ModelRouteStrategy,
): bool {
    return match (strategy) {
        ModelRouteStrategy.lowest_cost => {
            if (a.estimated_cost_micros != b.estimated_cost_micros) {
                return a.estimated_cost_micros < b.estimated_cost_micros;
            }
            if (a.ewma_latency_ms != b.ewma_latency_ms) {
                return a.ewma_latency_ms < b.ewma_latency_ms;
            }
            if (a.quality_bps != b.quality_bps) {
                return a.quality_bps > b.quality_bps;
            }
            return a.model_id < b.model_id;
        },
        ModelRouteStrategy.lowest_latency => {
            if (a.ewma_latency_ms != b.ewma_latency_ms) {
                return a.ewma_latency_ms < b.ewma_latency_ms;
            }
            if (a.estimated_cost_micros != b.estimated_cost_micros) {
                return a.estimated_cost_micros < b.estimated_cost_micros;
            }
            if (a.quality_bps != b.quality_bps) {
                return a.quality_bps > b.quality_bps;
            }
            return a.model_id < b.model_id;
        },
        ModelRouteStrategy.highest_quality => {
            if (a.quality_bps != b.quality_bps) {
                return a.quality_bps > b.quality_bps;
            }
            if (a.estimated_cost_micros != b.estimated_cost_micros) {
                return a.estimated_cost_micros < b.estimated_cost_micros;
            }
            if (a.ewma_latency_ms != b.ewma_latency_ms) {
                return a.ewma_latency_ms < b.ewma_latency_ms;
            }
            return a.model_id < b.model_id;
        },
        ModelRouteStrategy.balanced_sla => {
            if (a.score != b.score) {
                return a.score > b.score;
            }
            if (a.estimated_cost_micros != b.estimated_cost_micros) {
                return a.estimated_cost_micros < b.estimated_cost_micros;
            }
            if (a.ewma_latency_ms != b.ewma_latency_ms) {
                return a.ewma_latency_ms < b.ewma_latency_ms;
            }
            return a.model_id < b.model_id;
        },
    };
}

// ============================================================================
// 3. Core Zero-Alloc Multi-Model LLM Gateway Router
// ============================================================================

/// Zero-heap-allocation multi-model LLM gateway router with SLA optimization & exponential-backoff fallback.
pub struct ModelGatewayRouter {
    config: ModelRouterConfig = ModelRouterConfig{},
    endpoints: [MODEL_ROUTE_MAX_ENDPOINTS]ModelEndpointState = [ModelEndpointState{}; MODEL_ROUTE_MAX_ENDPOINTS],
    endpoint_count: usize = 0,
    total_routes_planned: u64 = 0,
    total_routes_empty: u64 = 0,
    total_outcomes_ok: u64 = 0,
    total_outcomes_failed: u64 = 0,
    total_circuit_trips: u64 = 0,

    /// Creates a new router with default circuit-breaker and EWMA settings.
    pub static fn init(): ModelGatewayRouter {
        return ModelGatewayRouter{};
    }

    /// Creates a new router with custom configuration (zero/out-of-range fields normalize to safe defaults).
    pub static fn initWithConfig(cfg: ModelRouterConfig): ModelGatewayRouter {
        const base_cd = if (cfg.base_cooldown_ms == 0) MODEL_ROUTE_DEFAULT_BASE_COOLDOWN_MS else cfg.base_cooldown_ms;
        const max_cd = if (cfg.max_cooldown_ms < base_cd) base_cd else cfg.max_cooldown_ms;
        const fail_th = if (cfg.fail_threshold == 0) MODEL_ROUTE_DEFAULT_FAIL_THRESHOLD else cfg.fail_threshold;
        const alpha = if (cfg.ewma_alpha_bps == 0 || cfg.ewma_alpha_bps > 10000) 2500 as u16 else cfg.ewma_alpha_bps;

        return ModelGatewayRouter{
            config: ModelRouterConfig{
                base_cooldown_ms: base_cd,
                max_cooldown_ms: max_cd,
                fail_threshold: fail_th,
                ewma_alpha_bps: alpha,
            },
        };
    }

    // ---- Internal Helpers ----

    fn findSlot(model_id: u64): usize? {
        if (model_id == 0) return null;
        let i: usize = 0;
        while (i < MODEL_ROUTE_MAX_ENDPOINTS) : (i += 1) {
            if (_.endpoints[i].occupied && _.endpoints[i].spec.model_id == model_id) {
                return i;
            }
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < MODEL_ROUTE_MAX_ENDPOINTS) : (i += 1) {
            if (!_.endpoints[i].occupied) return i;
        }
        return null;
    }

    fn tripCooldown(*_, idx: usize, now_ms: u64) {
        const shift: u6 = if (_.endpoints[idx].cooldown_level > 10) 10 else (_.endpoints[idx].cooldown_level as u6);
        const factor: u64 = (1 as u64) << shift;
        const raw_cd: u64 = _.config.base_cooldown_ms *| factor;
        const clamped_cd: u64 = if (raw_cd > _.config.max_cooldown_ms) _.config.max_cooldown_ms else raw_cd;

        _.endpoints[idx].cooldown_until_ms = now_ms +| clamped_cd;
        if (_.endpoints[idx].cooldown_level < 10) {
            _.endpoints[idx].cooldown_level += 1;
        }
        if (_.endpoints[idx].enabled) {
            _.endpoints[idx].health = ModelEndpointHealth.cooldown;
        }
        _.total_circuit_trips += 1;
    }

    // ---- Endpoint Management ----

    /// Registers an LLM endpoint. Returns `false` on `model_id == 0`, `quality_bps > 10000`, duplicate ID, or full table.
    pub fn registerEndpoint(*_, spec: ModelEndpointSpec): bool {
        if (spec.model_id == 0 || spec.quality_bps > 10000) return false;
        if (_.findSlot(spec.model_id) != null) return false;
        const slot = _.findFreeSlot() ?? return false;

        _.endpoints[slot] = ModelEndpointState{
            occupied: true,
            spec: spec,
            enabled: true,
            health: ModelEndpointHealth.healthy,
            ewma_latency_ms: spec.initial_latency_ms,
        };
        _.endpoint_count += 1;
        return true;
    }

    /// Administratively enables or disables a registered endpoint.
    pub fn setEnabled(*_, model_id: u64, enabled: bool): bool {
        const idx = _.findSlot(model_id) ?? return false;
        _.endpoints[idx].enabled = enabled;
        if (!enabled) {
            _.endpoints[idx].health = ModelEndpointHealth.disabled;
        } else if (_.endpoints[idx].health == ModelEndpointHealth.disabled) {
            _.endpoints[idx].health = ModelEndpointHealth.healthy;
            _.endpoints[idx].consecutive_failures = 0;
            _.endpoints[idx].cooldown_until_ms = 0;
            _.endpoints[idx].cooldown_level = 0;
        }
        return true;
    }

    // ---- Route Planning & Cascading Fallback ----

    /// Filters eligible endpoints, ranks them according to `req.strategy`, and writes the ordered
    /// primary-to-fallback chain into `out_chain`. Returns the number of candidates written.
    pub fn planRoute(
        *_,
        req: ModelRouteRequest,
        now_ms: u64,
        out_chain: *[]ModelRouteCandidate,
    ): usize {
        _.total_routes_planned += 1;
        if (out_chain.len == 0 || _.endpoint_count == 0) {
            _.total_routes_empty += 1;
            return 0;
        }

        const needed_tokens: u64 = (req.estimated_input_tokens as u64) + (req.estimated_output_tokens as u64);
        let pool: [MODEL_ROUTE_MAX_ENDPOINTS]ModelRouteCandidate = [ModelRouteCandidate{}; MODEL_ROUTE_MAX_ENDPOINTS];
        let count: usize = 0;
        let max_cost_seen: u64 = 0;
        let max_lat_seen: u32 = 0;

        let i: usize = 0;
        while (i < MODEL_ROUTE_MAX_ENDPOINTS) : (i += 1) {
            const ep = _.endpoints[i];
            if (!ep.occupied || !ep.enabled) continue;
            if (ep.health == ModelEndpointHealth.cooldown && now_ms < ep.cooldown_until_ms) continue;
            if ((ep.spec.cap_mask & req.required_caps) != req.required_caps) continue;
            if (needed_tokens > (ep.spec.max_context_tokens as u64)) continue;
            if (ep.spec.quality_bps < req.min_quality_bps) continue;
            if (req.max_latency_ms > 0 && ep.ewma_latency_ms > req.max_latency_ms) continue;

            const est_cost = estimateRequestCostMicros(
                req.estimated_input_tokens,
                req.estimated_output_tokens,
                ep.spec.input_price_micros_per_1m,
                ep.spec.output_price_micros_per_1m,
            );
            if (req.max_cost_micros > 0 && est_cost > req.max_cost_micros) continue;

            if (est_cost > max_cost_seen) max_cost_seen = est_cost;
            if (ep.ewma_latency_ms > max_lat_seen) max_lat_seen = ep.ewma_latency_ms;

            pool[count] = ModelRouteCandidate{
                model_id: ep.spec.model_id,
                estimated_cost_micros: est_cost,
                ewma_latency_ms: ep.ewma_latency_ms,
                quality_bps: ep.spec.quality_bps,
                score: ep.spec.quality_bps as u32,
            };
            count += 1;
        }

        if (count == 0) {
            _.total_routes_empty += 1;
            return 0;
        }

        // Compute composite SLA scores for `balanced_sla` (50% quality + 30% cost efficiency + 20% latency efficiency)
        if (req.strategy == ModelRouteStrategy.balanced_sla) {
            const cost_ref: u64 = if (req.max_cost_micros > 0) req.max_cost_micros else max_cost_seen;
            const lat_ref: u32 = if (req.max_latency_ms > 0) req.max_latency_ms else max_lat_seen;

            let j: usize = 0;
            while (j < count) : (j += 1) {
                const cost_eff: u32 = if (cost_ref == 0)
                    MODEL_ROUTE_BPS_SCALE
                else
                    (MODEL_ROUTE_BPS_SCALE - (((pool[j].estimated_cost_micros * (MODEL_ROUTE_BPS_SCALE as u64)) / cost_ref) as u32));

                const lat_eff: u32 = if (lat_ref == 0)
                    MODEL_ROUTE_BPS_SCALE
                else
                    (MODEL_ROUTE_BPS_SCALE - (((pool[j].ewma_latency_ms as u64) * (MODEL_ROUTE_BPS_SCALE as u64) / (lat_ref as u64)) as u32));

                const q_part: u32 = (pool[j].quality_bps as u32) * 50;
                const c_part: u32 = cost_eff * 30;
                const l_part: u32 = lat_eff * 20;
                pool[j].score = (q_part + c_part + l_part) / 100;
            }
        }

        // In-place insertion sort on at most 16 candidates (zero heap allocation)
        let k: usize = 1;
        while (k < count) : (k += 1) {
            const key = pool[k];
            let pos: usize = k;
            while (pos > 0 && candidatePrecedes(key, pool[pos - 1], req.strategy)) {
                pool[pos] = pool[pos - 1];
                pos -= 1;
            }
            pool[pos] = key;
        }

        const written = if (count < out_chain.len) count else out_chain.len;
        let w: usize = 0;
        while (w < written) : (w += 1) {
            out_chain[w] = pool[w];
        }
        return written;
    }

    // ---- Closed-Loop Outcome Feedback ----

    /// Records the result of an LLM call, updating EWMA latency, failure streaks, and exponential cooldowns.
    pub fn recordOutcome(
        *_,
        model_id: u64,
        latency_ms: u32,
        outcome: ModelCallOutcomeKind,
        now_ms: u64,
    ): bool {
        const idx = _.findSlot(model_id) ?? return false;
        _.endpoints[idx].total_calls += 1;

        const alpha: u64 = _.config.ewma_alpha_bps as u64;
        const keep: u64 = (MODEL_ROUTE_BPS_SCALE as u64) - alpha;

        match (outcome) {
            ModelCallOutcomeKind.ok => {
                const prev_lat: u64 = _.endpoints[idx].ewma_latency_ms as u64;
                const next_lat: u64 = (prev_lat * keep + (latency_ms as u64) * alpha) / (MODEL_ROUTE_BPS_SCALE as u64);
                _.endpoints[idx].ewma_latency_ms = next_lat as u32;
                _.endpoints[idx].consecutive_failures = 0;
                _.endpoints[idx].cooldown_level = 0;
                _.endpoints[idx].cooldown_until_ms = 0;
                _.endpoints[idx].total_successes += 1;
                if (_.endpoints[idx].enabled) {
                    _.endpoints[idx].health = ModelEndpointHealth.healthy;
                }
                _.total_outcomes_ok += 1;
            },
            ModelCallOutcomeKind.rate_limited => {
                _.endpoints[idx].total_rate_limits += 1;
                _.endpoints[idx].consecutive_failures += 1;
                _.total_outcomes_failed += 1;
                _.tripCooldown(idx, now_ms);
            },
            ModelCallOutcomeKind.server_error => {
                _.endpoints[idx].total_errors += 1;
                _.endpoints[idx].consecutive_failures += 1;
                _.total_outcomes_failed += 1;
                if (_.endpoints[idx].consecutive_failures >= _.config.fail_threshold) {
                    _.tripCooldown(idx, now_ms);
                } else if (_.endpoints[idx].enabled) {
                    _.endpoints[idx].health = ModelEndpointHealth.degraded;
                }
            },
            ModelCallOutcomeKind.timeout => {
                _.endpoints[idx].total_errors += 1;
                _.endpoints[idx].consecutive_failures += 1;
                _.total_outcomes_failed += 1;
                if (latency_ms > 0) {
                    const prev_lat: u64 = _.endpoints[idx].ewma_latency_ms as u64;
                    const next_lat: u64 = (prev_lat * keep + (latency_ms as u64) * alpha) / (MODEL_ROUTE_BPS_SCALE as u64);
                    _.endpoints[idx].ewma_latency_ms = next_lat as u32;
                }
                if (_.endpoints[idx].consecutive_failures >= _.config.fail_threshold) {
                    _.tripCooldown(idx, now_ms);
                } else if (_.endpoints[idx].enabled) {
                    _.endpoints[idx].health = ModelEndpointHealth.degraded;
                }
            },
        }
        return true;
    }

    // ---- Queries & Telemetry ----

    /// Looks up a registered endpoint's runtime state by `model_id`.
    pub fn getEndpoint(model_id: u64): ModelEndpointState? {
        const idx = _.findSlot(model_id) ?? return null;
        return _.endpoints[idx];
    }

    /// Returns a global telemetry snapshot evaluated at `now_ms`.
    pub fn stats(now_ms: u64): ModelRouterStats {
        let healthy: usize = 0;
        let degraded: usize = 0;
        let cooldown: usize = 0;
        let disabled: usize = 0;

        let i: usize = 0;
        while (i < MODEL_ROUTE_MAX_ENDPOINTS) : (i += 1) {
            const ep = _.endpoints[i];
            if (!ep.occupied) continue;
            if (!ep.enabled) {
                disabled += 1;
            } else if (ep.health == ModelEndpointHealth.cooldown && now_ms < ep.cooldown_until_ms) {
                cooldown += 1;
            } else if (ep.health == ModelEndpointHealth.degraded) {
                degraded += 1;
            } else {
                healthy += 1;
            }
        }

        return ModelRouterStats{
            endpoint_count: _.endpoint_count,
            capacity: MODEL_ROUTE_MAX_ENDPOINTS,
            healthy_count: healthy,
            degraded_count: degraded,
            cooldown_count: cooldown,
            disabled_count: disabled,
            total_routes_planned: _.total_routes_planned,
            total_routes_empty: _.total_routes_empty,
            total_outcomes_ok: _.total_outcomes_ok,
            total_outcomes_failed: _.total_outcomes_failed,
            total_circuit_trips: _.total_circuit_trips,
        };
    }
}
