// Origin: Pure-CP Stdlib (AI Agent Budget, LoopGuard & ContextWindowAdvisor) | Created: 2026-11-22

const FNV1A_OFFSET_BASIS: u64 = 0xCBF29CE484222325;
const FNV1A_PRIME: u64 = 0x00000100000001B3;
const ONE_MILLION: u64 = 1000000;

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

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV1A_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) {
        h = (h ^ (data[i] as u64)) *% FNV1A_PRIME;
        i += 1;
    }
    return h;
}

/// Multi-dimensional execution budget status for an AI Agent run.
pub enum BudgetStatus {
    Ok,
    SoftLimit,
    ExceededTokens,
    ExceededCost,
    ExceededSteps,
    ExceededTime,
}

/// Exact integer pricing table in micro-USD per 1,000,000 tokens (`1 USD = 1_000_000 micro_usd`).
pub struct ModelPricing {
    prompt_micro_usd_per_1m: u64,
    completion_micro_usd_per_1m: u64,
    cached_micro_usd_per_1m: u64,

    static fn init(prompt_per_1m: u64, completion_per_1m: u64, cached_per_1m: u64): ModelPricing {
        return ModelPricing {
            prompt_micro_usd_per_1m: prompt_per_1m,
            completion_micro_usd_per_1m: completion_per_1m,
            cached_micro_usd_per_1m: cached_per_1m,
        };
    }

    /// Computes exact rounded cost in micro-USD with zero floating-point drift and overflow protection.
    fn computeMicroUsd(prompt_tokens: u64, completion_tokens: u64, cached_tokens: u64): u64 {
        const c1 = scaledMicroCost(prompt_tokens, _.prompt_micro_usd_per_1m);
        const c2 = scaledMicroCost(completion_tokens, _.completion_micro_usd_per_1m);
        const c3 = scaledMicroCost(cached_tokens, _.cached_micro_usd_per_1m);
        return satAddU64(satAddU64(c1, c2), c3);
    }
}

fn scaledMicroCost(tokens: u64, rate_per_1m: u64): u64 {
    if (tokens == 0 || rate_per_1m == 0) {
        return 0;
    }
    const whole_m = tokens / ONE_MILLION;
    const rem = tokens % ONE_MILLION;
    const whole_cost = if (whole_m > 0 && rate_per_1m > 0xFFFFFFFFFFFFFFFF / whole_m)
        0xFFFFFFFFFFFFFFFF
    else
        whole_m * rate_per_1m;
    const rem_cost = if (rem > 0 && rate_per_1m > (0xFFFFFFFFFFFFFFFF - 500000) / rem)
        (rem / 1000) * (rate_per_1m / 1000)
    else
        (rem * rate_per_1m + 500000) / ONE_MILLION;
    return satAddU64(whole_cost, rem_cost);
}

/// Hard and soft limits for `AgentBudget` (`0` disables a specific dimension).
pub struct BudgetLimits {
    max_total_tokens: u64,
    max_cost_micro_usd: u64,
    max_steps: u32,
    max_elapsed_ms: u64,
    soft_limit_pct: u8,
}

fn reachedSoftPct(used: u64, limit: u64, pct: u8): bool {
    if (limit == 0 || pct == 0) {
        return false;
    }
    const threshold = (limit / 100) * (pct as u64) + ((limit % 100) * (pct as u64)) / 100;
    return used >= threshold;
}

/// Zero-alloc multi-dimensional AI Agent budget tracker (tokens, micro-USD cost, steps, wall time).
pub struct AgentBudget {
    limits: BudgetLimits,
    prompt_tokens: u64,
    completion_tokens: u64,
    cached_tokens: u64,
    cost_micro_usd: u64,
    steps: u32,
    elapsed_ms: u64,

    /// Creates a new `AgentBudget`. Returns `null` if `soft_limit_pct > 99`.
    static fn init(limits: BudgetLimits): AgentBudget? {
        if (limits.soft_limit_pct > 99) {
            return null;
        }
        return AgentBudget {
            limits: limits,
            prompt_tokens: 0,
            completion_tokens: 0,
            cached_tokens: 0,
            cost_micro_usd: 0,
            steps: 0,
            elapsed_ms: 0,
        };
    }

    /// Resets accumulated usage counters while preserving configured limits.
    fn reset(*_) {
        _.prompt_tokens = 0;
        _.completion_tokens = 0;
        _.cached_tokens = 0;
        _.cost_micro_usd = 0;
        _.steps = 0;
        _.elapsed_ms = 0;
    }

    /// Returns total accumulated tokens (`prompt + completion + cached`).
    fn totalTokens(): u64 {
        return satAddU64(satAddU64(_.prompt_tokens, _.completion_tokens), _.cached_tokens);
    }

