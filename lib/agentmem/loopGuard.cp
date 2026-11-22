// Origin: Pure-CP Stdlib (Zero-Alloc Agent ReAct Trajectory & Loop/Oscillation Circuit Guard) | Created: 2026-11-23
// Pure CP zero-heap-allocation AI Agent ReAct Trajectory & Loop/Oscillation Circuit Breaker:
//   - Fixed 64-step circular trajectory buffer tracking `(step_num, tool_id, args_hash, obs_hash,
//     obs_ok, made_progress, tokens_spent)` with deterministic FNV-1a hashing helper
//   - Multi-dimensional real-time anomaly detector (`recordStep`):
//       * Exact repetition loop detection (N consecutive identical `(tool_id, args_hash)`)
//       * Periodic oscillation detection (exact A-B-A-B period-2 & A-B-C-A-B-C period-3 cycles)
//       * Consecutive tool failure streak & no-progress stall detection
//       * Hard max-step & per-trajectory token budget circuit breaker
//   - Temporary per-tool cooldown blacklist (`forbidTool` / `isToolForbidden`) to force strategy pivot
//   - Zero-heap telemetry snapshot (`LoopGuardStats`) & zero-alloc audit summary formatter (`formatAuditSummary`)

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed capacity of the circular ReAct step trajectory buffer.
pub const LOOP_GUARD_MAX_STEPS: usize = 64;

/// Maximum number of concurrent tools tracked in the temporary cooldown blacklist.
pub const LOOP_GUARD_MAX_COOLDOWNS: usize = 32;

/// Default maximum steps per trajectory before hard abort.
pub const LOOP_GUARD_DEFAULT_MAX_STEPS: u32 = 64;

/// Default per-trajectory token budget before hard abort.
pub const LOOP_GUARD_DEFAULT_MAX_TOKENS: u64 = 128_000;

/// Intervention action recommended by `AgentLoopGuard.recordStep`.
pub enum LoopAction {
    continue_run,
    inject_reflection,
    forbid_tool,
    escalate_human,
    abort_loop,
}

/// Primary trigger reason explaining why an intervention was emitted.
pub enum LoopTriggerReason {
    none,
    exact_repeat,
    cycle_2_oscillation,
    cycle_3_oscillation,
    consecutive_failures,
    stalled_progress,
    tool_in_cooldown,
    max_steps_exceeded,
    token_budget_exceeded,
}

/// Single ReAct execution step recorded in the circular trajectory ring.
pub struct TrajectoryStep {
    step_num: u32 = 0;
    tool_id: u32 = 0;
    args_hash: u64 = 0;
    obs_hash: u64 = 0;
    obs_ok: bool = true;
    made_progress: bool = false;
    tokens_spent: u32 = 0;
}

/// Tunable thresholds for `AgentLoopGuard`.
pub struct LoopGuardConfig {
    exact_repeat_limit: u32 = 3;
    consecutive_fail_limit: u32 = 3;
    stall_limit: u32 = 5;
    max_steps: u32 = LOOP_GUARD_DEFAULT_MAX_STEPS;
    max_tokens: u64 = LOOP_GUARD_DEFAULT_MAX_TOKENS;
    default_cooldown_steps: u32 = 3;
    auto_cooldown_on_loop: bool = true;
    max_interventions_before_abort: u32 = 6;

    /// Return production-balanced default guard configuration.
    static fn defaults(): LoopGuardConfig {
        return LoopGuardConfig{};
    }
}

/// Real-time intervention verdict returned by `AgentLoopGuard.recordStep`.
pub struct LoopGuardDecision {
    action: LoopAction = LoopAction.continue_run;
    reason: LoopTriggerReason = LoopTriggerReason.none;
    step_num: u32 = 0;
    offending_tool_id: u32 = 0;
    cycle_period: u8 = 0;
    repeat_streak: u32 = 0;
    failure_streak: u32 = 0;
    stall_streak: u32 = 0;
    cooldown_remaining: u32 = 0;
    tokens_total: u64 = 0;
}

