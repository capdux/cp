// Origin: Pure-CP Stdlib Test Suite (lib/web/hedged_policy.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Hedged Requests & Retry Budget Storm-Prevention Controller:
//   1. Slow primary past hedge_delay_ms triggers hedge; hedge wins race and cancels in-flight primary
//   2. Non-idempotent requests strictly disable hedging and preserve retry budget
//   3. RetryBudget 10% ratio storm cap + per-second floor replenishment & burst clamping
//   4. Budget-exhausted graceful degradation to single-request wait
//   5. Overall total_timeout_ms give-up cancellation & all-attempts-failed termination
//   6. Invalid configs, clock-skew safety & invalid attempt index defensive guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HEDGED_MAX_ATTEMPTS,
    RetryBudgetConfig,
    RetryBudget,
    HedgedRequestConfig,
    HedgedAction,
    HedgedAttemptState,
    HedgedRequestTracker,
} from "cap:web";

// ============================================================================
// Test 1: Slow Primary Triggers Hedge -> Hedge Wins Race & Cancels Siblings
// ============================================================================

fn testHedgeLaunchAndWinnerCancellation()! {
    let budget = cap.web.RetryBudget.init(RetryBudgetConfig.default(), 1000) ?? return error.InitFailed;
    budget.recordRequest(1000);

    let tracker = HedgedRequestTracker.init(HedgedRequestConfig{
        hedge_delay_ms: 50,
        max_hedges: 2,
        total_timeout_ms: 300,
        idempotent_only: true,
    }, true, 1000) ?? return error.InitFailed;

    // Before hedge_delay_ms: Wait
    expect_eq(tracker.evaluate(1030, &budget) == HedgedAction.Wait, true);
    expect_eq(tracker.hedges_launched, 0);

    // At +50ms: launches hedge attempt 1
    expect_eq(tracker.evaluate(1050, &budget) == HedgedAction.LaunchHedge, true);
    expect_eq(tracker.hedges_launched, 1);
    expect_eq(tracker.latestAttemptIdx(), 1);
    expect_eq(tracker.inflight_count, 2);

    // At +100ms: launches hedge attempt 2 (max_hedges = 2)
    expect_eq(tracker.evaluate(1100, &budget) == HedgedAction.LaunchHedge, true);
    expect_eq(tracker.hedges_launched, 2);
    expect_eq(tracker.latestAttemptIdx(), 2);
    expect_eq(tracker.inflight_count, 3);

    // At +160ms: max_hedges reached -> Wait
    expect_eq(tracker.evaluate(1160, &budget) == HedgedAction.Wait, true);

    // Hedge attempt 1 wins the race -> cancels attempt 0 and attempt 2!
    const win = tracker.onReplicaSucceeded(1);
    expect_eq(win.accepted, true);
    expect_eq(win.winner_attempt, 1);
    expect_eq(win.cancelled_inflight, 2);
    expect_eq(tracker.winner_attempt ?? 99, 1);
    expect_eq(tracker.attemptState(0) == HedgedAttemptState.Cancelled, true);
    expect_eq(tracker.attemptState(1) == HedgedAttemptState.Succeeded, true);
    expect_eq(tracker.attemptState(2) == HedgedAttemptState.Cancelled, true);

    // Late response from attempt 0 is rejected; evaluate reports Complete
    expect_eq(tracker.onReplicaSucceeded(0).accepted, false);
    expect_eq(tracker.evaluate(1170, &budget) == HedgedAction.Complete, true);
}

// ============================================================================
// Test 2: Non-Idempotent Requests Strictly Forbid Hedging
// ============================================================================

fn testNonIdempotentForbidsHedging()! {
    let budget = RetryBudget.init(RetryBudgetConfig.default(), 1000) ?? return error.InitFailed;
    const before_avail = budget.availableRetries();

    // Non-idempotent POST with idempotent_only = true
    let tracker = HedgedRequestTracker.init(HedgedRequestConfig{
        hedge_delay_ms: 40,
        max_hedges: 2,
        total_timeout_ms: 200,
        idempotent_only: true,
    }, false, 1000) ?? return error.InitFailed;

    // Even at +100ms (well past hedge_delay_ms = 40), no hedge is launched and budget untouched
    expect_eq(tracker.evaluate(1100, &budget) == HedgedAction.Wait, true);
    expect_eq(tracker.hedges_launched, 0);
    expect_eq(budget.availableRetries(), before_avail);

    // Primary completes at +120ms -> 0 cancelled siblings
    const win = tracker.onReplicaSucceeded(0);
    expect_eq(win.accepted, true);
    expect_eq(win.winner_attempt, 0);
    expect_eq(win.cancelled_inflight, 0);
}