    /// Returns remaining token allowance (`0` if unlimited or exhausted).
    fn remainingTokens(): u64 {
        if (_.limits.max_total_tokens == 0) return 0;
        const tot = _.totalTokens();
        return if (tot >= _.limits.max_total_tokens) 0 else (_.limits.max_total_tokens - tot);
    }

    /// Returns remaining micro-USD cost allowance (`0` if unlimited or exhausted).
    fn remainingCostMicroUsd(): u64 {
        if (_.limits.max_cost_micro_usd == 0) return 0;
        return if (_.cost_micro_usd >= _.limits.max_cost_micro_usd) 0 else (_.limits.max_cost_micro_usd - _.cost_micro_usd);
    }

    /// Returns remaining step allowance (`0` if unlimited or exhausted).
    fn remainingSteps(): u32 {
        if (_.limits.max_steps == 0) return 0;
        return if (_.steps >= _.limits.max_steps) 0 else (_.limits.max_steps - _.steps);
    }

    /// Evaluates current budget status across all 4 dimensions.
    fn status(): BudgetStatus {
        if (_.limits.max_elapsed_ms > 0 && _.elapsed_ms > _.limits.max_elapsed_ms) {
            return BudgetStatus.ExceededTime;
        }
        if (_.limits.max_cost_micro_usd > 0 && _.cost_micro_usd > _.limits.max_cost_micro_usd) {
            return BudgetStatus.ExceededCost;
        }
        const tot = _.totalTokens();
        if (_.limits.max_total_tokens > 0 && tot > _.limits.max_total_tokens) {
            return BudgetStatus.ExceededTokens;
        }
        if (_.limits.max_steps > 0 && _.steps > _.limits.max_steps) {
            return BudgetStatus.ExceededSteps;
        }
        const pct = _.limits.soft_limit_pct;
        if (
            reachedSoftPct(tot, _.limits.max_total_tokens, pct) ||
            reachedSoftPct(_.cost_micro_usd, _.limits.max_cost_micro_usd, pct) ||
            reachedSoftPct(_.steps as u64, _.limits.max_steps as u64, pct) ||
            reachedSoftPct(_.elapsed_ms, _.limits.max_elapsed_ms, pct)
        ) {
            return BudgetStatus.SoftLimit;
        }
        return BudgetStatus.Ok;
    }

    /// Returns true if any hard limit has been exceeded.
    fn isExhausted(): bool {
        const st = _.status();
        return st != BudgetStatus.Ok && st != BudgetStatus.SoftLimit;
    }

    /// Records one Agent turn/tool step and returns the updated `BudgetStatus`.
    fn recordStep(
        *_,
        prompt_toks: u64,
        completion_toks: u64,
        cached_toks: u64,
        pricing: ModelPricing,
        step_elapsed_ms: u64,
    ): BudgetStatus {
        _.prompt_tokens = satAddU64(_.prompt_tokens, prompt_toks);
        _.completion_tokens = satAddU64(_.completion_tokens, completion_toks);
        _.cached_tokens = satAddU64(_.cached_tokens, cached_toks);
        const step_cost = pricing.computeMicroUsd(prompt_toks, completion_toks, cached_toks);
        _.cost_micro_usd = satAddU64(_.cost_micro_usd, step_cost);
        _.steps = satAddU32(_.steps, 1);
        _.elapsed_ms = satAddU64(_.elapsed_ms, step_elapsed_ms);
        return _.status();
    }
}

/// Action returned by `LoopGuard.recordAndCheck`.
pub enum LoopAction {
    Allow,
    RepeatLoop,
    PingPongLoop,
}

/// Compact 128-bit signature of a tool invocation `(tool_hash, args_hash)`.
pub struct CallSignature {
    tool_hash: u64,
    args_hash: u64,

    /// Computes a deterministic FNV-1a signature from `tool_name` and serialized `args`.
    static fn fromBytes(tool_name: []const u8, args: []const u8): CallSignature {
        return CallSignature {
            tool_hash: fnv1a64(tool_name),
            args_hash: fnv1a64(args),
        };
    }

    fn eql(other: CallSignature): bool {
        return _.tool_hash == other.tool_hash && _.args_hash == other.args_hash;
    }
}

/// Zero-alloc fixed ring-buffer detector for consecutive repeat loops (`A-A-A`) and ping-pong deadlocks (`A-B-A-B`).
pub struct LoopGuard {
    ring: []CallSignature,
    write_pos: usize,
    count: usize,
    max_consecutive_repeats: usize,
    max_ping_pong_cycles: usize,

