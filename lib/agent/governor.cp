// Origin: Pure-CP Stdlib (AI Agent Tool Execution Governor: Concurrency, Sliding-Window Rate/Cost, Dedupe & Circuit Breaker) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation multi-dimensional AI Agent tool call governance engine:
//   - Per-tool policy registry (max_concurrency, sliding-window rate & micro-USD cost limits, dedupe window, failure breaker)
//   - Strict ordered admission pipeline (`acquireToolCall`): registration -> breaker -> dedupe -> concurrency -> rate/cost
//   - Lease-tracked completion feedback (`releaseToolCall`): concurrency release, actual-cost reconciliation & closed/open/half_open transitions
//   - Full boundary guards: invalid policies, double-release prevention, monotonic clock-rollback protection & fixed-pool safety

// ============================================================================
// Constants
// ============================================================================

/// Maximum number of distinct tools governed simultaneously.
pub const TOOL_GOV_MAX_TOOLS: usize = 32;

/// Maximum sliding-window event history slots per tool.
pub const TOOL_GOV_MAX_WINDOW_EVENTS: usize = 64;

/// Fixed ring capacity for short-window duplicate argument suppression.
pub const TOOL_GOV_MAX_DEDUPE_ENTRIES: usize = 64;

/// Maximum concurrent in-flight tool call leases tracked globally.
pub const TOOL_GOV_MAX_ACTIVE_LEASES: usize = 64;

/// Sentinel lease ID representing an invalid / unissued lease handle.
pub const TOOL_GOV_INVALID_LEASE: u64 = 0;

// ============================================================================
// 1. Enums, Policies & Telemetry Structs
// ============================================================================

/// Per-tool failure circuit breaker state.
pub enum ToolBreakerState: u8 {
    closed = 0,
    open = 1,
    half_open = 2,
}

/// Admission verdict returned by `AgentToolGovernor.acquireToolCall(...)`.
pub enum ToolGovDecisionKind: u8 {
    allowed = 0,
    reject_unknown_tool = 1,
    reject_circuit_open = 2,
    reject_duplicate_call = 3,
    reject_concurrency_limit = 4,
    reject_rate_limit = 5,
    reject_cost_limit = 6,
    reject_lease_table_full = 7,
}

/// Outcome returned by `AgentToolGovernor.releaseToolCall(...)`.
pub enum ToolReleaseStatus: u8 {
    ok = 0,
    invalid_lease = 1,
    already_released = 2,
}

/// Multi-dimensional governance policy for a single tool.
pub struct ToolGovPolicy {
    tool_id: u32 = 0;
    /// Maximum concurrent in-flight calls allowed (`> 0`).
    max_concurrency: u32 = 4;
    /// Maximum calls allowed per sliding window (`1..=TOOL_GOV_MAX_WINDOW_EVENTS`).
    max_calls_per_window: u32 = 32;
    /// Sliding window duration in milliseconds (`> 0`).
    window_ms: u64 = 60_000;
    /// Short-window duplicate argument suppression duration (`0` disables dedupe).
    dedupe_window_ms: u64 = 5_000;
    /// Consecutive failures required to trip the circuit breaker (`0` disables breaker).
    failure_trip_threshold: u32 = 3;
    /// Cooldown duration in milliseconds before `open -> half_open` probe (`> 0` when breaker enabled).
    cooldown_ms: u64 = 30_000;
    /// Maximum cost in micro-USD per sliding window (`0` disables cost cap).
    max_cost_micro_usd_per_window: u64 = 0;

    /// Returns production defaults for `tool_id`.
    static fn defaults(tool_id: u32): ToolGovPolicy {
        return ToolGovPolicy{
            tool_id: tool_id,
        };
    }

    /// Validates all policy parameters.
    fn isValid(): bool {
        if (_.max_concurrency == 0) return false;
        if (_.max_calls_per_window == 0 || (_.max_calls_per_window as usize) > TOOL_GOV_MAX_WINDOW_EVENTS) {
            return false;
        }
        if (_.window_ms == 0) return false;
        if (_.failure_trip_threshold > 0 && _.cooldown_ms == 0) return false;
        return true;
    }
}

