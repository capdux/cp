// Origin: Pure-CP Stdlib (Unbounded-Slice Weighted DRF, Multi-Dimensional Vector Bin-Packing, Preemption Planner & EDF Real-Time Scheduler) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice heterogeneous cluster scheduler:
//   - Multi-dimensional `ResourceVec` (up to 8 dimensions: CPU, RAM, GPU, Net, Disk, ...)
//   - Weighted Dominant Resource Fairness (Ghodsi et al., NSDI 2011) with min-guarantee priority,
//     max-quota enforcement, batch saturation scheduling & Jain's Fairness Index
//   - Heterogeneous node bin-packing placement (`first_fit | best_fit_remaining | dot_product_align | worst_fit_spread`)
//     with Kubernetes-style Taint/Toleration bitmasks and Cordon maintenance isolation
//   - Minimum-cost priority preemption eviction planner (`planNodePreemption`)
//   - Event-driven Earliest Deadline First (EDF) real-time scheduler & Liu-Layland schedulability test

const std = @import("std");

// ============================================================================
// 1. Constants, Error Set & Multi-Dimensional Resource Vector
// ============================================================================

/// Maximum number of resource dimensions represented by a `ResourceVec` (e.g. CPU, RAM, GPU, Net, Disk).
pub const DRF_MAX_RES_DIMS: usize = 8;

const RES_EPS_F64: f64 = 1.0e-9;

