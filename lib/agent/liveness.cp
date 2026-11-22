// Origin: Pure-CP Stdlib (AI Agent Cluster Liveness Heartbeat Monitor & Restart Storm Quarantine) | Created: 2026-11-22
// Zero-heap-allocation BEAM-inspired Agent cluster liveness monitor providing two-stage heartbeat
// lease timeout detection (Healthy -> Suspect -> Unreachable) and sliding-window restart-storm quarantine.

/// Lifecycle health state of a monitored Agent instance.
pub enum AgentHealthState {
    Healthy,
    Suspect,
    Unreachable,
    Quarantined,
    Stopped,
}

/// Timeout and restart-storm circuit-breaker configuration for `AgentLivenessMonitor`.
pub struct LivenessConfig {
    heartbeat_timeout_ms: u64 = 5000;
    suspect_grace_ms: u64 = 3000;
    max_restarts_in_window: u16 = 3;
    restart_window_ms: u64 = 60000;
    quarantine_duration_ms: u64 = 30000;
}

/// Fixed-slot liveness and restart-window telemetry record for a single Agent.
pub struct AgentLivenessSlot {
    agent_id: u32 = 0;
    state: AgentHealthState = AgentHealthState.Stopped;
    last_heartbeat_ms: u64 = 0;
    window_start_ms: u64 = 0;
    restarts_in_window: u16 = 0;
    quarantine_until_ms: u64 = 0;
    total_restarts: u32 = 0;
    occupied: bool = false;
}

/// Cluster-wide health distribution snapshot returned by `AgentLivenessMonitor.summary`.
pub struct LivenessSummary {
    total: usize = 0;
    healthy: usize = 0;
    suspect: usize = 0;
    unreachable_count: usize = 0;
    quarantined: usize = 0;
    stopped: usize = 0;
}

/// Fixed-capacity, zero-heap-allocation Agent cluster liveness monitor & restart-storm quarantine.
pub struct AgentLivenessMonitor {
    slots: []AgentLivenessSlot;
    cfg: LivenessConfig;

    /// Initializes a monitor backed by caller-owned `slots`.
    /// Returns `null` if `slots` is empty or required timeout/window thresholds are zero.
    static fn init(
        slots: *[]AgentLivenessSlot,
        cfg: LivenessConfig,
    ): AgentLivenessMonitor? {
        if (slots.len == 0) return null;
        if (cfg.heartbeat_timeout_ms == 0 || cfg.max_restarts_in_window == 0 ||
            cfg.restart_window_ms == 0 || cfg.quarantine_duration_ms == 0)
        {
            return null;
        }

        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = AgentLivenessSlot{};
        }

