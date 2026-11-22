// Origin: Pure-CP Stdlib (LLM Structured Output Self-Healing Retry Controller & Feedback Generator) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation structured output self-healing retry state machine for AI Agents:
//   - Deterministic FNV-1a 64-bit error-signature hashing with identical-loop circuit breaking
//   - Stepped temperature escalation (`initial_temp_milli` + `temp_step_milli` capped at `max_temp_milli`)
//   - Automatic `max_tokens` boost on `truncated_output` (`token_boost_factor_pct` capped at `max_tokens_cap`)
//   - Automatic model fallback escalation after `fallback_after_attempt` failures
//   - Immediate abort on `model_refusal` and zero-alloc targeted incremental feedback prompt formatting

// ============================================================================
// 1. Constants & Enums
// ============================================================================

/// Maximum number of retry attempt records stored inline in `StructuredRetryController`.
pub const STRUCT_RETRY_MAX_ATTEMPTS: usize = 8;

/// Maximum inline byte length for a failure detail message stored per attempt.
pub const STRUCT_RETRY_MSG_MAX_LEN: usize = 160;

/// Classification of an LLM structured output validation failure.
pub enum RetryFailureKind {
    none,
    json_syntax,
    missing_field,
    type_mismatch,
    constraint_violation,
    truncated_output,
    model_refusal,
}

/// Directive action emitted by `StructuredRetryController`.
pub enum RetryDirectiveAction {
    accept,
    retry_with_feedback,
    retry_increase_max_tokens,
    retry_fallback_model,
    abort_identical_loop,
    abort_exhausted,
    abort_refusal,
}

// ============================================================================
// 2. Configuration, Records & Telemetry Models
// ============================================================================

/// Configuration for `StructuredRetryController`.
pub struct StructuredRetryConfig {
    /// Maximum total attempts allowed (`1..=8`).
    max_attempts: u8 = 5;
    /// Initial sampling temperature in milli-units (e.g. `200` = `0.200`).
    initial_temp_milli: u16 = 200;
    /// Temperature increment applied on each retry in milli-units.
    temp_step_milli: u16 = 150;
    /// Maximum sampling temperature cap in milli-units.
    max_temp_milli: u16 = 900;
    /// Initial `max_tokens` budget for generation.
    initial_max_tokens: u32 = 1024;
    /// Percentage multiplier applied to `max_tokens` on `truncated_output` (e.g. `150` = `1.5x`).
    token_boost_factor_pct: u16 = 150;
    /// Hard upper bound on `max_tokens` after boosts.
    max_tokens_cap: u32 = 8192;
    /// Consecutive identical error hash count that triggers `abort_identical_loop` (`0` = disabled).
    max_identical_errors: u8 = 2;
    /// Attempt count threshold after which failures escalate to `retry_fallback_model` (`0` = disabled).
    fallback_after_attempt: u8 = 3;

    /// Returns production-tested default retry configuration.
    static fn defaults(): StructuredRetryConfig {
        return StructuredRetryConfig{};
    }
}

/// Recorded telemetry for a single generation/validation attempt.
pub struct RetryAttemptRecord {
    attempt_no: u8 = 0;
    failure_kind: RetryFailureKind = RetryFailureKind.none;
    error_hash: u64 = 0;
    temperature_milli: u16 = 0;
    max_tokens: u32 = 0;
    detail_buf: [STRUCT_RETRY_MSG_MAX_LEN]u8 = [0; STRUCT_RETRY_MSG_MAX_LEN];
    detail_len: usize = 0;

    /// Returns the stored failure detail message slice.
    fn detail(): []const u8 {
        return detail_buf[0..detail_len];
    }
}

/// Next-step directive returned by `recordSuccess` / `recordFailure`.
pub struct RetryDecision {
    action: RetryDirectiveAction = RetryDirectiveAction.accept;
    next_attempt: u8 = 0;
    next_temperature_milli: u16 = 0;
    next_max_tokens: u32 = 0;
    use_fallback_model: bool = false;
    consecutive_same_error: u8 = 0;
}

/// Aggregate session statistics for `StructuredRetryController`.
pub struct StructuredRetryStats {
    total_attempts: u32 = 0;
    succeeded: bool = false;
    syntax_failures: u32 = 0;
    schema_failures: u32 = 0;
    truncations: u32 = 0;
    identical_loop_aborts: u32 = 0;
}