/// Temporary cooldown entry blocking a looping tool for `remaining_steps` turns.
pub struct ToolCooldownEntry {
    active: bool = false;
    tool_id: u32 = 0;
    remaining_steps: u32 = 0;
    trigger_count: u32 = 0;
}

/// Zero-heap global trajectory telemetry snapshot.
pub struct LoopGuardStats {
    total_steps: u32 = 0;
    ring_count: usize = 0;
    ring_capacity: usize = LOOP_GUARD_MAX_STEPS;
    total_tokens: u64 = 0;
    progress_steps: u32 = 0;
    failed_steps: u32 = 0;
    exact_repeat_triggers: u32 = 0;
    oscillation_2_triggers: u32 = 0;
    oscillation_3_triggers: u32 = 0;
    failure_streak_triggers: u32 = 0;
    stall_triggers: u32 = 0;
    cooldown_violations: u32 = 0;
    budget_aborts: u32 = 0;
    total_interventions: u32 = 0;
    active_cooldowns: usize = 0;
    current_failure_streak: u32 = 0;
    current_stall_streak: u32 = 0;
}

// ============================================================================
// 2. Deterministic Hashing & Formatting Helpers (Zero-Alloc)
// ============================================================================

/// Compute a deterministic 64-bit FNV-1a fingerprint for tool arguments or observation text.
pub fn hashTrajectoryBytes(data: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h = h ^ @as(u64, data[i]);
        h = h *% 1099511628211;
    }
    return h;
}

fn sameCallSig(a: TrajectoryStep, b: TrajectoryStep): bool {
    return a.tool_id == b.tool_id and a.args_hash == b.args_hash;
}