    /// Initializes `LoopGuard` over caller-provided `ring_buf`.
    /// Requires `ring_buf.len >= max_consecutive_repeats`, `ring_buf.len >= max_ping_pong_cycles * 2`,
    /// `max_consecutive_repeats >= 2`, and `max_ping_pong_cycles >= 2`.
    static fn init(
        ring_buf: *[]CallSignature,
        max_consecutive_repeats: usize,
        max_ping_pong_cycles: usize,
    ): LoopGuard? {
        if (
            max_consecutive_repeats < 2 ||
            max_ping_pong_cycles < 2 ||
            ring_buf.*.len < max_consecutive_repeats ||
            ring_buf.*.len < max_ping_pong_cycles * 2
        ) {
            return null;
        }
        return LoopGuard {
            ring: ring_buf.*,
            write_pos: 0,
            count: 0,
            max_consecutive_repeats: max_consecutive_repeats,
            max_ping_pong_cycles: max_ping_pong_cycles,
        };
    }

    /// Clears recorded tool-call history.
    fn reset(*_) {
        _.write_pos = 0;
        _.count = 0;
    }

    fn recentAt(ago: usize): CallSignature {
        const cap = _.ring.len;
        const idx = (_.write_pos + cap - 1 - ago) % cap;
        return _.ring[idx];
    }

    /// Records `sig` into the ring buffer and checks for consecutive repeat or A-B-A-B ping-pong loops.
    fn recordAndCheck(*_, sig: CallSignature): LoopAction {
        _.ring[_.write_pos] = sig;
        _.write_pos = (_.write_pos + 1) % _.ring.len;
        if (_.count < _.ring.len) {
            _.count += 1;
        }

        // 1. Check consecutive identical calls: A, A, ..., A
        if (_.count >= _.max_consecutive_repeats) {
            let all_same: bool = true;
            let k: usize = 1;
            while (k < _.max_consecutive_repeats) {
                if (!_.recentAt(k).eql(sig)) {
                    all_same = false;
                    break;
                }
                k += 1;
            }
            if (all_same) {
                return LoopAction.RepeatLoop;
            }
        }

        // 2. Check alternating ping-pong cycles: ..., A, B, A, B (with A != B)
        const pp_len = _.max_ping_pong_cycles * 2;
        if (_.count >= pp_len) {
            const sig_b = _.recentAt(0);
            const sig_a = _.recentAt(1);
            if (!sig_a.eql(sig_b)) {
                let is_pp: bool = true;
                let k: usize = 2;
                while (k < pp_len) {
                    const expected = if ((k & 1) == 0) sig_b else sig_a;
                    if (!_.recentAt(k).eql(expected)) {
                        is_pp = false;
                        break;
                    }
                    k += 1;
                }
                if (is_pp) {
                    return LoopAction.PingPongLoop;
                }
            }
        }

        return LoopAction.Allow;
    }
}

/// Circuit breaker state for flaky external tools/APIs.
pub enum CircuitState {
    Closed,
    Open,
    HalfOpen,
}

/// Zero-alloc per-tool circuit breaker (`Closed -> Open -> HalfOpen -> Closed`).
pub struct ToolCircuitBreaker {
    state: CircuitState,
    failure_threshold: u32,
    success_threshold: u32,
    cooldown_ms: u64,
    consecutive_failures: u32,
    consecutive_successes: u32,
    opened_at_ms: u64,

    /// Initializes a circuit breaker (`failure_threshold >= 1`, `success_threshold >= 1`).
    static fn init(failure_threshold: u32, success_threshold: u32, cooldown_ms: u64): ToolCircuitBreaker? {
        if (failure_threshold == 0 || success_threshold == 0) {
            return null;
        }
        return ToolCircuitBreaker {
            state: CircuitState.Closed,
            failure_threshold: failure_threshold,
            success_threshold: success_threshold,
            cooldown_ms: cooldown_ms,
            consecutive_failures: 0,
            consecutive_successes: 0,
            opened_at_ms: 0,
        };
    }

    /// Resets the breaker to `Closed`.
    fn reset(*_) {
        _.state = CircuitState.Closed;
        _.consecutive_failures = 0;
        _.consecutive_successes = 0;
        _.opened_at_ms = 0;
    }

    /// Checks whether a tool call is permitted at timestamp `now_ms`.
    /// Automatically transitions `Open -> HalfOpen` once `cooldown_ms` has elapsed.
    fn allowCall(*_, now_ms: u64): bool {
        match (_.state) {
            CircuitState.Closed => return true,
            CircuitState.HalfOpen => return true,
            CircuitState.Open => {
                if (now_ms >= satAddU64(_.opened_at_ms, _.cooldown_ms)) {
                    _.state = CircuitState.HalfOpen;
                    _.consecutive_successes = 0;
                    return true;
                }
                return false;
            }
        }
    }

