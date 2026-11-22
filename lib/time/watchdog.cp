// Origin: Pure-CP Stdlib (Zero-Alloc Distributed Heartbeat Watchdog & Phi-Accrual Failure Detector) | Created: 2026-11-23
// Pure CP zero-heap-allocation distributed liveness watchdog & adaptive Phi-Accrual failure detector:
//   - Fixed 64-node inline monitor table with 4-state liveness FSM (`healthy | suspect | dead | paused`)
//     and 16-sample circular heartbeat inter-arrival window per node
//   - Dual-mode detection: deterministic fixed timeout (`suspect_timeout_ms` / `dead_timeout_ms` +
//     `grace_ms`) and Akka/Cassandra-style Phi-Accrual adaptive suspicion scoring (`computePhiAccrual`)
//   - Batch transition event polling (`pollTransitions`) emitting `became_suspect | became_dead | recovered`
//     events, maintenance `pause`/`resume` windows & cumulative telemetry snapshot

const std = @import("std");

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of monitored nodes stored inline (zero dynamic heap).
pub const WATCHDOG_MAX_NODES: usize = 64;

/// Circular heartbeat interval sample window size per node.
pub const WATCHDOG_SAMPLE_WINDOW: usize = 16;

/// Failure detection algorithm mode for a monitored node.
pub enum WatchdogMode {
    fixed_timeout,
    phi_accrual,
}

/// 4-state liveness lifecycle for a monitored node.
pub enum WatchdogState {
    healthy,
    suspect,
    dead,
    paused,
}

/// State transition event kind emitted by `pollTransitions`.
pub enum WatchdogEventKind {
    became_suspect,
    became_dead,
    recovered,
}

/// Per-node configuration supporting both fixed timeout and Phi-Accrual detection.
pub struct WatchdogNodeConfig {
    mode: WatchdogMode = WatchdogMode.fixed_timeout,
    expected_interval_ms: u64 = 1000,
    grace_ms: u64 = 0,
    suspect_timeout_ms: u64 = 3000,
    dead_timeout_ms: u64 = 8000,
    phi_suspect_threshold: f64 = 3.0,
    phi_dead_threshold: f64 = 8.0,
    min_std_dev_ms: f64 = 50.0,

    pub fn fixedDefaults(): WatchdogNodeConfig {
        return WatchdogNodeConfig{};
    }

    pub fn phiDefaults(): WatchdogNodeConfig {
        return WatchdogNodeConfig{
            mode: WatchdogMode.phi_accrual,
            expected_interval_ms: 1000,
            grace_ms: 0,
            suspect_timeout_ms: 3000,
            dead_timeout_ms: 8000,
            phi_suspect_threshold: 3.0,
            phi_dead_threshold: 8.0,
            min_std_dev_ms: 50.0,
        };
    }

    pub fn isValid(): bool {
        if (expected_interval_ms == 0) return false;
        if (mode == WatchdogMode.fixed_timeout) {
            return suspect_timeout_ms > 0 && dead_timeout_ms >= suspect_timeout_ms;
        }
        if (std.math.isNan(phi_suspect_threshold) || std.math.isInf(phi_suspect_threshold)) return false;
        if (std.math.isNan(phi_dead_threshold) || std.math.isInf(phi_dead_threshold)) return false;
        if (std.math.isNan(min_std_dev_ms) || std.math.isInf(min_std_dev_ms)) return false;
        return phi_suspect_threshold > 0.0 &&
            phi_dead_threshold >= phi_suspect_threshold &&
            min_std_dev_ms > 0.0;
    }
}

/// Inline monitored node runtime state.
pub struct WatchdogNodeState {
    active: bool = false;
    node_id: u64 = 0;
    state: WatchdogState = WatchdogState.healthy;
    cfg: WatchdogNodeConfig = WatchdogNodeConfig{},
    last_heartbeat_ms: u64 = 0;
    interval_ring: [WATCHDOG_SAMPLE_WINDOW]u64 = [0; WATCHDOG_SAMPLE_WINDOW];
    sample_count: usize = 0;
    sample_head: usize = 0;
    pending_recovered: bool = false;
    recovered_from: WatchdogState = WatchdogState.healthy;
    last_phi: f64 = 0.0;

