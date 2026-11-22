// Origin: Pure-CP Stdlib (A2A Task Delegation, Lease Renewal, Timeout Escalation & Fencing Token Verifier) | Created: 2026-11-22
// Pure CP zero-heap-allocation multi-agent task delegation engine:
//   - Monotonically increasing fencing tokens (`fence_token: u64`) preventing stale/zombie worker write-backs
//   - Lease-bound active ownership with deadline-capped renewal (`renewLease`)
//   - Automatic lease-timeout failover to `backup_agent_id` with fence rotation (`sweepTimeouts`)
//   - Hard deadline escalation (`Escalated`) and terminal slot reclamation (`releaseTerminal`)

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

fn satAddI64(a: i64, b: i64): i64 {
    const i64_max: i64 = 9223372036854775807;
    if (b > 0 && a > i64_max - b) {
        return i64_max;
    }
    return a + b;
}

/// Lifecycle state of a delegated A2A task.
pub enum DelegationState {
    Pending,
    Active,
    Completed,
    Failed,
    TimedOut,
    Escalated,
    Cancelled,
}

fn isTerminalState(st: DelegationState): bool {
    return st == DelegationState.Completed ||
        st == DelegationState.Failed ||
        st == DelegationState.TimedOut ||
        st == DelegationState.Escalated ||
        st == DelegationState.Cancelled;
}

/// Fixed-layout delegation slot storing ownership, lease window, deadline, and fencing token.
pub struct DelegationRecord {
    task_id: u64 = 0;
    fence_token: u64 = 0;
    owner_agent_id: u32 = 0;
    backup_agent_id: u32 = 0;
    state: DelegationState = DelegationState.Pending;
    created_ms: i64 = 0;
    lease_ms: i64 = 0;
    lease_expires_ms: i64 = 0;
    deadline_ms: i64 = 0;
    updated_ms: i64 = 0;
    attempt: u32 = 0;
    failed_over: bool = false;
    result_summary: string = "";
    occupied: bool = false;
}

/// Aggregate operational statistics across all delegation slots.
pub struct DelegationStats {
    total: usize = 0;
    pending: usize = 0;
    active: usize = 0;
    completed: usize = 0;
    failed: usize = 0;
    timed_out: usize = 0;
    escalated: usize = 0;
    cancelled: usize = 0;
    total_failovers: u64 = 0;
    stale_fence_rejections: u64 = 0;
    last_fence_token: u64 = 0;
}

/// Zero-heap-allocation A2A task delegation, lease renewal, failover, and fencing-token manager.
pub struct A2aDelegationManager {
    slots: []DelegationRecord;
    next_fence_token: u64 = 1;
    total_failovers: u64 = 0;
    stale_fence_rejections: u64 = 0;

    /// Initializes a delegation manager backed by caller-owned `slots`.
    /// Returns `null` if `slots.len == 0`.
    static fn init(slots: *[]DelegationRecord): A2aDelegationManager? {
        if (slots.len == 0) return null;

        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = DelegationRecord{};
        }

