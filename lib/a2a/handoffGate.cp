// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Agent Control Handoff Anti-Loop Gate & Rollback FSM) | Created: 2026-11-23
// Pure CP zero-heap-allocation Multi-Agent Handoff Anti-Loop Gate & Rollback State Machine:
//   - Fixed 16-hop audit chain tracking `(from_agent_id, to_agent_id, required_caps, context_hash,
//     reason, ts_ms, rollback_ts_ms, state: committed | rolled_back)`
//   - Multi-layer loop & capability guard (`requestHandoff`):
//       * Blocks self-handoff (`self_handoff`)
//       * Blocks immediate ping-pong oscillation A->B->A (`ping_pong_loop`)
//       * Enforces per-agent revisit ceiling (`revisit_limit_exceeded`)
//       * Enforces maximum active handoff depth (`max_depth_exceeded`)
//       * Validates target agent capability bitmask (`capability_mismatch`, reporting `missing_caps`)
//   - Safe failure rollback (`rollbackLast`) restoring previous valid owner while preserving full
//     audit history for post-mortem inspection
//   - Zero-alloc audit trace formatter (`formatTrace`) & global telemetry snapshot (`HandoffGateStats`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed capacity of the zero-heap handoff audit history ring/table.
pub const HANDOFF_GATE_MAX_HOPS: usize = 16;

/// Default maximum active committed handoff chain depth.
pub const HANDOFF_GATE_DEFAULT_MAX_DEPTH: usize = 8;

/// Default maximum times a single agent may appear in the active ownership chain.
pub const HANDOFF_GATE_DEFAULT_MAX_REVISITS: u8 = 2;

/// Sentinel invalid agent ID.
pub const HANDOFF_GATE_NO_AGENT: u32 = 0;

/// Business reason for requesting a control handoff between agents.
pub enum HandoffReason {
    specialist_escalation,
    load_shed,
    fallback_recovery,
    user_request,
    completion_return,
}

/// Audit lifecycle state of a recorded handoff hop.
pub enum HandoffHopState {
    committed,
    rolled_back,
}

/// Gate verdict returned by `HandoffGate.requestHandoff`.
pub enum HandoffDecisionKind {
    allowed,
    self_handoff,
    ping_pong_loop,
    revisit_limit_exceeded,
    max_depth_exceeded,
    capability_mismatch,
    invalid_agent_id,
    history_full,
}

/// Tunable policy configuration for `HandoffGate`.
pub struct HandoffGateConfig {
    /// Maximum active committed hops allowed (clamped to `HANDOFF_GATE_MAX_HOPS`).
    max_depth: usize = HANDOFF_GATE_DEFAULT_MAX_DEPTH;
    /// Maximum visits allowed per agent across the active ownership chain (>= 1).
    max_revisits_per_agent: u8 = HANDOFF_GATE_DEFAULT_MAX_REVISITS;
    /// When true, blocks immediate A->B->A ping-pong return handoffs.
    block_ping_pong: bool = true;

    /// Return production-balanced default configuration.
    static fn defaults(): HandoffGateConfig {
        return HandoffGateConfig{};
    }
}

/// Single immutable-style audit record of a handoff hop.
pub struct HandoffHopRecord {
    hop_seq: u32 = 0;
    from_agent_id: u32 = 0;
    to_agent_id: u32 = 0;
    required_caps: u64 = 0;
    context_hash: u64 = 0;
    reason: HandoffReason = HandoffReason.specialist_escalation;
    ts_ms: u64 = 0;
    rollback_ts_ms: u64 = 0;
    state: HandoffHopState = HandoffHopState.committed;
}

/// Detailed gate verdict returned by `HandoffGate.requestHandoff`.
pub struct HandoffDecision {
    kind: HandoffDecisionKind = HandoffDecisionKind.allowed;
    allowed: bool = false;
    from_agent_id: u32 = 0;
    to_agent_id: u32 = 0;
    active_depth: usize = 0;
    target_visits: u8 = 0;
    missing_caps: u64 = 0;
}

/// Outcome returned by `HandoffGate.rollbackLast`.
pub struct HandoffRollbackResult {
    rolled_back: bool = false;
    rolled_back_hop_seq: u32 = 0;
    reverted_from_agent_id: u32 = 0;
    restored_owner_agent_id: u32 = 0;
    active_depth: usize = 0;
}

/// Zero-heap global telemetry snapshot for `HandoffGate`.
pub struct HandoffGateStats {
    origin_agent_id: u32 = 0;
    current_owner_id: u32 = 0;
    active_depth: usize = 0;
    total_hops_recorded: usize = 0;
    capacity: usize = HANDOFF_GATE_MAX_HOPS;
    allowed_count: u64 = 0;
    rolled_back_count: u64 = 0;
    blocked_self_count: u64 = 0;
    blocked_ping_pong_count: u64 = 0;
    blocked_revisit_count: u64 = 0;
    blocked_max_depth_count: u64 = 0;
    blocked_capability_count: u64 = 0;
    blocked_other_count: u64 = 0;
}

