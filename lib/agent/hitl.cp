// Origin: Pure-CP Stdlib (AI Agent Human-in-the-Loop Approval Queue, Multi-Sign Quorum & SLA) | Created: 2026-11-22
// Pure CP zero-allocation Human-in-the-Loop (HITL) approval queue for AI Agent governance:
// - Risk-tiered auto-approval (`Low`/`Medium`/`High`/`Critical`) vs pending human review
// - Multi-signature quorum enforcement for `Critical` operations (`critical_quorum`, default 2)
// - Deterministic SLA deadline tracking, lazy expiration on approve/reject/cancel, and batch sweep
// - Irreversible terminal states (`Approved`, `Rejected`, `Expired`, `Cancelled`) & audit timestamps

/// Lifecycle state of an approval request inside `HitlQueue`.
pub enum ApprovalState {
    Pending,
    Approved,
    Rejected,
    Expired,
    Cancelled,
}

/// Risk classification for an agent tool or side-effecting action.
pub enum ActionRisk {
    Low,
    Medium,
    High,
    Critical,
}

/// Audit record and state descriptor for a single HITL approval request.
pub struct ApprovalRequest {
    id: u64 = 0,
    agent_id: u32 = 0,
    action_name: string = "",
    args_hash: u64 = 0,
    risk: ActionRisk = ActionRisk.Low,
    required_approvals: u8 = 1,
    approvals_received: u8 = 0,
    created_ms: i64 = 0,
    deadline_ms: i64 = 0,
    resolved_ms: i64 = 0,
    state: ApprovalState = ApprovalState.Pending,
}

/// Governance policy controlling auto-approval thresholds, SLA windows, and critical quorum.
pub struct HitlPolicy {
    auto_approve_max_risk: ActionRisk = ActionRisk.Low,
    default_sla_ms: i64 = 300000,
    critical_quorum: u8 = 2,

    pub static fn default(): HitlPolicy {
        return HitlPolicy{
            auto_approve_max_risk: ActionRisk.Low,
            default_sla_ms: 300000,
            critical_quorum: 2,
        };
    }
}

fn riskLevel(r: ActionRisk): u8 {
    if (r == ActionRisk.Low) return 0;
    if (r == ActionRisk.Medium) return 1;
    if (r == ActionRisk.High) return 2;
    return 3;
}

fn saturatingAddI64(a: i64, b: i64): i64 {
    const i64_max: i64 = 9223372036854775807;
    if (b > 0 && a > i64_max - b) {
        return i64_max;
    }
    return a + b;
}

/// Zero-allocation fixed-pool Human-in-the-Loop approval queue.
pub struct HitlQueue {
    requests: *[]ApprovalRequest,
    len: usize = 0,
    policy: HitlPolicy = HitlPolicy.default(),

    /// Initializes a HITL approval queue backed by caller-provided `requests` storage.
    pub static fn init(requests: *[]ApprovalRequest, policy: HitlPolicy): HitlQueue {
        const eff_sla: i64 = if (policy.default_sla_ms < 0) 0 else policy.default_sla_ms;
        const eff_quorum: u8 = if (policy.critical_quorum == 0) 1 else policy.critical_quorum;
        return HitlQueue{
            requests: requests,
            len: 0,
            policy: HitlPolicy{
                auto_approve_max_risk: policy.auto_approve_max_risk,
                default_sla_ms: eff_sla,
                critical_quorum: eff_quorum,
            },
        };
    }