// ============================================================================
// 3. Internal Hash & Formatting Helpers
// ============================================================================

fn failureKindTag(kind: RetryFailureKind): u8 {
    if (kind == RetryFailureKind.none) return 0;
    if (kind == RetryFailureKind.json_syntax) return 1;
    if (kind == RetryFailureKind.missing_field) return 2;
    if (kind == RetryFailureKind.type_mismatch) return 3;
    if (kind == RetryFailureKind.constraint_violation) return 4;
    if (kind == RetryFailureKind.truncated_output) return 5;
    return 6;
}

fn fnv1aHash64WithTag(tag: u8, bytes: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    h = (h ^ @as(u64, tag)) *% 1099511628211;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h = (h ^ @as(u64, bytes[i])) *% 1099511628211;
    }
    return h;
}

fn appendBytes(dst: *[]u8, pos: usize, src: []const u8): usize? {
    if (src.len > dst.len - pos) return null;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[pos + i] = src[i];
    }
    return pos + src.len;
}

fn feedbackPrefixForKind(kind: RetryFailureKind): []const u8 {
    if (kind == RetryFailureKind.json_syntax) {
        return "[Fix JSON Syntax] Return strictly valid JSON with no extra prose. Error: ";
    }
    if (kind == RetryFailureKind.missing_field) {
        return "[Fix Missing Field] Include all required schema fields. Missing: ";
    }
    if (kind == RetryFailureKind.type_mismatch) {
        return "[Fix Type Mismatch] Match the exact JSON field types. Mismatch: ";
    }
    if (kind == RetryFailureKind.constraint_violation) {
        return "[Fix Constraint] Satisfy all value/range constraints. Violation: ";
    }
    if (kind == RetryFailureKind.truncated_output) {
        return "[Fix Truncation] Keep values concise and close all JSON brackets. Context: ";
    }
    if (kind == RetryFailureKind.model_refusal) {
        return "[Refusal] Model declined generation: ";
    }
    return "[Fix Output] ";
}

// ============================================================================
// 4. Core Engine: `StructuredRetryController`
// ============================================================================

/// Zero-heap-allocation self-healing retry state machine for LLM structured outputs.
pub struct StructuredRetryController {
    cfg: StructuredRetryConfig = StructuredRetryConfig.defaults();
    attempts: [STRUCT_RETRY_MAX_ATTEMPTS]RetryAttemptRecord = [RetryAttemptRecord{}; STRUCT_RETRY_MAX_ATTEMPTS];
    attempt_count: u8 = 0;
    current_temp_milli: u16 = 200;
    current_max_tokens: u32 = 1024;
    last_error_hash: u64 = 0;
    consecutive_same_error: u8 = 0;
    using_fallback: bool = false;
    stat: StructuredRetryStats = StructuredRetryStats{};

    /// Initializes a retry controller with sanitized bounds from `cfg`.
    static fn init(cfg: StructuredRetryConfig): StructuredRetryController {
        let max_att: u8 = cfg.max_attempts;
        if (max_att == 0) {
            max_att = 1;
        } else if (max_att > (STRUCT_RETRY_MAX_ATTEMPTS as u8)) {
            max_att = STRUCT_RETRY_MAX_ATTEMPTS as u8;
        }
        const max_temp: u16 = if (cfg.max_temp_milli < cfg.initial_temp_milli) cfg.initial_temp_milli else cfg.max_temp_milli;
        const max_toks: u32 = if (cfg.max_tokens_cap < cfg.initial_max_tokens) cfg.initial_max_tokens else cfg.max_tokens_cap;
        const safe_cfg = StructuredRetryConfig{
            max_attempts: max_att,
            initial_temp_milli: cfg.initial_temp_milli,
            temp_step_milli: cfg.temp_step_milli,
            max_temp_milli: max_temp,
            initial_max_tokens: cfg.initial_max_tokens,
            token_boost_factor_pct: cfg.token_boost_factor_pct,
            max_tokens_cap: max_toks,
            max_identical_errors: cfg.max_identical_errors,
            fallback_after_attempt: cfg.fallback_after_attempt,
        };
        return StructuredRetryController{
            cfg: safe_cfg,
            current_temp_milli: safe_cfg.initial_temp_milli,
            current_max_tokens: safe_cfg.initial_max_tokens,
        };
    }

