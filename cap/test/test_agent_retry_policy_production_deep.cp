// Origin: Pure-CP Stdlib Test Suite (AI LLM API Adaptive Retry, Rate-Limit & Error Classifier) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/retry_policy.cp
// Covers:
//   1. HTTP status code & context-overflow classification matrix
//   2. Retry-After header parsing (sec/decimal/ms/m/invalid), priority override & max_delay_ms clamping
//   3. Deterministic jittered exponential backoff & reproducibility across seeds/attempts
//   4. ContextLengthExceeded shrink-and-retry, fallback & abort recovery branches
//   5. Immediate abort for auth/invalid/fatal, proactive 429/529 fallback & budget exhaustion fallback

import { test, expect_eq } from "cap:test";
import {
    LlmErrorClass,
    RetryDecisionKind,
    RetryDecision,
    RateLimitSnapshot,
    LlmRetryPolicy,
    classifyHttpStatus,
    parseRetryAfterMs,
    decideLlmRetry,
} from "../../lib/agent/index.cp";

fn testHttpStatusClassification()! {
    expect_eq(classifyHttpStatus(401, false) == LlmErrorClass.AuthFailed, true);
    expect_eq(classifyHttpStatus(403, true) == LlmErrorClass.AuthFailed, true);

    expect_eq(classifyHttpStatus(400, true) == LlmErrorClass.ContextLengthExceeded, true);
    expect_eq(classifyHttpStatus(413, true) == LlmErrorClass.ContextLengthExceeded, true);
    expect_eq(classifyHttpStatus(422, true) == LlmErrorClass.ContextLengthExceeded, true);

    expect_eq(classifyHttpStatus(400, false) == LlmErrorClass.InvalidRequest, true);
    expect_eq(classifyHttpStatus(404, false) == LlmErrorClass.InvalidRequest, true);
    expect_eq(classifyHttpStatus(422, false) == LlmErrorClass.InvalidRequest, true);

    expect_eq(classifyHttpStatus(429, false) == LlmErrorClass.RateLimited, true);
    expect_eq(classifyHttpStatus(503, false) == LlmErrorClass.Overloaded, true);
    expect_eq(classifyHttpStatus(529, false) == LlmErrorClass.Overloaded, true);

    expect_eq(classifyHttpStatus(408, false) == LlmErrorClass.Timeout, true);
    expect_eq(classifyHttpStatus(504, false) == LlmErrorClass.Timeout, true);
    expect_eq(classifyHttpStatus(524, false) == LlmErrorClass.Timeout, true);

    expect_eq(classifyHttpStatus(500, false) == LlmErrorClass.TransientServer, true);
    expect_eq(classifyHttpStatus(502, false) == LlmErrorClass.TransientServer, true);

    expect_eq(classifyHttpStatus(0, false) == LlmErrorClass.Fatal, true);
    expect_eq(classifyHttpStatus(418, false) == LlmErrorClass.Fatal, true);
}

fn testRetryAfterParsingAndPriority()! {
    // Parsing valid formats
    expect_eq(parseRetryAfterMs("5"), 5000);
    expect_eq(parseRetryAfterMs("0"), 0);
    expect_eq(parseRetryAfterMs("  1.5  "), 1500);
    expect_eq(parseRetryAfterMs("0.25s"), 250);
    expect_eq(parseRetryAfterMs("250ms"), 250);
    expect_eq(parseRetryAfterMs("250 MS"), 250);
    expect_eq(parseRetryAfterMs("2s"), 2000);
    expect_eq(parseRetryAfterMs("1m"), 60000);

    // Reject malformed, empty, negative & overflow values
    expect_eq(parseRetryAfterMs("") == null, true);
    expect_eq(parseRetryAfterMs("   ") == null, true);
    expect_eq(parseRetryAfterMs("-5") == null, true);
    expect_eq(parseRetryAfterMs("abc") == null, true);
    expect_eq(parseRetryAfterMs("1.2.3") == null, true);
    expect_eq(parseRetryAfterMs("999999999999999") == null, true);

    const policy = LlmRetryPolicy{
        max_retries: 4,
        base_delay_ms: 500,
        max_delay_ms: 10000,
        jitter_pct: 25,
        allow_context_shrink: true,
        allow_fallback: true,
        fallback_after_consecutive_overload: 3,
    };

    // Retry-After overrides jittered exponential backoff
    const rl_explicit = RateLimitSnapshot{ retry_after_ms: 4200 };
    const d1 = decideLlmRetry(policy, LlmErrorClass.RateLimited, 0, rl_explicit, 12345);
    expect_eq(d1.kind == RetryDecisionKind.RetryAfterDelay, true);
    expect_eq(d1.delay_ms, 4200);
    expect_eq(d1.next_attempt, 1);

    // Retry-After above max_delay_ms is clamped to max_delay_ms
    const rl_huge = RateLimitSnapshot{ retry_after_ms: 90000 };
    const d2 = decideLlmRetry(policy, LlmErrorClass.RateLimited, 0, rl_huge, 12345);
    expect_eq(d2.kind == RetryDecisionKind.RetryAfterDelay, true);
    expect_eq(d2.delay_ms, 10000);

    // Token reset window respected when remaining_tokens == 0 and retry_after_ms == 0
    const rl_tokens = RateLimitSnapshot{
        remaining_requests: 10,
        remaining_tokens: 0,
        reset_requests_ms: 100,
        reset_tokens_ms: 3600,
        retry_after_ms: 0,
    };
    const d3 = decideLlmRetry(policy, LlmErrorClass.RateLimited, 0, rl_tokens, 99);
    expect_eq(d3.kind == RetryDecisionKind.RetryAfterDelay, true);
    expect_eq(d3.delay_ms, 3600);
}

