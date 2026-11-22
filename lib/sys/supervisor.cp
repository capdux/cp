// Origin: Pure-CP Stdlib (Zero-Alloc Process / Sidecar Supervisor & Health-Probe FSM) | Created: 2026-11-22
// Pure CP zero-heap-allocation Kubernetes/systemd-style service & sidecar lifecycle supervision FSM:
//   - Fixed inline 32-slot service registry (`PROC_SUP_MAX_SLOTS = 32`, 0 heap allocation)
//   - 7-state lifecycle (`stopped | starting | healthy | degraded | backoff_waiting | quarantined | stopping`)
//   - Exponential restart backoff (`initial * 2^k` capped at `max_backoff_ms` + deterministic jitter)
//     with automatic backoff reset after sustained healthy uptime (`reset_backoff_after_ms`)
//   - Liveness/readiness probe evaluator (`recordProbeResult`) with `success_threshold` & `failure_threshold`
//   - Sliding-window restart throttle transitioning to `quarantined` isolation + manual `resetQuarantine`
//   - Non-blocking tick action dispatcher (`pollNextAction`: `spawn | send_graceful_stop | force_stop_after_grace | none`)

// ============================================================================
// 1. Constants & Public Enums
// ============================================================================

/// Maximum number of supervised services/sidecars tracked inline.
pub const PROC_SUP_MAX_SLOTS: usize = 32;

/// Maximum sliding-window restart timestamps tracked inline per service slot.
pub const PROC_SUP_WINDOW_HISTORY: usize = 16;

/// Sentinel slot identifier representing an invalid or absent service slot.
pub const PROC_SUP_INVALID_SLOT: u16 = 0xFFFF;

/// Lifecycle state of a supervised service slot.
pub enum ProcSupState {
    stopped,
    starting,
    healthy,
    degraded,
    backoff_waiting,
    quarantined,
    stopping,
}

/// Action emitted by `ProcSupervisor.pollNextAction`.
pub enum ProcSupActionKind {
    none,
    spawn,
    send_graceful_stop,
    force_stop_after_grace,
}

/// Restart policy applied when a supervised process exits.
pub enum ProcSupRestartPolicy {
    always,
    on_failure,
    never,
}

/// Operation status returned by supervisor control methods.
pub enum ProcSupStatus {
    ok,
    invalid_config,
    slots_full,
    slot_not_found,
    invalid_state,
}

// ============================================================================
// 2. Public Data Models
// ============================================================================

/// Supervision, backoff, probe, and quarantine configuration for one service slot.
pub struct ProcSupConfig {
    service_id: u32 = 0;
    restart_policy: ProcSupRestartPolicy = ProcSupRestartPolicy.on_failure;
    initial_backoff_ms: u64 = 1000;
    max_backoff_ms: u64 = 60000;
    jitter_pct: u8 = 10;
    reset_backoff_after_ms: u64 = 30000;
    max_restarts_in_window: u32 = 5;
    restart_window_ms: u64 = 60000;
    probe_success_threshold: u16 = 1;
    probe_failure_threshold: u16 = 3;
    graceful_stop_timeout_ms: u64 = 5000;

    /// Returns `true` iff all configuration parameters are within valid production bounds.
    fn isValid(): bool {
        if (_.initial_backoff_ms == 0 || _.max_backoff_ms < _.initial_backoff_ms) {
            return false;
        }
        if (_.jitter_pct > 50) {
            return false;
        }
        if (_.max_restarts_in_window == 0 || (_.max_restarts_in_window as usize) > PROC_SUP_WINDOW_HISTORY) {
            return false;
        }
        if (_.restart_window_ms == 0) {
            return false;
        }
        if (_.probe_success_threshold == 0 || _.probe_failure_threshold == 0) {
            return false;
        }
        if (_.graceful_stop_timeout_ms == 0) {
            return false;
        }
        return true;
    }
}

/// Inline supervision state for one registered service slot.
pub struct ProcSupSlot {
    occupied: bool = false;
    slot_id: u16 = 0;
    cfg: ProcSupConfig = ProcSupConfig{};
    state: ProcSupState = ProcSupState.stopped;
    consecutive_failures: u32 = 0;
    consecutive_probe_ok: u16 = 0;
    consecutive_probe_fail: u16 = 0;
    total_restarts: u64 = 0;
    last_exit_code: i32 = 0;
    last_state_change_ms: u64 = 0;
    healthy_since_ms: u64 = 0;
    next_restart_ms: u64 = 0;
    stop_deadline_ms: u64 = 0;
    pending_graceful_stop: bool = false;
    restart_after_stop: bool = false;
    window_timestamps: [PROC_SUP_WINDOW_HISTORY]u64 = [0; PROC_SUP_WINDOW_HISTORY];
    window_count: usize = 0;
    last_observed_ms: u64 = 0;
}