    /// Resets attempt history, counters, temperature, and token budget back to initial config.
    fn reset(*_) {
        attempt_count = 0;
        current_temp_milli = cfg.initial_temp_milli;
        current_max_tokens = cfg.initial_max_tokens;
        last_error_hash = 0;
        consecutive_same_error = 0;
        using_fallback = false;
        stat = StructuredRetryStats{};
    }

    /// Records a valid structured output and emits `RetryDirectiveAction.accept`.
    fn recordSuccess(*_): RetryDecision {
        const used_temp: u16 = current_temp_milli;
        const used_tokens: u32 = current_max_tokens;
        if (attempt_count < (STRUCT_RETRY_MAX_ATTEMPTS as u8)) {
            attempts[attempt_count as usize] = RetryAttemptRecord{
                attempt_no: attempt_count + 1,
                failure_kind: RetryFailureKind.none,
                error_hash: 0,
                temperature_milli: used_temp,
                max_tokens: used_tokens,
            };
        }
        attempt_count += 1;
        stat.total_attempts += 1;
        stat.succeeded = true;
        consecutive_same_error = 0;
        last_error_hash = 0;

        return RetryDecision{
            action: RetryDirectiveAction.accept,
            next_attempt: attempt_count,
            next_temperature_milli: used_temp,
            next_max_tokens: used_tokens,
            use_fallback_model: using_fallback,
            consecutive_same_error: 0,
        };
    }