    /// Returns mean heartbeat inter-arrival interval in milliseconds (falls back to `expected_interval_ms`).
    pub fn meanIntervalMs(): f64 {
        if (sample_count == 0) return cfg.expected_interval_ms as f64;
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < sample_count) : (i += 1) {
            sum += interval_ring[i] as f64;
        }
        return sum / (sample_count as f64);
    }

    /// Returns clamped heartbeat interval standard deviation (`>= cfg.min_std_dev_ms`).
    pub fn stdDevIntervalMs(): f64 {
        if (sample_count < 2) return cfg.min_std_dev_ms;
        const mean = meanIntervalMs();
        let var_sum: f64 = 0.0;
        let i: usize = 0;
        while (i < sample_count) : (i += 1) {
            const diff: f64 = (interval_ring[i] as f64) - mean;
            var_sum += diff * diff;
        }
        const sd = @sqrt(var_sum / (sample_count as f64));
        return if (sd < cfg.min_std_dev_ms) cfg.min_std_dev_ms else sd;
    }
}

/// Transition event emitted by `HeartbeatWatchdog.pollTransitions`.
pub struct WatchdogTransitionEvent {
    node_id: u64 = 0;
    kind: WatchdogEventKind = WatchdogEventKind.became_suspect;
    prev_state: WatchdogState = WatchdogState.healthy;
    new_state: WatchdogState = WatchdogState.suspect;
    silent_ms: u64 = 0;
    phi: f64 = 0.0;
}

/// Global telemetry snapshot for `HeartbeatWatchdog`.
pub struct WatchdogStats {
    node_count: usize = 0;
    capacity: usize = WATCHDOG_MAX_NODES;
    healthy_count: usize = 0;
    suspect_count: usize = 0;
    dead_count: usize = 0;
    paused_count: usize = 0;
    total_heartbeats: u64 = 0;
    total_suspect_events: u64 = 0;
    total_dead_events: u64 = 0;
    total_recoveries: u64 = 0;
}

// ============================================================================
// 2. Phi-Accrual Adaptive Suspicion Function
// ============================================================================

/// Computes the Akka/Cassandra-style Phi-Accrual suspicion level for a silence duration
/// `elapsed_ms` given historical heartbeat interval `mean_ms` and clamped `std_dev_ms`.
/// Higher `phi` indicates stronger confidence that the monitored node has failed.
pub fn computePhiAccrual(elapsed_ms: f64, mean_ms: f64, std_dev_ms: f64): f64 {
    if (elapsed_ms <= 0.0 || std_dev_ms <= 0.0) return 0.0;
    const y: f64 = (elapsed_ms - mean_ms) / std_dev_ms;
    const poly: f64 = y * (1.5976 + 0.070566 * y * y);

    if (elapsed_ms > mean_ms) {
        // Asymptotic fallback prevents exp(-poly) underflow to 0.0 -> -log10(0) = Inf.
        if (poly > 40.0) {
            return poly / 2.302585092994046;
        }
        const e = std.math.exp(-poly);
        const p_later = e / (1.0 + e);
        if (p_later <= 1.0e-300) return 300.0;
        return -std.math.log10(p_later);
    }

    if (poly < -40.0) return 0.0;
    const e = std.math.exp(-poly);
    const p_later = 1.0 - (1.0 / (1.0 + e));
    if (p_later >= 1.0) return 0.0;
    if (p_later <= 1.0e-300) return 300.0;
    return -std.math.log10(p_later);
}

// ============================================================================
// 3. Core Engine (`HeartbeatWatchdog`)
// ============================================================================

/// Zero-heap distributed heartbeat watchdog & Phi-Accrual adaptive failure detector.
pub struct HeartbeatWatchdog {
    nodes: [WATCHDOG_MAX_NODES]WatchdogNodeState = [WatchdogNodeState{}; WATCHDOG_MAX_NODES];
    total_heartbeats: u64 = 0;
    total_suspect_events: u64 = 0;
    total_dead_events: u64 = 0;
    total_recoveries: u64 = 0;

    /// Initializes an empty `HeartbeatWatchdog`.
    pub fn init(): HeartbeatWatchdog {
        return HeartbeatWatchdog{};
    }

    fn findSlot(node_id: u64): usize? {
        if (node_id == 0) return null;
        let i: usize = 0;
        while (i < WATCHDOG_MAX_NODES) : (i += 1) {
            if (nodes[i].active && nodes[i].node_id == node_id) return i;
        }
        return null;
    }