        return AgentLivenessMonitor{
            slots: slots,
            cfg: cfg,
        };
    }

    fn findSlotIndex(agent_id: u32): usize? {
        if (agent_id == 0) return null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].agent_id == agent_id) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new Agent into the fixed pool as `Healthy`.
    /// Rejects `agent_id == 0`, duplicate IDs, or full pool by returning `null`.
    fn registerAgent(*_, agent_id: u32, now_ms: u64): usize? {
        if (agent_id == 0) return null;
        let free_slot: usize? = null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].agent_id == agent_id) return null;
            } else if (free_slot == null) {
                free_slot = i;
            }
        }

        if (slot := free_slot) {
            _.slots[slot] = AgentLivenessSlot{
                agent_id: agent_id,
                state: AgentHealthState.Healthy,
                last_heartbeat_ms: now_ms,
                window_start_ms: now_ms,
                restarts_in_window: 0,
                quarantine_until_ms: 0,
                total_restarts: 0,
                occupied: true,
            };
            return slot;
        }
        return null;
    }

    /// Removes an Agent from the monitor pool. Returns `false` if not found.
    fn unregisterAgent(*_, agent_id: u32): bool {
        if (idx := _.findSlotIndex(agent_id)) {
            _.slots[idx] = AgentLivenessSlot{};
            return true;
        }
        return false;
    }

    /// Marks an Agent as gracefully `Stopped` (exempt from timeout sweeps).
    fn stopAgent(*_, agent_id: u32): bool {
        if (idx := _.findSlotIndex(agent_id)) {
            _.slots[idx].state = AgentHealthState.Stopped;
            return true;
        }
        return false;
    }

    /// Renews an Agent's liveness lease.
    /// Rejects `Stopped` or actively `Quarantined` agents (before `quarantine_until_ms`).
    /// If quarantine has expired (`now_ms >= quarantine_until_ms`), automatically releases quarantine
    /// and restores `Healthy`.
    fn heartbeat(*_, agent_id: u32, now_ms: u64): bool {
        const idx_opt = _.findSlotIndex(agent_id);
        if (idx_opt == null) return false;
        const idx = idx_opt ?? 0;

        if (_.slots[idx].state == AgentHealthState.Stopped) return false;

        if (_.slots[idx].state == AgentHealthState.Quarantined) {
            if (now_ms < _.slots[idx].quarantine_until_ms) {
                return false;
            }
            _.slots[idx].quarantine_until_ms = 0;
            _.slots[idx].restarts_in_window = 0;
            _.slots[idx].window_start_ms = now_ms;
        }

        if (now_ms > _.slots[idx].last_heartbeat_ms) {
            _.slots[idx].last_heartbeat_ms = now_ms;
        }
        _.slots[idx].state = AgentHealthState.Healthy;
        return true;
    }

    /// Records a supervisor restart event for `agent_id`.
    /// Resets the window counter once `restart_window_ms` elapses. When `restarts_in_window`
    /// exceeds `max_restarts_in_window`, automatically transitions the Agent to `Quarantined`
    /// for `quarantine_duration_ms`.
    fn recordRestart(*_, agent_id: u32, now_ms: u64): AgentHealthState? {
        const idx_opt = _.findSlotIndex(agent_id);
        if (idx_opt == null) return null;
        const idx = idx_opt ?? 0;

        if (_.slots[idx].state == AgentHealthState.Stopped) {
            return AgentHealthState.Stopped;
        }

        if (_.slots[idx].state == AgentHealthState.Quarantined) {
            if (now_ms < _.slots[idx].quarantine_until_ms) {
                _.slots[idx].total_restarts += 1;
                return AgentHealthState.Quarantined;
            }
            // Quarantine expired -> start a fresh restart window.
            _.slots[idx].quarantine_until_ms = 0;
            _.slots[idx].restarts_in_window = 0;
            _.slots[idx].window_start_ms = now_ms;
        }

        if (now_ms >= _.slots[idx].window_start_ms &&
            (now_ms - _.slots[idx].window_start_ms) >= _.cfg.restart_window_ms)
        {
            _.slots[idx].window_start_ms = now_ms;
            _.slots[idx].restarts_in_window = 0;
        }

        _.slots[idx].restarts_in_window += 1;
        _.slots[idx].total_restarts += 1;

        if (_.slots[idx].restarts_in_window > _.cfg.max_restarts_in_window) {
            _.slots[idx].state = AgentHealthState.Quarantined;
            _.slots[idx].quarantine_until_ms = now_ms + _.cfg.quarantine_duration_ms;
            return AgentHealthState.Quarantined;
        }

        _.slots[idx].state = AgentHealthState.Healthy;
        _.slots[idx].last_heartbeat_ms = now_ms;
        return AgentHealthState.Healthy;
    }

    /// Sweeps all monitored Agents at `now_ms`:
    /// - Advances `Healthy -> Suspect` after `heartbeat_timeout_ms`;
    /// - Advances `Suspect -> Unreachable` after `heartbeat_timeout_ms + suspect_grace_ms` and
    ///   collects `Unreachable` IDs into `out_unreachable_ids` (truncation-safe);
    /// - Releases `Quarantined` agents whose `quarantine_until_ms <= now_ms` back to `Healthy`.
    /// Returns the number of unreachable IDs written to `out_unreachable_ids`.
    fn sweepTimeouts(*_, now_ms: u64, out_unreachable_ids: *[]u32): usize {
        let written: usize = 0;
        const unreach_limit: u64 = _.cfg.heartbeat_timeout_ms + _.cfg.suspect_grace_ms;

        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) continue;

            if (_.slots[i].state == AgentHealthState.Stopped) {
                continue;
            }

            if (_.slots[i].state == AgentHealthState.Quarantined) {
                if (now_ms >= _.slots[i].quarantine_until_ms) {
                    _.slots[i].state = AgentHealthState.Healthy;
                    _.slots[i].quarantine_until_ms = 0;
                    _.slots[i].restarts_in_window = 0;
                    _.slots[i].window_start_ms = now_ms;
                    _.slots[i].last_heartbeat_ms = now_ms;
                }
                continue;
            }

            const elapsed: u64 = if (now_ms > _.slots[i].last_heartbeat_ms)
                now_ms - _.slots[i].last_heartbeat_ms
            else
                0;

            if (elapsed >= unreach_limit) {
                _.slots[i].state = AgentHealthState.Unreachable;
                if (written < out_unreachable_ids.len) {
                    out_unreachable_ids[written] = _.slots[i].agent_id;
                    written += 1;
                }
            } else if (elapsed >= _.cfg.heartbeat_timeout_ms) {
                _.slots[i].state = AgentHealthState.Suspect;
            }
        }

        return written;
    }

    /// Returns the current `AgentHealthState` of `agent_id`, or `null` if unregistered.
    fn getState(agent_id: u32): AgentHealthState? {
        if (idx := _.findSlotIndex(agent_id)) {
            return _.slots[idx].state;
        }
        return null;
    }

    /// Returns a copy of the full liveness slot record for `agent_id`, or `null` if unregistered.
    fn getSlot(agent_id: u32): AgentLivenessSlot? {
        if (idx := _.findSlotIndex(agent_id)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Returns a cluster-wide summary of Agent counts across all health states.
    fn summary(): LivenessSummary {
        let s = LivenessSummary{};
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            s.total += 1;
            switch (_.slots[i].state) {
                AgentHealthState.Healthy => { s.healthy += 1; },
                AgentHealthState.Suspect => { s.suspect += 1; },
                AgentHealthState.Unreachable => { s.unreachable_count += 1; },
                AgentHealthState.Quarantined => { s.quarantined += 1; },
                AgentHealthState.Stopped => { s.stopped += 1; },
            }
        }
        return s;
    }
}