fn testDeterministicJitterExponentialBackoff()! {
    const empty_rl = RateLimitSnapshot{};

    // Pure exponential backoff (jitter_pct == 0)
    const no_jitter = LlmRetryPolicy{
        max_retries: 5,
        base_delay_ms: 500,
        max_delay_ms: 3000,
        jitter_pct: 0,
        allow_context_shrink: true,
        allow_fallback: false,
        fallback_after_consecutive_overload: 0,
    };
    expect_eq(decideLlmRetry(no_jitter, LlmErrorClass.TransientServer, 0, empty_rl, 1).delay_ms, 500);
    expect_eq(decideLlmRetry(no_jitter, LlmErrorClass.TransientServer, 1, empty_rl, 1).delay_ms, 1000);
    expect_eq(decideLlmRetry(no_jitter, LlmErrorClass.TransientServer, 2, empty_rl, 1).delay_ms, 2000);
    expect_eq(decideLlmRetry(no_jitter, LlmErrorClass.TransientServer, 3, empty_rl, 1).delay_ms, 3000); // clamped

    // Jittered backoff (25% around 1000ms => [750, 1250]) is deterministic & bounded
    const jittered = LlmRetryPolicy{
        max_retries: 5,
        base_delay_ms: 500,
        max_delay_ms: 30000,
        jitter_pct: 25,
        allow_context_shrink: true,
        allow_fallback: true,
        fallback_after_consecutive_overload: 2,
    };
    const r_a = decideLlmRetry(jittered, LlmErrorClass.Timeout, 1, empty_rl, 777);
    const r_b = decideLlmRetry(jittered, LlmErrorClass.Timeout, 1, empty_rl, 777);
    expect_eq(r_a.kind == RetryDecisionKind.RetryAfterDelay, true);
    expect_eq(r_a.delay_ms, r_b.delay_ms);
    expect_eq(r_a.delay_ms >= 750 && r_a.delay_ms <= 1250, true);
    expect_eq(r_a.next_attempt, 2);

    // Different seeds spread across the jitter window
    let saw_diff = false;
    let s: u64 = 1;
    while (s <= 16) : (s += 1) {
        const d = decideLlmRetry(jittered, LlmErrorClass.Timeout, 1, empty_rl, s);
        expect_eq(d.delay_ms >= 750 && d.delay_ms <= 1250, true);
        if (d.delay_ms != r_a.delay_ms) {
            saw_diff = true;
        }
    }
    expect_eq(saw_diff, true);
}