    /// Registers a monitored node (`node_id > 0`) starting in `healthy` state at `now_ms`.
    /// Returns slot index, or `null` on invalid ID/config, duplicate ID, or full 64-node table.
    pub fn registerNode(*_, node_id: u64, config: WatchdogNodeConfig, now_ms: u64): usize? {
        if (node_id == 0 || !config.isValid()) return null;
        if (findSlot(node_id) != null) return null;

        let i: usize = 0;
        while (i < WATCHDOG_MAX_NODES) : (i += 1) {
            if (!nodes[i].active) {
                nodes[i] = WatchdogNodeState{
                    active: true,
                    node_id: node_id,
                    state: WatchdogState.healthy,
                    cfg: config,
                    last_heartbeat_ms: now_ms,
                };
                return i;
            }
        }
        return null;
    }

    /// Removes a monitored node from the watchdog table.
    pub fn unregisterNode(*_, node_id: u64): bool {
        const slot = findSlot(node_id) ?? return false;
        nodes[slot] = WatchdogNodeState{};
        return true;
    }

    /// Records a heartbeat from `node_id` at `now_ms`, updating the 16-sample interval ring.
    /// If the node was `suspect` or `dead`, transitions it back to `healthy` and queues a
    /// `recovered` transition event for the next `pollTransitions` call.
    pub fn heartbeat(*_, node_id: u64, now_ms: u64): bool {
        const slot = findSlot(node_id) ?? return false;
        total_heartbeats += 1;

        if (nodes[slot].state == WatchdogState.paused) {
            if (now_ms > nodes[slot].last_heartbeat_ms) {
                nodes[slot].last_heartbeat_ms = now_ms;
            }
            return true;
        }

        if (now_ms > nodes[slot].last_heartbeat_ms) {
            const delta: u64 = now_ms - nodes[slot].last_heartbeat_ms;
            const head = nodes[slot].sample_head;
            nodes[slot].interval_ring[head] = delta;
            nodes[slot].sample_head = (head + 1) % WATCHDOG_SAMPLE_WINDOW;
            if (nodes[slot].sample_count < WATCHDOG_SAMPLE_WINDOW) {
                nodes[slot].sample_count += 1;
            }
            nodes[slot].last_heartbeat_ms = now_ms;
        }

        nodes[slot].last_phi = 0.0;
        if (nodes[slot].state == WatchdogState.suspect || nodes[slot].state == WatchdogState.dead) {
            nodes[slot].recovered_from = nodes[slot].state;
            nodes[slot].state = WatchdogState.healthy;
            nodes[slot].pending_recovered = true;
            total_recoveries += 1;
        }
        return true;
    }

    /// Puts `node_id` into `paused` maintenance mode (exempt from timeout/phi failure detection).
    pub fn pauseNode(*_, node_id: u64, now_ms: u64): bool {
        * = now_ms;
        const slot = findSlot(node_id) ?? return false;
        nodes[slot].state = WatchdogState.paused;
        nodes[slot].pending_recovered = false;
        return true;
    }

    /// Resumes a `paused` node back to `healthy` and resets its heartbeat clock to `now_ms`.
    pub fn resumeNode(*_, node_id: u64, now_ms: u64): bool {
        const slot = findSlot(node_id) ?? return false;
        if (nodes[slot].state != WatchdogState.paused) return false;
        nodes[slot].state = WatchdogState.healthy;
        nodes[slot].last_heartbeat_ms = now_ms;
        nodes[slot].last_phi = 0.0;
        nodes[slot].pending_recovered = false;
        return true;
    }

    /// Computes the current Phi-Accrual suspicion score for `node_id` at `now_ms`.
    pub fn phiOf(node_id: u64, now_ms: u64): f64? {
        const slot = findSlot(node_id) ?? return null;
        if (nodes[slot].state == WatchdogState.paused) return 0.0;
        const silent: u64 = if (now_ms > nodes[slot].last_heartbeat_ms)
            (now_ms - nodes[slot].last_heartbeat_ms)
        else
            0;
        const eff: u64 = if (silent > nodes[slot].cfg.grace_ms)
            (silent - nodes[slot].cfg.grace_ms)
        else
            0;
        return computePhiAccrual(
            eff as f64,
            nodes[slot].meanIntervalMs(),
            nodes[slot].stdDevIntervalMs(),
        );
    }