// ============================================================================
// Test 3: RetryBudget 10% Ratio Storm Cap & Floor Replenishment
// ============================================================================

fn testRetryBudgetRatioAndFloorRefill()! {
    // Pure 10% ratio mode (min_retries_per_sec = 0)
    let ratio_budget = RetryBudget.init(RetryBudgetConfig{
        retry_ratio_bps: 1000, // 10%
        min_retries_per_sec: 0,
        window_ms: 1000,
        max_burst_retries: 50,
    }, 0) ?? return error.InitFailed;

    // 100 normal requests -> exactly 10 retries allowed
    let i: usize = 0;
    while (i < 100) : (i += 1) {
        ratio_budget.recordRequest(100);
    }
    expect_eq(ratio_budget.availableRetries(), 10);

    let allowed: usize = 0;
    i = 0;
    while (i < 15) : (i += 1) {
        if (ratio_budget.tryConsumeRetry(100)) {
            allowed += 1;
        }
    }
    expect_eq(allowed, 10);
    const st = ratio_budget.stats();
    expect_eq(st.total_requests, 100);
    expect_eq(st.retries_allowed, 10);
    expect_eq(st.retries_rejected, 5);

    // Floor replenishment mode: 3 retries/sec, burst cap 5
    let floor_budget = RetryBudget.init(RetryBudgetConfig{
        retry_ratio_bps: 1000,
        min_retries_per_sec: 3,
        window_ms: 1000,
        max_burst_retries: 5,
    }, 1000) ?? return error.InitFailed;

    // Starts with 3 floor credits -> drain all 3
    expect_eq(floor_budget.tryConsumeRetry(1000), true);
    expect_eq(floor_budget.tryConsumeRetry(1000), true);
    expect_eq(floor_budget.tryConsumeRetry(1000), true);
    expect_eq(floor_budget.tryConsumeRetry(1000), false);

    // Advance 2000ms (2 windows * 3 = 6, clamped to max_burst_retries = 5)
    expect_eq(floor_budget.tryConsumeRetry(3000), true);
    expect_eq(floor_budget.availableRetries(), 4);
}

// ============================================================================
// Test 4: Budget Exhaustion Graceful Degradation to Single-Request Wait
// ============================================================================

fn testBudgetExhaustedGracefulDegradation()! {
    let empty_budget = RetryBudget.init(RetryBudgetConfig{
        retry_ratio_bps: 1000,
        min_retries_per_sec: 0,
        window_ms: 1000,
        max_burst_retries: 10,
    }, 1000) ?? return error.InitFailed;

    // Only 1 normal request = 1000 bps (< 10000 bps needed for 1 hedge)
    empty_budget.recordRequest(1000);

    let tracker = HedgedRequestTracker.init(HedgedRequestConfig{
        hedge_delay_ms: 50,
        max_hedges: 2,
        total_timeout_ms: 300,
        idempotent_only: true,
    }, true, 1000) ?? return error.InitFailed;

    // Past hedge_delay_ms, budget rejects hedge -> degrades gracefully to Wait!
    expect_eq(tracker.evaluate(1060, &empty_budget) == HedgedAction.Wait, true);
    expect_eq(tracker.hedges_launched, 0);
    expect_eq(tracker.hedges_blocked_by_budget, 1);

    // Primary eventually finishes at +120ms -> request still succeeds
    const win = tracker.onReplicaSucceeded(0);
    expect_eq(win.accepted, true);
    expect_eq(win.winner_attempt, 0);
}

// ============================================================================
// Test 5: Total Timeout Give-Up & All-Attempts-Failed Termination
// ============================================================================