fn appendStr(out_buf: *[]u8, pos: *usize, s: []const u8): bool {
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

/// Format a `LoopAction` enum variant as a canonical ASCII token.
pub fn formatLoopAction(action: LoopAction): []const u8 {
    if (action == LoopAction.continue_run) return "continue_run";
    if (action == LoopAction.inject_reflection) return "inject_reflection";
    if (action == LoopAction.forbid_tool) return "forbid_tool";
    if (action == LoopAction.escalate_human) return "escalate_human";
    return "abort_loop";
}

/// Format a `LoopTriggerReason` enum variant as a canonical ASCII token.
pub fn formatLoopReason(reason: LoopTriggerReason): []const u8 {
    if (reason == LoopTriggerReason.none) return "none";
    if (reason == LoopTriggerReason.exact_repeat) return "exact_repeat";
    if (reason == LoopTriggerReason.cycle_2_oscillation) return "cycle_2_oscillation";
    if (reason == LoopTriggerReason.cycle_3_oscillation) return "cycle_3_oscillation";
    if (reason == LoopTriggerReason.consecutive_failures) return "consecutive_failures";
    if (reason == LoopTriggerReason.stalled_progress) return "stalled_progress";
    if (reason == LoopTriggerReason.tool_in_cooldown) return "tool_in_cooldown";
    if (reason == LoopTriggerReason.max_steps_exceeded) return "max_steps_exceeded";
    return "token_budget_exceeded";
}

// ============================================================================
// 3. AgentLoopGuard — Zero-Heap ReAct Trajectory & Loop Circuit Breaker
// ============================================================================

/// Zero-heap-allocation ReAct trajectory ring buffer & loop/oscillation circuit breaker.
pub struct AgentLoopGuard {
    config: LoopGuardConfig = LoopGuardConfig{},
    ring: [LOOP_GUARD_MAX_STEPS]TrajectoryStep = [TrajectoryStep{}; LOOP_GUARD_MAX_STEPS];
    ring_head: usize = 0;
    ring_count: usize = 0;

    cooldowns: [LOOP_GUARD_MAX_COOLDOWNS]ToolCooldownEntry = [ToolCooldownEntry{}; LOOP_GUARD_MAX_COOLDOWNS];

    total_steps: u32 = 0;
    total_tokens: u64 = 0;
    progress_steps: u32 = 0;
    failed_steps: u32 = 0;
    failure_streak: u32 = 0;
    stall_streak: u32 = 0;

    exact_repeat_triggers: u32 = 0;
    oscillation_2_triggers: u32 = 0;
    oscillation_3_triggers: u32 = 0;
    failure_streak_triggers: u32 = 0;
    stall_triggers: u32 = 0;
    cooldown_violations: u32 = 0;
    budget_aborts: u32 = 0;
    total_interventions: u32 = 0;

    last_action: LoopAction = LoopAction.continue_run;
    last_reason: LoopTriggerReason = LoopTriggerReason.none;

    /// Initialize guard with production defaults.
    static fn init(): AgentLoopGuard {
        return AgentLoopGuard{};
    }

    /// Initialize guard with custom thresholds (normalizes zero limits safely).
    static fn initWithConfig(cfg: LoopGuardConfig): AgentLoopGuard {
        let g = AgentLoopGuard{};
        g.config = cfg;
        if (g.config.exact_repeat_limit < 2) g.config.exact_repeat_limit = 2;
        if (g.config.consecutive_fail_limit < 1) g.config.consecutive_fail_limit = 1;
        if (g.config.stall_limit < 1) g.config.stall_limit = 1;
        if (g.config.default_cooldown_steps < 1) g.config.default_cooldown_steps = 1;
        return g;
    }

    /// Reset trajectory history, cooldown blacklist, and counters while preserving config.
    pub fn reset(*_) {
        const saved_cfg = config;
        let i: usize = 0;
        while (i < LOOP_GUARD_MAX_STEPS) : (i += 1) {
            ring[i] = TrajectoryStep{};
        }
        i = 0;
        while (i < LOOP_GUARD_MAX_COOLDOWNS) : (i += 1) {
            cooldowns[i] = ToolCooldownEntry{};
        }
        config = saved_cfg;
        ring_head = 0;
        ring_count = 0;
        total_steps = 0;
        total_tokens = 0;
        progress_steps = 0;
        failed_steps = 0;
        failure_streak = 0;
        stall_streak = 0;
        exact_repeat_triggers = 0;
        oscillation_2_triggers = 0;
        oscillation_3_triggers = 0;
        failure_streak_triggers = 0;
        stall_triggers = 0;
        cooldown_violations = 0;
        budget_aborts = 0;
        total_interventions = 0;
        last_action = LoopAction.continue_run;
        last_reason = LoopTriggerReason.none;
    }

    /// Return a recent step from the circular ring (`offset_from_latest == 0` is the latest step).
    pub fn getRecentStep(offset_from_latest: usize): TrajectoryStep? {
        if (offset_from_latest >= ring_count) return null;
        // ring_head points to the next write slot; latest entry is at (ring_head - 1) mod MAX.
        const idx = (ring_head + LOOP_GUARD_MAX_STEPS - 1 - offset_from_latest) % LOOP_GUARD_MAX_STEPS;
        return ring[idx];
    }

    /// Place `tool_id` on temporary cooldown for `cooldown_steps` turns.
    /// Returns false only if `cooldown_steps == 0` or all 32 cooldown slots are occupied.
    pub fn forbidTool(*_, tool_id: u32, cooldown_steps: u32): bool {
        if (cooldown_steps == 0) return false;
        let free_slot: usize = LOOP_GUARD_MAX_COOLDOWNS;
        let i: usize = 0;
        while (i < LOOP_GUARD_MAX_COOLDOWNS) : (i += 1) {
            if (cooldowns[i].active) {
                if (cooldowns[i].tool_id == tool_id) {
                    if (cooldown_steps > cooldowns[i].remaining_steps) {
                        cooldowns[i].remaining_steps = cooldown_steps;
                    }
                    cooldowns[i].trigger_count += 1;
                    return true;
                }
            } else if (free_slot == LOOP_GUARD_MAX_COOLDOWNS) {
                free_slot = i;
            }
        }
        if (free_slot == LOOP_GUARD_MAX_COOLDOWNS) return false;
        cooldowns[free_slot] = ToolCooldownEntry{
            active: true,
            tool_id: tool_id,
            remaining_steps: cooldown_steps,
            trigger_count: 1,
        };
        return true;
    }

    /// Check whether `tool_id` is currently blocked by an active cooldown.
    pub fn isToolForbidden(tool_id: u32): bool {
        return toolCooldownRemaining(tool_id) > 0;
    }

    /// Return remaining cooldown steps for `tool_id` (0 if not in cooldown).
    pub fn toolCooldownRemaining(tool_id: u32): u32 {
        let i: usize = 0;
        while (i < LOOP_GUARD_MAX_COOLDOWNS) : (i += 1) {
            if (cooldowns[i].active and cooldowns[i].tool_id == tool_id) {
                return cooldowns[i].remaining_steps;
            }
        }
        return 0;
    }

    /// Manually clear cooldown for `tool_id`.
    pub fn clearToolCooldown(*_, tool_id: u32): bool {
        let i: usize = 0;
        while (i < LOOP_GUARD_MAX_COOLDOWNS) : (i += 1) {
            if (cooldowns[i].active and cooldowns[i].tool_id == tool_id) {
                cooldowns[i] = ToolCooldownEntry{};
                return true;
            }
        }
        return false;
    }

    /// Count active cooldown entries.
    fn activeCooldownCount(): usize {
        let n: usize = 0;
        let i: usize = 0;
        while (i < LOOP_GUARD_MAX_COOLDOWNS) : (i += 1) {
            if (cooldowns[i].active) n += 1;
        }
        return n;
    }

    /// Decrement cooldown counters for all tools except `skip_tool_id` (when `has_skip` is true).
    fn tickCooldowns(*_, has_skip: bool, skip_tool_id: u32) {
        let i: usize = 0;
        while (i < LOOP_GUARD_MAX_COOLDOWNS) : (i += 1) {
            if (!cooldowns[i].active) continue;
            if (has_skip and cooldowns[i].tool_id == skip_tool_id) continue;
            if (cooldowns[i].remaining_steps <= 1) {
                cooldowns[i] = ToolCooldownEntry{};
            } else {
                cooldowns[i].remaining_steps -= 1;
            }
        }
    }

    /// Count consecutive trailing steps matching the latest `(tool_id, args_hash)`.
    fn countTrailingExactRepeats(): u32 {
        if (ring_count == 0) return 0;
        const latest = getRecentStep(0) ?? TrajectoryStep{};
        let streak: u32 = 1;
        let off: usize = 1;
        while (off < ring_count) : (off += 1) {
            const prev = getRecentStep(off) ?? TrajectoryStep{};
            if (sameCallSig(latest, prev)) {
                streak += 1;
            } else {
                break;
            }
        }
        return streak;
    }

    /// Detect exact A-B-A-B period-2 oscillation across the latest 4 steps (`A != B`).
    fn hasPeriod2Oscillation(): bool {
        if (ring_count < 4) return false;
        const s3 = getRecentStep(0) ?? TrajectoryStep{}; // latest (B)
        const s2 = getRecentStep(1) ?? TrajectoryStep{}; // A
        const s1 = getRecentStep(2) ?? TrajectoryStep{}; // B
        const s0 = getRecentStep(3) ?? TrajectoryStep{}; // A
        return sameCallSig(s0, s2) and sameCallSig(s1, s3) and !sameCallSig(s0, s1);
    }

    /// Detect exact A-B-C-A-B-C period-3 oscillation across the latest 6 steps (not all identical).
    fn hasPeriod3Oscillation(): bool {
        if (ring_count < 6) return false;
        const s5 = getRecentStep(0) ?? TrajectoryStep{}; // C
        const s4 = getRecentStep(1) ?? TrajectoryStep{}; // B
        const s3 = getRecentStep(2) ?? TrajectoryStep{}; // A
        const s2 = getRecentStep(3) ?? TrajectoryStep{}; // C
        const s1 = getRecentStep(4) ?? TrajectoryStep{}; // B
        const s0 = getRecentStep(5) ?? TrajectoryStep{}; // A
        if (!sameCallSig(s0, s3) or !sameCallSig(s1, s4) or !sameCallSig(s2, s5)) {
            return false;
        }
        // Exclude degenerate A-A-A-A-A-A (handled by exact repeat detector).
        return !sameCallSig(s0, s1) or !sameCallSig(s1, s2);
    }

    /// Record a ReAct execution step and evaluate all loop, oscillation, stall, cooldown & budget guards.
    pub fn recordStep(
        *_,
        tool_id: u32,
        args_hash: u64,
        obs_hash: u64,
        obs_ok: bool,
        made_progress: bool,
        tokens_spent: u32,
    ): LoopGuardDecision {
        const was_forbidden_rem = toolCooldownRemaining(tool_id);

        total_steps += 1;
        total_tokens += @as(u64, tokens_spent);

        const step = TrajectoryStep{
            step_num: total_steps,
            tool_id: tool_id,
            args_hash: args_hash,
            obs_hash: obs_hash,
            obs_ok: obs_ok,
            made_progress: made_progress,
            tokens_spent: tokens_spent,
        };
        ring[ring_head] = step;
        ring_head = (ring_head + 1) % LOOP_GUARD_MAX_STEPS;
        if (ring_count < LOOP_GUARD_MAX_STEPS) {
            ring_count += 1;
        }

        if (obs_ok) {
            failure_streak = 0;
        } else {
            failed_steps += 1;
            failure_streak += 1;
        }

        if (made_progress) {
            progress_steps += 1;
            stall_streak = 0;
        } else {
            stall_streak += 1;
        }

        // Tick existing cooldowns (do not reward a tool that just violated its own cooldown).
        tickCooldowns(was_forbidden_rem > 0, tool_id);

        const repeat_streak = countTrailingExactRepeats();

        // 1. Hard budget guard: token limit exceeded
        if (config.max_tokens > 0 and total_tokens > config.max_tokens) {
            budget_aborts += 1;
            total_interventions += 1;
            last_action = LoopAction.abort_loop;
            last_reason = LoopTriggerReason.token_budget_exceeded;
            return LoopGuardDecision{
                action: LoopAction.abort_loop,
                reason: LoopTriggerReason.token_budget_exceeded,
                step_num: total_steps,
                offending_tool_id: tool_id,
                repeat_streak: repeat_streak,
                failure_streak: failure_streak,
                stall_streak: stall_streak,
                cooldown_remaining: toolCooldownRemaining(tool_id),
                tokens_total: total_tokens,
            };
        }

        // 2. Hard budget guard: max steps exceeded
        if (config.max_steps > 0 and total_steps >= config.max_steps) {
            budget_aborts += 1;
            total_interventions += 1;
            last_action = LoopAction.abort_loop;
            last_reason = LoopTriggerReason.max_steps_exceeded;
            return LoopGuardDecision{
                action: LoopAction.abort_loop,
                reason: LoopTriggerReason.max_steps_exceeded,
                step_num: total_steps,
                offending_tool_id: tool_id,
                repeat_streak: repeat_streak,
                failure_streak: failure_streak,
                stall_streak: stall_streak,
                cooldown_remaining: toolCooldownRemaining(tool_id),
                tokens_total: total_tokens,
            };
        }

        // 3. Cooldown blacklist violation
        if (was_forbidden_rem > 0) {
            cooldown_violations += 1;
            total_interventions += 1;
            const act = if (config.max_interventions_before_abort > 0 and total_interventions >= config.max_interventions_before_abort)
                LoopAction.abort_loop
            else
                LoopAction.forbid_tool;
            last_action = act;
            last_reason = LoopTriggerReason.tool_in_cooldown;
            return LoopGuardDecision{
                action: act,
                reason: LoopTriggerReason.tool_in_cooldown,
                step_num: total_steps,
                offending_tool_id: tool_id,
                repeat_streak: repeat_streak,
                failure_streak: failure_streak,
                stall_streak: stall_streak,
                cooldown_remaining: was_forbidden_rem,
                tokens_total: total_tokens,
            };
        }

        // 4. Exact repetition loop: N consecutive identical (tool_id, args_hash)
        if (repeat_streak >= config.exact_repeat_limit) {
            exact_repeat_triggers += 1;
            total_interventions += 1;
            if (config.auto_cooldown_on_loop) {
                * = forbidTool(tool_id, config.default_cooldown_steps);
            }
            const act = if (config.max_interventions_before_abort > 0 and total_interventions >= config.max_interventions_before_abort)
                LoopAction.abort_loop
            else
                LoopAction.forbid_tool;
            last_action = act;
            last_reason = LoopTriggerReason.exact_repeat;
            return LoopGuardDecision{
                action: act,
                reason: LoopTriggerReason.exact_repeat,
                step_num: total_steps,
                offending_tool_id: tool_id,
                cycle_period: 1,
                repeat_streak: repeat_streak,
                failure_streak: failure_streak,
                stall_streak: stall_streak,
                cooldown_remaining: toolCooldownRemaining(tool_id),
                tokens_total: total_tokens,
            };
        }

        // 5. Period-2 oscillation: A-B-A-B
        if (hasPeriod2Oscillation()) {
            oscillation_2_triggers += 1;
            total_interventions += 1;
            if (config.auto_cooldown_on_loop) {
                * = forbidTool(tool_id, config.default_cooldown_steps);
            }
            const act = if (config.max_interventions_before_abort > 0 and total_interventions >= config.max_interventions_before_abort)
                LoopAction.abort_loop
            else
                LoopAction.inject_reflection;
            last_action = act;
            last_reason = LoopTriggerReason.cycle_2_oscillation;
            return LoopGuardDecision{
                action: act,
                reason: LoopTriggerReason.cycle_2_oscillation,
                step_num: total_steps,
                offending_tool_id: tool_id,
                cycle_period: 2,
                repeat_streak: repeat_streak,
                failure_streak: failure_streak,
                stall_streak: stall_streak,
                cooldown_remaining: toolCooldownRemaining(tool_id),
                tokens_total: total_tokens,
            };
        }

        // 6. Period-3 oscillation: A-B-C-A-B-C
        if (hasPeriod3Oscillation()) {
            oscillation_3_triggers += 1;
            total_interventions += 1;
            if (config.auto_cooldown_on_loop) {
                * = forbidTool(tool_id, config.default_cooldown_steps);
            }
            const act = if (config.max_interventions_before_abort > 0 and total_interventions >= config.max_interventions_before_abort)
                LoopAction.abort_loop
            else
                LoopAction.inject_reflection;
            last_action = act;
            last_reason = LoopTriggerReason.cycle_3_oscillation;
            return LoopGuardDecision{
                action: act,
                reason: LoopTriggerReason.cycle_3_oscillation,
                step_num: total_steps,
                offending_tool_id: tool_id,
                cycle_period: 3,
                repeat_streak: repeat_streak,
                failure_streak: failure_streak,
                stall_streak: stall_streak,
                cooldown_remaining: toolCooldownRemaining(tool_id),
                tokens_total: total_tokens,
            };
        }

        // 7. Consecutive tool failures
        if (failure_streak >= config.consecutive_fail_limit) {
            failure_streak_triggers += 1;
            total_interventions += 1;
            const act = if (failure_streak >= config.consecutive_fail_limit * 2)
                LoopAction.escalate_human
            else
                LoopAction.inject_reflection;
            last_action = act;
            last_reason = LoopTriggerReason.consecutive_failures;
            return LoopGuardDecision{
                action: act,
                reason: LoopTriggerReason.consecutive_failures,
                step_num: total_steps,
                offending_tool_id: tool_id,
                repeat_streak: repeat_streak,
                failure_streak: failure_streak,
                stall_streak: stall_streak,
                cooldown_remaining: toolCooldownRemaining(tool_id),
                tokens_total: total_tokens,
            };
        }

        // 8. No-progress stall detection
        if (stall_streak >= config.stall_limit) {
            stall_triggers += 1;
            total_interventions += 1;
            const act = if (stall_streak >= config.stall_limit + 2)
                LoopAction.escalate_human
            else
                LoopAction.inject_reflection;
            last_action = act;
            last_reason = LoopTriggerReason.stalled_progress;
            return LoopGuardDecision{
                action: act,
                reason: LoopTriggerReason.stalled_progress,
                step_num: total_steps,
                offending_tool_id: tool_id,
                repeat_streak: repeat_streak,
                failure_streak: failure_streak,
                stall_streak: stall_streak,
                cooldown_remaining: toolCooldownRemaining(tool_id),
                tokens_total: total_tokens,
            };
        }

        last_action = LoopAction.continue_run;
        last_reason = LoopTriggerReason.none;
        return LoopGuardDecision{
            action: LoopAction.continue_run,
            reason: LoopTriggerReason.none,
            step_num: total_steps,
            offending_tool_id: 0,
            repeat_streak: repeat_streak,
            failure_streak: failure_streak,
            stall_streak: stall_streak,
            cooldown_remaining: 0,
            tokens_total: total_tokens,
        };
    }

    /// Return a zero-heap global telemetry snapshot.
    pub fn stats(): LoopGuardStats {
        return LoopGuardStats{
            total_steps: total_steps,
            ring_count: ring_count,
            ring_capacity: LOOP_GUARD_MAX_STEPS,
            total_tokens: total_tokens,
            progress_steps: progress_steps,
            failed_steps: failed_steps,
            exact_repeat_triggers: exact_repeat_triggers,
            oscillation_2_triggers: oscillation_2_triggers,
            oscillation_3_triggers: oscillation_3_triggers,
            failure_streak_triggers: failure_streak_triggers,
            stall_triggers: stall_triggers,
            cooldown_violations: cooldown_violations,
            budget_aborts: budget_aborts,
            total_interventions: total_interventions,
            active_cooldowns: activeCooldownCount(),
            current_failure_streak: failure_streak,
            current_stall_streak: stall_streak,
        };
    }

    /// Format a compact zero-allocation trajectory audit summary into `out_buf`.
    /// Returns the number of bytes written, or `null` if `out_buf` is too small.
    pub fn formatAuditSummary(out_buf: *[]u8): usize? {
        let pos: usize = 0;
        if (!appendStr(out_buf, *pos, "steps=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, total_steps))) return null;
        if (!appendStr(out_buf, *pos, " tokens=")) return null;
        if (!appendU64(out_buf, *pos, total_tokens)) return null;
        if (!appendStr(out_buf, *pos, " progress=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, progress_steps))) return null;
        if (!appendStr(out_buf, *pos, " failed=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, failed_steps))) return null;
        if (!appendStr(out_buf, *pos, " repeats=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, exact_repeat_triggers))) return null;
        if (!appendStr(out_buf, *pos, " osc2=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, oscillation_2_triggers))) return null;
        if (!appendStr(out_buf, *pos, " osc3=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, oscillation_3_triggers))) return null;
        if (!appendStr(out_buf, *pos, " stalls=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, stall_triggers))) return null;
        if (!appendStr(out_buf, *pos, " cooldowns=")) return null;
        if (!appendU64(out_buf, *pos, @as(u64, @intCast(activeCooldownCount())))) return null;
        if (!appendStr(out_buf, *pos, " last_action=")) return null;
        if (!appendStr(out_buf, *pos, formatLoopAction(last_action))) return null;
        if (!appendStr(out_buf, *pos, " last_reason=")) return null;
        if (!appendStr(out_buf, *pos, formatLoopReason(last_reason))) return null;
        return pos;
    }
}
