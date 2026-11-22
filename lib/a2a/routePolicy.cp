// Origin: Pure-CP Stdlib (Zero-Alloc A2A Capability/Cost/Latency/SLA Policy Router & Fallback Chain) | Created: 2026-11-23
// Pure CP zero-heap-allocation multi-agent policy router and ordered fallback chain engine:
//   - Fixed 32-target agent table (`A2A_ROUTE_MAX_TARGETS`) with inline agent IDs (`48B`)
//   - Hard filtering by 64-bit capability bitmask, cost cap, p95 latency SLA, reliability floor,
//     in-flight concurrency headroom, and target health state (`circuit_open` / `draining` isolation)
//   - Four deterministic policy goals (`lowest_latency`, `lowest_cost`, `highest_reliability`,
//     `balanced_sla`) producing a primary target plus up to 4 ordered deduplicated fallbacks
//   - Graceful degradation to `degraded` targets when all `healthy` targets are unavailable and
//     `allow_degraded_fallback == true`

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of agent targets stored inline in `A2aRoutePolicyEngine`.
pub const A2A_ROUTE_MAX_TARGETS: usize = 32;

/// Maximum number of ordered fallback targets emitted in a `RoutePlan`.
pub const A2A_ROUTE_MAX_FALLBACKS: usize = 4;

/// Maximum UTF-8 byte length of an agent identifier stored inline.
pub const A2A_ROUTE_ID_MAX_LEN: usize = 48;

/// Basis-point scale (`10000 = 100.00%`) for reliability and canary weights.
pub const A2A_ROUTE_BPS_SCALE: u16 = 10_000;

/// Primary optimization objective used to rank eligible agent targets.
pub enum RoutePolicyGoal {
    lowest_latency,
    lowest_cost,
    highest_reliability,
    balanced_sla,
}

/// Runtime availability state of a registered agent target.
pub enum RouteTargetState {
    healthy,
    degraded,
    circuit_open,
    draining,
}

/// Routing decision outcome / rejection classification.
pub enum RouteRejectKind {
    ok,
    no_matching_capability,
    budget_exceeded,
    latency_sla_exceeded,
    all_unavailable,
}

/// Registered agent target specification and live telemetry metrics.
pub struct RouteTargetSpec {
    active: bool = false;
    agent_id_buf: [A2A_ROUTE_ID_MAX_LEN]u8 = [0; A2A_ROUTE_ID_MAX_LEN];
    agent_id_len: u16 = 0;
    capability_mask: u64 = 0;
    cost_per_1k_micros: u32 = 0;
    p95_latency_ms: u32 = 0;
    success_rate_bps: u16 = A2A_ROUTE_BPS_SCALE;
    max_inflight: u16 = 64;
    active_inflight: u16 = 0;
    state: RouteTargetState = RouteTargetState.healthy;
    canary_weight_bps: u16 = A2A_ROUTE_BPS_SCALE;

    /// Return the agent identifier slice.
    fn agent_id(): []const u8 {
        return agent_id_buf[0..@as(usize, agent_id_len)];
    }

    /// Return true if this target has remaining concurrency headroom.
    fn hasCapacity(): bool {
        return max_inflight > 0 && active_inflight < max_inflight;
    }
}

/// Task routing constraints and policy goal submitted to `A2aRoutePolicyEngine.planRoute`.
pub struct RouteTaskSpec {
    required_capabilities: u64 = 0;
    max_cost_micros: u32 = 0;
    max_latency_ms: u32 = 0;
    min_success_rate_bps: u16 = 0;
    goal: RoutePolicyGoal = RoutePolicyGoal.balanced_sla;
    allow_degraded_fallback: bool = true;
    sticky_key_hash: u64 = 0;
}

/// Deterministic primary + ordered fallback routing plan returned by `planRoute`.
pub struct RoutePlan {
    status: RouteRejectKind = RouteRejectKind.all_unavailable;
    primary_index: usize = 0;
    fallback_count: usize = 0;
    fallback_indices: [A2A_ROUTE_MAX_FALLBACKS]usize = [0; A2A_ROUTE_MAX_FALLBACKS];
    evaluated_candidates: usize = 0;
    filtered_by_cap: usize = 0;
    filtered_by_sla: usize = 0;
    filtered_by_state: usize = 0;
    primary_score: f32 = 0.0;
    used_degraded_fallback: bool = false;
}

