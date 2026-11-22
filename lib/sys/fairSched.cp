// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Tenant Deficit Round Robin Fair Task Scheduler) | Created: 2026-11-23
// Pure CP zero-heap-allocation Deficit Round Robin (DRR) weighted fair task scheduler:
//   - Fixed 32-tenant flow table, each with configurable weight (`quantum`) and inline 32-slot FIFO ring
//     storing `(tenant_id, task_id, cost, enqueued_ms)`
//   - Standard O(1) active-list Deficit Round Robin (`dequeue`): credits `deficit += quantum` once per
//     round, dispatches head task whenever `cost <= deficit`, zeroes `deficit` when a flow drains, and
//     carries residual deficit across rounds so large & small tasks share throughput strictly by weight
//   - Queue-full backpressure (`enqueue`), dynamic `setQuantum`, safe `removeTenant`, per-tenant & global
//     telemetry snapshots, and weighted Jain's Fairness Index (`jainFairnessBps`, 0..10000 bps)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of independent tenant flows supported inline.
pub const FAIR_SCHED_MAX_TENANTS: usize = 32;

/// Fixed FIFO task queue capacity per tenant flow.
pub const FAIR_SCHED_QUEUE_CAP: usize = 32;

/// Basis-point scale (10000 bps = 1.0000 perfect fairness) for Jain's Fairness Index.
pub const FAIR_SCHED_BPS_SCALE: u32 = 10_000;

/// Operation status returned by tenant/queue mutation APIs.
pub enum FairSchedOpStatus {
    ok,
    invalid_input,
    tenant_exists,
    tenant_not_found,
    tenant_table_full,
    queue_full,
}

/// Scheduled work item carrying tenant identity, cost (bytes/tokens/compute units) & enqueue time.
pub struct FairTask {
    tenant_id: u32 = 0;
    task_id: u64 = 0;
    cost: u64 = 0;
    enqueued_ms: u64 = 0;
}

/// Per-tenant telemetry snapshot.
pub struct FairTenantStats {
    tenant_id: u32 = 0;
    quantum: u64 = 0;
    deficit: u64 = 0;
    queued_count: usize = 0;
    queue_capacity: usize = FAIR_SCHED_QUEUE_CAP;
    enqueued_total: u64 = 0;
    dequeued_total: u64 = 0;
    dropped_total: u64 = 0;
    scheduled_cost_total: u64 = 0;
    in_active_ring: bool = false;
}

/// Global scheduler telemetry snapshot including weighted Jain's Fairness Index.
pub struct FairSchedStats {
    registered_tenants: usize = 0;
    tenant_capacity: usize = FAIR_SCHED_MAX_TENANTS;
    active_tenants: usize = 0;
    pending_tasks: usize = 0;
    total_enqueued: u64 = 0;
    total_dequeued: u64 = 0;
    total_dropped: u64 = 0;
    total_scheduled_cost: u64 = 0;
    jain_fairness_bps: u32 = FAIR_SCHED_BPS_SCALE;
}

/// Internal inline per-tenant flow state with 32-slot circular FIFO.
struct FairTenantFlow {
    active: bool = false;
    tenant_id: u32 = 0;
    quantum: u64 = 0;
    deficit: u64 = 0;
    in_active_ring: bool = false;

    queue: [FAIR_SCHED_QUEUE_CAP]FairTask = [FairTask{}; FAIR_SCHED_QUEUE_CAP];
    q_head: usize = 0;
    q_count: usize = 0;

    enqueued_total: u64 = 0;
    dequeued_total: u64 = 0;
    dropped_total: u64 = 0;
    scheduled_cost_total: u64 = 0;
}

// ============================================================================
// 2. DeficitRoundRobinScheduler — Zero-Heap O(1) Weighted Fair Scheduler
// ============================================================================

/// Zero-heap-allocation multi-tenant Deficit Round Robin (DRR) weighted fair task scheduler.
pub struct DeficitRoundRobinScheduler {
    flows: [FAIR_SCHED_MAX_TENANTS]FairTenantFlow = [FairTenantFlow{}; FAIR_SCHED_MAX_TENANTS];
    tenant_count: usize = 0;