        return A2aDelegationManager{
            slots: slots,
            next_fence_token: 1,
            total_failovers: 0,
            stale_fence_rejections: 0,
        };
    }

    fn allocFence(*_): u64 {
        const tok = _.next_fence_token;
        _.next_fence_token = satAddU64(_.next_fence_token, 1);
        return tok;
    }

    fn findSlot(task_id: u64): usize? {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].task_id == task_id) {
                return i;
            }
        }
        return null;
    }

    /// Delegates `task_id` to `owner_agent_id` (with optional `backup_agent_id`, `0` = none).
    /// Returns a unique monotonically increasing `fence_token`, or `null` on invalid arguments,
    /// duplicate `task_id`, or full slot table.
    fn delegate(
        *_,
        task_id: u64,
        owner_agent_id: u32,
        backup_agent_id: u32,
        lease_ms: i64,
        deadline_ms: i64,
        now_ms: i64,
    ): u64? {
        if (task_id == 0 || owner_agent_id == 0) return null;
        if (lease_ms <= 0 || deadline_ms <= now_ms) return null;
        if (_.findSlot(task_id) != null) return null;

        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) {
                free_idx = i;
                break;
            }
        }
        const idx = free_idx ?? return null;

        const tok = _.allocFence();
        const raw_exp = satAddI64(now_ms, lease_ms);
        const exp = if (raw_exp > deadline_ms) deadline_ms else raw_exp;

        _.slots[idx] = DelegationRecord{
            task_id: task_id,
            fence_token: tok,
            owner_agent_id: owner_agent_id,
            backup_agent_id: backup_agent_id,
            state: DelegationState.Active,
            created_ms: now_ms,
            lease_ms: lease_ms,
            lease_expires_ms: exp,
            deadline_ms: deadline_ms,
            updated_ms: now_ms,
            attempt: 1,
            failed_over: false,
            result_summary: "",
            occupied: true,
        };
        return tok;
    }

    /// Renews the active lease for `task_id` by `extend_ms` from `now_ms`, capped at `deadline_ms`.
    /// Requires exact `fence_token` match and active unexpired lease before `deadline_ms`.
    fn renewLease(
        *_,
        task_id: u64,
        fence_token: u64,
        extend_ms: i64,
        now_ms: i64,
    ): bool {
        if (extend_ms <= 0 || fence_token == 0) return false;
        const idx = _.findSlot(task_id) ?? return false;

        if (_.slots[idx].state != DelegationState.Active) return false;
        if (_.slots[idx].fence_token != fence_token) {
            _.stale_fence_rejections = satAddU64(_.stale_fence_rejections, 1);
            return false;
        }
        if (now_ms >= _.slots[idx].deadline_ms || now_ms >= _.slots[idx].lease_expires_ms) {
            return false;
        }

        const raw_exp = satAddI64(now_ms, extend_ms);
        const capped_exp = if (raw_exp > _.slots[idx].deadline_ms)
            _.slots[idx].deadline_ms
        else
            raw_exp;
        if (capped_exp > _.slots[idx].lease_expires_ms) {
            _.slots[idx].lease_expires_ms = capped_exp;
        }
        if (now_ms > _.slots[idx].updated_ms) {
            _.slots[idx].updated_ms = now_ms;
        }
        return true;
    }

    /// Completes `task_id` with `result_summary`.
    /// Strictly verifies `fence_token` and active unexpired lease to reject stale/zombie worker write-backs.
    fn complete(
        *_,
        task_id: u64,
        fence_token: u64,
        result_summary: string,
        now_ms: i64,
    ): bool {
        const idx = _.findSlot(task_id) ?? return false;

        if (_.slots[idx].state != DelegationState.Active) {
            if (fence_token != 0 && _.slots[idx].fence_token != fence_token) {
                _.stale_fence_rejections = satAddU64(_.stale_fence_rejections, 1);
            }
            return false;
        }
        if (fence_token == 0 || _.slots[idx].fence_token != fence_token) {
            _.stale_fence_rejections = satAddU64(_.stale_fence_rejections, 1);
            return false;
        }
        if (now_ms >= _.slots[idx].deadline_ms || now_ms >= _.slots[idx].lease_expires_ms) {
            return false;
        }

        _.slots[idx].state = DelegationState.Completed;
        _.slots[idx].result_summary = result_summary;
        _.slots[idx].updated_ms = now_ms;
        return true;
    }

    /// Marks `task_id` as `Failed` if `fence_token` matches the current active owner.
    fn fail(
        *_,
        task_id: u64,
        fence_token: u64,
        reason: string,
        now_ms: i64,
    ): bool {
        const idx = _.findSlot(task_id) ?? return false;
        if (_.slots[idx].state != DelegationState.Active) return false;
        if (fence_token == 0 || _.slots[idx].fence_token != fence_token) {
            _.stale_fence_rejections = satAddU64(_.stale_fence_rejections, 1);
            return false;
        }

        _.slots[idx].state = DelegationState.Failed;
        _.slots[idx].result_summary = reason;
        _.slots[idx].updated_ms = now_ms;
        return true;
    }

    /// Cancels an active or pending delegation and rotates its `fence_token` to block late write-backs.
    fn cancel(*_, task_id: u64, now_ms: i64): bool {
        const idx = _.findSlot(task_id) ?? return false;
        if (isTerminalState(_.slots[idx].state)) return false;

        _.slots[idx].state = DelegationState.Cancelled;
        _.slots[idx].fence_token = _.allocFence();
        _.slots[idx].updated_ms = now_ms;
        return true;
    }

    /// Sweeps all active delegations at `now_ms`:
    ///   - Past `deadline_ms`: transitions to `Escalated` and rotates `fence_token`.
    ///   - Past `lease_expires_ms` with an unused `backup_agent_id`: fails over ownership to
    ///     `backup_agent_id`, increments `attempt`, rotates `fence_token`, and grants a fresh lease.
    ///   - Past `lease_expires_ms` without available backup: transitions to `TimedOut` and rotates `fence_token`.
    /// Returns the number of delegations transitioned or failed over.
    fn sweepTimeouts(*_, now_ms: i64): usize {
        let changed: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied || _.slots[i].state != DelegationState.Active) {
                continue;
            }

            if (now_ms >= _.slots[i].deadline_ms) {
                _.slots[i].state = DelegationState.Escalated;
                _.slots[i].fence_token = _.allocFence();
                _.slots[i].updated_ms = now_ms;
                changed += 1;
                continue;
            }

            if (now_ms >= _.slots[i].lease_expires_ms) {
                const has_backup = _.slots[i].backup_agent_id != 0 &&
                    !_.slots[i].failed_over &&
                    _.slots[i].backup_agent_id != _.slots[i].owner_agent_id;

                if (has_backup) {
                    _.slots[i].owner_agent_id = _.slots[i].backup_agent_id;
                    _.slots[i].failed_over = true;
                    _.slots[i].attempt = satAddU32(_.slots[i].attempt, 1);
                    _.slots[i].fence_token = _.allocFence();
                    const raw_exp = satAddI64(now_ms, _.slots[i].lease_ms);
                    _.slots[i].lease_expires_ms = if (raw_exp > _.slots[i].deadline_ms)
                        _.slots[i].deadline_ms
                    else
                        raw_exp;
                    _.slots[i].updated_ms = now_ms;
                    _.total_failovers = satAddU64(_.total_failovers, 1);
                } else {
                    _.slots[i].state = DelegationState.TimedOut;
                    _.slots[i].fence_token = _.allocFence();
                    _.slots[i].updated_ms = now_ms;
                }
                changed += 1;
            }
        }
        return changed;
    }

    /// Releases a slot whose delegation is in a terminal state so it can be reused without heap allocation.
    fn releaseTerminal(*_, task_id: u64): bool {
        const idx = _.findSlot(task_id) ?? return false;
        if (!isTerminalState(_.slots[idx].state)) return false;
        _.slots[idx] = DelegationRecord{};
        return true;
    }

    /// Returns a read-only copy of the delegation record for `task_id`, or `null` if not found.
    fn get(task_id: u64): DelegationRecord? {
        const idx = _.findSlot(task_id) ?? return null;
        return _.slots[idx];
    }

    /// Returns `true` iff `task_id` is currently `Active` and matches `fence_token`.
    fn verifyFence(task_id: u64, fence_token: u64): bool {
        if (fence_token == 0) return false;
        const idx = _.findSlot(task_id) ?? return false;
        return _.slots[idx].state == DelegationState.Active &&
            _.slots[idx].fence_token == fence_token;
    }

    /// Returns the number of currently active delegations.
    fn activeCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == DelegationState.Active) {
                n += 1;
            }
        }
        return n;
    }

    /// Computes live aggregate statistics across all occupied slots.
    fn stats(): DelegationStats {
        let st = DelegationStats{
            total_failovers: _.total_failovers,
            stale_fence_rejections: _.stale_fence_rejections,
            last_fence_token: if (_.next_fence_token > 1) _.next_fence_token - 1 else 0,
        };
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            st.total += 1;
            match (_.slots[i].state) {
                DelegationState.Pending => st.pending += 1,
                DelegationState.Active => st.active += 1,
                DelegationState.Completed => st.completed += 1,
                DelegationState.Failed => st.failed += 1,
                DelegationState.TimedOut => st.timed_out += 1,
                DelegationState.Escalated => st.escalated += 1,
                DelegationState.Cancelled => st.cancelled += 1,
            }
        }
        return st;
    }
}