/// Cumulative routing telemetry snapshot for `A2aRoutePolicyEngine`.
pub struct RoutePolicyStats {
    total_routes: u64 = 0;
    routed_ok: u64 = 0;
    routed_degraded: u64 = 0;
    rejected_count: u64 = 0;
    target_count: usize = 0;
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

fn scoreTargetForGoal(t: RouteTargetSpec, goal: RoutePolicyGoal): f32 {
    const lat_f: f32 = @as(f32, @floatFromInt(t.p95_latency_ms));
    const cost_f: f32 = @as(f32, @floatFromInt(t.cost_per_1k_micros));
    const rel_f: f32 = @as(f32, @floatFromInt(t.success_rate_bps)) / 10000.0;

    if (goal == RoutePolicyGoal.lowest_latency) {
        return 1000000.0 / (1.0 + lat_f);
    }
    if (goal == RoutePolicyGoal.lowest_cost) {
        return 1000000.0 / (1.0 + cost_f);
    }
    if (goal == RoutePolicyGoal.highest_reliability) {
        return rel_f * 1000.0 + (1.0 / (1.0 + lat_f));
    }

    // balanced_sla: 40% reliability + 30% latency + 20% cost + 10% concurrency headroom
    const lat_score: f32 = 1000.0 / (1000.0 + lat_f);
    const cost_score: f32 = 1000.0 / (1000.0 + cost_f);
    let headroom: f32 = 0.0;
    if (t.max_inflight > 0 && t.active_inflight < t.max_inflight) {
        headroom = @as(f32, @floatFromInt(t.max_inflight - t.active_inflight)) / @as(f32, @floatFromInt(t.max_inflight));
    }
    return 0.40 * rel_f + 0.30 * lat_score + 0.20 * cost_score + 0.10 * headroom;
}

fn sortCandidatesDesc(indices: *[]usize, scores: *[]f32, count: usize) {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        let j: usize = i + 1;
        while (j < count) : (j += 1) {
            const swap = (scores[j] > scores[i]) || (scores[j] == scores[i] && indices[j] < indices[i]);
            if (swap) {
                const ts = scores[i];
                scores[i] = scores[j];
                scores[j] = ts;
                const ti = indices[i];
                indices[i] = indices[j];
                indices[j] = ti;
            }
        }
    }
}

// ============================================================================
// 3. A2aRoutePolicyEngine — Zero-Heap Multi-Agent Policy Router
// ============================================================================

/// Zero-heap-allocation A2A multi-agent policy router and fallback chain planner.
pub struct A2aRoutePolicyEngine {
    targets: [A2A_ROUTE_MAX_TARGETS]RouteTargetSpec = [RouteTargetSpec{}; A2A_ROUTE_MAX_TARGETS];
    target_count: usize = 0;
    total_routes: u64 = 0;
    routed_ok: u64 = 0;
    routed_degraded: u64 = 0;
    rejected_count: u64 = 0;

    /// Initialize an empty `A2aRoutePolicyEngine`.
    static fn init(): A2aRoutePolicyEngine {
        return A2aRoutePolicyEngine{};
    }

    fn findTargetIndex(agent_id: []const u8): usize? {
        if (agent_id.len == 0 || agent_id.len > A2A_ROUTE_ID_MAX_LEN) return null;
        let i: usize = 0;
        while (i < A2A_ROUTE_MAX_TARGETS) : (i += 1) {
            if (!targets[i].active) continue;
            const cur: []const u8 = targets[i].agent_id_buf[0..@as(usize, targets[i].agent_id_len)];
            if (bytesEqual(cur, agent_id)) return i;
        }
        return null;
    }

