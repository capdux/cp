// Origin: Pure-CP Stdlib (End-to-End AI Agent Turn Orchestrator & Audit Trace State Machine) | Created: 2026-11-22
// Pure CP zero-heap-allocation turn runner for driving the complete agent execution loop:
//   budget guard -> LLM response accounting -> tool dispatch & consecutive-error circuit breaker
//   -> chronological step trace -> deterministic termination outcome.

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

/// Hard execution limits for `AgentTurnRunner`.
/// `max_turns` must be `>= 1`. Setting any other limit field to `0` disables that guard.
pub struct TurnRunnerConfig {
    max_turns: u32 = 8;
    max_tool_calls: u32 = 32;
    max_input_tokens: u64 = 0;
    max_output_tokens: u64 = 0;
    max_cost_micro_usd: u64 = 0;
    max_consecutive_errors: u32 = 3;
}

/// Deterministic reason explaining why an `AgentTurnRunner` session stopped.
pub enum TurnStopReason {
    Completed,
    MaxTurnsReached,
    MaxToolCallsReached,
    InputTokenBudgetExceeded,
    OutputTokenBudgetExceeded,
    CostBudgetExceeded,
    ConsecutiveErrorsExceeded,
    StepBufferFull,
    Cancelled,
    LlmError,
}

/// Event kind stored in each `TurnStepEntry` of the turn audit trace.
pub enum TurnStepKind {
    TurnStarted,
    LlmResponse,
    ToolSucceeded,
    ToolFailed,
    Stopped,
}

/// Single chronological audit record stored in the caller-owned step buffer.
pub struct TurnStepEntry {
    seq: u32 = 0;
    turn: u32 = 0;
    kind: TurnStepKind = TurnStepKind.TurnStarted;
    timestamp_ms: i64 = 0;
    in_tokens: u64 = 0;
    out_tokens: u64 = 0;
    cost_micro_usd: u64 = 0;
    tool_name: string = "";
    summary: string = "";
    succeeded: bool = true;
    has_tool_call: bool = false;
}

/// Complete final summary returned by `AgentTurnRunner.finish`.
pub struct TurnOutcome {
    stop_reason: TurnStopReason = TurnStopReason.Completed;
    turns_executed: u32 = 0;
    tool_calls: u32 = 0;
    tool_errors: u32 = 0;
    consecutive_errors: u32 = 0;
    total_input_tokens: u64 = 0;
    total_output_tokens: u64 = 0;
    total_tokens: u64 = 0;
    total_cost_micro_usd: u64 = 0;
    started_at_ms: i64 = 0;
    ended_at_ms: i64 = 0;
    elapsed_ms: u64 = 0;
    steps_dropped: u32 = 0;
    steps: []const TurnStepEntry;

    /// Returns `true` when the agent converged normally with `TurnStopReason.Completed`.
    fn isSuccess(): bool {
        return _.stop_reason == TurnStopReason.Completed;
    }

    /// Returns `true` when termination was triggered by any turn, tool, token, or cost limit.
    fn isBudgetExceeded(): bool {
        return _.stop_reason == TurnStopReason.MaxTurnsReached ||
            _.stop_reason == TurnStopReason.MaxToolCallsReached ||
            _.stop_reason == TurnStopReason.InputTokenBudgetExceeded ||
            _.stop_reason == TurnStopReason.OutputTokenBudgetExceeded ||
            _.stop_reason == TurnStopReason.CostBudgetExceeded;
    }
}

/// Zero-heap-allocation end-to-end AI Agent turn orchestration state machine.
pub struct AgentTurnRunner {
    steps_buf: []TurnStepEntry;
    config: TurnRunnerConfig;
    step_count: usize = 0;
    steps_dropped: u32 = 0;
    next_seq: u32 = 1;
    turns_executed: u32 = 0;
    tool_calls: u32 = 0;
    tool_errors: u32 = 0;
    consecutive_errors: u32 = 0;
    total_input_tokens: u64 = 0;
    total_output_tokens: u64 = 0;
    total_cost_micro_usd: u64 = 0;
    has_started: bool = false;
    started_at_ms: i64 = 0;
    last_timestamp_ms: i64 = 0;
    stopped: bool = false;
    stop_reason: TurnStopReason = TurnStopReason.Completed;

    /// Initializes a turn runner backed by caller-owned `steps_buf`.
    /// Returns `null` if `steps_buf.len == 0` or `config.max_turns == 0`.
    static fn init(
        steps_buf: *[]TurnStepEntry,
        config: TurnRunnerConfig,
    ): AgentTurnRunner? {
        if (steps_buf.len == 0 || config.max_turns == 0) return null;

        let i: usize = 0;
        while (i < steps_buf.len) : (i += 1) {
            steps_buf[i] = TurnStepEntry{};
        }

        return AgentTurnRunner{
            steps_buf: steps_buf,
            config: config,
            step_count: 0,
            steps_dropped: 0,
            next_seq: 1,
            turns_executed: 0,
            tool_calls: 0,
            tool_errors: 0,
            consecutive_errors: 0,
            total_input_tokens: 0,
            total_output_tokens: 0,
            total_cost_micro_usd: 0,
            has_started: false,
            started_at_ms: 0,
            last_timestamp_ms: 0,
            stopped: false,
            stop_reason: TurnStopReason.Completed,
        };
    }