/// Opaque lease handle issued on an allowed `acquireToolCall` and returned to `releaseToolCall`.
pub struct ToolLeaseHandle {
    lease_id: u64 = TOOL_GOV_INVALID_LEASE;
    tool_id: u32 = 0;
    slot_idx: u16 = 0;
    is_probe: bool = false;

    fn isValid(): bool {
        return _.lease_id != TOOL_GOV_INVALID_LEASE;
    }
}

/// Full admission decision returned by `acquireToolCall(...)`.
pub struct ToolAcquireDecision {
    kind: ToolGovDecisionKind = ToolGovDecisionKind.reject_unknown_tool;
    lease: ToolLeaseHandle = ToolLeaseHandle{};
    breaker_state: ToolBreakerState = ToolBreakerState.closed;
    active_concurrency: u32 = 0;
    window_calls_used: u32 = 0;
    window_cost_used_micro_usd: u64 = 0;
}

/// Point-in-time state snapshot for a single governed tool.
pub struct ToolGovToolSnapshot {
    tool_id: u32 = 0;
    breaker_state: ToolBreakerState = ToolBreakerState.closed;
    active_concurrency: u32 = 0;
    consecutive_failures: u32 = 0;
    window_calls_used: u32 = 0;
    window_cost_used_micro_usd: u64 = 0;
    total_allowed: u64 = 0;
    total_rejected: u64 = 0;
    total_trips: u64 = 0;
}

/// Global telemetry statistics across all governed tools.
pub struct ToolGovernorStats {
    registered_tools: usize = 0;
    active_leases: usize = 0;
    total_acquires: u64 = 0;
    total_allowed: u64 = 0;
    total_releases: u64 = 0;
    rejected_unknown: u64 = 0;
    rejected_circuit_open: u64 = 0;
    rejected_duplicate: u64 = 0;
    rejected_concurrency: u64 = 0;
    rejected_rate_limit: u64 = 0;
    rejected_cost_limit: u64 = 0;
    duplicate_release_rejects: u64 = 0;
}

// ============================================================================
// 2. Internal Storage Slots
// ============================================================================

struct WindowCallEvent {
    active: bool = false;
    ts_ms: u64 = 0;
    cost_micro_usd: u64 = 0;
    lease_id: u64 = 0;
}

struct DedupeSlot {
    active: bool = false;
    tool_id: u32 = 0;
    args_hash: u64 = 0;
    expires_at_ms: u64 = 0;
}

struct ActiveLeaseSlot {
    active: bool = false;
    lease_id: u64 = 0;
    tool_slot: u16 = 0;
    win_event_idx: u16 = 0;
    is_probe: bool = false;
}

struct ToolGovSlot {
    active: bool = false;
    policy: ToolGovPolicy = ToolGovPolicy{};
    breaker_state: ToolBreakerState = ToolBreakerState.closed;
    open_until_ms: u64 = 0;
    half_open_probe_inflight: bool = false;
    consecutive_failures: u32 = 0;
    active_concurrency: u32 = 0;
    total_allowed: u64 = 0;
    total_rejected: u64 = 0;
    total_trips: u64 = 0;
    events: [TOOL_GOV_MAX_WINDOW_EVENTS]WindowCallEvent = [WindowCallEvent{}; TOOL_GOV_MAX_WINDOW_EVENTS];
}

// ============================================================================
// 3. AgentToolGovernor (Zero-Alloc Multi-Dimensional Tool Governance Engine)
// ============================================================================

/// Zero-heap-allocation AI Agent tool execution governor.
pub struct AgentToolGovernor {
    tools: [TOOL_GOV_MAX_TOOLS]ToolGovSlot = [ToolGovSlot{}; TOOL_GOV_MAX_TOOLS];
    tool_count: usize = 0;
    dedupe: [TOOL_GOV_MAX_DEDUPE_ENTRIES]DedupeSlot = [DedupeSlot{}; TOOL_GOV_MAX_DEDUPE_ENTRIES];
    dedupe_cursor: usize = 0;
    leases: [TOOL_GOV_MAX_ACTIVE_LEASES]ActiveLeaseSlot = [ActiveLeaseSlot{}; TOOL_GOV_MAX_ACTIVE_LEASES];
    active_leases: usize = 0;
    next_lease_id: u64 = 1;
    last_now_ms: u64 = 0;
    total_acquires: u64 = 0;
    total_allowed: u64 = 0;
    total_releases: u64 = 0;
    rejected_unknown: u64 = 0;
    rejected_circuit_open: u64 = 0;
    rejected_duplicate: u64 = 0;
    rejected_concurrency: u64 = 0;
    rejected_rate_limit: u64 = 0;
    rejected_cost_limit: u64 = 0;
    duplicate_release_rejects: u64 = 0;

