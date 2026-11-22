// Origin: Pure-CP Stdlib Test Suite (AI Multi-Agent Cost Attribution Ledger & Prompt-Cache Accounting) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/cost_ledger.cp
// Covers:
//   1. Exact fixed-point micro-USD calculation without floating-point drift & u128 overflow safety
//   2. Prompt-cache read savings accounting & non-discount guard
//   3. Multi-level parent/child/grandchild hierarchical cost attribution
//   4. Self-cycle & mutual-cycle parent hierarchy immunity
//   5. topSpendingAgent, unknown model rejection, zero ID & full-pool boundary guards

import { test, expect_eq } from "cap:test";
import {
    ModelRateCard,
    UsageEvent,
    AgentCostSummary,
    TopSpender,
    computeUsageCostMicroUsd,
    computeCacheSavingsMicroUsd,
    CostLedger,
} from "../../lib/agent/index.cp";

fn testFixedPointMicroUsdPrecision()! {
    // $3.00/MTok input, $15.00/MTok output, $0.30/MTok cache read, $3.75/MTok cache write
    const rate = ModelRateCard{
        model_id: 1,
        input_per_mtok_micro: 3000000,
        output_per_mtok_micro: 15000000,
        cache_read_per_mtok_micro: 300000,
        cache_write_per_mtok_micro: 3750000,
    };

    // 1000 input ($0.003 = 3000) + 200 output ($0.003 = 3000) + 10000 cache_read ($0.003 = 3000) + 400 cache_write ($0.0015 = 1500)
    // Total = 10500 micro-USD exact
    const ev = UsageEvent{
        agent_id: 1,
        parent_agent_id: 0,
        model_id: 1,
        input_tokens: 1000,
        output_tokens: 200,
        cache_read_tokens: 10000,
        cache_write_tokens: 400,
    };
    expect_eq(computeUsageCostMicroUsd(rate, ev), 10500);

    // 100 identical micro-calls accumulate with zero drift: 100 * 10500 = 1,050,000 micro-USD ($1.05 exact)
    let rates_buf: [2]ModelRateCard = [ModelRateCard{}; 2];
    let events_buf: [100]UsageEvent = [UsageEvent{}; 100];
    let rates: []ModelRateCard = rates_buf[0..];
    let events: []UsageEvent = events_buf[0..];
    let ledger = CostLedger.init(rates, events);
    expect_eq(ledger.registerRate(rate), true);

    let i: usize = 0;
    while (i < 100) : (i += 1) {
        expect_eq(ledger.recordUsage(ev), true);
    }
    expect_eq(ledger.totalMicroUsd(), 1050000);
    expect_eq(ledger.agentSummary(1).call_count, 100);
}

fn testPromptCacheSavingsAccounting()! {
    const rate = ModelRateCard{
        model_id: 2,
        input_per_mtok_micro: 3000000,
        output_per_mtok_micro: 15000000,
        cache_read_per_mtok_micro: 300000,
        cache_write_per_mtok_micro: 3750000,
    };

    // 1,000,000 cached read tokens save ($3.00 - $0.30) = $2.70 = 2,700,000 micro-USD
    expect_eq(computeCacheSavingsMicroUsd(rate, 1000000), 2700000);
    expect_eq(computeCacheSavingsMicroUsd(rate, 0), 0);

    // No discount (cache_read >= input) -> savings is 0
    const flat_rate = ModelRateCard{
        model_id: 3,
        input_per_mtok_micro: 1000000,
        output_per_mtok_micro: 2000000,
        cache_read_per_mtok_micro: 1000000,
        cache_write_per_mtok_micro: 1000000,
    };
    expect_eq(computeCacheSavingsMicroUsd(flat_rate, 500000), 0);

    let rates_buf: [2]ModelRateCard = [ModelRateCard{}; 2];
    let events_buf: [4]UsageEvent = [UsageEvent{}; 4];
    let rates: []ModelRateCard = rates_buf[0..];
    let events: []UsageEvent = events_buf[0..];
    let ledger = CostLedger.init(rates, events);
    expect_eq(ledger.registerRate(rate), true);

    expect_eq(ledger.recordUsage(UsageEvent{
        agent_id: 7,
        parent_agent_id: 0,
        model_id: 2,
        input_tokens: 0,
        output_tokens: 0,
        cache_read_tokens: 500000,
        cache_write_tokens: 0,
    }), true);

    const sum = ledger.agentSummary(7);
    expect_eq(sum.direct_micro_usd, 150000);
    expect_eq(sum.cache_read_tokens, 500000);
    expect_eq(sum.cache_savings_micro_usd, 1350000);
}