    // Circular active-flow list holding flow slot indices with non-empty queues.
    active_ring: [FAIR_SCHED_MAX_TENANTS]usize = [0; FAIR_SCHED_MAX_TENANTS];
    active_head: usize = 0;
    active_count: usize = 0;
    current_round_credited: bool = false;

    pending_tasks: usize = 0;
    total_enqueued: u64 = 0;
    total_dequeued: u64 = 0;
    total_dropped: u64 = 0;
    total_scheduled_cost: u64 = 0;

    /// Initialize an empty DRR fair scheduler.
    static fn init(): DeficitRoundRobinScheduler {
        return DeficitRoundRobinScheduler{};
    }

    /// Clear all tenants, queued tasks, and counters.
    pub fn reset(*_) {
        let i: usize = 0;
        while (i < FAIR_SCHED_MAX_TENANTS) : (i += 1) {
            flows[i] = FairTenantFlow{};
            active_ring[i] = 0;
        }
        tenant_count = 0;
        active_head = 0;
        active_count = 0;
        current_round_credited = false;
        pending_tasks = 0;
        total_enqueued = 0;
        total_dequeued = 0;
        total_dropped = 0;
        total_scheduled_cost = 0;
    }

    fn findTenantSlot(tenant_id: u32): usize? {
        if (tenant_id == 0) return null;
        let i: usize = 0;
        while (i < FAIR_SCHED_MAX_TENANTS) : (i += 1) {
            if (flows[i].active and flows[i].tenant_id == tenant_id) {
                return i;
            }
        }
        return null;
    }

    fn pushActiveSlot(*_, slot: usize) {
        if (flows[slot].in_active_ring or active_count >= FAIR_SCHED_MAX_TENANTS) return;
        const tail = (active_head + active_count) % FAIR_SCHED_MAX_TENANTS;
        active_ring[tail] = slot;
        active_count += 1;
        flows[slot].in_active_ring = true;
    }

    fn popActiveHead(*_): usize {
        const slot = active_ring[active_head];
        active_head = (active_head + 1) % FAIR_SCHED_MAX_TENANTS;
        active_count -= 1;
        flows[slot].in_active_ring = false;
        current_round_credited = false;
        return slot;
    }

    fn rotateActiveHeadToTail(*_) {
        if (active_count <= 1) {
            current_round_credited = false;
            return;
        }
        const slot = active_ring[active_head];
        active_head = (active_head + 1) % FAIR_SCHED_MAX_TENANTS;
        const tail = (active_head + active_count - 1) % FAIR_SCHED_MAX_TENANTS;
        active_ring[tail] = slot;
        current_round_credited = false;
    }

    /// Register a new tenant flow with weight `quantum` (`tenant_id > 0`, `quantum > 0`).
    pub fn registerTenant(*_, tenant_id: u32, quantum: u64): FairSchedOpStatus {
        if (tenant_id == 0 or quantum == 0) return FairSchedOpStatus.invalid_input;

        let free_slot: usize = FAIR_SCHED_MAX_TENANTS;
        let i: usize = 0;
        while (i < FAIR_SCHED_MAX_TENANTS) : (i += 1) {
            if (flows[i].active) {
                if (flows[i].tenant_id == tenant_id) {
                    return FairSchedOpStatus.tenant_exists;
                }
            } else if (free_slot == FAIR_SCHED_MAX_TENANTS) {
                free_slot = i;
            }
        }
        if (free_slot == FAIR_SCHED_MAX_TENANTS) return FairSchedOpStatus.tenant_table_full;

        let f = FairTenantFlow{};
        f.active = true;
        f.tenant_id = tenant_id;
        f.quantum = quantum;
        flows[free_slot] = f;
        tenant_count += 1;
        return FairSchedOpStatus.ok;
    }