    /// Creates an empty `AgentToolGovernor`.
    static fn init(): AgentToolGovernor {
        return AgentToolGovernor{};
    }

    fn monotonicNow(*_, now_ms: u64): u64 {
        if (now_ms > _.last_now_ms) {
            _.last_now_ms = now_ms;
        }
        return _.last_now_ms;
    }

    fn findToolSlot(tool_id: u32): usize? {
        let i: usize = 0;
        while (i < _.tool_count) : (i += 1) {
            if (_.tools[i].active && _.tools[i].policy.tool_id == tool_id) {
                return i;
            }
        }
        return null;
    }

    /// Sweeps expired sliding-window events for `t_idx` and returns `(calls_used, cost_used)`.
    fn pruneAndSumWindow(*_, t_idx: usize, eff_now_ms: u64): [2]u64 {
        const win_ms: u64 = _.tools[t_idx].policy.window_ms;
        let calls: u64 = 0;
        let cost: u64 = 0;
        let i: usize = 0;
        while (i < TOOL_GOV_MAX_WINDOW_EVENTS) : (i += 1) {
            if (_.tools[t_idx].events[i].active) {
                const ev_ts: u64 = _.tools[t_idx].events[i].ts_ms;
                if (eff_now_ms >= ev_ts && (eff_now_ms - ev_ts) >= win_ms) {
                    _.tools[t_idx].events[i].active = false;
                } else {
                    calls += 1;
                    cost += _.tools[t_idx].events[i].cost_micro_usd;
                }
            }
        }
        return [calls, cost];
    }

    fn isDuplicateInWindow(tool_id: u32, args_hash: u64, eff_now_ms: u64): bool {
        let i: usize = 0;
        while (i < TOOL_GOV_MAX_DEDUPE_ENTRIES) : (i += 1) {
            const d = _.dedupe[i];
            if (d.active && d.tool_id == tool_id && d.args_hash == args_hash && eff_now_ms < d.expires_at_ms) {
                return true;
            }
        }
        return false;
    }

    fn recordDedupe(*_, tool_id: u32, args_hash: u64, expires_at_ms: u64) {
        // Prefer an inactive or already-expired slot before ring overwrite.
        let target: usize = TOOL_GOV_MAX_DEDUPE_ENTRIES;
        let i: usize = 0;
        while (i < TOOL_GOV_MAX_DEDUPE_ENTRIES) : (i += 1) {
            if (!_.dedupe[i].active) {
                target = i;
                break;
            }
        }
        if (target >= TOOL_GOV_MAX_DEDUPE_ENTRIES) {
            target = _.dedupe_cursor;
            _.dedupe_cursor = (_.dedupe_cursor + 1) % TOOL_GOV_MAX_DEDUPE_ENTRIES;
        }
        _.dedupe[target] = DedupeSlot{
            active: true,
            tool_id: tool_id,
            args_hash: args_hash,
            expires_at_ms: expires_at_ms,
        };
    }

    /// Registers a governance policy for `policy.tool_id`.
    /// Returns `false` if `policy` is invalid, `tool_id` is already registered, or tool table is full.
    fn registerTool(*_, policy: ToolGovPolicy): bool {
        if (!policy.isValid()) return false;
        if (_.findToolSlot(policy.tool_id) != null) return false;
        if (_.tool_count >= TOOL_GOV_MAX_TOOLS) return false;

        const idx: usize = _.tool_count;
        _.tools[idx] = ToolGovSlot{
            active: true,
            policy: policy,
        };
        _.tool_count += 1;
        return true;
    }

