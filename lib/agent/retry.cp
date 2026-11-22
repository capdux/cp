// Origin: Pure-CP Stdlib (AI LLM API Adaptive Retry, Rate-Limit & Error Classifier) | Created: 2026-11-22
// Zero-heap-allocation LLM API fault classifier, Retry-After parser, and adaptive backoff/fallback policy engine.

/// Normalized fault classification for LLM provider HTTP and runtime errors.
pub enum LlmErrorClass {
    RateLimited,
    Overloaded,
    TransientServer,
    Timeout,
    ContextLengthExceeded,
    AuthFailed,
    InvalidRequest,
    Fatal,
}

/// Action recommended by `decideLlmRetry`.
pub enum RetryDecisionKind {
    RetryAfterDelay,
    FallbackProvider,
    ShrinkContextAndRetry,
    Abort,
}

/// Deterministic decision returned by `decideLlmRetry`.
pub struct RetryDecision {
    kind: RetryDecisionKind = RetryDecisionKind.Abort;
    delay_ms: u64 = 0;
    next_attempt: u32 = 0;
}

/// Snapshot of provider rate-limit headers (e.g. `Retry-After`, `x-ratelimit-*`).
pub struct RateLimitSnapshot {
    remaining_requests: u32 = 0;
    remaining_tokens: u32 = 0;
    reset_requests_ms: u64 = 0;
    reset_tokens_ms: u64 = 0;
    retry_after_ms: u64 = 0;
}

/// Configuration for adaptive LLM API retries, jittered backoff, context shrinking, and provider fallback.
pub struct LlmRetryPolicy {
    max_retries: u32 = 3;
    base_delay_ms: u64 = 500;
    max_delay_ms: u64 = 30000;
    jitter_pct: u8 = 25;
    allow_context_shrink: bool = true;
    allow_fallback: bool = true;
    fallback_after_consecutive_overload: u32 = 2;

    /// Returns the default production LLM retry policy.
    static fn default(): LlmRetryPolicy {
        return LlmRetryPolicy{};
    }
}