/// Error set returned by DRF validation and resource scheduling helpers.
pub err DrfError {
    InvalidDimensions,
    ZeroClusterCapacity,
    InvalidWeight,
    BufferTooSmall,
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

/// Multi-dimensional resource quantity vector (value semantics, zero dynamic allocation).
pub struct ResourceVec {
    dims: usize = 0;
    values: [DRF_MAX_RES_DIMS]f64 = [0.0; DRF_MAX_RES_DIMS];

    /// Creates a zero-initialized resource vector of `dims` dimensions (clamped to `DRF_MAX_RES_DIMS`).
    static fn zero(dims: usize): ResourceVec {
        const d: usize = if (dims > DRF_MAX_RES_DIMS) DRF_MAX_RES_DIMS else dims;
        return ResourceVec{ dims: d };
    }

    /// Creates a 2-dimensional resource vector (e.g. CPU, Memory).
    static fn from2(v0: f64, v1: f64): ResourceVec {
        let r = ResourceVec{ dims: 2 };
        r.values[0] = v0;
        r.values[1] = v1;
        return r;
    }

    /// Creates a 3-dimensional resource vector (e.g. CPU, Memory, GPU).
    static fn from3(v0: f64, v1: f64, v2: f64): ResourceVec {
        let r = ResourceVec{ dims: 3 };
        r.values[0] = v0;
        r.values[1] = v1;
        r.values[2] = v2;
        return r;
    }

    /// Creates a 4-dimensional resource vector (e.g. CPU, Memory, GPU, Network).
    static fn from4(v0: f64, v1: f64, v2: f64, v3: f64): ResourceVec {
        let r = ResourceVec{ dims: 4 };
        r.values[0] = v0;
        r.values[1] = v1;
        r.values[2] = v2;
        r.values[3] = v3;
        return r;
    }

    /// Returns `true` if all active resource dimensions are effectively zero.
    fn isZero(): bool {
        if (dims == 0) return true;
        let i: usize = 0;
        while (i < dims) : (i += 1) {
            if (values[i] > RES_EPS_F64) return false;
        }
        return true;
    }

    /// Component-wise addition `self + other`.
    fn add(other: ResourceVec): ResourceVec {
        const d: usize = if (dims >= other.dims) dims else other.dims;
        let out = ResourceVec{ dims: if (d > DRF_MAX_RES_DIMS) DRF_MAX_RES_DIMS else d };
        let i: usize = 0;
        while (i < out.dims) : (i += 1) {
            out.values[i] = values[i] + other.values[i];
        }
        return out;
    }

    /// Component-wise saturating subtraction `max(0.0, self - other)`.
    fn sub(other: ResourceVec): ResourceVec {
        const d: usize = if (dims >= other.dims) dims else other.dims;
        let out = ResourceVec{ dims: if (d > DRF_MAX_RES_DIMS) DRF_MAX_RES_DIMS else d };
        let i: usize = 0;
        while (i < out.dims) : (i += 1) {
            let diff: f64 = values[i] - other.values[i];
            if (diff < 0.0) diff = 0.0;
            out.values[i] = diff;
        }
        return out;
    }

    /// Scales all active dimensions by `factor`.
    fn scale(factor: f64): ResourceVec {
        let out = ResourceVec{ dims: dims };
        let i: usize = 0;
        while (i < dims) : (i += 1) {
            out.values[i] = values[i] * factor;
        }
        return out;
    }

    /// Returns `true` iff every active dimension satisfies `self[d] <= cap[d] + eps`.
    fn fitsWithin(cap: ResourceVec): bool {
        if (dims == 0 || cap.dims < dims) return false;
        let i: usize = 0;
        while (i < dims) : (i += 1) {
            if (values[i] > cap.values[i] + RES_EPS_F64) {
                return false;
            }
        }
        return true;
    }

    /// Computes weighted dominant share: `max_d(self[d] / capacity[d]) / weight`.
    fn dominantShare(capacity: ResourceVec, weight: f64): f64 {
        if (dims == 0 || capacity.dims == 0 || weight <= 0.0 || !isFinite(weight)) {
            return 0.0;
        }
        const d_lim: usize = if (dims < capacity.dims) dims else capacity.dims;
        let max_share: f64 = 0.0;
        let i: usize = 0;
        while (i < d_lim) : (i += 1) {
            if (capacity.values[i] > 0.0) {
                const s: f64 = values[i] / capacity.values[i];
                if (s > max_share) {
                    max_share = s;
                }
            }
        }
        return max_share / weight;
    }

    /// Computes mean normalized utilization across all positive-capacity dimensions `[0.0 .. 1.0+]`.
    fn utilizationRatio(capacity: ResourceVec): f64 {
        if (dims == 0 || capacity.dims == 0) {
            return 0.0;
        }
        const d_lim: usize = if (dims < capacity.dims) dims else capacity.dims;
        let sum_u: f64 = 0.0;
        let active_dims: usize = 0;
        let i: usize = 0;
        while (i < d_lim) : (i += 1) {
            if (capacity.values[i] > 0.0) {
                sum_u += values[i] / capacity.values[i];
                active_dims += 1;
            }
        }
        if (active_dims == 0) return 0.0;
        return sum_u / @as(f64, @floatFromInt(active_dims));
    }
}

/// Validates that `cap` has valid dimensions (`1..=8`) and strictly positive total capacity.
pub fn validateClusterCapacity(cap: ResourceVec): void ! DrfError {
    if (cap.dims == 0 || cap.dims > DRF_MAX_RES_DIMS) {
        return DrfError.InvalidDimensions;
    }
    let has_pos: bool = false;
    let i: usize = 0;
    while (i < cap.dims) : (i += 1) {
        if (!isFinite(cap.values[i]) || cap.values[i] < 0.0) {
            return DrfError.ZeroClusterCapacity;
        }
        if (cap.values[i] > 0.0) {
            has_pos = true;
        }
    }
    if (!has_pos) {
        return DrfError.ZeroClusterCapacity;
    }
}

// ============================================================================
// 2. Weighted Dominant Resource Fairness (DRF) Multi-Tenant Scheduler
// ============================================================================

/// Multi-tenant scheduling profile for Weighted DRF with min-guarantee and max-quota bounds.
pub struct DrfTenant {
    tenant_id: u32 = 0;
    weight: f64 = 1.0;
    min_guarantee: ResourceVec = ResourceVec{};
    max_quota: ResourceVec = ResourceVec{};
    allocated: ResourceVec = ResourceVec{};
    pending_demand: ResourceVec = ResourceVec{};
    tasks_assigned: usize = 0;

    /// Returns this tenant's weighted dominant share relative to `cluster_cap`.
    fn effectiveDominantShare(cluster_cap: ResourceVec): f64 {
        return allocated.dominantShare(cluster_cap, weight);
    }

    /// Returns `true` if any dimension with a positive `min_guarantee` is still unmet.
    fn isBelowMinGuarantee(): bool {
        if (min_guarantee.dims == 0) return false;
        let i: usize = 0;
        while (i < min_guarantee.dims) : (i += 1) {
            if (min_guarantee.values[i] > RES_EPS_F64 &&
                allocated.values[i] + RES_EPS_F64 < min_guarantee.values[i])
            {
                return true;
            }
        }
        return false;
    }

    /// Returns `true` if adding `add_vec` respects all positive `max_quota` dimensions.
    fn respectsMaxQuotaAfter(add_vec: ResourceVec): bool {
        if (max_quota.dims == 0) return true;
        const d_lim: usize = if (max_quota.dims < add_vec.dims) max_quota.dims else add_vec.dims;
        let i: usize = 0;
        while (i < d_lim) : (i += 1) {
            if (max_quota.values[i] > 0.0) {
                if (allocated.values[i] + add_vec.values[i] > max_quota.values[i] + RES_EPS_F64) {
                    return false;
                }
            }
        }
        return true;
    }
}

/// Record of a single Weighted DRF allocation decision.
pub struct DrfAllocDecision {
    tenant_index: usize = 0;
    tenant_id: u32 = 0;
    dominant_share_before: f64 = 0.0;
    dominant_share_after: f64 = 0.0;
    allocated_vec: ResourceVec = ResourceVec{};
}

/// Selects and allocates one task unit to the highest-priority eligible tenant according to
/// Weighted DRF:
///   1. Eligible tenants currently below `min_guarantee` take strict precedence.
///   2. Within each tier, selects the eligible tenant with the smallest `effectiveDominantShare`
///      (tie-broken deterministically by smallest `tenant_id`).
/// Updates `tenants[picked]` and `remaining_cap` in-place; returns `null` when saturated.
pub fn drfScheduleNext(
    tenants: *[]DrfTenant,
    cluster_cap: ResourceVec,
    remaining_cap: *ResourceVec,
): DrfAllocDecision? {
    if (tenants.len == 0 || cluster_cap.dims == 0) {
        return null;
    }

    let found: bool = false;
    let best_idx: usize = 0;
    let best_below_min: bool = false;
    let best_share: f64 = 0.0;
    let best_tid: u32 = 0;

    let i: usize = 0;
    while (i < tenants.len) : (i += 1) {
        const t = tenants[i];
        if (!isFinite(t.weight) || t.weight <= 0.0) continue;
        if (t.pending_demand.isZero()) continue;
        if (!t.pending_demand.fitsWithin(remaining_cap.*)) continue;
        if (!t.respectsMaxQuotaAfter(t.pending_demand)) continue;

        const below_min: bool = t.isBelowMinGuarantee();
        const share: f64 = t.effectiveDominantShare(cluster_cap);

        let better: bool = false;
        if (!found) {
            better = true;
        } else if (below_min && !best_below_min) {
            better = true;
        } else if (below_min == best_below_min) {
            if (share < best_share - RES_EPS_F64) {
                better = true;
            } else if (absF64(share - best_share) <= RES_EPS_F64 && t.tenant_id < best_tid) {
                better = true;
            }
        }

        if (better) {
            found = true;
            best_idx = i;
            best_below_min = below_min;
            best_share = share;
            best_tid = t.tenant_id;
        }
    }

    if (!found) {
        return null;
    }

    const req = tenants[best_idx].pending_demand;
    const before_share: f64 = tenants[best_idx].effectiveDominantShare(cluster_cap);
    tenants[best_idx].allocated = tenants[best_idx].allocated.add(req);
    tenants[best_idx].tasks_assigned += 1;
    remaining_cap.* = remaining_cap.*.sub(req);
    const after_share: f64 = tenants[best_idx].effectiveDominantShare(cluster_cap);

    return DrfAllocDecision{
        tenant_index: best_idx,
        tenant_id: tenants[best_idx].tenant_id,
        dominant_share_before: before_share,
        dominant_share_after: after_share,
        allocated_vec: req,
    };
}

/// Runs Weighted DRF scheduling repeatedly until the cluster is saturated, no eligible tenant
/// remains, or `out_decisions` is full. Returns the number of allocations performed.
pub fn drfScheduleBatch(
    tenants: *[]DrfTenant,
    cluster_cap: ResourceVec,
    remaining_cap: *ResourceVec,
    out_decisions: *[]DrfAllocDecision,
): usize {
    let count: usize = 0;
    while (count < out_decisions.len) {
        if (decision := drfScheduleNext(tenants, cluster_cap, remaining_cap)) {
            out_decisions[count] = decision;
            count += 1;
        } else {
            break;
        }
    }
    return count;
}

/// Computes Jain's Fairness Index `J = (sum s_i)^2 / (n * sum s_i^2)` in `(0, 1]` over the
/// weighted dominant shares of all active tenants (`weight > 0`).
pub fn computeJainFairnessIndex(tenants: []const DrfTenant, cluster_cap: ResourceVec): f64 {
    let sum_s: f64 = 0.0;
    let sum_sq: f64 = 0.0;
    let active: usize = 0;

    let i: usize = 0;
    while (i < tenants.len) : (i += 1) {
        if (tenants[i].weight <= 0.0 || !isFinite(tenants[i].weight)) continue;
        const s: f64 = tenants[i].effectiveDominantShare(cluster_cap);
        sum_s += s;
        sum_sq += s * s;
        active += 1;
    }

    if (active == 0 || sum_sq <= 1.0e-18) {
        return 1.0;
    }
    return (sum_s * sum_s) / (@as(f64, @floatFromInt(active)) * sum_sq);
}

// ============================================================================
// 3. Multi-Dimensional Vector Bin-Packing Node Placement
// ============================================================================

/// Multi-dimensional placement heuristic for heterogeneous cluster nodes.
pub enum PlacementStrategy {
    first_fit,
    best_fit_remaining,
    dot_product_align,
    worst_fit_spread,
}

/// Heterogeneous compute node with multi-dimensional capacity, taint bitmask & cordon flag.
pub struct ClusterNode {
    node_id: u32 = 0;
    capacity: ResourceVec = ResourceVec{};
    used: ResourceVec = ResourceVec{};
    cordoned: bool = false;
    taint_mask: u64 = 0;
    tasks_running: usize = 0;

    /// Returns currently available resource capacity on this node.
    fn remaining(): ResourceVec {
        return capacity.sub(used);
    }

    /// Returns `true` iff node is uncordoned, all taints are tolerated, and `req` fits in remaining capacity.
    fn canFit(req: ResourceVec, toleration_mask: u64): bool {
        if (cordoned) return false;
        if ((taint_mask & (~toleration_mask)) != 0) return false;
        return req.fitsWithin(remaining());
    }
}

/// Result of placing a single task onto a cluster node slice.
pub struct PlacementOutcome {
    placed: bool = false;
    node_index: usize = 0;
    node_id: u32 = 0;
    score: f64 = 0.0;
}

fn scoreNodePlacement(node: ClusterNode, req: ResourceVec, strategy: PlacementStrategy): f64 {
    const rem = node.remaining();
    const d_lim: usize = if (req.dims < node.capacity.dims) req.dims else node.capacity.dims;
    if (d_lim == 0) return 0.0;

    if (strategy == PlacementStrategy.first_fit) {
        return 1.0;
    }

    if (strategy == PlacementStrategy.best_fit_remaining) {
        // Minimize residual normalized capacity after placement (maximize 1 - residual).
        let rem_after_sum: f64 = 0.0;
        let active: usize = 0;
        let i: usize = 0;
        while (i < d_lim) : (i += 1) {
            if (node.capacity.values[i] > 0.0) {
                let left: f64 = rem.values[i] - req.values[i];
                if (left < 0.0) left = 0.0;
                rem_after_sum += left / node.capacity.values[i];
                active += 1;
            }
        }
        if (active == 0) return 0.0;
        return 1.0 - (rem_after_sum / @as(f64, @floatFromInt(active)));
    }

    if (strategy == PlacementStrategy.worst_fit_spread) {
        // Maximize residual normalized capacity after placement to spread load evenly.
        let rem_after_sum: f64 = 0.0;
        let active: usize = 0;
        let i: usize = 0;
        while (i < d_lim) : (i += 1) {
            if (node.capacity.values[i] > 0.0) {
                let left: f64 = rem.values[i] - req.values[i];
                if (left < 0.0) left = 0.0;
                rem_after_sum += left / node.capacity.values[i];
                active += 1;
            }
        }
        if (active == 0) return 0.0;
        return rem_after_sum / @as(f64, @floatFromInt(active));
    }

    // `dot_product_align` (Tetris SIGCOMM 2014):
    // Cosine alignment between normalized remaining capacity and normalized task demand,
    // preferring nodes whose dominant free resource matches the task's dominant demand.
    let dot_val: f64 = 0.0;
    let rem_sq: f64 = 0.0;
    let req_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < d_lim) : (i += 1) {
        const cap_d: f64 = node.capacity.values[i];
        if (cap_d > 0.0) {
            const rn: f64 = rem.values[i] / cap_d;
            const qn: f64 = req.values[i] / cap_d;
            dot_val += rn * qn;
            rem_sq += rn * rn;
            req_sq += qn * qn;
        }
    }
    const denom: f64 = std.math.sqrt(rem_sq * req_sq);
    if (denom <= 1.0e-15) return 0.0;
    return dot_val / denom;
}