// ============================================================================
// 2. Pure Formatting Helpers
// ============================================================================

/// Format a `HandoffReason` enum variant as a canonical ASCII token.
pub fn formatHandoffReason(reason: HandoffReason): []const u8 {
    if (reason == HandoffReason.specialist_escalation) return "specialist_escalation";
    if (reason == HandoffReason.load_shed) return "load_shed";
    if (reason == HandoffReason.fallback_recovery) return "fallback_recovery";
    if (reason == HandoffReason.user_request) return "user_request";
    return "completion_return";
}

/// Format a `HandoffDecisionKind` enum variant as a canonical ASCII token.
pub fn formatHandoffDecision(kind: HandoffDecisionKind): []const u8 {
    if (kind == HandoffDecisionKind.allowed) return "allowed";
    if (kind == HandoffDecisionKind.self_handoff) return "self_handoff";
    if (kind == HandoffDecisionKind.ping_pong_loop) return "ping_pong_loop";
    if (kind == HandoffDecisionKind.revisit_limit_exceeded) return "revisit_limit_exceeded";
    if (kind == HandoffDecisionKind.max_depth_exceeded) return "max_depth_exceeded";
    if (kind == HandoffDecisionKind.capability_mismatch) return "capability_mismatch";
    if (kind == HandoffDecisionKind.invalid_agent_id) return "invalid_agent_id";
    return "history_full";
}

/// Format a `HandoffHopState` enum variant as a canonical ASCII token.
pub fn formatHandoffHopState(state: HandoffHopState): []const u8 {
    if (state == HandoffHopState.committed) return "committed";
    return "rolled_back";
}

fn appendBytes(out_buf: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos + i] = s[i];
    }
    pos += s.len;
    return true;
}

fn appendU64(out_buf: *[]u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) : (n += 1) {
            tmp[n] = @as(u8, @intCast('0' + (v % 10)));
            v = v / 10;
        }
    }
    if (pos + n > out_buf.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_buf[pos + i] = tmp[n - 1 - i];
    }
    pos += n;
    return true;
}

// ============================================================================
// 3. HandoffGate — Multi-Agent Anti-Loop Handoff Gate & Rollback FSM
// ============================================================================

/// Zero-heap-allocation multi-agent handoff anti-loop gate and rollback state machine.
pub struct HandoffGate {
    cfg: HandoffGateConfig = HandoffGateConfig{},
    origin_agent_id: u32 = 0;
    current_owner_id: u32 = 0;
    hops: [HANDOFF_GATE_MAX_HOPS]HandoffHopRecord = [HandoffHopRecord{}; HANDOFF_GATE_MAX_HOPS];
    hop_count: usize = 0;
    active_depth: usize = 0;

    allowed_count: u64 = 0;
    rolled_back_count: u64 = 0;
    blocked_self_count: u64 = 0;
    blocked_ping_pong_count: u64 = 0;
    blocked_revisit_count: u64 = 0;
    blocked_max_depth_count: u64 = 0;
    blocked_capability_count: u64 = 0;
    blocked_other_count: u64 = 0;

    /// Initialize a new `HandoffGate` rooted at `initial_agent_id` with default policy.
    /// Returns `null` if `initial_agent_id == 0`.
    static fn init(initial_agent_id: u32): HandoffGate? {
        return HandoffGate.initWithConfig(initial_agent_id, HandoffGateConfig.defaults());
    }

    /// Initialize a new `HandoffGate` with custom configuration.
    /// Returns `null` if `initial_agent_id == 0` or `cfg.max_revisits_per_agent == 0`.
    static fn initWithConfig(initial_agent_id: u32, cfg: HandoffGateConfig): HandoffGate? {
        if (initial_agent_id == HANDOFF_GATE_NO_AGENT) return null;
        if (cfg.max_revisits_per_agent == 0) return null;
        let g = HandoffGate{};
        g.cfg = cfg;
        if (g.cfg.max_depth > HANDOFF_GATE_MAX_HOPS) {
            g.cfg.max_depth = HANDOFF_GATE_MAX_HOPS;
        }
        g.origin_agent_id = initial_agent_id;
        g.current_owner_id = initial_agent_id;
        return g;
    }

    /// Return the current valid control owner agent ID.
    fn currentOwner(): u32 {
        return current_owner_id;
    }

    /// Return the initial origin agent ID that started the session.
    fn originAgent(): u32 {
        return origin_agent_id;
    }