/// Classifies an HTTP status code (plus provider context-overflow hint) into `LlmErrorClass`.
pub fn classifyHttpStatus(status_code: u16, is_context_overflow: bool): LlmErrorClass {
    if (status_code == 401 || status_code == 403) {
        return LlmErrorClass.AuthFailed;
    }
    if (is_context_overflow && (status_code == 400 || status_code == 413 || status_code == 422)) {
        return LlmErrorClass.ContextLengthExceeded;
    }
    if (status_code == 429) {
        return LlmErrorClass.RateLimited;
    }
    if (status_code == 503 || status_code == 529) {
        return LlmErrorClass.Overloaded;
    }
    if (status_code == 408 || status_code == 504 || status_code == 524) {
        return LlmErrorClass.Timeout;
    }
    if (status_code == 400 || status_code == 404 || status_code == 405 || status_code == 413 || status_code == 422) {
        return LlmErrorClass.InvalidRequest;
    }
    if (status_code >= 500 && status_code <= 599) {
        return LlmErrorClass.TransientServer;
    }
    return LlmErrorClass.Fatal;
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

const MAX_SAFE_RETRY_MS: u64 = 86400000000; // 1000 days in ms, prevents u64 overflow

/// Parses a `Retry-After` or rate-limit reset duration header into milliseconds.
/// Supports integer/decimal seconds (`"5"`, `"1.5"`, `"2s"`) and explicit millisecond/minute suffixes (`"250ms"`, `"1m"`).
/// Returns `null` on empty, negative, malformed, or overflowing input.
pub fn parseRetryAfterMs(header_val: string): u64? {
    let start: usize = 0;
    let end: usize = header_val.len;
    while (start < end && isAsciiSpace(header_val[start])) : (start += 1) {}
    while (end > start && isAsciiSpace(header_val[end - 1])) : (end -= 1) {}
    if (start >= end) return null;

    // Detect optional unit suffix ("ms", "s", "m")
    let unit_mult: u64 = 1000; // default: HTTP Retry-After delta-seconds
    let num_end = end;
    if (end - start >= 2 &&
        asciiLower(header_val[end - 2]) == 'm' &&
        asciiLower(header_val[end - 1]) == 's')
    {
        unit_mult = 1;
        num_end = end - 2;
    } else if (asciiLower(header_val[end - 1]) == 's') {
        unit_mult = 1000;
        num_end = end - 1;
    } else if (asciiLower(header_val[end - 1]) == 'm') {
        unit_mult = 60000;
        num_end = end - 1;
    }

    while (num_end > start && isAsciiSpace(header_val[num_end - 1])) : (num_end -= 1) {}
    if (start >= num_end) return null;

    let int_part: u64 = 0;
    let frac_num: u64 = 0;
    let frac_den: u64 = 1;
    let seen_dot = false;
    let seen_digit = false;

    let i = start;
    while (i < num_end) : (i += 1) {
        const c = header_val[i];
        if (isDigit(c)) {
            seen_digit = true;
            const d = u64(c - '0');
            if (!seen_dot) {
                if (int_part > MAX_SAFE_RETRY_MS / 10) return null;
                int_part = int_part * 10 + d;
            } else if (frac_den < 1000000) {
                frac_num = frac_num * 10 + d;
                frac_den = frac_den * 10;
            }
        } else if (c == '.' && !seen_dot) {
            seen_dot = true;
        } else {
            return null;
        }
    }

    if (!seen_digit) return null;
    if (unit_mult > 0 && int_part > MAX_SAFE_RETRY_MS / unit_mult) {
        return null;
    }
    const base_ms = int_part * unit_mult;
    const frac_ms = if (frac_den > 1) (frac_num * unit_mult) / frac_den else 0;
    if (base_ms > MAX_SAFE_RETRY_MS - frac_ms) return null;
    return base_ms + frac_ms;
}

/// Deterministic 64-bit SplitMix64 mixing function (zero global state, fully reproducible).
fn splitMix64(x_in: u64): u64 {
    const c1: u64 = (u64(0x9e3779b9) << 32) | u64(0x7f4a7c15);
    const c2: u64 = (u64(0xbf58476d) << 32) | u64(0x1ce4e5b9);
    const c3: u64 = (u64(0x94d049bb) << 32) | u64(0x133111eb);
    let z = x_in +% c1;
    z = (z ^ (z >> 30)) *% c2;
    z = (z ^ (z >> 27)) *% c3;
    return z ^ (z >> 31);
}

fn applyDeterministicJitter(base_ms: u64, jitter_pct: u8, attempt: u32, seed: u64): u64 {
    if (base_ms == 0 || jitter_pct == 0) return base_ms;
    const pct: u64 = if (jitter_pct > 100) 100 else u64(jitter_pct);
    const span = (base_ms * pct) / 100;
    if (span == 0) return base_ms;

    const mixed = splitMix64(seed ^ (u64(attempt) *% 0x517cc1b7));
    const width = span * 2 + 1;
    const offset = mixed % width;
    return (base_ms - span) + offset;
}

fn computeExpBackoff(base_delay_ms: u64, max_delay_ms: u64, attempt: u32): u64 {
    if (base_delay_ms == 0) return 0;
    const cap = if (max_delay_ms > 0) max_delay_ms else MAX_SAFE_RETRY_MS;
    let delay = base_delay_ms;
    let step: u32 = 0;
    const max_steps: u32 = if (attempt > 30) 30 else attempt;
    while (step < max_steps) : (step += 1) {
        if (delay >= cap / 2) {
            return cap;
        }
        delay = delay * 2;
    }
    if (delay > cap) return cap;
    return delay;
}

/// Evaluates the next retry/recovery action for a failed LLM API call.
/// - `AuthFailed` / `InvalidRequest` / `Fatal`: aborts immediately.
/// - `ContextLengthExceeded`: recommends `ShrinkContextAndRetry` when enabled and retries remain,
///   otherwise falls back to an alternate provider (if allowed) or aborts.
/// - `RateLimited` / `Overloaded` / `TransientServer` / `Timeout`:
///   - Falls back when retries are exhausted or consecutive 429/529 reaches `fallback_after_consecutive_overload`.
///   - Computes deterministic jittered exponential backoff while prioritizing `Retry-After` and rate-limit resets,
///     clamped to `policy.max_delay_ms`.
pub fn decideLlmRetry(
    policy: LlmRetryPolicy,
    err_class: LlmErrorClass,
    attempt: u32,
    rl_snapshot: RateLimitSnapshot,
    seed: u64,
): RetryDecision {
    // 1. Non-retryable terminal errors
    if (err_class == LlmErrorClass.AuthFailed ||
        err_class == LlmErrorClass.InvalidRequest ||
        err_class == LlmErrorClass.Fatal)
    {
        return RetryDecision{
            kind: RetryDecisionKind.Abort,
            delay_ms: 0,
            next_attempt: attempt,
        };
    }

    // 2. Context window overflow recovery
    if (err_class == LlmErrorClass.ContextLengthExceeded) {
        if (policy.allow_context_shrink && attempt < policy.max_retries) {
            return RetryDecision{
                kind: RetryDecisionKind.ShrinkContextAndRetry,
                delay_ms: 0,
                next_attempt: attempt + 1,
            };
        }
        if (policy.allow_fallback) {
            return RetryDecision{
                kind: RetryDecisionKind.FallbackProvider,
                delay_ms: 0,
                next_attempt: attempt,
            };
        }
        return RetryDecision{
            kind: RetryDecisionKind.Abort,
            delay_ms: 0,
            next_attempt: attempt,
        };
    }

    // 3. Exhausted retry budget -> FallbackProvider or Abort
    if (attempt >= policy.max_retries) {
        if (policy.allow_fallback) {
            return RetryDecision{
                kind: RetryDecisionKind.FallbackProvider,
                delay_ms: 0,
                next_attempt: attempt,
            };
        }
        return RetryDecision{
            kind: RetryDecisionKind.Abort,
            delay_ms: 0,
            next_attempt: attempt,
        };
    }

    // 4. Consecutive 429 / 529 proactive provider fallback
    const is_capacity_err = err_class == LlmErrorClass.RateLimited || err_class == LlmErrorClass.Overloaded;
    if (is_capacity_err &&
        policy.allow_fallback &&
        policy.fallback_after_consecutive_overload > 0 &&
        attempt >= policy.fallback_after_consecutive_overload)
    {
        return RetryDecision{
            kind: RetryDecisionKind.FallbackProvider,
            delay_ms: 0,
            next_attempt: attempt,
        };
    }

    // 5. Backoff calculation with Retry-After / reset priority & max_delay_ms clamping
    const exp_ms = computeExpBackoff(policy.base_delay_ms, policy.max_delay_ms, attempt);
    let delay = applyDeterministicJitter(exp_ms, policy.jitter_pct, attempt, seed);

    if (rl_snapshot.retry_after_ms > 0) {
        delay = rl_snapshot.retry_after_ms;
    } else if (err_class == LlmErrorClass.RateLimited) {
        if (rl_snapshot.remaining_requests == 0 && rl_snapshot.reset_requests_ms > delay) {
            delay = rl_snapshot.reset_requests_ms;
        }
        if (rl_snapshot.remaining_tokens == 0 && rl_snapshot.reset_tokens_ms > delay) {
            delay = rl_snapshot.reset_tokens_ms;
        }
    }

    if (policy.max_delay_ms > 0 && delay > policy.max_delay_ms) {
        delay = policy.max_delay_ms;
    }

    return RetryDecision{
        kind: RetryDecisionKind.RetryAfterDelay,
        delay_ms: delay,
        next_attempt: attempt + 1,
    };
}