/// Pending orchestration action returned by `ProcSupervisor.pollNextAction`.
pub struct ProcSupAction {
    kind: ProcSupActionKind = ProcSupActionKind.none;
    slot_id: u16 = PROC_SUP_INVALID_SLOT;
    service_id: u32 = 0;
    attempt: u32 = 0;
    scheduled_at_ms: u64 = 0;
}

/// Supervisor-wide telemetry snapshot returned by `ProcSupervisor.stats`.
pub struct ProcSupStats {
    total_slots: usize = 0;
    healthy_count: usize = 0;
    starting_count: usize = 0;
    degraded_count: usize = 0;
    backoff_waiting_count: usize = 0;
    quarantined_count: usize = 0;
    stopping_count: usize = 0;
    stopped_count: usize = 0;
    total_restarts: u64 = 0;
}

// ============================================================================
// 3. Internal Backoff & Sliding-Window Helpers
// ============================================================================

const U64_MAX: u64 = 0xFFFFFFFFFFFFFFFF;

fn satAddU64(a: u64, b: u64): u64 {
    if (a > U64_MAX - b) return U64_MAX;
    return a + b;
}

fn clampMonotonicMs(now_ms: u64, last_ms: u64): u64 {
    return if (now_ms >= last_ms) now_ms else last_ms;
}

/// Computes `min(max_backoff_ms, initial_backoff_ms * 2^(failures - 1))` plus deterministic jitter.
fn computeBackoffDelayMs(cfg: ProcSupConfig, consecutive_failures: u32, total_restarts: u64): u64 {
    let base: u64 = cfg.initial_backoff_ms;
    let exp: u32 = if (consecutive_failures > 1) (consecutive_failures - 1) else 0;
    if (exp > 30) {
        exp = 30;
    }
    let step: u32 = 0;
    while (step < exp) : (step += 1) {
        if (base >= cfg.max_backoff_ms / 2) {
            base = cfg.max_backoff_ms;
            break;
        }
        base = base * 2;
    }
    if (base > cfg.max_backoff_ms) {
        base = cfg.max_backoff_ms;
    }

    if (cfg.jitter_pct == 0) {
        return base;
    }

    // Deterministic additive jitter in `[0, base * jitter_pct / 100]` capped at `max_backoff_ms`.
    const mix: u64 = ((cfg.service_id as u64) *% 2654435761) ^ (total_restarts *% 11400714819323198485);
    const max_jitter: u64 = (base * (cfg.jitter_pct as u64)) / 100;
    if (max_jitter == 0) {
        return base;
    }
    const jitter: u64 = mix % (max_jitter + 1);
    const total: u64 = satAddU64(base, jitter);
    return if (total > cfg.max_backoff_ms) cfg.max_backoff_ms else total;
}

fn pruneAndRecordWindow(slot: *ProcSupSlot, now_ms: u64): usize {
    const win_ms: u64 = slot.cfg.restart_window_ms;
    let kept: usize = 0;
    let i: usize = 0;
    while (i < slot.window_count) : (i += 1) {
        const ts = slot.window_timestamps[i];
        if (now_ms >= ts && (now_ms - ts) < win_ms) {
            slot.window_timestamps[kept] = ts;
            kept += 1;
        }
    }
    if (kept < PROC_SUP_WINDOW_HISTORY) {
        slot.window_timestamps[kept] = now_ms;
        kept += 1;
    } else {
        // Shift left by 1 when saturated
        let j: usize = 1;
        while (j < PROC_SUP_WINDOW_HISTORY) : (j += 1) {
            slot.window_timestamps[j - 1] = slot.window_timestamps[j];
        }
        slot.window_timestamps[PROC_SUP_WINDOW_HISTORY - 1] = now_ms;
        kept = PROC_SUP_WINDOW_HISTORY;
    }
    slot.window_count = kept;
    return kept;
}

// ============================================================================
// 4. Zero-Alloc Process / Sidecar Supervisor FSM
// ============================================================================

/// Zero-heap-allocation service & sidecar supervision state machine with exponential
/// restart backoff, probe-driven recovery, CrashLoopBackOff quarantine, and graceful stop timeouts.
pub struct ProcSupervisor {
    slots: [PROC_SUP_MAX_SLOTS]ProcSupSlot = [ProcSupSlot{}; PROC_SUP_MAX_SLOTS];
    count: usize = 0;

