// Origin: Pure-CP Stdlib Test Suite (lib/net/adaptive_retry.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Adaptive Retry & Anti-Storm Budget Controller:
//   1. All 4 jitter modes (none, full_jitter, equal_jitter, decorrelated_jitter) bounds, determinism & overflow cap
//   2. Server Retry-After adoption, max_retry_after_ms clamping & respect_retry_after toggle
//   3. Anti-storm global Retry Budget shedding (`reject_budget_exhausted`) & normal-request token recovery
//   4. Low-QPS `min_retries_per_sec` floor refill, bucket cap & monotonic clock-rollback guard
//   5. Error class retryability guard (`client_4xx` / `cancelled` rejected without consuming budget)
//   6. Per-request deadline enforcement (`deadline_ms`) with overflow-safe remaining-time check
//   7. Invalid configuration & `max_attempts` boundary guards
//   8. Full `RetryControllerStats` accounting invariant across all 5 decision kinds

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RETRY_BPS_DENOM,
    RETRY_DEFAULT_RATIO_BPS,
    RETRY_DEFAULT_MAX_DELAY_MS,
    RetryJitterMode,
    RetryErrorClass,
    RetryDecisionKind,
    RetryPolicyConfig,
    RetryBudgetConfig,
    RetryDecision,
    RetryControllerStats,
    isRetryableError,
    computeBackoffDelayMs,
    AdaptiveRetryController,
} from "cap:net";

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

// ============================================================================
// Test 1: All 4 Jitter Modes Bounds, Determinism & Overflow Cap
// ============================================================================

fn testAllFourJitterModesAndOverflowSafety()! {
    let rng: u64 = 123456789;

    // 1) Pure exponential (none) with cap = 1000ms
    const cfg_none = RetryPolicyConfig{
        max_attempts: 10,
        base_delay_ms: 100,
        max_delay_ms: 1000,
        jitter_mode: RetryJitterMode.none,
        respect_retry_after: true,
        max_retry_after_ms: 5000,
    };
    expect_eq(cap.net.computeBackoffDelayMs(cfg_none, 1, 0, &rng), 100);
    expect_eq(computeBackoffDelayMs(cfg_none, 2, 100, &rng), 200);
    expect_eq(computeBackoffDelayMs(cfg_none, 3, 200, &rng), 400);
    expect_eq(computeBackoffDelayMs(cfg_none, 4, 400, &rng), 800);
    expect_eq(computeBackoffDelayMs(cfg_none, 5, 800, &rng), 1000);
    // Huge attempt index must saturate cleanly without u64 shift overflow
    expect_eq(computeBackoffDelayMs(cfg_none, 200, 1000, &rng), 1000);

    // 2) Full jitter in [0, 400] for attempt 3
    const cfg_full = RetryPolicyConfig{
        max_attempts: 5,
        base_delay_ms: 100,
        max_delay_ms: 1000,
        jitter_mode: RetryJitterMode.full_jitter,
        respect_retry_after: true,
        max_retry_after_ms: 5000,
    };
    let saw_lt_400: bool = false;
    let saw_gt_0: bool = false;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        const d = computeBackoffDelayMs(cfg_full, 3, 200, &rng);
        expect_eq(d <= 400, true);
        if (d < 400) saw_lt_400 = true;
        if (d > 0) saw_gt_0 = true;
    }
    expect_eq(saw_lt_400 && saw_gt_0, true);

    // 3) Equal jitter in [200, 400] for attempt 3
    const cfg_equal = RetryPolicyConfig{
        max_attempts: 5,
        base_delay_ms: 100,
        max_delay_ms: 1000,
        jitter_mode: RetryJitterMode.equal_jitter,
        respect_retry_after: true,
        max_retry_after_ms: 5000,
    };
    let e: usize = 0;
    while (e < 64) : (e += 1) {
        const d = computeBackoffDelayMs(cfg_equal, 3, 200, &rng);
        expect_eq(d >= 200 && d <= 400, true);
    }

    // 4) Decorrelated jitter in [100, min(1000, prev * 3)]
    const cfg_decorr = RetryPolicyConfig{
        max_attempts: 5,
        base_delay_ms: 100,
        max_delay_ms: 1000,
        jitter_mode: RetryJitterMode.decorrelated_jitter,
        respect_retry_after: true,
        max_retry_after_ms: 5000,
    };
    let r1: u64 = 42;
    let r2: u64 = 42;
    let prev: u64 = 150;
    let k: usize = 0;
    while (k < 32) : (k += 1) {
        const d1 = computeBackoffDelayMs(cfg_decorr, 2, prev, &r1);
        const d2 = computeBackoffDelayMs(cfg_decorr, 2, prev, &r2);
        expect_eq(d1, d2); // deterministic under identical seed
        expect_eq(d1 >= 100 && d1 <= 450, true);
    }
}