    /// Insert a new agent target or update an existing target's core spec.
    /// Returns the target slot index, or `null` if `agent_id` is empty / too long / table is full.
    fn upsertTarget(
        *_,
        agent_id: []const u8,
        capability_mask: u64,
        cost_per_1k_micros: u32,
        p95_latency_ms: u32,
        success_rate_bps: u16,
        max_inflight: u16,
    ): usize? {
        if (agent_id.len == 0 || agent_id.len > A2A_ROUTE_ID_MAX_LEN) return null;
        const clamped_bps: u16 = if (success_rate_bps > A2A_ROUTE_BPS_SCALE) A2A_ROUTE_BPS_SCALE else success_rate_bps;

        if (existing := findTargetIndex(agent_id)) {
            targets[existing].capability_mask = capability_mask;
            targets[existing].cost_per_1k_micros = cost_per_1k_micros;
            targets[existing].p95_latency_ms = p95_latency_ms;
            targets[existing].success_rate_bps = clamped_bps;
            targets[existing].max_inflight = max_inflight;
            return existing;
        }

        let i: usize = 0;
        while (i < A2A_ROUTE_MAX_TARGETS) : (i += 1) {
            if (!targets[i].active) {
                let spec = RouteTargetSpec{};
                spec.active = true;
                let k: usize = 0;
                while (k < agent_id.len) : (k += 1) {
                    spec.agent_id_buf[k] = agent_id[k];
                }
                spec.agent_id_len = @as(u16, @intCast(agent_id.len));
                spec.capability_mask = capability_mask;
                spec.cost_per_1k_micros = cost_per_1k_micros;
                spec.p95_latency_ms = p95_latency_ms;
                spec.success_rate_bps = clamped_bps;
                spec.max_inflight = max_inflight;
                spec.state = RouteTargetState.healthy;
                targets[i] = spec;
                target_count += 1;
                return i;
            }
        }
        return null;
    }

    /// Update the operational health state of a registered agent target.
    fn setTargetState(*_, agent_id: []const u8, state: RouteTargetState): bool {
        const idx = findTargetIndex(agent_id) ?? return false;
        targets[idx].state = state;
        return true;
    }

    /// Update live telemetry metrics (latency, success rate, active inflight) for a target.
    fn updateTargetMetrics(
        *_,
        agent_id: []const u8,
        p95_latency_ms: u32,
        success_rate_bps: u16,
        active_inflight: u16,
    ): bool {
        const idx = findTargetIndex(agent_id) ?? return false;
        targets[idx].p95_latency_ms = p95_latency_ms;
        targets[idx].success_rate_bps = if (success_rate_bps > A2A_ROUTE_BPS_SCALE) A2A_ROUTE_BPS_SCALE else success_rate_bps;
        targets[idx].active_inflight = active_inflight;
        return true;
    }

    /// Look up a target specification snapshot by `agent_id`.
    fn getTarget(agent_id: []const u8): RouteTargetSpec? {
        const idx = findTargetIndex(agent_id) ?? return null;
        return targets[idx];
    }

    /// Look up an active target specification snapshot by slot index.
    fn getTargetAt(idx: usize): RouteTargetSpec? {
        if (idx >= A2A_ROUTE_MAX_TARGETS || !targets[idx].active) return null;
        return targets[idx];
    }