fn testHierarchicalSubtreeAttribution()! {
    let rates_buf: [2]ModelRateCard = [ModelRateCard{}; 2];
    let events_buf: [8]UsageEvent = [UsageEvent{}; 8];
    let rates: []ModelRateCard = rates_buf[0..];
    let events: []UsageEvent = events_buf[0..];
    let ledger = CostLedger.init(rates, events);

    // Simple $1.00 / MTok input => 1 micro-USD per input token
    expect_eq(ledger.registerRate(ModelRateCard{
        model_id: 1,
        input_per_mtok_micro: 1000000,
        output_per_mtok_micro: 1000000,
        cache_read_per_mtok_micro: 0,
        cache_write_per_mtok_micro: 0,
    }), true);

    // Hierarchy: Root(1, cost=100) -> ChildA(2, cost=200) -> GrandchildA1(4, cost=400)
    //                              -> ChildB(3, cost=300)
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 1, parent_agent_id: 0, model_id: 1, input_tokens: 100 }), true);
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 2, parent_agent_id: 1, model_id: 1, input_tokens: 200 }), true);
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 3, parent_agent_id: 1, model_id: 1, input_tokens: 300 }), true);
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 4, parent_agent_id: 2, model_id: 1, input_tokens: 400 }), true);

    expect_eq(ledger.totalMicroUsd(), 1000);

    const s_root = ledger.agentSummary(1);
    expect_eq(s_root.direct_micro_usd, 100);
    expect_eq(s_root.subtree_micro_usd, 1000);

    const s_child_a = ledger.agentSummary(2);
    expect_eq(s_child_a.direct_micro_usd, 200);
    expect_eq(s_child_a.subtree_micro_usd, 600); // 200 + 400

    const s_child_b = ledger.agentSummary(3);
    expect_eq(s_child_b.direct_micro_usd, 300);
    expect_eq(s_child_b.subtree_micro_usd, 300);

    const s_grand = ledger.agentSummary(4);
    expect_eq(s_grand.direct_micro_usd, 400);
    expect_eq(s_grand.subtree_micro_usd, 400);
}

fn testCycleParentHierarchyImmunity()! {
    let rates_buf: [2]ModelRateCard = [ModelRateCard{}; 2];
    let events_buf: [6]UsageEvent = [UsageEvent{}; 6];
    let rates: []ModelRateCard = rates_buf[0..];
    let events: []UsageEvent = events_buf[0..];
    let ledger = CostLedger.init(rates, events);

    expect_eq(ledger.registerRate(ModelRateCard{
        model_id: 1,
        input_per_mtok_micro: 1000000,
        output_per_mtok_micro: 1000000,
    }), true);

    // Self-cycle (10 -> 10) and mutual 2-cycle (20 <-> 21)
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 10, parent_agent_id: 10, model_id: 1, input_tokens: 50 }), true);
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 20, parent_agent_id: 21, model_id: 1, input_tokens: 100 }), true);
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 21, parent_agent_id: 20, model_id: 1, input_tokens: 250 }), true);

    const s10 = ledger.agentSummary(10);
    expect_eq(s10.direct_micro_usd, 50);
    expect_eq(s10.subtree_micro_usd, 50);

    // Mutual cycle terminates cleanly without infinite loop or double counting
    const s20 = ledger.agentSummary(20);
    expect_eq(s20.direct_micro_usd, 100);
    expect_eq(s20.subtree_micro_usd, 350);

    const s21 = ledger.agentSummary(21);
    expect_eq(s21.direct_micro_usd, 250);
    expect_eq(s21.subtree_micro_usd, 350);
}

fn testTopSpenderUnknownModelAndPoolGuards()! {
    let rates_buf: [1]ModelRateCard = [ModelRateCard{}; 1];
    let events_buf: [3]UsageEvent = [UsageEvent{}; 3];
    let rates: []ModelRateCard = rates_buf[0..];
    let events: []UsageEvent = events_buf[0..];
    let ledger = CostLedger.init(rates, events);

    // Empty ledger guards
    expect_eq(ledger.totalMicroUsd(), 0);
    expect_eq(ledger.topSpendingAgent() == null, true);

    // Invalid rate & full rate buffer guards
    expect_eq(ledger.registerRate(ModelRateCard{ model_id: 0, input_per_mtok_micro: 1000000 }), false);
    expect_eq(ledger.registerRate(ModelRateCard{ model_id: 5, input_per_mtok_micro: 1000000 }), true);
    expect_eq(ledger.registerRate(ModelRateCard{ model_id: 6, input_per_mtok_micro: 1000000 }), false); // pool full
    // Updating existing model_id 5 in-place succeeds
    expect_eq(ledger.registerRate(ModelRateCard{ model_id: 5, input_per_mtok_micro: 2000000 }), true);

    // Reject zero agent_id and unregistered model_id
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 0, model_id: 5, input_tokens: 100 }), false);
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 1, model_id: 99, input_tokens: 100 }), false);

    // Record 3 events (filling event buffer): agent 1 (100+200=300*2=600), agent 2 (500*2=1000)
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 1, parent_agent_id: 0, model_id: 5, input_tokens: 100 }), true);
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 2, parent_agent_id: 1, model_id: 5, input_tokens: 500 }), true);
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 1, parent_agent_id: 0, model_id: 5, input_tokens: 200 }), true);
    // 4th event rejected (buffer full)
    expect_eq(ledger.recordUsage(UsageEvent{ agent_id: 3, parent_agent_id: 0, model_id: 5, input_tokens: 9999 }), false);

    const top = ledger.topSpendingAgent() ?? TopSpender{};
    expect_eq(top.agent_id, 2);
    expect_eq(top.direct_micro_usd, 1000);
    expect_eq(top.subtree_micro_usd, 1000);
}

fn main() {
    test("1. agent cost_ledger: fixed-point micro-USD exactness & zero drift", () => {
        testFixedPointMicroUsdPrecision()?;
    });
    test("2. agent cost_ledger: prompt-cache read discount & savings accounting", () => {
        testPromptCacheSavingsAccounting()?;
    });
    test("3. agent cost_ledger: parent/child/grandchild hierarchical cost attribution", () => {
        testHierarchicalSubtreeAttribution()?;
    });
    test("4. agent cost_ledger: self-cycle & mutual-cycle parent hierarchy immunity", () => {
        testCycleParentHierarchyImmunity()?;
    });
    test("5. agent cost_ledger: topSpendingAgent, unknown model & full-pool guards", () => {
        testTopSpenderUnknownModelAndPoolGuards()?;
    });
}