    /// Records a structured output validation failure and computes the next self-healing directive:
    ///   1. Computes deterministic FNV-1a 64-bit hash of `(kind, detail)`.
    ///   2. `model_refusal` immediately returns `abort_refusal`.
    ///   3. Consecutive identical error hashes reaching `max_identical_errors` trigger `abort_identical_loop`.
    ///   4. Exhausting `max_attempts` returns `abort_exhausted`.
    ///   5. `truncated_output` boosts `max_tokens` by `token_boost_factor_pct` (capped at `max_tokens_cap`)
    ///      and returns `retry_increase_max_tokens`.
    ///   6. Reaching `fallback_after_attempt` escalates to `retry_fallback_model`.
    ///   7. Standard validation failures step temperature upward and return `retry_with_feedback`.
    fn recordFailure(*_, kind: RetryFailureKind, detail: []const u8): RetryDecision {
        const used_temp: u16 = current_temp_milli;
        const used_tokens: u32 = current_max_tokens;
        const err_hash: u64 = fnv1aHash64WithTag(failureKindTag(kind), detail);

        if (attempt_count > 0 && last_error_hash == err_hash) {
            consecutive_same_error += 1;
        } else {
            consecutive_same_error = 1;
            last_error_hash = err_hash;
        }

        if (attempt_count < (STRUCT_RETRY_MAX_ATTEMPTS as u8)) {
            let rec = RetryAttemptRecord{
                attempt_no: attempt_count + 1,
                failure_kind: kind,
                error_hash: err_hash,
                temperature_milli: used_temp,
                max_tokens: used_tokens,
            };
            const copy_len: usize = if (detail.len < STRUCT_RETRY_MSG_MAX_LEN) detail.len else STRUCT_RETRY_MSG_MAX_LEN;
            let ci: usize = 0;
            while (ci < copy_len) : (ci += 1) {
                rec.detail_buf[ci] = detail[ci];
            }
            rec.detail_len = copy_len;
            attempts[attempt_count as usize] = rec;
        }

        attempt_count += 1;
        stat.total_attempts += 1;
        if (kind == RetryFailureKind.json_syntax) {
            stat.syntax_failures += 1;
        } else if (kind == RetryFailureKind.missing_field || kind == RetryFailureKind.type_mismatch || kind == RetryFailureKind.constraint_violation) {
            stat.schema_failures += 1;
        } else if (kind == RetryFailureKind.truncated_output) {
            stat.truncations += 1;
        }

        // Priority 1: model refusal -> immediate abort
        if (kind == RetryFailureKind.model_refusal) {
            return RetryDecision{
                action: RetryDirectiveAction.abort_refusal,
                next_attempt: attempt_count,
                next_temperature_milli: current_temp_milli,
                next_max_tokens: current_max_tokens,
                use_fallback_model: using_fallback,
                consecutive_same_error: consecutive_same_error,
            };
        }

        // Priority 2: identical error hash circuit breaker
        if (cfg.max_identical_errors > 0 && consecutive_same_error >= cfg.max_identical_errors) {
            stat.identical_loop_aborts += 1;
            return RetryDecision{
                action: RetryDirectiveAction.abort_identical_loop,
                next_attempt: attempt_count,
                next_temperature_milli: current_temp_milli,
                next_max_tokens: current_max_tokens,
                use_fallback_model: using_fallback,
                consecutive_same_error: consecutive_same_error,
            };
        }

        // Priority 3: attempt budget exhausted
        if (attempt_count >= cfg.max_attempts) {
            return RetryDecision{
                action: RetryDirectiveAction.abort_exhausted,
                next_attempt: attempt_count,
                next_temperature_milli: current_temp_milli,
                next_max_tokens: current_max_tokens,
                use_fallback_model: using_fallback,
                consecutive_same_error: consecutive_same_error,
            };
        }

        // Priority 4: truncated output -> boost max_tokens up to cap
        if (kind == RetryFailureKind.truncated_output) {
            const pct: u64 = if (cfg.token_boost_factor_pct > 100) @as(u64, cfg.token_boost_factor_pct) else 150;
            const boosted_u64: u64 = (@as(u64, current_max_tokens) * pct) / 100;
            const boosted: u32 = if (boosted_u64 > @as(u64, cfg.max_tokens_cap)) cfg.max_tokens_cap else @as(u32, @intCast(boosted_u64));
            current_max_tokens = if (boosted > current_max_tokens) boosted else cfg.max_tokens_cap;
            return RetryDecision{
                action: RetryDirectiveAction.retry_increase_max_tokens,
                next_attempt: attempt_count + 1,
                next_temperature_milli: current_temp_milli,
                next_max_tokens: current_max_tokens,
                use_fallback_model: using_fallback,
                consecutive_same_error: consecutive_same_error,
            };
        }

        // Step temperature upward (capped at max_temp_milli)
        const next_t_u32: u32 = @as(u32, current_temp_milli) + @as(u32, cfg.temp_step_milli);
        current_temp_milli = if (next_t_u32 > @as(u32, cfg.max_temp_milli)) cfg.max_temp_milli else @as(u16, @intCast(next_t_u32));

        // Priority 5: escalate to fallback model after threshold
        if (cfg.fallback_after_attempt > 0 && attempt_count >= cfg.fallback_after_attempt) {
            using_fallback = true;
            return RetryDecision{
                action: RetryDirectiveAction.retry_fallback_model,
                next_attempt: attempt_count + 1,
                next_temperature_milli: current_temp_milli,
                next_max_tokens: current_max_tokens,
                use_fallback_model: true,
                consecutive_same_error: consecutive_same_error,
            };
        }

        // Priority 6: standard targeted feedback retry
        return RetryDecision{
            action: RetryDirectiveAction.retry_with_feedback,
            next_attempt: attempt_count + 1,
            next_temperature_milli: current_temp_milli,
            next_max_tokens: current_max_tokens,
            use_fallback_model: using_fallback,
            consecutive_same_error: consecutive_same_error,
        };
    }

    /// Formats a targeted, incremental LLM repair prompt into `out_buf` based on `kind` and `last_detail`.
    /// Returns bytes written, or `null` if `out_buf` is too small.
    fn formatFeedbackPrompt(
        last_detail: []const u8,
        kind: RetryFailureKind,
        out_buf: *[]u8,
    ): usize? {
        const prefix: []const u8 = feedbackPrefixForKind(kind);
        let pos: usize = appendBytes(out_buf, 0, prefix) ?? return null;
        pos = appendBytes(out_buf, pos, last_detail) ?? return null;
        return pos;
    }

    /// Returns the most recent attempt record, or `null` if no attempts have been recorded.
    fn lastAttempt(): RetryAttemptRecord? {
        if (attempt_count == 0) return null;
        const idx: usize = if (attempt_count <= (STRUCT_RETRY_MAX_ATTEMPTS as u8)) (@as(usize, attempt_count) - 1) else (STRUCT_RETRY_MAX_ATTEMPTS - 1);
        return attempts[idx];
    }

    /// Returns current session retry statistics.
    fn stats(): StructuredRetryStats {
        return stat;
    }
}