/// Places a single task request onto the best-scoring eligible node in `nodes` and updates
/// that node's `used` resources and `tasks_running` count in-place.
pub fn placeTaskOnNodes(
    nodes: *[]ClusterNode,
    req: ResourceVec,
    toleration_mask: u64,
    strategy: PlacementStrategy,
): PlacementOutcome {
    let found: bool = false;
    let best_idx: usize = 0;
    let best_score: f64 = -1.0e30;

    let i: usize = 0;
    while (i < nodes.len) : (i += 1) {
        if (!nodes[i].canFit(req, toleration_mask)) {
            continue;
        }
        const sc: f64 = scoreNodePlacement(nodes[i], req, strategy);
        if (strategy == PlacementStrategy.first_fit) {
            found = true;
            best_idx = i;
            best_score = sc;
            break;
        }
        if (!found || sc > best_score + RES_EPS_F64) {
            found = true;
            best_idx = i;
            best_score = sc;
        }
    }

    if (!found) {
        return PlacementOutcome{};
    }

    nodes[best_idx].used = nodes[best_idx].used.add(req);
    nodes[best_idx].tasks_running += 1;

    return PlacementOutcome{
        placed: true,
        node_index: best_idx,
        node_id: nodes[best_idx].node_id,
        score: best_score,
    };
}

