// Source: cp/cap/test/test_agent_budget_production_deep.cp (Production deep tests for cap:agent budget/guardrail)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BudgetStatus,
    ModelPricing,
    BudgetLimits,
    AgentBudget,
    LoopAction,
    CallSignature,
    LoopGuard,
    CircuitState,
    ToolCircuitBreaker,
    ContextStatus,
    ContextWindowAdvisor,
} from "cap:agent";

test("1. AgentBudget exact micro-USD integer pricing, soft warning, and 4D hard enforcement", () => {
    // $3.00 / 1M prompt, $15.00 / 1M completion, $0.30 / 1M cached
    const pricing = ModelPricing.init(3000000, 15000000, 300000);
    // 100k prompt ($0.30 = 300_000) + 10k completion ($0.15 = 150_000) + 100k cached ($0.03 = 30_000) = 480_000 micro-USD
    expect_eq(pricing.computeMicroUsd(100000, 10000, 100000), 480000);

    const limits = BudgetLimits {
        max_total_tokens: 50000,
        max_cost_micro_usd: 100000, // $0.10
        max_steps: 5,
        max_elapsed_ms: 10000,
        soft_limit_pct: 80,
    };
    let b = AgentBudget.init(limits) ?? unreachable;
    expect_eq(b.status() == BudgetStatus.Ok, true);
    expect_eq(b.isExhausted(), false);

    // Step 1: 10k prompt ($0.03 = 30_000), 1k completion ($0.015 = 15_000) -> 45_000 micro-USD, 11k tokens
    const s1 = b.recordStep(10000, 1000, 0, pricing, 2000);
    expect_eq(s1 == BudgetStatus.Ok, true);
    expect_eq(b.cost_micro_usd, 45000);
    expect_eq(b.totalTokens(), 11000);
    expect_eq(b.remainingTokens(), 39000);
    expect_eq(b.remainingCostMicroUsd(), 55000);
    expect_eq(b.remainingSteps(), 4);

    // Step 2: another 45_000 micro-USD -> total 90_000 >= 80% soft limit -> SoftLimit
    const s2 = b.recordStep(10000, 1000, 0, pricing, 2000);
    expect_eq(s2 == BudgetStatus.SoftLimit, true);
    expect_eq(b.isExhausted(), false);

    // Step 3: pushes cost to 135_000 > 100_000 -> ExceededCost
    const s3 = b.recordStep(10000, 1000, 0, pricing, 1000);
    expect_eq(s3 == BudgetStatus.ExceededCost, true);
    expect_eq(b.isExhausted(), true);

    b.reset();
    expect_eq(b.totalTokens(), 0);
    expect_eq(b.cost_micro_usd, 0);
    expect_eq(b.steps, 0);
});

test("2. AgentBudget enforces token, step, and wall-time hard limits independently", () => {
    const free = ModelPricing.init(0, 0, 0);

    // ExceededTokens
    let bt = AgentBudget.init(BudgetLimits {
        max_total_tokens: 1000,
        max_cost_micro_usd: 0,
        max_steps: 0,
        max_elapsed_ms: 0,
        soft_limit_pct: 80,
    }) ?? unreachable;
    expect_eq(bt.recordStep(600, 500, 0, free, 10) == BudgetStatus.ExceededTokens, true);

    // ExceededSteps
    let bs = AgentBudget.init(BudgetLimits {
        max_total_tokens: 0,
        max_cost_micro_usd: 0,
        max_steps: 2,
        max_elapsed_ms: 0,
        soft_limit_pct: 0,
    }) ?? unreachable;
    expect_eq(bs.recordStep(10, 10, 0, free, 10) == BudgetStatus.Ok, true);
    expect_eq(bs.recordStep(10, 10, 0, free, 10) == BudgetStatus.Ok, true);
    expect_eq(bs.recordStep(10, 10, 0, free, 10) == BudgetStatus.ExceededSteps, true);

    // ExceededTime
    let bm = AgentBudget.init(BudgetLimits {
        max_total_tokens: 0,
        max_cost_micro_usd: 0,
        max_steps: 0,
        max_elapsed_ms: 500,
        soft_limit_pct: 0,
    }) ?? unreachable;
    expect_eq(bm.recordStep(10, 10, 0, free, 600) == BudgetStatus.ExceededTime, true);
});