    /// Initializes an empty `ProcSupervisor`.
    static fn init(): ProcSupervisor {
        return ProcSupervisor{};
    }

    /// Registers a new service configuration and returns its assigned `slot_id`, or `null` if invalid/full.
    fn registerService(*_, cfg: ProcSupConfig): u16? {
        if (!cfg.isValid() || _.count >= PROC_SUP_MAX_SLOTS) {
            return null;
        }
        const slot_id: u16 = _.count as u16;
        _.slots[_.count] = ProcSupSlot{
            occupied: true,
            slot_id: slot_id,
            cfg: cfg,
            state: ProcSupState.stopped,
        };
        _.count += 1;
        return slot_id;
    }

    /// Schedules a `stopped` service for immediate spawn on the next `pollNextAction(now_ms)`.
    fn startService(*_, slot_id: u16, now_ms: u64): ProcSupStatus {
        const idx: usize = slot_id as usize;
        if (idx >= _.count || !_.slots[idx].occupied) {
            return ProcSupStatus.slot_not_found;
        }
        if (_.slots[idx].state != ProcSupState.stopped) {
            return ProcSupStatus.invalid_state;
        }
        const eff_now = clampMonotonicMs(now_ms, _.slots[idx].last_observed_ms);
        _.slots[idx].last_observed_ms = eff_now;
        _.slots[idx].state = ProcSupState.backoff_waiting;
        _.slots[idx].next_restart_ms = eff_now;
        _.slots[idx].last_state_change_ms = eff_now;
        _.slots[idx].restart_after_stop = false;
        return ProcSupStatus.ok;
    }

    /// Initiates a graceful lifecycle stop for an active (`starting | healthy | degraded`) service.
    fn requestGracefulStop(*_, slot_id: u16, now_ms: u64): ProcSupStatus {
        const idx: usize = slot_id as usize;
        if (idx >= _.count || !_.slots[idx].occupied) {
            return ProcSupStatus.slot_not_found;
        }
        const st = _.slots[idx].state;
        if (st == ProcSupState.stopped || st == ProcSupState.stopping) {
            return ProcSupStatus.invalid_state;
        }
        const eff_now = clampMonotonicMs(now_ms, _.slots[idx].last_observed_ms);
        _.slots[idx].last_observed_ms = eff_now;
        if (st == ProcSupState.backoff_waiting || st == ProcSupState.quarantined) {
            _.slots[idx].state = ProcSupState.stopped;
            _.slots[idx].last_state_change_ms = eff_now;
            return ProcSupStatus.ok;
        }
        _.slots[idx].state = ProcSupState.stopping;
        _.slots[idx].pending_graceful_stop = true;
        _.slots[idx].restart_after_stop = false;
        _.slots[idx].stop_deadline_ms = satAddU64(eff_now, _.slots[idx].cfg.graceful_stop_timeout_ms);
        _.slots[idx].last_state_change_ms = eff_now;
        return ProcSupStatus.ok;
    }

    /// Evaluates a health probe outcome (`ok == true` or `false`) for `slot_id` at `now_ms`.
    /// Transitions `starting/degraded -> healthy`, resets backoff after sustained healthy uptime,
    /// and transitions to `stopping` (with restart) once `probe_failure_threshold` is reached.
    fn recordProbeResult(*_, slot_id: u16, ok: bool, now_ms: u64): ProcSupState? {
        const idx: usize = slot_id as usize;
        if (idx >= _.count || !_.slots[idx].occupied) {
            return null;
        }
        const eff_now = clampMonotonicMs(now_ms, _.slots[idx].last_observed_ms);
        _.slots[idx].last_observed_ms = eff_now;

        const st = _.slots[idx].state;
        if (st != ProcSupState.starting && st != ProcSupState.healthy && st != ProcSupState.degraded) {
            return st;
        }

        if (ok) {
            _.slots[idx].consecutive_probe_fail = 0;
            _.slots[idx].consecutive_probe_ok += 1;

            if (st != ProcSupState.healthy) {
                if (_.slots[idx].consecutive_probe_ok >= _.slots[idx].cfg.probe_success_threshold) {
                    _.slots[idx].state = ProcSupState.healthy;
                    _.slots[idx].healthy_since_ms = eff_now;
                    _.slots[idx].last_state_change_ms = eff_now;
                }
            } else {
                // Reset exponential backoff once sustained healthy uptime exceeds threshold.
                if (eff_now >= _.slots[idx].healthy_since_ms &&
                    (eff_now - _.slots[idx].healthy_since_ms) >= _.slots[idx].cfg.reset_backoff_after_ms) {
                    _.slots[idx].consecutive_failures = 0;
                }
            }
            return _.slots[idx].state;
        }

        // Probe failed
        _.slots[idx].consecutive_probe_ok = 0;
        _.slots[idx].consecutive_probe_fail += 1;

        if (_.slots[idx].consecutive_probe_fail >= _.slots[idx].cfg.probe_failure_threshold) {
            _.slots[idx].state = ProcSupState.stopping;
            _.slots[idx].pending_graceful_stop = true;
            _.slots[idx].restart_after_stop = true;
            _.slots[idx].stop_deadline_ms = satAddU64(eff_now, _.slots[idx].cfg.graceful_stop_timeout_ms);
            _.slots[idx].last_state_change_ms = eff_now;
        } else if (st == ProcSupState.healthy || st == ProcSupState.starting) {
            _.slots[idx].state = ProcSupState.degraded;
            _.slots[idx].last_state_change_ms = eff_now;
        }

        return _.slots[idx].state;
    }