/// Places a batch of task requests onto `nodes`, recording each result in `out_outcomes`.
/// Returns the total number of tasks successfully placed.
pub fn placeTasksBatch(
    nodes: *[]ClusterNode,
    reqs: []const ResourceVec,
    toleration_masks: []const u64,
    strategy: PlacementStrategy,
    out_outcomes: *[]PlacementOutcome,
): usize {
    const n_tasks: usize = if (reqs.len < out_outcomes.len) reqs.len else out_outcomes.len;
    let placed_count: usize = 0;
    let i: usize = 0;
    while (i < n_tasks) : (i += 1) {
        const tol: u64 = if (i < toleration_masks.len) toleration_masks[i] else 0;
        const outcome = placeTaskOnNodes(nodes, reqs[i], tol, strategy);
        out_outcomes[i] = outcome;
        if (outcome.placed) {
            placed_count += 1;
        }
    }
    return placed_count;
}

// ============================================================================
// 4. Minimum-Cost Priority Preemption Eviction Planner
// ============================================================================

/// Metadata of a workload currently running on a cluster node.
pub struct RunningWorkload {
    task_id: u64 = 0;
    tenant_id: u32 = 0;
    node_id: u32 = 0;
    priority: i32 = 0;
    usage: ResourceVec = ResourceVec{};
    preemptible: bool = true;
}

