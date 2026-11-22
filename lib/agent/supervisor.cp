// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc OTP-Style Agent Fault-Tolerant Supervisor Tree) | Created: 2026-11-22
// Pure CP zero-heap-allocation Erlang/OTP-inspired fault-tolerant supervisor tree for AI Agent runtimes:
//   - Restart strategies: `one_for_one`, `one_for_all`, `rest_for_one` with deterministic `start_order` ordering
//   - Child restart policies: `permanent`, `transient`, `temporary`
//   - Sliding-window restart intensity circuit breaker (`escalate_shutdown`) to prevent restart storms
//   - Overflow-safe capped exponential backoff with healthy-window consecutive crash reset

// ============================================================================
// 1. Constants & Enums
// ============================================================================

/// Maximum supervised child agents per supervisor tree node (zero dynamic heap).
pub const SUPERVISOR_MAX_CHILDREN: usize = 32;
/// Maximum sliding-window restart timestamps retained inline.
pub const SUPERVISOR_MAX_RESTART_HISTORY: usize = 64;

/// OTP supervision strategy determining which sibling agents restart on a child exit.
pub enum AgentSupervisorStrategy {
    one_for_one,
    one_for_all,
    rest_for_one,
}

/// Child restart policy determining whether a given exit reason triggers a restart.
pub enum AgentRestartPolicy {
    permanent,
    transient,
    temporary,
}

/// Reason reported when a supervised AI Agent exits.
pub enum AgentExitReason {
    normal,
    shutdown,
    crash_error,
    timeout,
    budget_exceeded,
}

/// Lifecycle state of a supervised child agent slot.
pub enum AgentChildState {
    stopped,
    running,
    backoff_wait,
    terminated,
}

/// Action emitted by the supervisor tree upon handling a child exit event.
pub enum SupervisorActionKind {
    none,
    restart_children,
    escalate_shutdown,
}

// ============================================================================
// 2. Configuration, Slot, Decision & Statistics Models
// ============================================================================

/// Configuration for `AgentSupervisorTree`.
pub struct AgentSupervisorConfig {
    strategy: AgentSupervisorStrategy = AgentSupervisorStrategy.one_for_one;
    max_restarts: u32 = 3;
    intensity_window_ms: u64 = 5000;
    base_backoff_ms: u64 = 100;
    max_backoff_ms: u64 = 10000;

    /// Validates supervisor configuration (`intensity_window_ms > 0` and `max_backoff_ms >= base_backoff_ms`).
    fn isValid(): bool {
        if (_.intensity_window_ms == 0) {
            return false;
        }
        if (_.max_backoff_ms < _.base_backoff_ms) {
            return false;
        }
        return true;
    }
}

/// Inline state slot for a single supervised child agent.
pub struct SupervisedChildSlot {
    active: bool = false;
    child_id: u64 = 0;
    policy: AgentRestartPolicy = AgentRestartPolicy.permanent;
    state: AgentChildState = AgentChildState.stopped;
    start_order: u16 = 0;
    restart_count: u32 = 0;
    consecutive_crashes: u32 = 0;
    next_restart_at_ms: u64 = 0;
    last_started_ms: u64 = 0;
}

/// Deterministic decision returned by `AgentSupervisorTree.report_exit`.
pub struct SupervisorDecision {
    action: SupervisorActionKind = SupervisorActionKind.none;
    restart_child_ids: [SUPERVISOR_MAX_CHILDREN]u64 = [0; SUPERVISOR_MAX_CHILDREN];
    restart_count: usize = 0;
    backoff_ms: u64 = 0;
    intensity_exceeded: bool = false;
}

/// Aggregate operational snapshot of `AgentSupervisorTree`.
pub struct SupervisorStats {
    total_children: usize = 0;
    running_children: usize = 0;
    backoff_children: usize = 0;
    terminated_children: usize = 0;
    total_restarts: u64 = 0;
    escalated: bool = false;
}