    /// Return the number of active committed handoff hops.
    fn activeDepth(): usize {
        return active_depth;
    }

    /// Return total recorded audit hops (both committed and rolled_back).
    fn historyLen(): usize {
        return hop_count;
    }

    /// Retrieve an audit hop record by 0-based chronological index.
    fn getHop(index: usize): HandoffHopRecord? {
        if (index >= hop_count) return null;
        return hops[index];
    }

    /// Count how many times `agent_id` appears in the active (non-rolled-back) ownership chain.
    fn visitCount(agent_id: u32): u8 {
        if (agent_id == HANDOFF_GATE_NO_AGENT) return 0;
        let visits: u8 = if (origin_agent_id == agent_id) 1 else 0;
        let i: usize = 0;
        while (i < hop_count) : (i += 1) {
            if (hops[i].state == HandoffHopState.committed && hops[i].to_agent_id == agent_id) {
                if (visits < 255) visits += 1;
            }
        }
        return visits;
    }

    /// Find index of the most recent committed hop, or `null` if at origin.
    fn lastCommittedHopIdx(): usize? {
        if (hop_count == 0) return null;
        let i = hop_count;
        while (i > 0) {
            i -= 1;
            if (hops[i].state == HandoffHopState.committed) {
                return i;
            }
        }
        return null;
    }

    /// Evaluate and, if permitted, commit a control handoff to `target_agent_id`.
    /// Enforces self-handoff guard, capability bitmask match, direct ping-pong loop guard,
    /// per-agent revisit ceiling, max active depth, and audit table capacity.
    fn requestHandoff(
        *_,
        target_agent_id: u32,
        target_caps: u64,
        required_caps: u64,
        context_hash: u64,
        reason: HandoffReason,
        now_ms: u64,
    ): HandoffDecision {
        const from_id = current_owner_id;
        const target_visits = visitCount(target_agent_id);

        if (target_agent_id == HANDOFF_GATE_NO_AGENT) {
            blocked_other_count += 1;
            return HandoffDecision{
                kind: HandoffDecisionKind.invalid_agent_id,
                allowed: false,
                from_agent_id: from_id,
                to_agent_id: target_agent_id,
                active_depth: active_depth,
                target_visits: 0,
                missing_caps: 0,
            };
        }

        if (target_agent_id == from_id) {
            blocked_self_count += 1;
            return HandoffDecision{
                kind: HandoffDecisionKind.self_handoff,
                allowed: false,
                from_agent_id: from_id,
                to_agent_id: target_agent_id,
                active_depth: active_depth,
                target_visits: target_visits,
                missing_caps: 0,
            };
        }

        const missing_caps = required_caps & (~target_caps);
        if (missing_caps != 0) {
            blocked_capability_count += 1;
            return HandoffDecision{
                kind: HandoffDecisionKind.capability_mismatch,
                allowed: false,
                from_agent_id: from_id,
                to_agent_id: target_agent_id,
                active_depth: active_depth,
                target_visits: target_visits,
                missing_caps: missing_caps,
            };
        }

        if (cfg.block_ping_pong) {
            if (last_idx := lastCommittedHopIdx()) {
                if (hops[last_idx].from_agent_id == target_agent_id) {
                    blocked_ping_pong_count += 1;
                    return HandoffDecision{
                        kind: HandoffDecisionKind.ping_pong_loop,
                        allowed: false,
                        from_agent_id: from_id,
                        to_agent_id: target_agent_id,
                        active_depth: active_depth,
                        target_visits: target_visits,
                        missing_caps: 0,
                    };
                }
            }
        }

        if (target_visits >= cfg.max_revisits_per_agent) {
            blocked_revisit_count += 1;
            return HandoffDecision{
                kind: HandoffDecisionKind.revisit_limit_exceeded,
                allowed: false,
                from_agent_id: from_id,
                to_agent_id: target_agent_id,
                active_depth: active_depth,
                target_visits: target_visits,
                missing_caps: 0,
            };
        }

        const eff_max_depth = if (cfg.max_depth > HANDOFF_GATE_MAX_HOPS) HANDOFF_GATE_MAX_HOPS else cfg.max_depth;
        if (active_depth >= eff_max_depth) {
            blocked_max_depth_count += 1;
            return HandoffDecision{
                kind: HandoffDecisionKind.max_depth_exceeded,
                allowed: false,
                from_agent_id: from_id,
                to_agent_id: target_agent_id,
                active_depth: active_depth,
                target_visits: target_visits,
                missing_caps: 0,
            };
        }

        if (hop_count >= HANDOFF_GATE_MAX_HOPS) {
            blocked_other_count += 1;
            return HandoffDecision{
                kind: HandoffDecisionKind.history_full,
                allowed: false,
                from_agent_id: from_id,
                to_agent_id: target_agent_id,
                active_depth: active_depth,
                target_visits: target_visits,
                missing_caps: 0,
            };
        }

        const seq = @as(u32, @intCast(hop_count + 1));
        hops[hop_count] = HandoffHopRecord{
            hop_seq: seq,
            from_agent_id: from_id,
            to_agent_id: target_agent_id,
            required_caps: required_caps,
            context_hash: context_hash,
            reason: reason,
            ts_ms: now_ms,
            rollback_ts_ms: 0,
            state: HandoffHopState.committed,
        };
        hop_count += 1;
        active_depth += 1;
        current_owner_id = target_agent_id;
        allowed_count += 1;

        const new_visits: u8 = if (target_visits < 255) target_visits + 1 else 255;
        return HandoffDecision{
            kind: HandoffDecisionKind.allowed,
            allowed: true,
            from_agent_id: from_id,
            to_agent_id: target_agent_id,
            active_depth: active_depth,
            target_visits: new_visits,
            missing_caps: 0,
        };
    }