/// Eviction plan computed by `planNodePreemption`.
pub struct PreemptionPlan {
    feasible: bool = false;
    victim_count: usize = 0;
    freed_resources: ResourceVec = ResourceVec{};
    min_victim_priority: i32 = 0;
}

fn taskIdInSlice(ids: []const u64, count: usize, target: u64): bool {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (ids[i] == target) return true;
    }
    return false;
}

fn deficitReductionScore(usage: ResourceVec, avail: ResourceVec, req: ResourceVec): f64 {
    let score: f64 = 0.0;
    let d: usize = 0;
    while (d < req.dims) : (d += 1) {
        const deficit: f64 = req.values[d] - avail.values[d];
        if (deficit > RES_EPS_F64) {
            const contrib: f64 = if (usage.values[d] < deficit) usage.values[d] else deficit;
            score += contrib / deficit;
        }
    }
    return score;
}

/// Computes a minimum-cost eviction plan on `node` to admit `incoming_req` at `incoming_priority`.
/// Only considers running workloads on `node.node_id` with `preemptible == true && priority < incoming_priority`,
/// greedily selecting lowest-priority victims first (tie-broken by highest deficit reduction).
/// Writes selected victim `task_id`s into `out_victim_ids` with zero dynamic heap allocation.
pub fn planNodePreemption(
    node: ClusterNode,
    incoming_req: ResourceVec,
    incoming_priority: i32,
    running_on_node: []const RunningWorkload,
    out_victim_ids: *[]u64,
): PreemptionPlan {
    let plan = PreemptionPlan{
        freed_resources: ResourceVec.zero(incoming_req.dims),
    };

    if (node.cordoned || !incoming_req.fitsWithin(node.capacity)) {
        return plan;
    }

    let avail = node.remaining();
    if (incoming_req.fitsWithin(avail)) {
        plan.feasible = true;
        return plan;
    }

    let picked_count: usize = 0;
    let has_victim: bool = false;
    let min_prio: i32 = 0;

    while (!incoming_req.fitsWithin(avail)) {
        if (picked_count >= out_victim_ids.len) {
            return plan;
        }

        let found_cand: bool = false;
        let best_idx: usize = 0;
        let best_prio: i32 = 0;
        let best_contrib: f64 = 0.0;

        let i: usize = 0;
        while (i < running_on_node.len) : (i += 1) {
            const w = running_on_node[i];
            if (w.node_id != node.node_id) continue;
            if (!w.preemptible || w.priority >= incoming_priority) continue;
            if (taskIdInSlice(out_victim_ids.*, picked_count, w.task_id)) continue;

            const contrib: f64 = deficitReductionScore(w.usage, avail, incoming_req);
            if (contrib <= RES_EPS_F64) continue;

            let better: bool = false;
            if (!found_cand) {
                better = true;
            } else if (w.priority < best_prio) {
                better = true;
            } else if (w.priority == best_prio && contrib > best_contrib + RES_EPS_F64) {
                better = true;
            }

            if (better) {
                found_cand = true;
                best_idx = i;
                best_prio = w.priority;
                best_contrib = contrib;
            }
        }

        if (!found_cand) {
            return plan;
        }

        const victim = running_on_node[best_idx];
        out_victim_ids[picked_count] = victim.task_id;
        picked_count += 1;
        plan.freed_resources = plan.freed_resources.add(victim.usage);
        avail = avail.add(victim.usage);
        if (!has_victim || victim.priority < min_prio) {
            min_prio = victim.priority;
            has_victim = true;
        }
    }

    plan.feasible = true;
    plan.victim_count = picked_count;
    plan.min_victim_priority = min_prio;
    return plan;
}