// ============================================================================
// 3. Internal Pure Helpers
// ============================================================================

fn shouldRestartChild(policy: AgentRestartPolicy, reason: AgentExitReason): bool {
    return switch (policy) {
        AgentRestartPolicy.permanent => true,
        AgentRestartPolicy.transient => switch (reason) {
            AgentExitReason.normal, AgentExitReason.shutdown => false,
            else => true,
        },
        AgentRestartPolicy.temporary => false,
    };
}

fn computeCappedBackoff(base_ms: u64, max_ms: u64, consecutive: u32): u64 {
    if (base_ms == 0 || max_ms == 0) {
        return 0;
    }
    if (base_ms >= max_ms) {
        return max_ms;
    }
    const raw_exp: u32 = if (consecutive > 1) consecutive - 1 else 0;
    const clamped_exp: u6 = (if (raw_exp > 16) 16 else raw_exp) as u6;
    const safe_limit: u64 = max_ms >> clamped_exp;
    if (base_ms > safe_limit) {
        return max_ms;
    }
    return base_ms << clamped_exp;
}

// ============================================================================
// 4. Zero-Alloc OTP Agent Supervisor Tree
// ============================================================================

/// Zero-heap-allocation OTP-style fault-tolerant supervisor tree managing up to 32 child agents.
pub struct AgentSupervisorTree {
    cfg: AgentSupervisorConfig = AgentSupervisorConfig{};
    children: [SUPERVISOR_MAX_CHILDREN]SupervisedChildSlot = [SupervisedChildSlot{}; SUPERVISOR_MAX_CHILDREN];
    child_count: usize = 0;
    restart_timestamps: [SUPERVISOR_MAX_RESTART_HISTORY]u64 = [0; SUPERVISOR_MAX_RESTART_HISTORY];
    restart_history_len: usize = 0;
    total_restarts: u64 = 0;
    escalated: bool = false;

    /// Initializes a validated supervisor tree. Returns `null` if `cfg.isValid()` is `false`.
    static fn init(cfg: AgentSupervisorConfig): AgentSupervisorTree? {
        if (!cfg.isValid()) {
            return null;
        }
        return AgentSupervisorTree{
            cfg: cfg,
            children: [SupervisedChildSlot{}; SUPERVISOR_MAX_CHILDREN],
            child_count: 0,
            restart_timestamps: [0; SUPERVISOR_MAX_RESTART_HISTORY],
            restart_history_len: 0,
            total_restarts: 0,
            escalated: false,
        };
    }