fn testContextLengthExceededRecovery()! {
    const empty_rl = RateLimitSnapshot{};

    // Enabled shrink + budget remaining -> ShrinkContextAndRetry
    const p_shrink = LlmRetryPolicy{
        max_retries: 2,
        base_delay_ms: 500,
        max_delay_ms: 10000,
        jitter_pct: 20,
        allow_context_shrink: true,
        allow_fallback: true,
        fallback_after_consecutive_overload: 2,
    };
    const d0 = decideLlmRetry(p_shrink, LlmErrorClass.ContextLengthExceeded, 0, empty_rl, 1);
    expect_eq(d0.kind == RetryDecisionKind.ShrinkContextAndRetry, true);
    expect_eq(d0.delay_ms, 0);
    expect_eq(d0.next_attempt, 1);

    // Shrink retries exhausted -> FallbackProvider (e.g. larger context model)
    const d_exhaust = decideLlmRetry(p_shrink, LlmErrorClass.ContextLengthExceeded, 2, empty_rl, 1);
    expect_eq(d_exhaust.kind == RetryDecisionKind.FallbackProvider, true);

    // Shrink disabled + fallback enabled -> immediate FallbackProvider
    const p_no_shrink = LlmRetryPolicy{
        max_retries: 3,
        base_delay_ms: 500,
        max_delay_ms: 10000,
        jitter_pct: 20,
        allow_context_shrink: false,
        allow_fallback: true,
        fallback_after_consecutive_overload: 2,
    };
    const d_fb = decideLlmRetry(p_no_shrink, LlmErrorClass.ContextLengthExceeded, 0, empty_rl, 1);
    expect_eq(d_fb.kind == RetryDecisionKind.FallbackProvider, true);

    // Both disabled -> Abort
    const p_strict = LlmRetryPolicy{
        max_retries: 3,
        base_delay_ms: 500,
        max_delay_ms: 10000,
        jitter_pct: 20,
        allow_context_shrink: false,
        allow_fallback: false,
        fallback_after_consecutive_overload: 0,
    };
    const d_abort = decideLlmRetry(p_strict, LlmErrorClass.ContextLengthExceeded, 0, empty_rl, 1);
    expect_eq(d_abort.kind == RetryDecisionKind.Abort, true);
}

fn testAbortAndFallbackTransitions()! {
    const empty_rl = RateLimitSnapshot{};
    const policy = LlmRetryPolicy{
        max_retries: 4,
        base_delay_ms: 500,
        max_delay_ms: 10000,
        jitter_pct: 10,
        allow_context_shrink: true,
        allow_fallback: true,
        fallback_after_consecutive_overload: 2,
    };

    // AuthFailed / InvalidRequest / Fatal always Abort immediately
    expect_eq(decideLlmRetry(policy, LlmErrorClass.AuthFailed, 0, empty_rl, 1).kind == RetryDecisionKind.Abort, true);
    expect_eq(decideLlmRetry(policy, LlmErrorClass.InvalidRequest, 0, empty_rl, 1).kind == RetryDecisionKind.Abort, true);
    expect_eq(decideLlmRetry(policy, LlmErrorClass.Fatal, 0, empty_rl, 1).kind == RetryDecisionKind.Abort, true);

    // Consecutive 429 / 529 retries at attempt 0, 1 then proactively falls back at attempt 2 (< max_retries=4)
    expect_eq(decideLlmRetry(policy, LlmErrorClass.RateLimited, 0, empty_rl, 1).kind == RetryDecisionKind.RetryAfterDelay, true);
    expect_eq(decideLlmRetry(policy, LlmErrorClass.Overloaded, 1, empty_rl, 1).kind == RetryDecisionKind.RetryAfterDelay, true);
    expect_eq(decideLlmRetry(policy, LlmErrorClass.RateLimited, 2, empty_rl, 1).kind == RetryDecisionKind.FallbackProvider, true);
    expect_eq(decideLlmRetry(policy, LlmErrorClass.Overloaded, 2, empty_rl, 1).kind == RetryDecisionKind.FallbackProvider, true);

    // TransientServer retries up to max_retries then falls back (or aborts when fallback disabled)
    expect_eq(decideLlmRetry(policy, LlmErrorClass.TransientServer, 3, empty_rl, 1).kind == RetryDecisionKind.RetryAfterDelay, true);
    expect_eq(decideLlmRetry(policy, LlmErrorClass.TransientServer, 4, empty_rl, 1).kind == RetryDecisionKind.FallbackProvider, true);

    const no_fb = LlmRetryPolicy{
        max_retries: 2,
        base_delay_ms: 500,
        max_delay_ms: 10000,
        jitter_pct: 10,
        allow_context_shrink: true,
        allow_fallback: false,
        fallback_after_consecutive_overload: 2,
    };
    expect_eq(decideLlmRetry(no_fb, LlmErrorClass.TransientServer, 2, empty_rl, 1).kind == RetryDecisionKind.Abort, true);
}

fn main() {
    test("1. agent retry_policy: HTTP status code & context-overflow classification", () => {
        testHttpStatusClassification()?;
    });
    test("2. agent retry_policy: Retry-After parsing, priority & max_delay_ms clamping", () => {
        testRetryAfterParsingAndPriority()?;
    });
    test("3. agent retry_policy: deterministic jittered exponential backoff", () => {
        testDeterministicJitterExponentialBackoff()?;
    });
    test("4. agent retry_policy: ContextLengthExceeded shrink-and-retry & fallback", () => {
        testContextLengthExceededRecovery()?;
    });
    test("5. agent retry_policy: immediate abort, consecutive 429/529 & exhaustion fallback", () => {
        testAbortAndFallbackTransitions()?;
    });
}