// ============================================================================
// Test 2: Retry-After Adoption & Max Clamping
// ============================================================================

fn testRetryAfterClampingAndToggle()! {
    const policy = RetryPolicyConfig{
        max_attempts: 5,
        base_delay_ms: 100,
        max_delay_ms: 2000,
        jitter_mode: RetryJitterMode.none,
        respect_retry_after: true,
        max_retry_after_ms: 5000,
    };
    let ctrl = AdaptiveRetryController.init(policy, RetryBudgetConfig.defaults(), 7, 1000) ?? return;

    // Within cap: exact 2500ms honored
    const d1 = ctrl.evaluateRetry(RetryErrorClass.rate_limited_429, 1, 0, 2500, 1000, 0);
    expect_eq(d1.kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(d1.delay_ms, 2500);
    expect_eq(d1.next_attempt, 2);
    expect_eq(d1.used_retry_after, true);
    expect_eq(d1.clamped_retry_after, false);

    // Oversized Retry-After (999_999ms) clamped to max_retry_after_ms (5000ms)
    const d2 = ctrl.evaluateRetry(RetryErrorClass.rate_limited_429, 2, 2500, 999_999, 1000, 0);
    expect_eq(d2.kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(d2.delay_ms, 5000);
    expect_eq(d2.used_retry_after, true);
    expect_eq(d2.clamped_retry_after, true);

    // Policy with respect_retry_after == false ignores server hint
    const policy_ign = RetryPolicyConfig{
        max_attempts: 5,
        base_delay_ms: 100,
        max_delay_ms: 2000,
        jitter_mode: RetryJitterMode.none,
        respect_retry_after: false,
        max_retry_after_ms: 5000,
    };
    let ctrl_ign = AdaptiveRetryController.init(policy_ign, RetryBudgetConfig.defaults(), 7, 1000) ?? return;
    const d3 = ctrl_ign.evaluateRetry(RetryErrorClass.rate_limited_429, 1, 0, 2500, 1000, 0);
    expect_eq(d3.kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(d3.delay_ms, 100);
    expect_eq(d3.used_retry_after, false);
}

// ============================================================================
// Test 3: Anti-Storm Retry Budget Shedding & Normal Request Recovery
// ============================================================================

fn testAntiStormRetryBudgetSheddingAndRecovery()! {
    const budget = RetryBudgetConfig{
        retry_ratio_bps: 2000, // 20% -> 5 normal requests earn 1 retry token
        min_retries_per_sec: 0,
        max_budget_tokens: 50.0,
        initial_tokens: 3.0,
    };
    let ctrl = AdaptiveRetryController.init(RetryPolicyConfig.defaults(), budget, 99, 1000) ?? return;

    // Downstream outage: 3 retries succeed, 4th & 5th shed to prevent retry storm
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.server_5xx, 1, 0, 0, 1000, 0).kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.server_5xx, 1, 0, 0, 1000, 0).kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.server_5xx, 1, 0, 0, 1000, 0).kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.server_5xx, 1, 0, 0, 1000, 0).kind == RetryDecisionKind.reject_budget_exhausted, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.server_5xx, 1, 0, 0, 1000, 0).kind == RetryDecisionKind.reject_budget_exhausted, true);

    // Record 10 normal requests -> earns 10 * 0.20 = 2.0 retry tokens
    let r: usize = 0;
    while (r < 10) : (r += 1) {
        ctrl.recordNormalRequest(1000);
    }
    expect_eq(absF64(ctrl.availableBudgetTokens() - 2.0) < 1.0e-6, true);

    // Exactly 2 more retries allowed, 3rd shed again
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.timeout, 1, 0, 0, 1000, 0).kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.timeout, 1, 0, 0, 1000, 0).kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.timeout, 1, 0, 0, 1000, 0).kind == RetryDecisionKind.reject_budget_exhausted, true);
}