    /// Evaluates all monitored nodes at `now_ms`, advances liveness states, and writes newly
    /// triggered transition events (`became_suspect`, `became_dead`, `recovered`) into `out_events`.
    /// Returns the number of events written (truncation-safe).
    pub fn pollTransitions(
        *_,
        now_ms: u64,
        out_events: *[]WatchdogTransitionEvent,
    ): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < WATCHDOG_MAX_NODES) : (i += 1) {
            if (!nodes[i].active || nodes[i].state == WatchdogState.paused) continue;

            // Emit queued recovery event first if node recovered via heartbeat().
            if (nodes[i].pending_recovered) {
                nodes[i].pending_recovered = false;
                if (written < out_events.len) {
                    out_events[written] = WatchdogTransitionEvent{
                        node_id: nodes[i].node_id,
                        kind: WatchdogEventKind.recovered,
                        prev_state: nodes[i].recovered_from,
                        new_state: WatchdogState.healthy,
                        silent_ms: 0,
                        phi: 0.0,
                    };
                    written += 1;
                }
            }

            const silent: u64 = if (now_ms > nodes[i].last_heartbeat_ms)
                (now_ms - nodes[i].last_heartbeat_ms)
            else
                0;
            const eff: u64 = if (silent > nodes[i].cfg.grace_ms)
                (silent - nodes[i].cfg.grace_ms)
            else
                0;

            const phi = computePhiAccrual(
                eff as f64,
                nodes[i].meanIntervalMs(),
                nodes[i].stdDevIntervalMs(),
            );
            nodes[i].last_phi = phi;

            const prev = nodes[i].state;
            let next = prev;

            if (nodes[i].cfg.mode == WatchdogMode.fixed_timeout) {
                if (eff >= nodes[i].cfg.dead_timeout_ms) {
                    next = WatchdogState.dead;
                } else if (eff >= nodes[i].cfg.suspect_timeout_ms) {
                    next = WatchdogState.suspect;
                }
            } else {
                if (phi >= nodes[i].cfg.phi_dead_threshold) {
                    next = WatchdogState.dead;
                } else if (phi >= nodes[i].cfg.phi_suspect_threshold) {
                    next = WatchdogState.suspect;
                }
            }

            if (next != prev) {
                nodes[i].state = next;
                if (next == WatchdogState.suspect) {
                    total_suspect_events += 1;
                    if (written < out_events.len) {
                        out_events[written] = WatchdogTransitionEvent{
                            node_id: nodes[i].node_id,
                            kind: WatchdogEventKind.became_suspect,
                            prev_state: prev,
                            new_state: next,
                            silent_ms: silent,
                            phi: phi,
                        };
                        written += 1;
                    }
                } else if (next == WatchdogState.dead) {
                    total_dead_events += 1;
                    if (written < out_events.len) {
                        out_events[written] = WatchdogTransitionEvent{
                            node_id: nodes[i].node_id,
                            kind: WatchdogEventKind.became_dead,
                            prev_state: prev,
                            new_state: next,
                            silent_ms: silent,
                            phi: phi,
                        };
                        written += 1;
                    }
                }
            }
        }
        return written;
    }

    /// Returns a read-only copy of `node_id`, or `null` if not registered.
    pub fn getNode(node_id: u64): WatchdogNodeState? {
        const slot = findSlot(node_id) ?? return null;
        return nodes[slot];
    }

    /// Returns a point-in-time operational telemetry snapshot.
    pub fn stats(): WatchdogStats {
        let st = WatchdogStats{
            capacity: WATCHDOG_MAX_NODES,
            total_heartbeats: total_heartbeats,
            total_suspect_events: total_suspect_events,
            total_dead_events: total_dead_events,
            total_recoveries: total_recoveries,
        };
        let i: usize = 0;
        while (i < WATCHDOG_MAX_NODES) : (i += 1) {
            if (!nodes[i].active) continue;
            st.node_count += 1;
            if (nodes[i].state == WatchdogState.healthy) {
                st.healthy_count += 1;
            } else if (nodes[i].state == WatchdogState.suspect) {
                st.suspect_count += 1;
            } else if (nodes[i].state == WatchdogState.dead) {
                st.dead_count += 1;
            } else {
                st.paused_count += 1;
            }
        }
        return st;
    }
}
