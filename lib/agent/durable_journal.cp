// Origin: Pure-CP Stdlib (Zero-Alloc AI Agent Durable Execution Journal & Deterministic Crash Replay Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation Durable Execution Journal (Temporal / Restate / LangGraph style):
//   - Fixed inline 128-step journal (`DURABLE_JOURNAL_MAX_STEPS = 128`) with 128B result summaries (0 heap)
//   - Deterministic crash replay (`begin_replay`): replays completed (`succeeded`/`failed`) steps from cache
//     without re-executing side-effectful tools or LLM calls
//   - Strict non-determinism guard: detects any `(kind, input_hash)` mismatch during replay (`divergence_detected`)
//   - Seamless handoff: resumes unfinished `pending` steps in place and transitions to `execute_live` past journal tip

/// Maximum recorded execution steps per durable run (zero dynamic heap allocation).
pub const DURABLE_JOURNAL_MAX_STEPS: usize = 128;

/// Maximum byte length of a stored step result summary payload.
pub const DURABLE_RESULT_MAX_LEN: usize = 128;

// ============================================================================
// 1. Public Enums, Step Entry & Replay Decision Models
// ============================================================================

/// Semantic kind of a durable AI Agent execution step.
pub enum DurableStepKind {
    llm_completion,
    tool_call,
    human_approval,
    timer_sleep,
    subagent_spawn,
}

/// Persisted completion status of a journaled step.
pub enum DurableStepOutcome {
    pending,
    succeeded,
    failed,
}

/// Action instructed by `DurableExecutionJournal.next_step`.
pub enum DurableReplayAction {
    execute_live,
    replay_cached,
    divergence_detected,
    journal_full,
}

/// Single persisted step entry inside `DurableExecutionJournal`.
pub struct DurableStepEntry {
    step_seq: u32 = 0;
    kind: DurableStepKind = DurableStepKind.llm_completion;
    input_hash: u64 = 0;
    outcome: DurableStepOutcome = DurableStepOutcome.pending;
    result_buf: [DURABLE_RESULT_MAX_LEN]u8 = [0; DURABLE_RESULT_MAX_LEN];
    result_len: usize = 0;
    timestamp_ms: u64 = 0;

    /// Returns the persisted result payload as a read-only byte slice.
    fn resultSlice(): []const u8 {
        return _.result_buf[0.._.result_len];
    }
}

/// Decision returned by `DurableExecutionJournal.next_step`.
pub struct DurableStepDecision {
    action: DurableReplayAction = DurableReplayAction.execute_live;
    step_seq: u32 = 0;
    cached_outcome: DurableStepOutcome = DurableStepOutcome.pending;
    expected_kind: DurableStepKind = DurableStepKind.llm_completion;
    expected_input_hash: u64 = 0;
}

/// Operational telemetry snapshot for `DurableExecutionJournal`.
pub struct DurableJournalStats {
    recorded_steps: usize = 0;
    replay_cursor: usize = 0;
    replayed_hits: u64 = 0;
    live_executions: u64 = 0;
    divergences_detected: u64 = 0;
    is_replaying: bool = false;
}

// ============================================================================
// 2. Zero-Alloc Durable Execution Journal & Replay Engine
// ============================================================================

/// Fixed-capacity (128 steps) zero-heap-allocation durable execution journal for crash-proof AI Agents.
pub struct DurableExecutionJournal {
    run_id: u64 = 0;
    steps: [DURABLE_JOURNAL_MAX_STEPS]DurableStepEntry = [DurableStepEntry{}; DURABLE_JOURNAL_MAX_STEPS],
    recorded_steps: usize = 0;
    replay_cursor: usize = 0;
    is_replaying: bool = false;
    replayed_hits: u64 = 0;
    live_executions: u64 = 0;
    divergences_detected: u64 = 0;

    /// Initializes a fresh durable execution journal for `run_id`.
    static fn init(run_id: u64): DurableExecutionJournal {
        return DurableExecutionJournal{
            run_id: run_id,
        };
    }

    /// Resets `replay_cursor` to `0` and enters deterministic replay mode (simulating crash recovery).
    fn begin_replay(*_) {
        _.replay_cursor = 0;
        _.is_replaying = true;
    }