// ============================================================================
// Test 4: Low-QPS Floor Refill, Bucket Cap & Clock-Rollback Protection
// ============================================================================

fn testFloorRefillAndClockRollbackGuard()! {
    const budget = RetryBudgetConfig{
        retry_ratio_bps: 0,
        min_retries_per_sec: 5, // 5 tokens / sec -> 2 tokens per 400ms
        max_budget_tokens: 10.0,
        initial_tokens: 0.0,
    };
    let ctrl = AdaptiveRetryController.init(RetryPolicyConfig.defaults(), budget, 1, 1000) ?? return;

    // Clock rollback (now=500 < last_refill=1000) must not underflow or add tokens
    ctrl.recordNormalRequest(500);
    expect_eq(ctrl.availableBudgetTokens() == 0.0, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.transient_network, 1, 0, 0, 500, 0).kind == RetryDecisionKind.reject_budget_exhausted, true);

    // Advance 400ms (t=1400) -> earns 2.0 floor tokens
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.transient_network, 1, 0, 0, 1400, 0).kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.transient_network, 1, 0, 0, 1400, 0).kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.transient_network, 1, 0, 0, 1400, 0).kind == RetryDecisionKind.reject_budget_exhausted, true);

    // Advance 10 seconds -> capped strictly at max_budget_tokens (10.0)
    ctrl.recordNormalRequest(11_400);
    expect_eq(absF64(ctrl.availableBudgetTokens() - 10.0) < 1.0e-6, true);
}

// ============================================================================
// Test 5: Error Class Retryability Guard Without Consuming Budget
// ============================================================================

fn testErrorClassRetryabilityPreservesBudget()! {
    expect_eq(isRetryableError(RetryErrorClass.transient_network), true);
    expect_eq(isRetryableError(RetryErrorClass.timeout), true);
    expect_eq(isRetryableError(RetryErrorClass.rate_limited_429), true);
    expect_eq(isRetryableError(RetryErrorClass.server_5xx), true);
    expect_eq(isRetryableError(RetryErrorClass.client_4xx), false);
    expect_eq(isRetryableError(RetryErrorClass.cancelled), false);

    const budget = RetryBudgetConfig{
        retry_ratio_bps: 2000,
        min_retries_per_sec: 0,
        max_budget_tokens: 10.0,
        initial_tokens: 2.0,
    };
    let ctrl = AdaptiveRetryController.init(RetryPolicyConfig.defaults(), budget, 5, 100) ?? return;

    expect_eq(ctrl.evaluateRetry(RetryErrorClass.client_4xx, 1, 0, 0, 100, 0).kind == RetryDecisionKind.reject_non_retryable, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.cancelled, 1, 0, 0, 100, 0).kind == RetryDecisionKind.reject_non_retryable, true);
    // Budget must remain completely untouched (2.0)
    expect_eq(absF64(ctrl.availableBudgetTokens() - 2.0) < 1.0e-6, true);
}

// ============================================================================
// Test 6: Per-Request Deadline Enforcement & Overflow-Safe Check
// ============================================================================