    fn findChildSlot(child_id: u64): usize? {
        if (child_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.child_count) : (i += 1) {
            if (_.children[i].active && _.children[i].child_id == child_id) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new child agent with a restart policy and deterministic `start_order`.
    /// Rejects `child_id == 0`, duplicate `child_id`, escalated tree, or full 32-child table.
    fn register_child(
        *_,
        child_id: u64,
        policy: AgentRestartPolicy,
        start_order: u16,
    ): bool {
        if (_.escalated || child_id == 0 || _.child_count >= SUPERVISOR_MAX_CHILDREN) {
            return false;
        }
        if (_.findChildSlot(child_id) != null) {
            return false;
        }
        _.children[_.child_count] = SupervisedChildSlot{
            active: true,
            child_id: child_id,
            policy: policy,
            state: AgentChildState.stopped,
            start_order: start_order,
            restart_count: 0,
            consecutive_crashes: 0,
            next_restart_at_ms: 0,
            last_started_ms: 0,
        };
        _.child_count += 1;
        return true;
    }

    /// Marks `child_id` as `running` at `now_ms`.
    /// If the child was already running continuously for at least `intensity_window_ms`, resets `consecutive_crashes`.
    fn mark_started(*_, child_id: u64, now_ms: u64): bool {
        if (_.escalated) {
            return false;
        }
        const idx = _.findChildSlot(child_id) ?? return false;
        if (_.children[idx].state == AgentChildState.terminated) {
            return false;
        }
        if (_.children[idx].state == AgentChildState.running &&
            _.children[idx].last_started_ms > 0 &&
            now_ms >= _.children[idx].last_started_ms &&
            (now_ms - _.children[idx].last_started_ms) >= _.cfg.intensity_window_ms)
        {
            _.children[idx].consecutive_crashes = 0;
        }
        _.children[idx].state = AgentChildState.running;
        _.children[idx].last_started_ms = now_ms;
        _.children[idx].next_restart_at_ms = 0;
        return true;
    }

    /// Handles a child exit event, applying restart policy, sliding-window intensity protection,
    /// exponential backoff, and strategy-specific sibling restart ordering.
    fn report_exit(
        *_,
        child_id: u64,
        reason: AgentExitReason,
        now_ms: u64,
    ): SupervisorDecision {
        if (_.escalated) {
            return SupervisorDecision{
                action: SupervisorActionKind.escalate_shutdown,
                restart_child_ids: [0; SUPERVISOR_MAX_CHILDREN],
                restart_count: 0,
                backoff_ms: 0,
                intensity_exceeded: true,
            };
        }

        const idx = _.findChildSlot(child_id) ?? return SupervisorDecision{};
        if (_.children[idx].state == AgentChildState.terminated) {
            return SupervisorDecision{};
        }

        if (!shouldRestartChild(_.children[idx].policy, reason)) {
            _.children[idx].state = AgentChildState.terminated;
            _.children[idx].next_restart_at_ms = 0;
            return SupervisorDecision{
                action: SupervisorActionKind.none,
                restart_child_ids: [0; SUPERVISOR_MAX_CHILDREN],
                restart_count: 0,
                backoff_ms: 0,
                intensity_exceeded: false,
            };
        }

        // 1. Prune restart history outside [now_ms - intensity_window_ms, now_ms]
        const window_start: u64 = if (now_ms >= _.cfg.intensity_window_ms)
            now_ms - _.cfg.intensity_window_ms
        else
            0;

        let kept: usize = 0;
        let h: usize = 0;
        while (h < _.restart_history_len) : (h += 1) {
            if (_.restart_timestamps[h] >= window_start) {
                _.restart_timestamps[kept] = _.restart_timestamps[h];
                kept += 1;
            }
        }
        _.restart_history_len = kept;

        // Check restart intensity threshold
        if (kept + 1 > (_.cfg.max_restarts as usize)) {
            _.escalated = true;
            let c: usize = 0;
            while (c < _.child_count) : (c += 1) {
                if (_.children[c].active) {
                    _.children[c].state = AgentChildState.terminated;
                    _.children[c].next_restart_at_ms = 0;
                }
            }
            return SupervisorDecision{
                action: SupervisorActionKind.escalate_shutdown,
                restart_child_ids: [0; SUPERVISOR_MAX_CHILDREN],
                restart_count: 0,
                backoff_ms: 0,
                intensity_exceeded: true,
            };
        }

        // Record current restart timestamp in bounded history ring
        if (_.restart_history_len >= SUPERVISOR_MAX_RESTART_HISTORY) {
            let s: usize = 1;
            while (s < SUPERVISOR_MAX_RESTART_HISTORY) : (s += 1) {
                _.restart_timestamps[s - 1] = _.restart_timestamps[s];
            }
            _.restart_history_len = SUPERVISOR_MAX_RESTART_HISTORY - 1;
        }
        _.restart_timestamps[_.restart_history_len] = now_ms;
        _.restart_history_len += 1;

        // 2. Healthy-window crash streak reset & capped exponential backoff
        if (_.children[idx].state == AgentChildState.running &&
            _.children[idx].last_started_ms > 0 &&
            now_ms >= _.children[idx].last_started_ms &&
            (now_ms - _.children[idx].last_started_ms) >= _.cfg.intensity_window_ms)
        {
            _.children[idx].consecutive_crashes = 0;
        }
        _.children[idx].consecutive_crashes += 1;
        const backoff = computeCappedBackoff(
            _.cfg.base_backoff_ms,
            _.cfg.max_backoff_ms,
            _.children[idx].consecutive_crashes,
        );

        // 3. Collect target child indices according to supervision strategy
        let selected: [SUPERVISOR_MAX_CHILDREN]usize = [0; SUPERVISOR_MAX_CHILDREN];
        let sel_count: usize = 0;
        const target_order = _.children[idx].start_order;

        switch (_.cfg.strategy) {
            AgentSupervisorStrategy.one_for_one => {
                selected[0] = idx;
                sel_count = 1;
            },
            AgentSupervisorStrategy.one_for_all => {
                let i: usize = 0;
                while (i < _.child_count) : (i += 1) {
                    if (!_.children[i].active || _.children[i].state == AgentChildState.terminated) {
                        continue;
                    }
                    if (i != idx && _.children[i].policy == AgentRestartPolicy.temporary) {
                        continue;
                    }
                    selected[sel_count] = i;
                    sel_count += 1;
                }
            },
            AgentSupervisorStrategy.rest_for_one => {
                let i: usize = 0;
                while (i < _.child_count) : (i += 1) {
                    if (!_.children[i].active || _.children[i].state == AgentChildState.terminated) {
                        continue;
                    }
                    if (i == idx) {
                        selected[sel_count] = i;
                        sel_count += 1;
                    } else if (_.children[i].policy != AgentRestartPolicy.temporary &&
                        (_.children[i].start_order > target_order ||
                        (_.children[i].start_order == target_order && _.children[i].child_id > child_id)))
                    {
                        selected[sel_count] = i;
                        sel_count += 1;
                    }
                }
            },
        }

        // Deterministic insertion sort by (start_order ascending, child_id ascending)
        let a: usize = 1;
        while (a < sel_count) : (a += 1) {
            const key = selected[a];
            let b: usize = a;
            while (b > 0) {
                const prev = selected[b - 1];
                const prev_after = _.children[prev].start_order > _.children[key].start_order ||
                    (_.children[prev].start_order == _.children[key].start_order &&
                    _.children[prev].child_id > _.children[key].child_id);
                if (!prev_after) {
                    break;
                }
                selected[b] = prev;
                b -= 1;
            }
            selected[b] = key;
        }

        // Apply backoff state and build decision output
        let decision = SupervisorDecision{
            action: SupervisorActionKind.restart_children,
            restart_child_ids: [0; SUPERVISOR_MAX_CHILDREN],
            restart_count: sel_count,
            backoff_ms: backoff,
            intensity_exceeded: false,
        };
        let k: usize = 0;
        while (k < sel_count) : (k += 1) {
            const slot = selected[k];
            _.children[slot].state = AgentChildState.backoff_wait;
            _.children[slot].restart_count += 1;
            _.children[slot].next_restart_at_ms = now_ms + backoff;
            decision.restart_child_ids[k] = _.children[slot].child_id;
            _.total_restarts += 1;
        }

        return decision;
    }

    /// Returns a read-only copy of a child agent slot, or `null` if not registered.
    fn get_child(child_id: u64): SupervisedChildSlot? {
        const idx = _.findChildSlot(child_id) ?? return null;
        return _.children[idx];
    }

    /// Computes a consistent operational snapshot of all supervised child agents.
    fn stats(): SupervisorStats {
        let s = SupervisorStats{
            total_children: _.child_count,
            running_children: 0,
            backoff_children: 0,
            terminated_children: 0,
            total_restarts: _.total_restarts,
            escalated: _.escalated,
        };
        let i: usize = 0;
        while (i < _.child_count) : (i += 1) {
            switch (_.children[i].state) {
                AgentChildState.running => {
                    s.running_children += 1;
                },
                AgentChildState.backoff_wait => {
                    s.backoff_children += 1;
                },
                AgentChildState.terminated => {
                    s.terminated_children += 1;
                },
                else => {},
            }
        }
        return s;
    }
}