    /// Records a successful tool call. Closes the circuit after `success_threshold` half-open successes.
    fn recordSuccess(*_) {
        match (_.state) {
            CircuitState.Closed => {
                _.consecutive_failures = 0;
            }
            CircuitState.HalfOpen => {
                _.consecutive_successes = satAddU32(_.consecutive_successes, 1);
                if (_.consecutive_successes >= _.success_threshold) {
                    _.state = CircuitState.Closed;
                    _.consecutive_failures = 0;
                    _.consecutive_successes = 0;
                }
            }
            CircuitState.Open => {}
        }
    }

    /// Records a failed tool call at `now_ms`. Trips to `Open` immediately from `HalfOpen` or on threshold.
    fn recordFailure(*_, now_ms: u64) {
        match (_.state) {
            CircuitState.Closed => {
                _.consecutive_failures = satAddU32(_.consecutive_failures, 1);
                if (_.consecutive_failures >= _.failure_threshold) {
                    _.state = CircuitState.Open;
                    _.opened_at_ms = now_ms;
                    _.consecutive_successes = 0;
                }
            }
            CircuitState.HalfOpen => {
                _.state = CircuitState.Open;
                _.opened_at_ms = now_ms;
                _.consecutive_successes = 0;
            }
            CircuitState.Open => {
                _.opened_at_ms = now_ms;
            }
        }
    }
}

/// Context window compaction recommendation.
pub enum ContextStatus {
    Ok,
    CompactSoon,
    MustCompact,
    Overflow,
}

/// Diagnostic snapshot returned by `ContextWindowAdvisor.evaluate`.
pub struct ContextReport {
    status: ContextStatus,
    used_tokens: u64,
    effective_limit: u64,
    headroom_tokens: u64,
    utilization_pct: u8,
}

/// Advises when an AI Agent must trigger conversation compaction before exhausting the model's context window.
pub struct ContextWindowAdvisor {
    max_context_tokens: u64,
    reserved_output_tokens: u64,
    compact_soon_pct: u8,
    must_compact_pct: u8,

    /// Initializes the advisor (`max_context_tokens > reserved_output_tokens`, `0 < compact_soon_pct < must_compact_pct <= 100`).
    static fn init(
        max_context_tokens: u64,
        reserved_output_tokens: u64,
        compact_soon_pct: u8,
        must_compact_pct: u8,
    ): ContextWindowAdvisor? {
        if (
            max_context_tokens <= reserved_output_tokens ||
            compact_soon_pct == 0 ||
            compact_soon_pct >= must_compact_pct ||
            must_compact_pct > 100
        ) {
            return null;
        }
        return ContextWindowAdvisor {
            max_context_tokens: max_context_tokens,
            reserved_output_tokens: reserved_output_tokens,
            compact_soon_pct: compact_soon_pct,
            must_compact_pct: must_compact_pct,
        };
    }

    /// Evaluates current context usage and returns headroom + compaction status.
    fn evaluate(used_tokens: u64): ContextReport {
        const eff = _.max_context_tokens - _.reserved_output_tokens;
        if (used_tokens > eff) {
            return ContextReport {
                status: ContextStatus.Overflow,
                used_tokens: used_tokens,
                effective_limit: eff,
                headroom_tokens: 0,
                utilization_pct: 100,
            };
        }
        const headroom = eff - used_tokens;
        const pct_u64 = if (used_tokens <= 0xFFFFFFFFFFFFFFFF / 100)
            (used_tokens * 100) / eff
        else
            used_tokens / (eff / 100);
        const pct: u8 = if (pct_u64 > 100) 100 else (pct_u64 as u8);

        let st = ContextStatus.Ok;
        if (pct >= _.must_compact_pct) {
            st = ContextStatus.MustCompact;
        } else if (pct >= _.compact_soon_pct) {
            st = ContextStatus.CompactSoon;
        }
        return ContextReport {
            status: st,
            used_tokens: used_tokens,
            effective_limit: eff,
            headroom_tokens: headroom,
            utilization_pct: pct,
        };
    }

    /// Returns true if `used_tokens + incoming_tokens` fits within `max_context_tokens - reserved_output_tokens`.
    fn canFitAdditional(used_tokens: u64, incoming_tokens: u64): bool {
        const eff = _.max_context_tokens - _.reserved_output_tokens;
        if (used_tokens > eff) {
            return false;
        }
        return incoming_tokens <= (eff - used_tokens);
    }
}
