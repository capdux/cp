// Origin: Pure-CP Stdlib Test Suite (AI Context Window Multi-Section Token Budget Allocator) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/context_budget.cp
// Covers:
//   1. Abundant budget satisfying all sections completely with exact unallocated slack accounting
//   2. Tight budget honoring priority-descending floors followed by strict tier-by-tier fill
//   3. Intra-tier weighted water-filling with cap-overflow redistribution and zero waste
//   4. Floor-exceeds-budget degradation favoring higher priority tiers and setting min_floors_met == false
//   5. Zero usable budget, empty requests, reserved_output > total_window & undersized buffer guards

import { test, expect_eq } from "cap:test";
import {
    ContextSectionRequest,
    ContextSectionAlloc,
    ContextBudgetSummary,
    allocateContextBudget,
} from "../../lib/agent/index.cp";

fn testAbundantBudgetFullSatisfaction()! {
    const reqs: [3]ContextSectionRequest = [
        ContextSectionRequest{ section_id: 1, priority: 100, min_tokens: 400, desired_tokens: 600, weight: 1 },
        ContextSectionRequest{ section_id: 2, priority: 50, min_tokens: 200, desired_tokens: 1200, weight: 2 },
        ContextSectionRequest{ section_id: 3, priority: 10, min_tokens: 100, desired_tokens: 700, weight: 1 },
    ];
    let alloc_buf: [3]ContextSectionAlloc = [ContextSectionAlloc{}; 3];
    let allocs: []ContextSectionAlloc = alloc_buf[0..];

    const sum = allocateContextBudget(4000, 1000, reqs[0..], allocs) ?? ContextBudgetSummary{};
    expect_eq(sum.total_budget, 4000);
    expect_eq(sum.reserved_output, 1000);
    expect_eq(sum.usable_input_budget, 3000);
    expect_eq(sum.total_allocated, 2500);
    expect_eq(sum.total_truncated, 0);
    expect_eq(sum.unallocated_slack, 500);
    expect_eq(sum.min_floors_met, true);

    expect_eq(allocs[0].allocated_tokens, 600);
    expect_eq(allocs[0].fully_satisfied, true);
    expect_eq(allocs[1].allocated_tokens, 1200);
    expect_eq(allocs[1].fully_satisfied, true);
    expect_eq(allocs[2].allocated_tokens, 700);
    expect_eq(allocs[2].fully_satisfied, true);
}

fn testTightBudgetPriorityFloorsAndTierFill()! {
    // Usable input = 1500
    // Floors: p100=300, p50=400, p10=200 => total floors = 900, remaining = 600
    // Phase 2:
    //   p100 needs +200 -> gets 200 (total 500, full), remaining = 400
    //   p50  needs +600 -> gets remaining 400 (total 800, truncated 200), remaining = 0
    //   p10  stays at floor 200 (truncated 300)
    const reqs: [3]ContextSectionRequest = [
        ContextSectionRequest{ section_id: 10, priority: 10, min_tokens: 200, desired_tokens: 500, weight: 1 },
        ContextSectionRequest{ section_id: 20, priority: 100, min_tokens: 300, desired_tokens: 500, weight: 1 },
        ContextSectionRequest{ section_id: 30, priority: 50, min_tokens: 400, desired_tokens: 1000, weight: 1 },
    ];
    let alloc_buf: [3]ContextSectionAlloc = [ContextSectionAlloc{}; 3];
    let allocs: []ContextSectionAlloc = alloc_buf[0..];

    const sum = allocateContextBudget(2000, 500, reqs[0..], allocs) ?? ContextBudgetSummary{};
    expect_eq(sum.usable_input_budget, 1500);
    expect_eq(sum.total_allocated, 1500);
    expect_eq(sum.total_truncated, 500);
    expect_eq(sum.unallocated_slack, 0);
    expect_eq(sum.min_floors_met, true);

    expect_eq(allocs[0].section_id, 10);
    expect_eq(allocs[0].allocated_tokens, 200);
    expect_eq(allocs[0].truncated_tokens, 300);
    expect_eq(allocs[0].fully_satisfied, false);

    expect_eq(allocs[1].section_id, 20);
    expect_eq(allocs[1].allocated_tokens, 500);
    expect_eq(allocs[1].truncated_tokens, 0);
    expect_eq(allocs[1].fully_satisfied, true);

    expect_eq(allocs[2].section_id, 30);
    expect_eq(allocs[2].allocated_tokens, 800);
    expect_eq(allocs[2].truncated_tokens, 200);
    expect_eq(allocs[2].fully_satisfied, false);
}