    /// Records a process exit event with `exit_code` at `now_ms`, applying restart policy,
    /// sliding-window quarantine detection, and exponential backoff scheduling.
    fn recordProcessExit(*_, slot_id: u16, exit_code: i32, now_ms: u64): ProcSupState? {
        const idx: usize = slot_id as usize;
        if (idx >= _.count || !_.slots[idx].occupied) {
            return null;
        }
        const eff_now = clampMonotonicMs(now_ms, _.slots[idx].last_observed_ms);
        _.slots[idx].last_observed_ms = eff_now;
        _.slots[idx].last_exit_code = exit_code;
        _.slots[idx].pending_graceful_stop = false;

        const prev_state = _.slots[idx].state;
        if (prev_state == ProcSupState.stopped || prev_state == ProcSupState.quarantined) {
            return prev_state;
        }

        // Intentional graceful stop without restart -> clean stopped state.
        if (prev_state == ProcSupState.stopping && !_.slots[idx].restart_after_stop) {
            _.slots[idx].state = ProcSupState.stopped;
            _.slots[idx].last_state_change_ms = eff_now;
            return ProcSupState.stopped;
        }

        const forced_restart: bool = (prev_state == ProcSupState.stopping && _.slots[idx].restart_after_stop);
        _.slots[idx].restart_after_stop = false;

        // Evaluate restart policy
        const policy = _.slots[idx].cfg.restart_policy;
        let should_restart: bool = false;
        if (forced_restart) {
            should_restart = (policy != ProcSupRestartPolicy.never);
        } else if (policy == ProcSupRestartPolicy.always) {
            should_restart = true;
        } else if (policy == ProcSupRestartPolicy.on_failure) {
            should_restart = (exit_code != 0);
        }

        if (!should_restart) {
            _.slots[idx].state = ProcSupState.stopped;
            _.slots[idx].last_state_change_ms = eff_now;
            return ProcSupState.stopped;
        }

        // If service ran healthy longer than `reset_backoff_after_ms`, clear prior failure streak.
        if (prev_state == ProcSupState.healthy &&
            eff_now >= _.slots[idx].healthy_since_ms &&
            (eff_now - _.slots[idx].healthy_since_ms) >= _.slots[idx].cfg.reset_backoff_after_ms) {
            _.slots[idx].consecutive_failures = 0;
        }

        const win_restarts = pruneAndRecordWindow(*_.slots[idx], eff_now);
        if (win_restarts >= (_.slots[idx].cfg.max_restarts_in_window as usize)) {
            _.slots[idx].state = ProcSupState.quarantined;
            _.slots[idx].last_state_change_ms = eff_now;
            return ProcSupState.quarantined;
        }

        _.slots[idx].consecutive_failures += 1;
        _.slots[idx].total_restarts += 1;
        const delay_ms = computeBackoffDelayMs(_.slots[idx].cfg, _.slots[idx].consecutive_failures, _.slots[idx].total_restarts);
        _.slots[idx].next_restart_ms = satAddU64(eff_now, delay_ms);
        _.slots[idx].state = ProcSupState.backoff_waiting;
        _.slots[idx].last_state_change_ms = eff_now;
        return ProcSupState.backoff_waiting;
    }

    /// Manually releases a `quarantined` service back into immediate restart scheduling.
    fn resetQuarantine(*_, slot_id: u16, now_ms: u64): ProcSupStatus {
        const idx: usize = slot_id as usize;
        if (idx >= _.count || !_.slots[idx].occupied) {
            return ProcSupStatus.slot_not_found;
        }
        if (_.slots[idx].state != ProcSupState.quarantined) {
            return ProcSupStatus.invalid_state;
        }
        const eff_now = clampMonotonicMs(now_ms, _.slots[idx].last_observed_ms);
        _.slots[idx].last_observed_ms = eff_now;
        _.slots[idx].consecutive_failures = 0;
        _.slots[idx].window_count = 0;
        _.slots[idx].state = ProcSupState.backoff_waiting;
        _.slots[idx].next_restart_ms = eff_now;
        _.slots[idx].last_state_change_ms = eff_now;
        return ProcSupStatus.ok;
    }