    fn findIndex(id: u64): usize? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.requests[i].id == id) {
                return i;
            }
        }
        return null;
    }

    /// Submits a new agent action for governance evaluation.
    /// Actions with `risk <= policy.auto_approve_max_risk` are immediately recorded as `Approved`.
    /// Higher-risk actions enter `Pending` (`Critical` requires `policy.critical_quorum` approvals).
    /// Returns `null` on empty `action_name`, duplicate `id`, or full request pool.
    pub fn submit(
        *_,
        id: u64,
        agent_id: u32,
        action_name: string,
        args_hash: u64,
        risk: ActionRisk,
        now_ms: i64,
    ): ApprovalState? {
        if (action_name.len == 0) {
            return null;
        }
        if (_.len >= _.requests.len) {
            return null;
        }
        if (_.findIndex(id) != null) {
            return null;
        }

        const auto_ok = riskLevel(risk) <= riskLevel(_.policy.auto_approve_max_risk);
        if (auto_ok) {
            _.requests[_.len] = ApprovalRequest{
                id: id,
                agent_id: agent_id,
                action_name: action_name,
                args_hash: args_hash,
                risk: risk,
                required_approvals: 1,
                approvals_received: 1,
                created_ms: now_ms,
                deadline_ms: now_ms,
                resolved_ms: now_ms,
                state: ApprovalState.Approved,
            };
            _.len += 1;
            return ApprovalState.Approved;
        }

        const req_quorum: u8 = if (risk == ActionRisk.Critical) _.policy.critical_quorum else 1;
        const deadline = saturatingAddI64(now_ms, _.policy.default_sla_ms);
        _.requests[_.len] = ApprovalRequest{
            id: id,
            agent_id: agent_id,
            action_name: action_name,
            args_hash: args_hash,
            risk: risk,
            required_approvals: req_quorum,
            approvals_received: 0,
            created_ms: now_ms,
            deadline_ms: deadline,
            resolved_ms: 0,
            state: ApprovalState.Pending,
        };
        _.len += 1;
        return ApprovalState.Pending;
    }

    /// Records a human approval signature for `id` at `now_ms`.
    /// Checks SLA expiration first (`now_ms >= deadline_ms` transitions to `Expired`).
    /// Terminal states are strictly irreversible and returned unchanged.
    /// Returns `null` if `id` is unknown.
    pub fn approve(*_, id: u64, now_ms: i64): ApprovalState? {
        if (idx := _.findIndex(id)) {
            const cur_state = _.requests[idx].state;
            if (cur_state != ApprovalState.Pending) {
                return cur_state;
            }
            if (now_ms >= _.requests[idx].deadline_ms) {
                _.requests[idx].state = ApprovalState.Expired;
                _.requests[idx].resolved_ms = now_ms;
                return ApprovalState.Expired;
            }
            _.requests[idx].approvals_received += 1;
            if (_.requests[idx].approvals_received >= _.requests[idx].required_approvals) {
                _.requests[idx].state = ApprovalState.Approved;
                _.requests[idx].resolved_ms = now_ms;
            }
            return _.requests[idx].state;
        }
        return null;
    }

    /// Rejects a `Pending` request before its SLA deadline.
    /// Returns `false` if `id` is unknown, already terminal, or already expired (marking it `Expired`).
    pub fn reject(*_, id: u64, now_ms: i64): bool {
        if (idx := _.findIndex(id)) {
            if (_.requests[idx].state != ApprovalState.Pending) {
                return false;
            }
            if (now_ms >= _.requests[idx].deadline_ms) {
                _.requests[idx].state = ApprovalState.Expired;
                _.requests[idx].resolved_ms = now_ms;
                return false;
            }
            _.requests[idx].state = ApprovalState.Rejected;
            _.requests[idx].resolved_ms = now_ms;
            return true;
        }
        return false;
    }

    /// Cancels a `Pending` request before its SLA deadline.
    /// Returns `false` if `id` is unknown, already terminal, or already expired (marking it `Expired`).
    pub fn cancel(*_, id: u64, now_ms: i64): bool {
        if (idx := _.findIndex(id)) {
            if (_.requests[idx].state != ApprovalState.Pending) {
                return false;
            }
            if (now_ms >= _.requests[idx].deadline_ms) {
                _.requests[idx].state = ApprovalState.Expired;
                _.requests[idx].resolved_ms = now_ms;
                return false;
            }
            _.requests[idx].state = ApprovalState.Cancelled;
            _.requests[idx].resolved_ms = now_ms;
            return true;
        }
        return false;
    }

    /// Scans all `Pending` requests and transitions any with `now_ms >= deadline_ms` to `Expired`.
    /// Returns the number of requests newly marked as `Expired`.
    pub fn sweepExpired(*_, now_ms: i64): usize {
        let swept: usize = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.requests[i].state == ApprovalState.Pending && now_ms >= _.requests[i].deadline_ms) {
                _.requests[i].state = ApprovalState.Expired;
                _.requests[i].resolved_ms = now_ms;
                swept += 1;
            }
        }
        return swept;
    }

    /// Counts currently active `Pending` requests whose SLA deadline has not yet passed at `now_ms`.
    pub fn pendingCount(now_ms: i64): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.requests[i].state == ApprovalState.Pending && now_ms < _.requests[i].deadline_ms) {
                count += 1;
            }
        }
        return count;
    }

    /// Returns a copy of the audit record for `id`, or `null` if not found.
    pub fn findRequest(id: u64): ApprovalRequest? {
        if (idx := _.findIndex(id)) {
            return _.requests[idx];
        }
        return null;
    }
}