    /// Safely roll back the most recent committed handoff when the target agent rejects
    /// or fails to take over, restoring control to the previous valid owner while retaining
    /// the audit record with `state = HandoffHopState.rolled_back`.
    fn rollbackLast(*_, now_ms: u64): HandoffRollbackResult {
        const idx_opt = lastCommittedHopIdx();
        if (idx_opt == null) {
            return HandoffRollbackResult{
                rolled_back: false,
                rolled_back_hop_seq: 0,
                reverted_from_agent_id: current_owner_id,
                restored_owner_agent_id: current_owner_id,
                active_depth: active_depth,
            };
        }
        const idx = idx_opt ?? 0;
        const reverted_from = hops[idx].to_agent_id;
        const restored_owner = hops[idx].from_agent_id;
        const hop_seq = hops[idx].hop_seq;

        hops[idx].state = HandoffHopState.rolled_back;
        hops[idx].rollback_ts_ms = now_ms;
        current_owner_id = restored_owner;
        if (active_depth > 0) active_depth -= 1;
        rolled_back_count += 1;

        return HandoffRollbackResult{
            rolled_back: true,
            rolled_back_hop_seq: hop_seq,
            reverted_from_agent_id: reverted_from,
            restored_owner_agent_id: restored_owner,
            active_depth: active_depth,
        };
    }

    /// Format a compact zero-allocation audit trace of the handoff chain into `out_buf`.
    /// Returns bytes written, or `null` if `out_buf` is too small.
    fn formatTrace(out_buf: *[]u8): usize? {
        let pos: usize = 0;
        if (!appendBytes(out_buf, *pos, "origin=#")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, origin_agent_id))) return null;
        if (!appendBytes(out_buf, *pos, " owner=#")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, current_owner_id))) return null;
        if (!appendBytes(out_buf, *pos, " depth=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, @intCast(active_depth)))) return null;
        if (!appendBytes(out_buf, *pos, " hops=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, @intCast(hop_count)))) return null;

        if (hop_count > 0) {
            if (!appendBytes(out_buf, *pos, " |")) return null;
            let i: usize = 0;
            while (i < hop_count) : (i += 1) {
                if (!appendBytes(out_buf, *pos, " #")) return null;
                if (!appendU64(out_buf, *pos, @as(u64, hops[i].from_agent_id))) return null;
                if (!appendBytes(out_buf, *pos, "->#")) return null;
                if (!appendU64(out_buf, *pos, @as(u64, hops[i].to_agent_id))) return null;
                if (!appendBytes(out_buf, *pos, "(")) return null;
                if (!appendBytes(out_buf, *pos, formatHandoffReason(hops[i].reason))) return null;
                if (!appendBytes(out_buf, *pos, ",")) return null;
                if (!appendBytes(out_buf, *pos, formatHandoffHopState(hops[i].state))) return null;
                if (!appendBytes(out_buf, *pos, ")")) return null;
            }
        }
        return pos;
    }

    /// Return a zero-heap global telemetry snapshot.
    fn stats(): HandoffGateStats {
        return HandoffGateStats{
            origin_agent_id: origin_agent_id,
            current_owner_id: current_owner_id,
            active_depth: active_depth,
            total_hops_recorded: hop_count,
            capacity: HANDOFF_GATE_MAX_HOPS,
            allowed_count: allowed_count,
            rolled_back_count: rolled_back_count,
            blocked_self_count: blocked_self_count,
            blocked_ping_pong_count: blocked_ping_pong_count,
            blocked_revisit_count: blocked_revisit_count,
            blocked_max_depth_count: blocked_max_depth_count,
            blocked_capability_count: blocked_capability_count,
            blocked_other_count: blocked_other_count,
        };
    }
}