    fn markStopped(*_, reason: TurnStopReason): TurnStopReason {
        _.stopped = true;
        _.stop_reason = reason;
        return reason;
    }

    fn touchTimestamp(*_, now_ms: i64) {
        if (!_.has_started) {
            _.has_started = true;
            _.started_at_ms = now_ms;
        }
        _.last_timestamp_ms = now_ms;
    }

    fn appendStep(*_, entry: TurnStepEntry): bool {
        if (_.step_count >= _.steps_buf.len) {
            _.steps_dropped = satAddU32(_.steps_dropped, 1);
            return false;
        }
        _.steps_buf[_.step_count] = entry;
        _.steps_buf[_.step_count].seq = _.next_seq;
        _.next_seq = satAddU32(_.next_seq, 1);
        _.step_count += 1;
        return true;
    }

    fn checkPreTurnLimits(): TurnStopReason? {
        if (_.turns_executed >= _.config.max_turns) {
            return TurnStopReason.MaxTurnsReached;
        }
        if (_.config.max_input_tokens > 0 && _.total_input_tokens >= _.config.max_input_tokens) {
            return TurnStopReason.InputTokenBudgetExceeded;
        }
        if (_.config.max_output_tokens > 0 && _.total_output_tokens >= _.config.max_output_tokens) {
            return TurnStopReason.OutputTokenBudgetExceeded;
        }
        if (_.config.max_cost_micro_usd > 0 && _.total_cost_micro_usd >= _.config.max_cost_micro_usd) {
            return TurnStopReason.CostBudgetExceeded;
        }
        if (_.config.max_consecutive_errors > 0 && _.consecutive_errors >= _.config.max_consecutive_errors) {
            return TurnStopReason.ConsecutiveErrorsExceeded;
        }
        if (_.config.max_tool_calls > 0 && _.tool_calls >= _.config.max_tool_calls) {
            return TurnStopReason.MaxToolCallsReached;
        }
        return null;
    }

    /// Starts the next agent turn if all turn, budget, error, and step-buffer guards pass.
    /// Returns `null` when the caller may proceed to invoke the LLM, or a `TurnStopReason` to halt.
    fn beginTurn(*_, now_ms: i64): TurnStopReason? {
        if (_.stopped) return _.stop_reason;
        _.touchTimestamp(now_ms);

        if (limit_reason := _.checkPreTurnLimits()) {
            return _.markStopped(limit_reason);
        }
        if (_.step_count >= _.steps_buf.len) {
            return _.markStopped(TurnStopReason.StepBufferFull);
        }

        _.turns_executed = satAddU32(_.turns_executed, 1);
        * = _.appendStep(TurnStepEntry{
            turn: _.turns_executed,
            kind: TurnStepKind.TurnStarted,
            timestamp_ms: now_ms,
            succeeded: true,
        });
        return null;
    }

    /// Accounts for an LLM response (tokens, micro-USD cost, tool-call intent, and summary).
    /// Returns `TurnStopReason.Completed` if `!has_tool_call` (and within budget),
    /// a budget/buffer stop reason if exceeded, or `null` when tool execution should continue.
    fn recordLlmResponse(
        *_,
        in_tokens: u64,
        out_tokens: u64,
        cost_micro_usd: u64,
        has_tool_call: bool,
        summary: string,
        now_ms: i64,
    ): TurnStopReason? {
        if (_.stopped) return _.stop_reason;
        _.touchTimestamp(now_ms);

        _.total_input_tokens = satAddU64(_.total_input_tokens, in_tokens);
        _.total_output_tokens = satAddU64(_.total_output_tokens, out_tokens);
        _.total_cost_micro_usd = satAddU64(_.total_cost_micro_usd, cost_micro_usd);

        const wrote = _.appendStep(TurnStepEntry{
            turn: _.turns_executed,
            kind: TurnStepKind.LlmResponse,
            timestamp_ms: now_ms,
            in_tokens: in_tokens,
            out_tokens: out_tokens,
            cost_micro_usd: cost_micro_usd,
            summary: summary,
            succeeded: true,
            has_tool_call: has_tool_call,
        });
        if (!wrote) {
            return _.markStopped(TurnStopReason.StepBufferFull);
        }

        if (_.config.max_input_tokens > 0 && _.total_input_tokens > _.config.max_input_tokens) {
            return _.markStopped(TurnStopReason.InputTokenBudgetExceeded);
        }
        if (_.config.max_output_tokens > 0 && _.total_output_tokens > _.config.max_output_tokens) {
            return _.markStopped(TurnStopReason.OutputTokenBudgetExceeded);
        }
        if (_.config.max_cost_micro_usd > 0 && _.total_cost_micro_usd > _.config.max_cost_micro_usd) {
            return _.markStopped(TurnStopReason.CostBudgetExceeded);
        }

        if (!has_tool_call) {
            return _.markStopped(TurnStopReason.Completed);
        }
        return null;
    }