// ============================================================================
// 5. Event-Driven EDF Real-Time Scheduler & Liu-Layland Schedulability Test
// ============================================================================

/// Real-time task descriptor for Earliest Deadline First (EDF) scheduling simulation.
pub struct EdfTask {
    task_id: u64 = 0;
    release_ms: u64 = 0;
    deadline_ms: u64 = 0;
    exec_ms: u64 = 0;
    remaining_ms: u64 = 0;
    base_priority: i32 = 0;
    effective_priority: i32 = 0;
    completed: bool = false;
}

/// Summary report emitted by `simulateEdfSchedule`.
pub struct EdfScheduleReport {
    total_tasks: usize = 0;
    completed_on_time: usize = 0;
    missed_deadlines: usize = 0;
    total_busy_ms: u64 = 0;
    utilization_bps: u32 = 0;
}

/// Checks the Liu & Layland (1973) exact uniprocessor EDF schedulability condition:
/// `sum_i (exec_ms[i] / period_ms[i]) <= 1.0`.
pub fn checkLiuLaylandSchedulable(
    exec_ms_list: []const u64,
    period_ms_list: []const u64,
): bool {
    if (exec_ms_list.len == 0 || exec_ms_list.len != period_ms_list.len) {
        return false;
    }
    let util: f64 = 0.0;
    let i: usize = 0;
    while (i < exec_ms_list.len) : (i += 1) {
        if (period_ms_list[i] == 0) {
            return false;
        }
        util += @as(f64, @floatFromInt(exec_ms_list[i])) / @as(f64, @floatFromInt(period_ms_list[i]));
    }
    return util <= 1.0 + 1.0e-12;
}