    /// Advances all slots to `now_ms` and returns the next pending orchestration action:
    ///   1. `send_graceful_stop` for newly stopping services
    ///   2. `force_stop_after_grace` when graceful stop deadline expires
    ///   3. `spawn` when a `backoff_waiting` service reaches `next_restart_ms`
    ///   4. `none` when no action is currently due
    fn pollNextAction(*_, now_ms: u64): ProcSupAction {
        // Priority 1: Pending graceful stop signals
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            const eff_now = clampMonotonicMs(now_ms, _.slots[i].last_observed_ms);
            _.slots[i].last_observed_ms = eff_now;

            if (_.slots[i].state == ProcSupState.stopping && _.slots[i].pending_graceful_stop) {
                _.slots[i].pending_graceful_stop = false;
                return ProcSupAction{
                    kind: ProcSupActionKind.send_graceful_stop,
                    slot_id: _.slots[i].slot_id,
                    service_id: _.slots[i].cfg.service_id,
                    attempt: _.slots[i].consecutive_failures,
                    scheduled_at_ms: eff_now,
                };
            }
        }

        // Priority 2: Graceful stop deadline exceeded -> force_stop_after_grace
        let j: usize = 0;
        while (j < _.count) : (j += 1) {
            if (!_.slots[j].occupied) continue;
            const eff_now = _.slots[j].last_observed_ms;
            if (_.slots[j].state == ProcSupState.stopping && eff_now >= _.slots[j].stop_deadline_ms) {
                _.slots[j].stop_deadline_ms = satAddU64(eff_now, _.slots[j].cfg.graceful_stop_timeout_ms);
                return ProcSupAction{
                    kind: ProcSupActionKind.force_stop_after_grace,
                    slot_id: _.slots[j].slot_id,
                    service_id: _.slots[j].cfg.service_id,
                    attempt: _.slots[j].consecutive_failures,
                    scheduled_at_ms: eff_now,
                };
            }
        }

        // Priority 3: Ready backoff_waiting -> spawn
        let k: usize = 0;
        while (k < _.count) : (k += 1) {
            if (!_.slots[k].occupied) continue;
            const eff_now = _.slots[k].last_observed_ms;
            if (_.slots[k].state == ProcSupState.backoff_waiting && eff_now >= _.slots[k].next_restart_ms) {
                _.slots[k].state = ProcSupState.starting;
                _.slots[k].consecutive_probe_ok = 0;
                _.slots[k].consecutive_probe_fail = 0;
                _.slots[k].last_state_change_ms = eff_now;
                return ProcSupAction{
                    kind: ProcSupActionKind.spawn,
                    slot_id: _.slots[k].slot_id,
                    service_id: _.slots[k].cfg.service_id,
                    attempt: _.slots[k].consecutive_failures + 1,
                    scheduled_at_ms: eff_now,
                };
            }
        }

        return ProcSupAction{ kind: ProcSupActionKind.none };
    }

    /// Returns a value snapshot of `slot_id`, or `null` if out of bounds.
    fn getSlot(slot_id: u16): ProcSupSlot? {
        const idx: usize = slot_id as usize;
        if (idx >= _.count || !_.slots[idx].occupied) {
            return null;
        }
        return _.slots[idx];
    }

    /// Returns supervisor-wide state counts and aggregate restart telemetry.
    fn stats(): ProcSupStats {
        let st = ProcSupStats{
            total_slots: _.count,
        };
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (!_.slots[i].occupied) continue;
            st.total_restarts += _.slots[i].total_restarts;
            const s = _.slots[i].state;
            if (s == ProcSupState.healthy) {
                st.healthy_count += 1;
            } else if (s == ProcSupState.starting) {
                st.starting_count += 1;
            } else if (s == ProcSupState.degraded) {
                st.degraded_count += 1;
            } else if (s == ProcSupState.backoff_waiting) {
                st.backoff_waiting_count += 1;
            } else if (s == ProcSupState.quarantined) {
                st.quarantined_count += 1;
            } else if (s == ProcSupState.stopping) {
                st.stopping_count += 1;
            } else if (s == ProcSupState.stopped) {
                st.stopped_count += 1;
            }
        }
        return st;
    }
}