    /// Plan a primary target and up to 4 ordered deduplicated fallback targets for `task`.
    fn planRoute(*_, task: RouteTaskSpec): RoutePlan {
        total_routes += 1;
        let plan = RoutePlan{};

        let healthy_idx: [A2A_ROUTE_MAX_TARGETS]usize = [0; A2A_ROUTE_MAX_TARGETS];
        let healthy_score: [A2A_ROUTE_MAX_TARGETS]f32 = [0.0; A2A_ROUTE_MAX_TARGETS];
        let healthy_count: usize = 0;

        let degraded_idx: [A2A_ROUTE_MAX_TARGETS]usize = [0; A2A_ROUTE_MAX_TARGETS];
        let degraded_score: [A2A_ROUTE_MAX_TARGETS]f32 = [0.0; A2A_ROUTE_MAX_TARGETS];
        let degraded_count: usize = 0;

        let rejected_budget: usize = 0;
        let rejected_latency: usize = 0;
        let rejected_reliability: usize = 0;

        let i: usize = 0;
        while (i < A2A_ROUTE_MAX_TARGETS) : (i += 1) {
            if (!targets[i].active) continue;
            plan.evaluated_candidates += 1;
            const t = targets[i];

            // 1. Capability bitmask hard filter
            if ((t.capability_mask & task.required_capabilities) != task.required_capabilities) {
                plan.filtered_by_cap += 1;
                continue;
            }

            // 2. State & concurrency capacity filter
            if (t.state == RouteTargetState.circuit_open || t.state == RouteTargetState.draining || !t.hasCapacity()) {
                plan.filtered_by_state += 1;
                continue;
            }
            if (t.state == RouteTargetState.degraded && !task.allow_degraded_fallback) {
                plan.filtered_by_state += 1;
                continue;
            }

            // 3. Cost, latency & reliability SLA hard gates
            if (task.max_cost_micros > 0 && t.cost_per_1k_micros > task.max_cost_micros) {
                plan.filtered_by_sla += 1;
                rejected_budget += 1;
                continue;
            }
            if (task.max_latency_ms > 0 && t.p95_latency_ms > task.max_latency_ms) {
                plan.filtered_by_sla += 1;
                rejected_latency += 1;
                continue;
            }
            if (t.success_rate_bps < task.min_success_rate_bps) {
                plan.filtered_by_sla += 1;
                rejected_reliability += 1;
                continue;
            }

            const sc = scoreTargetForGoal(t, task.goal);
            if (t.state == RouteTargetState.healthy) {
                healthy_idx[healthy_count] = i;
                healthy_score[healthy_count] = sc;
                healthy_count += 1;
            } else {
                degraded_idx[degraded_count] = i;
                degraded_score[degraded_count] = sc;
                degraded_count += 1;
            }
        }

        if (healthy_count == 0 && degraded_count == 0) {
            rejected_count += 1;
            if (plan.evaluated_candidates == 0) {
                plan.status = RouteRejectKind.all_unavailable;
            } else if (plan.filtered_by_cap == plan.evaluated_candidates) {
                plan.status = RouteRejectKind.no_matching_capability;
            } else if (rejected_budget > 0 && rejected_budget >= rejected_latency && rejected_budget >= rejected_reliability) {
                plan.status = RouteRejectKind.budget_exceeded;
            } else if (rejected_latency > 0 && rejected_latency >= rejected_reliability) {
                plan.status = RouteRejectKind.latency_sla_exceeded;
            } else {
                plan.status = RouteRejectKind.all_unavailable;
            }
            return plan;
        }

        let h_idx_s: []usize = healthy_idx[0..];
        let h_sc_s: []f32 = healthy_score[0..];
        sortCandidatesDesc(*h_idx_s, *h_sc_s, healthy_count);

        let d_idx_s: []usize = degraded_idx[0..];
        let d_sc_s: []f32 = degraded_score[0..];
        sortCandidatesDesc(*d_idx_s, *d_sc_s, degraded_count);

        plan.status = RouteRejectKind.ok;
        routed_ok += 1;

        if (healthy_count > 0) {
            plan.primary_index = healthy_idx[0];
            plan.primary_score = healthy_score[0];

            // Fill ordered fallbacks from remaining healthy targets first, then degraded (if allowed).
            let h: usize = 1;
            while (h < healthy_count && plan.fallback_count < A2A_ROUTE_MAX_FALLBACKS) : (h += 1) {
                plan.fallback_indices[plan.fallback_count] = healthy_idx[h];
                plan.fallback_count += 1;
            }
            if (task.allow_degraded_fallback) {
                let d: usize = 0;
                while (d < degraded_count && plan.fallback_count < A2A_ROUTE_MAX_FALLBACKS) : (d += 1) {
                    plan.fallback_indices[plan.fallback_count] = degraded_idx[d];
                    plan.fallback_count += 1;
                }
            }
        } else {
            // All healthy targets unavailable -> graceful degraded fallback
            plan.primary_index = degraded_idx[0];
            plan.primary_score = degraded_score[0];
            plan.used_degraded_fallback = true;
            routed_degraded += 1;

            let d: usize = 1;
            while (d < degraded_count && plan.fallback_count < A2A_ROUTE_MAX_FALLBACKS) : (d += 1) {
                plan.fallback_indices[plan.fallback_count] = degraded_idx[d];
                plan.fallback_count += 1;
            }
        }

        return plan;
    }

    /// Return cumulative routing telemetry statistics.
    fn stats(): RoutePolicyStats {
        return RoutePolicyStats{
            total_routes: total_routes,
            routed_ok: routed_ok,
            routed_degraded: routed_degraded,
            rejected_count: rejected_count,
            target_count: target_count,
        };
    }
}