    /// Dynamically update a tenant's DRR quantum weight (`new_quantum > 0`).
    pub fn setQuantum(*_, tenant_id: u32, new_quantum: u64): FairSchedOpStatus {
        if (new_quantum == 0) return FairSchedOpStatus.invalid_input;
        const slot = findTenantSlot(tenant_id) ?? return FairSchedOpStatus.tenant_not_found;
        flows[slot].quantum = new_quantum;
        return FairSchedOpStatus.ok;
    }

    /// Remove a tenant flow and cleanly excise it from the active round-robin ring.
    pub fn removeTenant(*_, tenant_id: u32): FairSchedOpStatus {
        const slot = findTenantSlot(tenant_id) ?? return FairSchedOpStatus.tenant_not_found;

        if (flows[slot].q_count > 0) {
            if (pending_tasks >= flows[slot].q_count) {
                pending_tasks -= flows[slot].q_count;
            } else {
                pending_tasks = 0;
            }
        }

        if (flows[slot].in_active_ring and active_count > 0) {
            // Compact active_ring while preserving exact relative order of all other active flows.
            let new_ring: [FAIR_SCHED_MAX_TENANTS]usize = [0; FAIR_SCHED_MAX_TENANTS];
            let new_cnt: usize = 0;
            let removed_was_head = false;
            let i: usize = 0;
            while (i < active_count) : (i += 1) {
                const s = active_ring[(active_head + i) % FAIR_SCHED_MAX_TENANTS];
                if (s == slot) {
                    if (i == 0) removed_was_head = true;
                } else {
                    new_ring[new_cnt] = s;
                    new_cnt += 1;
                }
            }
            let j: usize = 0;
            while (j < new_cnt) : (j += 1) {
                active_ring[j] = new_ring[j];
            }
            active_head = 0;
            active_count = new_cnt;
            if (removed_was_head) {
                current_round_credited = false;
            }
        }

        flows[slot] = FairTenantFlow{};
        if (tenant_count > 0) tenant_count -= 1;
        return FairSchedOpStatus.ok;
    }

    /// Enqueue a task for `tenant_id` with positive `cost`; applies backpressure (`queue_full`) when full.
    pub fn enqueue(
        *_,
        tenant_id: u32,
        task_id: u64,
        cost: u64,
        now_ms: u64,
    ): FairSchedOpStatus {
        if (cost == 0) return FairSchedOpStatus.invalid_input;
        const slot = findTenantSlot(tenant_id) ?? return FairSchedOpStatus.tenant_not_found;

        if (flows[slot].q_count >= FAIR_SCHED_QUEUE_CAP) {
            flows[slot].dropped_total += 1;
            total_dropped += 1;
            return FairSchedOpStatus.queue_full;
        }

        const tail = (flows[slot].q_head + flows[slot].q_count) % FAIR_SCHED_QUEUE_CAP;
        flows[slot].queue[tail] = FairTask{
            tenant_id: tenant_id,
            task_id: task_id,
            cost: cost,
            enqueued_ms: now_ms,
        };
        flows[slot].q_count += 1;
        flows[slot].enqueued_total += 1;
        pending_tasks += 1;
        total_enqueued += 1;

        if (!flows[slot].in_active_ring) {
            pushActiveSlot(slot);
        }
        return FairSchedOpStatus.ok;
    }