    /// Advances the durable execution state machine for the next workflow step:
    ///   - While replaying completed (`succeeded` or `failed`) journal entries:
    ///       * Exact `(kind, input_hash)` match -> advances cursor and returns `replay_cached`
    ///       * Mismatch -> locks cursor, increments `divergences_detected`, returns `divergence_detected`
    ///   - If an unfinished `pending` step is encountered during replay with matching `(kind, input_hash)`:
    ///       * Exits replay mode and returns `execute_live` with the existing `step_seq` for in-place resume.
    ///   - Past the end of recorded steps (or in live mode):
    ///       * Appends a new `pending` step and returns `execute_live` (or `journal_full` if 128 steps reached).
    fn next_step(
        *_,
        kind: DurableStepKind,
        input_hash: u64,
        now_ms: u64,
    ): DurableStepDecision {
        if (_.is_replaying && _.replay_cursor < _.recorded_steps) {
            const cur = _.steps[_.replay_cursor];
            if (cur.kind != kind || cur.input_hash != input_hash) {
                _.divergences_detected += 1;
                return DurableStepDecision{
                    action: DurableReplayAction.divergence_detected,
                    step_seq: cur.step_seq,
                    cached_outcome: cur.outcome,
                    expected_kind: cur.kind,
                    expected_input_hash: cur.input_hash,
                };
            }

            if (cur.outcome == DurableStepOutcome.succeeded || cur.outcome == DurableStepOutcome.failed) {
                _.replay_cursor += 1;
                _.replayed_hits += 1;
                return DurableStepDecision{
                    action: DurableReplayAction.replay_cached,
                    step_seq: cur.step_seq,
                    cached_outcome: cur.outcome,
                    expected_kind: cur.kind,
                    expected_input_hash: cur.input_hash,
                };
            }

            // Unfinished `pending` step from before crash: resume in place as live execution
            _.steps[_.replay_cursor].timestamp_ms = now_ms;
            _.is_replaying = false;
            _.replay_cursor += 1;
            _.live_executions += 1;
            return DurableStepDecision{
                action: DurableReplayAction.execute_live,
                step_seq: cur.step_seq,
                cached_outcome: DurableStepOutcome.pending,
                expected_kind: cur.kind,
                expected_input_hash: cur.input_hash,
            };
        }

        // Caught up to journal tip (or already running live)
        _.is_replaying = false;
        if (_.recorded_steps >= DURABLE_JOURNAL_MAX_STEPS) {
            return DurableStepDecision{
                action: DurableReplayAction.journal_full,
                step_seq: _.recorded_steps as u32,
                cached_outcome: DurableStepOutcome.pending,
                expected_kind: kind,
                expected_input_hash: input_hash,
            };
        }

        const seq: u32 = _.recorded_steps as u32;
        _.steps[_.recorded_steps] = DurableStepEntry{
            step_seq: seq,
            kind: kind,
            input_hash: input_hash,
            outcome: DurableStepOutcome.pending,
            result_buf: [0; DURABLE_RESULT_MAX_LEN],
            result_len: 0,
            timestamp_ms: now_ms,
        };
        _.recorded_steps += 1;
        _.replay_cursor = _.recorded_steps;
        _.live_executions += 1;

        return DurableStepDecision{
            action: DurableReplayAction.execute_live,
            step_seq: seq,
            cached_outcome: DurableStepOutcome.pending,
            expected_kind: kind,
            expected_input_hash: input_hash,
        };
    }

    /// Commits a `pending` step as `succeeded` or `failed` and stores `result_bytes`.
    /// Returns `false` if `step_seq` is out of bounds, already completed, or `result_bytes.len > DURABLE_RESULT_MAX_LEN`.
    fn complete_step(
        *_,
        step_seq: u32,
        succeeded: bool,
        result_bytes: []const u8,
    ): bool {
        const idx: usize = step_seq as usize;
        if (idx >= _.recorded_steps) return false;
        if (_.steps[idx].outcome != DurableStepOutcome.pending) return false;
        if (result_bytes.len > DURABLE_RESULT_MAX_LEN) return false;

        let i: usize = 0;
        while (i < result_bytes.len) : (i += 1) {
            _.steps[idx].result_buf[i] = result_bytes[i];
        }
        _.steps[idx].result_len = result_bytes.len;
        _.steps[idx].outcome = if (succeeded) DurableStepOutcome.succeeded else DurableStepOutcome.failed;
        return true;
    }

    /// Returns a copy of the persisted `DurableStepEntry` at `step_seq`, or `null` if out of bounds.
    fn get_step(step_seq: u32): DurableStepEntry? {
        const idx: usize = step_seq as usize;
        if (idx >= _.recorded_steps) return null;
        return _.steps[idx];
    }

    /// Returns a live snapshot of journal step counts, replay cursor, and divergence counters.
    fn stats(): DurableJournalStats {
        return DurableJournalStats{
            recorded_steps: _.recorded_steps,
            replay_cursor: _.replay_cursor,
            replayed_hits: _.replayed_hits,
            live_executions: _.live_executions,
            divergences_detected: _.divergences_detected,
            is_replaying: _.is_replaying,
        };
    }
}