fn testDeadlineEnforcementAndOverflowSafety()! {
    const policy = RetryPolicyConfig{
        max_attempts: 5,
        base_delay_ms: 200,
        max_delay_ms: 5000,
        jitter_mode: RetryJitterMode.none,
        respect_retry_after: true,
        max_retry_after_ms: 5000,
    };
    const budget = RetryBudgetConfig{
        retry_ratio_bps: 2000,
        min_retries_per_sec: 0,
        max_budget_tokens: 10.0,
        initial_tokens: 5.0,
    };
    let ctrl = AdaptiveRetryController.init(policy, budget, 11, 10_000) ?? return;

    // Attempt 1 delay=200ms <= remaining 300ms -> allowed (consumes 1 token -> 4.0)
    const ok = ctrl.evaluateRetry(RetryErrorClass.timeout, 1, 0, 0, 10_000, 10_300);
    expect_eq(ok.kind == RetryDecisionKind.retry_allowed, true);
    expect_eq(ok.delay_ms, 200);

    // Attempt 2 delay=400ms > remaining 300ms -> reject_deadline_exceeded (does NOT consume token -> still 4.0)
    const rej1 = ctrl.evaluateRetry(RetryErrorClass.timeout, 2, 200, 0, 10_000, 10_300);
    expect_eq(rej1.kind == RetryDecisionKind.reject_deadline_exceeded, true);
    expect_eq(absF64(ctrl.availableBudgetTokens() - 4.0) < 1.0e-6, true);

    // Already past deadline
    const rej2 = ctrl.evaluateRetry(RetryErrorClass.timeout, 1, 0, 0, 10_500, 10_300);
    expect_eq(rej2.kind == RetryDecisionKind.reject_deadline_exceeded, true);

    // Overflow-safe check near u64::MAX
    const near_max: u64 = 0xFFFFFFFFFFFFFFF0;
    const u64_max: u64 = 0xFFFFFFFFFFFFFFFF;
    const rej3 = ctrl.evaluateRetry(RetryErrorClass.timeout, 1, 0, 0, near_max, u64_max);
    expect_eq(rej3.kind == RetryDecisionKind.reject_deadline_exceeded, true);
}

// ============================================================================
// Test 7: Invalid Config & max_attempts Boundary Guards
// ============================================================================

fn testInvalidConfigAndMaxAttemptsGuards()! {
    const good_p = RetryPolicyConfig.defaults();
    const good_b = RetryBudgetConfig.defaults();

    // Invalid policies
    expect_eq(AdaptiveRetryController.init(RetryPolicyConfig{ max_attempts: 0 }, good_b, 1, 0) == null, true);
    expect_eq(AdaptiveRetryController.init(RetryPolicyConfig{ base_delay_ms: 0 }, good_b, 1, 0) == null, true);
    expect_eq(AdaptiveRetryController.init(RetryPolicyConfig{ base_delay_ms: 500, max_delay_ms: 100 }, good_b, 1, 0) == null, true);
    expect_eq(AdaptiveRetryController.init(RetryPolicyConfig{ max_retry_after_ms: 0 }, good_b, 1, 0) == null, true);

    // Invalid budgets
    expect_eq(AdaptiveRetryController.init(good_p, RetryBudgetConfig{ retry_ratio_bps: 10_001 }, 1, 0) == null, true);
    expect_eq(AdaptiveRetryController.init(good_p, RetryBudgetConfig{ max_budget_tokens: 0.5 }, 1, 0) == null, true);
    expect_eq(AdaptiveRetryController.init(good_p, RetryBudgetConfig{ max_budget_tokens: 10.0, initial_tokens: 12.0 }, 1, 0) == null, true);
    expect_eq(AdaptiveRetryController.init(good_p, RetryBudgetConfig{ initial_tokens: std.math.nan(f64) }, 1, 0) == null, true);

    // max_attempts boundary (max_attempts = 3 -> attempt 1 & 2 allowed, attempt 0 & 3 rejected)
    let ctrl = AdaptiveRetryController.init(RetryPolicyConfig{ max_attempts: 3 }, good_b, 1, 0) ?? return;
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.server_5xx, 0, 0, 0, 0, 0).kind == RetryDecisionKind.reject_max_attempts, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.server_5xx, 3, 0, 0, 0, 0).kind == RetryDecisionKind.reject_max_attempts, true);
    expect_eq(ctrl.evaluateRetry(RetryErrorClass.server_5xx, 2, 0, 0, 0, 0).kind == RetryDecisionKind.retry_allowed, true);

    expect_eq(RETRY_BPS_DENOM, 10_000);
    expect_eq(RETRY_DEFAULT_RATIO_BPS, 2_000);
    expect_eq(RETRY_DEFAULT_MAX_DELAY_MS, 30_000);
}