test("3. LoopGuard detects consecutive repeats (A-A-A) and alternating ping-pong deadlocks (A-B-A-B)", () => {
    let ring: [8]CallSignature = std.mem.zeroes([8]CallSignature);
    let ring_s: []CallSignature = ring[0..];
    let guard = LoopGuard.init(&ring_s, 3, 2) ?? unreachable;

    const sig_a = CallSignature.fromBytes("search", "{\"q\":\"cp\"}");
    const sig_b = CallSignature.fromBytes("read", "{\"f\":\"a.cp\"}");
    const sig_c = CallSignature.fromBytes("edit", "{\"f\":\"a.cp\"}");

    // 3 consecutive identical calls -> RepeatLoop on 3rd
    expect_eq(guard.recordAndCheck(sig_a) == LoopAction.Allow, true);
    expect_eq(guard.recordAndCheck(sig_a) == LoopAction.Allow, true);
    expect_eq(guard.recordAndCheck(sig_a) == LoopAction.RepeatLoop, true);

    // Reset & test A-B-A-B ping-pong deadlock
    guard.reset();
    expect_eq(guard.recordAndCheck(sig_c) == LoopAction.Allow, true);
    expect_eq(guard.recordAndCheck(sig_a) == LoopAction.Allow, true);
    expect_eq(guard.recordAndCheck(sig_b) == LoopAction.Allow, true);
    expect_eq(guard.recordAndCheck(sig_a) == LoopAction.Allow, true);
    expect_eq(guard.recordAndCheck(sig_b) == LoopAction.PingPongLoop, true);
});

test("4. ToolCircuitBreaker transitions Closed -> Open -> HalfOpen -> Closed and re-trips on half-open failure", () => {
    let cb = ToolCircuitBreaker.init(3, 2, 1000) ?? unreachable;
    expect_eq(cb.state == CircuitState.Closed, true);
    expect_eq(cb.allowCall(100), true);

    // 3 consecutive failures trip breaker to Open at t=300
    cb.recordFailure(100);
    cb.recordFailure(200);
    expect_eq(cb.state == CircuitState.Closed, true);
    cb.recordFailure(300);
    expect_eq(cb.state == CircuitState.Open, true);

    // Blocked during cooldown [300, 1300)
    expect_eq(cb.allowCall(1299), false);

    // Cooldown elapsed at t=1300 -> transitions to HalfOpen
    expect_eq(cb.allowCall(1300), true);
    expect_eq(cb.state == CircuitState.HalfOpen, true);

    // Failure during HalfOpen immediately re-opens at t=1350
    cb.recordFailure(1350);
    expect_eq(cb.state == CircuitState.Open, true);
    expect_eq(cb.allowCall(2349), false);

    // Next HalfOpen window at t=2350: 2 consecutive successes close the circuit
    expect_eq(cb.allowCall(2350), true);
    cb.recordSuccess();
    expect_eq(cb.state == CircuitState.HalfOpen, true);
    cb.recordSuccess();
    expect_eq(cb.state == CircuitState.Closed, true);
});

test("5. ContextWindowAdvisor evaluates headroom, compaction tiers, and overflow accurately", () => {
    // 100k context, 20k reserved for output -> 80k effective limit
    const adv = ContextWindowAdvisor.init(100000, 20000, 75, 90) ?? unreachable;

    const r_ok = adv.evaluate(40000); // 50%
    expect_eq(r_ok.status == ContextStatus.Ok, true);
    expect_eq(r_ok.effective_limit, 80000);
    expect_eq(r_ok.headroom_tokens, 40000);
    expect_eq(r_ok.utilization_pct, 50);
    expect_eq(adv.canFitAdditional(40000, 40000), true);
    expect_eq(adv.canFitAdditional(40000, 40001), false);

    const r_soon = adv.evaluate(60000); // 75%
    expect_eq(r_soon.status == ContextStatus.CompactSoon, true);

    const r_must = adv.evaluate(72000); // 90%
    expect_eq(r_must.status == ContextStatus.MustCompact, true);

    const r_over = adv.evaluate(85000); // > 80k effective limit
    expect_eq(r_over.status == ContextStatus.Overflow, true);
    expect_eq(r_over.headroom_tokens, 0);
    expect_eq(adv.canFitAdditional(85000, 1), false);
});

test("6. Defensive guards: invalid configs, short ring buffers, and zero thresholds", () => {
    expect_eq(AgentBudget.init(BudgetLimits {
        max_total_tokens: 100,
        max_cost_micro_usd: 100,
        max_steps: 5,
        max_elapsed_ms: 1000,
        soft_limit_pct: 100,
    }) == null, true);

    let small_ring: [2]CallSignature = std.mem.zeroes([2]CallSignature);
    let small_s: []CallSignature = small_ring[0..];
    expect_eq(LoopGuard.init(&small_s, 3, 2) == null, true);
    expect_eq(LoopGuard.init(&small_s, 1, 2) == null, true);
    expect_eq(LoopGuard.init(&small_s, 2, 1) == null, true);

    expect_eq(ToolCircuitBreaker.init(0, 1, 1000) == null, true);
    expect_eq(ToolCircuitBreaker.init(1, 0, 1000) == null, true);

    expect_eq(ContextWindowAdvisor.init(1000, 1000, 75, 90) == null, true);
    expect_eq(ContextWindowAdvisor.init(1000, 100, 0, 90) == null, true);
    expect_eq(ContextWindowAdvisor.init(1000, 100, 90, 90) == null, true);
    expect_eq(ContextWindowAdvisor.init(1000, 100, 75, 101) == null, true);
});