fn testTimeoutAndAllFailedTermination()! {
    let budget = RetryBudget.init(RetryBudgetConfig.default(), 1000) ?? return error.InitFailed;

    // Case A: Exceeds total_timeout_ms with 2 attempts in-flight -> cancels both and gives up
    let t1 = HedgedRequestTracker.init(HedgedRequestConfig{
        hedge_delay_ms: 50,
        max_hedges: 1,
        total_timeout_ms: 120,
        idempotent_only: true,
    }, true, 1000) ?? return error.InitFailed;

    expect_eq(t1.evaluate(1050, &budget) == HedgedAction.LaunchHedge, true);
    expect_eq(t1.inflight_count, 2);
    expect_eq(t1.evaluate(1120, &budget) == HedgedAction.GiveUpTimeout, true);
    expect_eq(t1.inflight_count, 0);
    expect_eq(t1.attemptState(0) == HedgedAttemptState.Cancelled, true);
    expect_eq(t1.attemptState(1) == HedgedAttemptState.Cancelled, true);

    // Case B: Primary fails immediately -> triggers hedge immediately; hedge also fails -> GiveUpTimeout
    let t2 = HedgedRequestTracker.init(HedgedRequestConfig{
        hedge_delay_ms: 100,
        max_hedges: 1,
        total_timeout_ms: 500,
        idempotent_only: true,
    }, true, 2000) ?? return error.InitFailed;

    expect_eq(t2.onReplicaFailed(0), true);
    // Even at +10ms (< hedge_delay_ms), since inflight_count == 0, hedge launches immediately!
    expect_eq(t2.evaluate(2010, &budget) == HedgedAction.LaunchHedge, true);
    expect_eq(t2.onReplicaFailed(1), true);
    expect_eq(t2.evaluate(2020, &budget) == HedgedAction.GiveUpTimeout, true);
}

// ============================================================================
// Test 6: Defensive Invalid Config & Boundary Guards
// ============================================================================

fn testDefensiveGuards()! {
    // Invalid RetryBudgetConfig
    expect_eq(RetryBudget.init(RetryBudgetConfig{ retry_ratio_bps: 0 }, 0) == null, true);
    expect_eq(RetryBudget.init(RetryBudgetConfig{ retry_ratio_bps: 10001 }, 0) == null, true);
    expect_eq(RetryBudget.init(RetryBudgetConfig{ window_ms: 0 }, 0) == null, true);
    expect_eq(RetryBudget.init(RetryBudgetConfig{ max_burst_retries: 0 }, 0) == null, true);

    // Invalid HedgedRequestConfig
    expect_eq(HedgedRequestTracker.init(HedgedRequestConfig{ hedge_delay_ms: 0 }, true, 0) == null, true);
    expect_eq(HedgedRequestTracker.init(HedgedRequestConfig{ total_timeout_ms: 0 }, true, 0) == null, true);
    expect_eq(HedgedRequestTracker.init(HedgedRequestConfig{ max_hedges: HEDGED_MAX_ATTEMPTS as u8 }, true, 0) == null, true);

    // Clock rollback safety & invalid attempt index guards
    let budget = RetryBudget.init(RetryBudgetConfig.default(), 5000) ?? return error.InitFailed;
    budget.recordRequest(4000); // older timestamp must not underflow
    let tracker = HedgedRequestTracker.init(HedgedRequestConfig.default(), true, 5000) ?? return error.InitFailed;
    expect_eq(tracker.evaluate(4000, &budget) == HedgedAction.Wait, true);
    expect_eq(tracker.onReplicaSucceeded(7).accepted, false);
    expect_eq(tracker.onReplicaFailed(99), false);
    expect_eq(tracker.attemptState(99) == HedgedAttemptState.Unused, true);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("web/hedged_policy: slow primary hedge launch & first-winner sibling cancellation", testHedgeLaunchAndWinnerCancellation);
test("web/hedged_policy: non-idempotent request strictly disables hedging", testNonIdempotentForbidsHedging);
test("web/hedged_policy: RetryBudget 10% storm cap & per-second floor replenishment", testRetryBudgetRatioAndFloorRefill);
test("web/hedged_policy: budget-exhausted graceful degradation to single wait", testBudgetExhaustedGracefulDegradation);
test("web/hedged_policy: total_timeout_ms give-up & all-attempts-failed termination", testTimeoutAndAllFailedTermination);
test("web/hedged_policy: invalid configs, clock rollback & attempt index guards", testDefensiveGuards);