    /// Evaluates a tool call request through the ordered governance pipeline:
    ///   1) Tool registration
    ///   2) Circuit breaker (`closed` / `open` -> cooldown `half_open` single probe)
    ///   3) Short-window duplicate `(tool_id, args_hash)` suppression
    ///   4) Concurrency semaphore (`max_concurrency`)
    ///   5) Sliding-window call rate & micro-USD cost cap
    /// On approval, reserves concurrency & window quota and issues a `ToolLeaseHandle`.
    fn acquireToolCall(
        *_,
        tool_id: u32,
        args_hash: u64,
        est_cost_micro_usd: u64,
        now_ms: u64,
    ): ToolAcquireDecision {
        const eff_now: u64 = _.monotonicNow(now_ms);
        _.total_acquires += 1;

        // 1) Registration check
        const t_opt = _.findToolSlot(tool_id);
        if (t_opt == null) {
            _.rejected_unknown += 1;
            return ToolAcquireDecision{
                kind: ToolGovDecisionKind.reject_unknown_tool,
            };
        }
        const t_idx: usize = t_opt ?? 0;

        // Refresh sliding window accounting up front for accurate telemetry
        const win_pair = _.pruneAndSumWindow(t_idx, eff_now);
        const calls_used: u32 = win_pair[0] as u32;
        const cost_used: u64 = win_pair[1];

        // 2) Circuit breaker check (auto-transition open -> half_open once cooldown expires)
        if (_.tools[t_idx].breaker_state == ToolBreakerState.open) {
            if (eff_now >= _.tools[t_idx].open_until_ms) {
                _.tools[t_idx].breaker_state = ToolBreakerState.half_open;
                _.tools[t_idx].half_open_probe_inflight = false;
            } else {
                _.tools[t_idx].total_rejected += 1;
                _.rejected_circuit_open += 1;
                return ToolAcquireDecision{
                    kind: ToolGovDecisionKind.reject_circuit_open,
                    breaker_state: ToolBreakerState.open,
                    active_concurrency: _.tools[t_idx].active_concurrency,
                    window_calls_used: calls_used,
                    window_cost_used_micro_usd: cost_used,
                };
            }
        }
        let is_probe: bool = false;
        if (_.tools[t_idx].breaker_state == ToolBreakerState.half_open) {
            if (_.tools[t_idx].half_open_probe_inflight) {
                _.tools[t_idx].total_rejected += 1;
                _.rejected_circuit_open += 1;
                return ToolAcquireDecision{
                    kind: ToolGovDecisionKind.reject_circuit_open,
                    breaker_state: ToolBreakerState.half_open,
                    active_concurrency: _.tools[t_idx].active_concurrency,
                    window_calls_used: calls_used,
                    window_cost_used_micro_usd: cost_used,
                };
            }
            is_probe = true;
        }

        const pol = _.tools[t_idx].policy;

        // 3) Duplicate argument short-window suppression
        if (pol.dedupe_window_ms > 0 && _.isDuplicateInWindow(tool_id, args_hash, eff_now)) {
            _.tools[t_idx].total_rejected += 1;
            _.rejected_duplicate += 1;
            return ToolAcquireDecision{
                kind: ToolGovDecisionKind.reject_duplicate_call,
                breaker_state: _.tools[t_idx].breaker_state,
                active_concurrency: _.tools[t_idx].active_concurrency,
                window_calls_used: calls_used,
                window_cost_used_micro_usd: cost_used,
            };
        }

        // 4) Concurrency semaphore check
        if (_.tools[t_idx].active_concurrency >= pol.max_concurrency) {
            _.tools[t_idx].total_rejected += 1;
            _.rejected_concurrency += 1;
            return ToolAcquireDecision{
                kind: ToolGovDecisionKind.reject_concurrency_limit,
                breaker_state: _.tools[t_idx].breaker_state,
                active_concurrency: _.tools[t_idx].active_concurrency,
                window_calls_used: calls_used,
                window_cost_used_micro_usd: cost_used,
            };
        }

        // 5) Sliding-window rate limit & cost cap checks
        if (calls_used >= pol.max_calls_per_window) {
            _.tools[t_idx].total_rejected += 1;
            _.rejected_rate_limit += 1;
            return ToolAcquireDecision{
                kind: ToolGovDecisionKind.reject_rate_limit,
                breaker_state: _.tools[t_idx].breaker_state,
                active_concurrency: _.tools[t_idx].active_concurrency,
                window_calls_used: calls_used,
                window_cost_used_micro_usd: cost_used,
            };
        }
        if (pol.max_cost_micro_usd_per_window > 0 &&
            (est_cost_micro_usd > pol.max_cost_micro_usd_per_window ||
                cost_used > pol.max_cost_micro_usd_per_window - est_cost_micro_usd))
        {
            _.tools[t_idx].total_rejected += 1;
            _.rejected_cost_limit += 1;
            return ToolAcquireDecision{
                kind: ToolGovDecisionKind.reject_cost_limit,
                breaker_state: _.tools[t_idx].breaker_state,
                active_concurrency: _.tools[t_idx].active_concurrency,
                window_calls_used: calls_used,
                window_cost_used_micro_usd: cost_used,
            };
        }

        // Allocate lease slot & window event slot
        let lease_slot: usize = TOOL_GOV_MAX_ACTIVE_LEASES;
        let l: usize = 0;
        while (l < TOOL_GOV_MAX_ACTIVE_LEASES) : (l += 1) {
            if (!_.leases[l].active) {
                lease_slot = l;
                break;
            }
        }
        if (lease_slot >= TOOL_GOV_MAX_ACTIVE_LEASES) {
            _.tools[t_idx].total_rejected += 1;
            return ToolAcquireDecision{
                kind: ToolGovDecisionKind.reject_lease_table_full,
                breaker_state: _.tools[t_idx].breaker_state,
                active_concurrency: _.tools[t_idx].active_concurrency,
                window_calls_used: calls_used,
                window_cost_used_micro_usd: cost_used,
            };
        }

        let ev_slot: usize = TOOL_GOV_MAX_WINDOW_EVENTS;
        let e: usize = 0;
        while (e < TOOL_GOV_MAX_WINDOW_EVENTS) : (e += 1) {
            if (!_.tools[t_idx].events[e].active) {
                ev_slot = e;
                break;
            }
        }
        if (ev_slot >= TOOL_GOV_MAX_WINDOW_EVENTS) {
            _.tools[t_idx].total_rejected += 1;
            _.rejected_rate_limit += 1;
            return ToolAcquireDecision{
                kind: ToolGovDecisionKind.reject_rate_limit,
                breaker_state: _.tools[t_idx].breaker_state,
                active_concurrency: _.tools[t_idx].active_concurrency,
                window_calls_used: calls_used,
                window_cost_used_micro_usd: cost_used,
            };
        }

        // Commit admission
        const lid: u64 = _.next_lease_id;
        _.next_lease_id += 1;

        _.tools[t_idx].events[ev_slot] = WindowCallEvent{
            active: true,
            ts_ms: eff_now,
            cost_micro_usd: est_cost_micro_usd,
            lease_id: lid,
        };
        _.leases[lease_slot] = ActiveLeaseSlot{
            active: true,
            lease_id: lid,
            tool_slot: t_idx as u16,
            win_event_idx: ev_slot as u16,
            is_probe: is_probe,
        };
        _.active_leases += 1;
        _.tools[t_idx].active_concurrency += 1;
        if (is_probe) {
            _.tools[t_idx].half_open_probe_inflight = true;
        }
        if (pol.dedupe_window_ms > 0) {
            _.recordDedupe(tool_id, args_hash, eff_now + pol.dedupe_window_ms);
        }

        _.tools[t_idx].total_allowed += 1;
        _.total_allowed += 1;

        return ToolAcquireDecision{
            kind: ToolGovDecisionKind.allowed,
            lease: ToolLeaseHandle{
                lease_id: lid,
                tool_id: tool_id,
                slot_idx: lease_slot as u16,
                is_probe: is_probe,
            },
            breaker_state: _.tools[t_idx].breaker_state,
            active_concurrency: _.tools[t_idx].active_concurrency,
            window_calls_used: calls_used + 1,
            window_cost_used_micro_usd: cost_used + est_cost_micro_usd,
        };
    }