/// Event-driven preemptive Earliest Deadline First (EDF) scheduler simulation over `[0, horizon_ms]`.
/// Ties in absolute `deadline_ms` are broken by higher `effective_priority`, then smaller `task_id`.
/// Handles arbitrary task counts and arbitrarily large `horizon_ms` in O(events * N) time with zero heap allocation.
pub fn simulateEdfSchedule(tasks: *[]EdfTask, horizon_ms: u64): EdfScheduleReport {
    let report = EdfScheduleReport{
        total_tasks: tasks.len,
    };
    if (tasks.len == 0 || horizon_ms == 0) {
        return report;
    }

    // Initialize remaining_ms & effective_priority for clean simulation runs.
    let init_i: usize = 0;
    while (init_i < tasks.len) : (init_i += 1) {
        if (tasks[init_i].remaining_ms == 0 && !tasks[init_i].completed) {
            tasks[init_i].remaining_ms = tasks[init_i].exec_ms;
        }
        if (tasks[init_i].effective_priority == 0 && tasks[init_i].base_priority != 0) {
            tasks[init_i].effective_priority = tasks[init_i].base_priority;
        }
        if (tasks[init_i].exec_ms == 0) {
            tasks[init_i].completed = true;
            if (tasks[init_i].release_ms <= tasks[init_i].deadline_ms) {
                report.completed_on_time += 1;
            } else {
                report.missed_deadlines += 1;
            }
        }
    }

    let now_ms: u64 = 0;
    while (now_ms < horizon_ms) {
        // 1. Pick ready task with earliest absolute deadline.
        let has_ready: bool = false;
        let cur_idx: usize = 0;

        let i: usize = 0;
        while (i < tasks.len) : (i += 1) {
            const tk = tasks[i];
            if (tk.completed || tk.remaining_ms == 0 || tk.release_ms > now_ms) {
                continue;
            }
            if (!has_ready) {
                has_ready = true;
                cur_idx = i;
            } else {
                const best = tasks[cur_idx];
                if (tk.deadline_ms < best.deadline_ms ||
                    (tk.deadline_ms == best.deadline_ms && tk.effective_priority > best.effective_priority) ||
                    (tk.deadline_ms == best.deadline_ms && tk.effective_priority == best.effective_priority && tk.task_id < best.task_id))
                {
                    cur_idx = i;
                }
            }
        }

        // 2. Find earliest future release time > now_ms.
        let next_release: u64 = horizon_ms;
        let r_i: usize = 0;
        while (r_i < tasks.len) : (r_i += 1) {
            const tk = tasks[r_i];
            if (!tk.completed && tk.remaining_ms > 0 && tk.release_ms > now_ms && tk.release_ms < next_release) {
                next_release = tk.release_ms;
            }
        }

        if (!has_ready) {
            if (next_release >= horizon_ms) {
                break;
            }
            now_ms = next_release;
            continue;
        }

        // 3. Advance clock to completion of `cur_idx`, next preempting release, or `horizon_ms`.
        const rem: u64 = tasks[cur_idx].remaining_ms;
        let finish_time: u64 = if (rem <= horizon_ms - now_ms) now_ms + rem else horizon_ms;
        if (next_release < finish_time) {
            finish_time = next_release;
        }
        const dt: u64 = finish_time - now_ms;
        if (dt == 0) {
            break;
        }

        tasks[cur_idx].remaining_ms -= dt;
        report.total_busy_ms += dt;
        now_ms = finish_time;

        if (tasks[cur_idx].remaining_ms == 0) {
            tasks[cur_idx].completed = true;
            if (now_ms <= tasks[cur_idx].deadline_ms) {
                report.completed_on_time += 1;
            } else {
                report.missed_deadlines += 1;
            }
        }
    }

    // Count any remaining unfinished tasks as missed deadlines.
    let f_i: usize = 0;
    while (f_i < tasks.len) : (f_i += 1) {
        if (!tasks[f_i].completed) {
            report.missed_deadlines += 1;
        }
    }

    const util_u64: u64 = (report.total_busy_ms * 10000) / horizon_ms;
    report.utilization_bps = if (util_u64 > 10000) 10000 else @as(u32, @intCast(util_u64));
    return report;
}