    /// Standard O(1) Deficit Round Robin dequeue.
    /// Credits `deficit += quantum` once per round for the head active tenant, dispatches head task
    /// whenever `cost <= deficit`, zeroes `deficit` when a tenant drains, and preserves residual
    /// deficit across rounds for strict weighted fairness with zero starvation.
    pub fn dequeue(*_): FairTask? {
        if (active_count == 0) return null;

        while (active_count > 0) {
            const slot = active_ring[active_head];
            if (flows[slot].q_count == 0) {
                flows[slot].deficit = 0;
                * = popActiveHead();
                continue;
            }

            if (!current_round_credited) {
                flows[slot].deficit += flows[slot].quantum;
                current_round_credited = true;
            }

            const head_task = flows[slot].queue[flows[slot].q_head];
            if (head_task.cost <= flows[slot].deficit) {
                // Pop task from tenant FIFO
                flows[slot].q_head = (flows[slot].q_head + 1) % FAIR_SCHED_QUEUE_CAP;
                flows[slot].q_count -= 1;
                flows[slot].deficit -= head_task.cost;
                flows[slot].dequeued_total += 1;
                flows[slot].scheduled_cost_total += head_task.cost;

                if (pending_tasks > 0) pending_tasks -= 1;
                total_dequeued += 1;
                total_scheduled_cost += head_task.cost;

                if (flows[slot].q_count == 0) {
                    // Drained queue resets deficit to 0 and leaves active ring.
                    flows[slot].deficit = 0;
                    * = popActiveHead();
                } else {
                    // If remaining deficit cannot cover next head task, end this tenant's round now.
                    const next_head = flows[slot].queue[flows[slot].q_head];
                    if (next_head.cost > flows[slot].deficit) {
                        rotateActiveHeadToTail();
                    }
                }
                return head_task;
            }

            // Head task cost exceeds current deficit: fast-forward if this is the sole active tenant,
            // otherwise carry deficit forward and rotate to the next active tenant.
            if (active_count == 1) {
                const gap = head_task.cost - flows[slot].deficit;
                const q = flows[slot].quantum;
                const extra_rounds = (gap + q - 1) / q;
                flows[slot].deficit += extra_rounds * q;
            } else {
                rotateActiveHeadToTail();
            }
        }
        return null;
    }

    /// Return a read-only telemetry snapshot for a single tenant, or `null` if not registered.
    pub fn tenantStats(tenant_id: u32): FairTenantStats? {
        const slot = findTenantSlot(tenant_id) ?? return null;
        return FairTenantStats{
            tenant_id: flows[slot].tenant_id,
            quantum: flows[slot].quantum,
            deficit: flows[slot].deficit,
            queued_count: flows[slot].q_count,
            queue_capacity: FAIR_SCHED_QUEUE_CAP,
            enqueued_total: flows[slot].enqueued_total,
            dequeued_total: flows[slot].dequeued_total,
            dropped_total: flows[slot].dropped_total,
            scheduled_cost_total: flows[slot].scheduled_cost_total,
            in_active_ring: flows[slot].in_active_ring,
        };
    }

    /// Compute weighted Jain's Fairness Index across all tenants that have received service,
    /// normalized by each tenant's `quantum` (`x_i = scheduled_cost_i / quantum_i`).
    /// Returns basis points in `[0, 10000]` where `10000` means perfect weighted fairness.
    pub fn jainFairnessBps(): u32 {
        let sum_x: f64 = 0.0;
        let sum_x2: f64 = 0.0;
        let n: u64 = 0;

        let i: usize = 0;
        while (i < FAIR_SCHED_MAX_TENANTS) : (i += 1) {
            if (!flows[i].active) continue;
            if (flows[i].scheduled_cost_total == 0 or flows[i].quantum == 0) continue;
            const x = @as(f64, @floatFromInt(flows[i].scheduled_cost_total)) /
                @as(f64, @floatFromInt(flows[i].quantum));
            sum_x += x;
            sum_x2 += x * x;
            n += 1;
        }

        if (n <= 1 or sum_x2 <= 0.0) return FAIR_SCHED_BPS_SCALE;
        const denom = @as(f64, @floatFromInt(n)) * sum_x2;
        let j = (sum_x * sum_x) / denom;
        if (j > 1.0) j = 1.0;
        if (j < 0.0) j = 0.0;
        return @as(u32, @intFromFloat(j * @as(f64, @floatFromInt(FAIR_SCHED_BPS_SCALE)) + 0.5));
    }

    /// Return global scheduler telemetry snapshot.
    pub fn stats(): FairSchedStats {
        return FairSchedStats{
            registered_tenants: tenant_count,
            tenant_capacity: FAIR_SCHED_MAX_TENANTS,
            active_tenants: active_count,
            pending_tasks: pending_tasks,
            total_enqueued: total_enqueued,
            total_dequeued: total_dequeued,
            total_dropped: total_dropped,
            total_scheduled_cost: total_scheduled_cost,
            jain_fairness_bps: jainFairnessBps(),
        };
    }
}