fn testSamePriorityWeightedWaterFilling()! {
    // Same priority (50), usable = 1000, min = 0:
    //   A: desired=100, weight=1 -> capped at 100, surplus overflows to B & C
    //   B: desired=1000, weight=1 -> gets 225 (1/4 of remaining 900)
    //   C: desired=1000, weight=3 -> gets 675 (3/4 of remaining 900)
    const reqs: [3]ContextSectionRequest = [
        ContextSectionRequest{ section_id: 1, priority: 50, min_tokens: 0, desired_tokens: 100, weight: 1 },
        ContextSectionRequest{ section_id: 2, priority: 50, min_tokens: 0, desired_tokens: 1000, weight: 1 },
        ContextSectionRequest{ section_id: 3, priority: 50, min_tokens: 0, desired_tokens: 1000, weight: 3 },
    ];
    let alloc_buf: [3]ContextSectionAlloc = [ContextSectionAlloc{}; 3];
    let allocs: []ContextSectionAlloc = alloc_buf[0..];

    const sum = allocateContextBudget(1200, 200, reqs[0..], allocs) ?? ContextBudgetSummary{};
    expect_eq(sum.total_allocated, 1000);
    expect_eq(sum.unallocated_slack, 0);
    expect_eq(sum.min_floors_met, true);

    expect_eq(allocs[0].allocated_tokens, 100);
    expect_eq(allocs[0].fully_satisfied, true);
    expect_eq(allocs[1].allocated_tokens, 225);
    expect_eq(allocs[2].allocated_tokens, 675);
}

fn testFloorsExceedBudgetDegradesGracefully()! {
    // Usable = 600, floors = p100(500) + p50(300) + p10(200) = 1000 > 600
    // High priority p100 gets full 500, p50 gets remaining 100, p10 gets 0, min_floors_met == false
    const reqs: [3]ContextSectionRequest = [
        ContextSectionRequest{ section_id: 1, priority: 100, min_tokens: 500, desired_tokens: 800, weight: 1 },
        ContextSectionRequest{ section_id: 2, priority: 50, min_tokens: 300, desired_tokens: 500, weight: 1 },
        ContextSectionRequest{ section_id: 3, priority: 10, min_tokens: 200, desired_tokens: 400, weight: 1 },
    ];
    let alloc_buf: [3]ContextSectionAlloc = [ContextSectionAlloc{}; 3];
    let allocs: []ContextSectionAlloc = alloc_buf[0..];

    const sum = allocateContextBudget(1000, 400, reqs[0..], allocs) ?? ContextBudgetSummary{};
    expect_eq(sum.usable_input_budget, 600);
    expect_eq(sum.total_allocated, 600);
    expect_eq(sum.unallocated_slack, 0);
    expect_eq(sum.min_floors_met, false);

    expect_eq(allocs[0].allocated_tokens, 500);
    expect_eq(allocs[1].allocated_tokens, 100);
    expect_eq(allocs[2].allocated_tokens, 0);
}

fn testZeroBudgetEmptyRequestsAndInvalidGuards()! {
    let alloc_buf: [2]ContextSectionAlloc = [ContextSectionAlloc{}; 2];
    let allocs: []ContextSectionAlloc = alloc_buf[0..];

    // 1. Empty requests list
    const empty_reqs: [0]ContextSectionRequest = [];
    const empty_sum = allocateContextBudget(4096, 1024, empty_reqs[0..], allocs) ?? ContextBudgetSummary{};
    expect_eq(empty_sum.usable_input_budget, 3072);
    expect_eq(empty_sum.total_allocated, 0);
    expect_eq(empty_sum.unallocated_slack, 3072);
    expect_eq(empty_sum.min_floors_met, true);

    // 2. Zero usable input budget (total_window == reserved_output)
    const reqs: [2]ContextSectionRequest = [
        ContextSectionRequest{ section_id: 1, priority: 10, min_tokens: 100, desired_tokens: 200, weight: 0 },
        ContextSectionRequest{ section_id: 2, priority: 10, min_tokens: 0, desired_tokens: 50, weight: 0 },
    ];
    const zero_sum = allocateContextBudget(1000, 1000, reqs[0..], allocs) ?? ContextBudgetSummary{};
    expect_eq(zero_sum.usable_input_budget, 0);
    expect_eq(zero_sum.total_allocated, 0);
    expect_eq(zero_sum.total_truncated, 250);
    expect_eq(zero_sum.min_floors_met, false);

    // 3. reserved_output > total_window rejected
    expect_eq(allocateContextBudget(1000, 1001, reqs[0..], allocs) == null, true);

    // 4. Undersized output buffer rejected
    let small_buf: [1]ContextSectionAlloc = [ContextSectionAlloc{}; 1];
    let small_allocs: []ContextSectionAlloc = small_buf[0..];
    expect_eq(allocateContextBudget(2000, 500, reqs[0..], small_allocs) == null, true);
}

test("agent context_budget: abundant budget satisfies all sections with exact slack", testAbundantBudgetFullSatisfaction);
test("agent context_budget: tight budget priority floors and tier-by-tier fill", testTightBudgetPriorityFloorsAndTierFill);
test("agent context_budget: intra-tier weighted water-filling with overflow redistribution", testSamePriorityWeightedWaterFilling);
test("agent context_budget: floor overflow prioritizes higher tiers and flags min_floors_met=false", testFloorsExceedBudgetDegradesGracefully);
test("agent context_budget: zero budget, empty requests and invalid parameter guards", testZeroBudgetEmptyRequestsAndInvalidGuards);