    /// Records a tool invocation result, updating total tool calls and the consecutive-error circuit breaker.
    /// Returns a `TurnStopReason` if circuit breaker, tool-call cap, or step buffer capacity is reached.
    fn recordToolCall(
        *_,
        tool_name: string,
        succeeded: bool,
        summary: string,
        now_ms: i64,
    ): TurnStopReason? {
        if (_.stopped) return _.stop_reason;
        _.touchTimestamp(now_ms);

        _.tool_calls = satAddU32(_.tool_calls, 1);
        if (succeeded) {
            _.consecutive_errors = 0;
        } else {
            _.tool_errors = satAddU32(_.tool_errors, 1);
            _.consecutive_errors = satAddU32(_.consecutive_errors, 1);
        }

        const kind = if (succeeded) TurnStepKind.ToolSucceeded else TurnStepKind.ToolFailed;
        const wrote = _.appendStep(TurnStepEntry{
            turn: _.turns_executed,
            kind: kind,
            timestamp_ms: now_ms,
            tool_name: tool_name,
            summary: summary,
            succeeded: succeeded,
        });
        if (!wrote) {
            return _.markStopped(TurnStopReason.StepBufferFull);
        }

        if (_.config.max_consecutive_errors > 0 && _.consecutive_errors >= _.config.max_consecutive_errors) {
            return _.markStopped(TurnStopReason.ConsecutiveErrorsExceeded);
        }
        if (_.config.max_tool_calls > 0 && _.tool_calls >= _.config.max_tool_calls) {
            return _.markStopped(TurnStopReason.MaxToolCallsReached);
        }
        return null;
    }

    /// Finalizes the session with `stop_reason` and returns the complete auditable `TurnOutcome`.
    fn finish(*_, stop_reason: TurnStopReason, now_ms: i64): TurnOutcome {
        _.touchTimestamp(now_ms);
        _.stopped = true;
        _.stop_reason = stop_reason;

        * = _.appendStep(TurnStepEntry{
            turn: _.turns_executed,
            kind: TurnStepKind.Stopped,
            timestamp_ms: now_ms,
            succeeded: stop_reason == TurnStopReason.Completed,
        });

        const elapsed_ms: u64 = if (now_ms >= _.started_at_ms)
            (now_ms - _.started_at_ms) as u64
        else
            0;

        return TurnOutcome{
            stop_reason: stop_reason,
            turns_executed: _.turns_executed,
            tool_calls: _.tool_calls,
            tool_errors: _.tool_errors,
            consecutive_errors: _.consecutive_errors,
            total_input_tokens: _.total_input_tokens,
            total_output_tokens: _.total_output_tokens,
            total_tokens: satAddU64(_.total_input_tokens, _.total_output_tokens),
            total_cost_micro_usd: _.total_cost_micro_usd,
            started_at_ms: _.started_at_ms,
            ended_at_ms: now_ms,
            elapsed_ms: elapsed_ms,
            steps_dropped: _.steps_dropped,
            steps: _.steps_buf[0.._.step_count],
        };
    }

    /// Returns the 1-based index of the current/last executed turn (`0` before first `beginTurn`).
    fn currentTurn(): u32 {
        return _.turns_executed;
    }

    /// Returns total recorded tool calls so far.
    fn toolCallCount(): u32 {
        return _.tool_calls;
    }

    /// Returns current consecutive tool failure streak.
    fn consecutiveErrors(): u32 {
        return _.consecutive_errors;
    }

    /// Returns total accumulated tokens (`input + output`, saturating).
    fn totalTokens(): u64 {
        return satAddU64(_.total_input_tokens, _.total_output_tokens);
    }

    /// Returns total accumulated cost in micro-USD.
    fn totalCostMicroUsd(): u64 {
        return _.total_cost_micro_usd;
    }

    /// Returns `true` once the runner has transitioned into a stopped state.
    fn isStopped(): bool {
        return _.stopped;
    }

    /// Returns the recorded `TurnStopReason` if stopped, or `null` while active.
    fn stopReason(): TurnStopReason? {
        if (!_.stopped) return null;
        return _.stop_reason;
    }

    /// Returns the number of recorded trace entries currently stored in `steps_buf`.
    fn stepCount(): usize {
        return _.step_count;
    }

    /// Returns a read-only chronological slice of all recorded trace entries.
    fn steps(): []const TurnStepEntry {
        return _.steps_buf[0.._.step_count];
    }

    /// Resets all counters and trace state while retaining `steps_buf` and `config`.
    fn reset(*_) {
        _.step_count = 0;
        _.steps_dropped = 0;
        _.next_seq = 1;
        _.turns_executed = 0;
        _.tool_calls = 0;
        _.tool_errors = 0;
        _.consecutive_errors = 0;
        _.total_input_tokens = 0;
        _.total_output_tokens = 0;
        _.total_cost_micro_usd = 0;
        _.has_started = false;
        _.started_at_ms = 0;
        _.last_timestamp_ms = 0;
        _.stopped = false;
        _.stop_reason = TurnStopReason.Completed;
    }
}