    /// Completes an in-flight tool call lease, releasing concurrency, reconciling actual cost,
    /// and advancing the failure circuit breaker state machine.
    fn releaseToolCall(
        *_,
        lease: ToolLeaseHandle,
        succeeded: bool,
        actual_cost_micro_usd: u64,
        now_ms: u64,
    ): ToolReleaseStatus {
        const eff_now: u64 = _.monotonicNow(now_ms);
        if (!lease.isValid() || (lease.slot_idx as usize) >= TOOL_GOV_MAX_ACTIVE_LEASES) {
            _.duplicate_release_rejects += 1;
            return ToolReleaseStatus.invalid_lease;
        }

        const l_idx: usize = lease.slot_idx as usize;
        if (!_.leases[l_idx].active || _.leases[l_idx].lease_id != lease.lease_id) {
            _.duplicate_release_rejects += 1;
            return ToolReleaseStatus.already_released;
        }

        const t_idx: usize = _.leases[l_idx].tool_slot as usize;
        const ev_idx: usize = _.leases[l_idx].win_event_idx as usize;
        const was_probe: bool = _.leases[l_idx].is_probe;

        // Release lease & concurrency slot
        _.leases[l_idx].active = false;
        _.leases[l_idx].lease_id = TOOL_GOV_INVALID_LEASE;
        if (_.active_leases > 0) {
            _.active_leases -= 1;
        }
        if (t_idx < _.tool_count) {
            if (_.tools[t_idx].active_concurrency > 0) {
                _.tools[t_idx].active_concurrency -= 1;
            }
            // Reconcile estimated cost -> actual cost if event still matches this lease
            if (ev_idx < TOOL_GOV_MAX_WINDOW_EVENTS &&
                _.tools[t_idx].events[ev_idx].active &&
                _.tools[t_idx].events[ev_idx].lease_id == lease.lease_id)
            {
                _.tools[t_idx].events[ev_idx].cost_micro_usd = actual_cost_micro_usd;
            }

            const pol = _.tools[t_idx].policy;
            if (succeeded) {
                _.tools[t_idx].consecutive_failures = 0;
                if (was_probe || _.tools[t_idx].breaker_state == ToolBreakerState.half_open) {
                    _.tools[t_idx].breaker_state = ToolBreakerState.closed;
                    _.tools[t_idx].half_open_probe_inflight = false;
                }
            } else {
                _.tools[t_idx].consecutive_failures += 1;
                const should_trip: bool = was_probe ||
                    (pol.failure_trip_threshold > 0 && _.tools[t_idx].consecutive_failures >= pol.failure_trip_threshold);
                if (should_trip) {
                    _.tools[t_idx].breaker_state = ToolBreakerState.open;
                    _.tools[t_idx].open_until_ms = eff_now + pol.cooldown_ms;
                    _.tools[t_idx].half_open_probe_inflight = false;
                    _.tools[t_idx].total_trips += 1;
                }
            }
        }

        _.total_releases += 1;
        return ToolReleaseStatus.ok;
    }