// ============================================================================
// Test 8: Full `RetryControllerStats` Accounting Invariant
// ============================================================================

fn testStatsAccountingInvariant()! {
    const policy = RetryPolicyConfig{
        max_attempts: 3,
        base_delay_ms: 100,
        max_delay_ms: 1000,
        jitter_mode: RetryJitterMode.none,
        respect_retry_after: true,
        max_retry_after_ms: 5000,
    };
    const budget = RetryBudgetConfig{
        retry_ratio_bps: 2000,
        min_retries_per_sec: 0,
        max_budget_tokens: 10.0,
        initial_tokens: 1.0,
    };
    let ctrl = AdaptiveRetryController.init(policy, budget, 3, 1000) ?? return;

    ctrl.recordNormalRequest(1000); // tokens = 1.2
    * = ctrl.evaluateRetry(RetryErrorClass.server_5xx, 1, 0, 0, 1000, 0);          // allowed (tokens = 0.2)
    * = ctrl.evaluateRetry(RetryErrorClass.client_4xx, 1, 0, 0, 1000, 0);          // non_retryable
    * = ctrl.evaluateRetry(RetryErrorClass.server_5xx, 3, 0, 0, 1000, 0);          // max_attempts
    * = ctrl.evaluateRetry(RetryErrorClass.timeout, 1, 0, 0, 1000, 1050);          // deadline
    * = ctrl.evaluateRetry(RetryErrorClass.transient_network, 1, 0, 0, 1000, 0);   // budget_exhausted

    const st: RetryControllerStats = ctrl.stats();
    expect_eq(st.total_requests_recorded, 1);
    expect_eq(st.retries_allowed, 1);
    expect_eq(st.rejected_non_retryable, 1);
    expect_eq(st.rejected_max_attempts, 1);
    expect_eq(st.rejected_deadline, 1);
    expect_eq(st.rejected_budget, 1);
    expect_eq(
        st.total_evaluations,
        st.retries_allowed + st.rejected_non_retryable + st.rejected_max_attempts + st.rejected_deadline + st.rejected_budget,
    );
}

// ============================================================================
// Suite Registration
// ============================================================================

test("net.adaptive_retry: all 4 jitter modes bounds, determinism & overflow cap", testAllFourJitterModesAndOverflowSafety);
test("net.adaptive_retry: Retry-After adoption, clamping & respect toggle", testRetryAfterClampingAndToggle);
test("net.adaptive_retry: anti-storm Retry Budget shedding & normal-request recovery", testAntiStormRetryBudgetSheddingAndRecovery);
test("net.adaptive_retry: min_retries_per_sec floor refill, cap & clock-rollback guard", testFloorRefillAndClockRollbackGuard);
test("net.adaptive_retry: non-retryable error classes preserve budget tokens", testErrorClassRetryabilityPreservesBudget);
test("net.adaptive_retry: per-request deadline enforcement & overflow-safe subtraction", testDeadlineEnforcementAndOverflowSafety);
test("net.adaptive_retry: invalid config & max_attempts boundary guards", testInvalidConfigAndMaxAttemptsGuards);
test("net.adaptive_retry: full RetryControllerStats accounting invariant", testStatsAccountingInvariant);