    /// Returns a point-in-time snapshot for `tool_id`, or `null` if unregistered.
    fn getToolSnapshot(*_, tool_id: u32, now_ms: u64): ToolGovToolSnapshot? {
        const eff_now: u64 = _.monotonicNow(now_ms);
        const t_opt = _.findToolSlot(tool_id);
        if (t_opt == null) return null;
        const t_idx: usize = t_opt ?? 0;
        const win_pair = _.pruneAndSumWindow(t_idx, eff_now);
        const slot = _.tools[t_idx];
        return ToolGovToolSnapshot{
            tool_id: slot.policy.tool_id,
            breaker_state: slot.breaker_state,
            active_concurrency: slot.active_concurrency,
            consecutive_failures: slot.consecutive_failures,
            window_calls_used: win_pair[0] as u32,
            window_cost_used_micro_usd: win_pair[1],
            total_allowed: slot.total_allowed,
            total_rejected: slot.total_rejected,
            total_trips: slot.total_trips,
        };
    }

    /// Returns global governance telemetry counters.
    fn stats(): ToolGovernorStats {
        return ToolGovernorStats{
            registered_tools: _.tool_count,
            active_leases: _.active_leases,
            total_acquires: _.total_acquires,
            total_allowed: _.total_allowed,
            total_releases: _.total_releases,
            rejected_unknown: _.rejected_unknown,
            rejected_circuit_open: _.rejected_circuit_open,
            rejected_duplicate: _.rejected_duplicate,
            rejected_concurrency: _.rejected_concurrency,
            rejected_rate_limit: _.rejected_rate_limit,
            rejected_cost_limit: _.rejected_cost_limit,
            duplicate_release_rejects: _.duplicate_release_rejects,
        };
    }
}
