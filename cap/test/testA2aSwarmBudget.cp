// Origin: Pure-CP Stdlib Test Suite (lib/a2a/swarmBudget.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Multi-Agent Hierarchical Swarm Budget Ledger & Cascading Circuit Breaker:
//   1. Hierarchical tree spawning, atomic ancestor-chain charging & effective remainingBudget calculation
//   2. Parent quota reservation (`reserve_from_parent`) & unused-quota refund on `completeAgent`
//   3. Ancestor budget breach -> atomic rollback (no partial charge) & iterative subtree `halted_budget` cascade
//   4. Deadline monotonic tightening & ancestor deadline breach -> cascading `halted_deadline`
//   5. Tool-call & cost-micros independent quota enforcement + deep 64-node chain iterative safety
//   6. Invalid ID/duplicate/capacity guards, telemetry stats & `cap.a2a` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SWARM_BUDGET_MAX_NODES,
    SwarmAgentState,
    SwarmBudgetViolation,
    SwarmSpawnStatus,
    SwarmBudgetLimits,
    SwarmAgentNode,
    SwarmBudgetLedger,
} from "cap:a2a";

test("SwarmBudget: hierarchical spawning, atomic ancestor charging & remainingBudget") {
    let ledger = SwarmBudgetLedger.init(1, SwarmBudgetLimits{
        max_tokens: 10000,
        max_cost_micros: 500_000,
        max_tool_calls: 20,
        deadline_ms: 100_000,
    }) ?? return;

    // Spawn planner(2) under root(1), and worker(3) under planner(2)
    expect_eq(ledger.spawnChild(1, 2, SwarmBudgetLimits{
        max_tokens: 6000,
        max_cost_micros: 300_000,
        max_tool_calls: 10,
    }), SwarmSpawnStatus.ok);

    expect_eq(ledger.spawnChild(2, 3, SwarmBudgetLimits{
        max_tokens: 4000,
        max_cost_micros: 200_000,
        max_tool_calls: 6,
    }), SwarmSpawnStatus.ok);

    // Charge worker(3): rolls up atomically to 3, 2, and 1
    const c1 = ledger.charge(3, 1500, 60_000, 2, 10_000);
    expect_true(c1.allowed);
    expect_eq(c1.violation, SwarmBudgetViolation.none);

    expect_eq((ledger.getAgent(3) ?? SwarmAgentNode{}).used_tokens, 1500 as u64);
    expect_eq((ledger.getAgent(2) ?? SwarmAgentNode{}).used_tokens, 1500 as u64);
    expect_eq((ledger.getAgent(1) ?? SwarmAgentNode{}).used_tokens, 1500 as u64);

    // Effective remaining for worker(3): min(3: 2500, 2: 4500, 1: 8500) == 2500
    const rem3 = ledger.remainingBudget(3, 20_000);
    expect_true(rem3.found && rem3.can_continue);
    expect_eq(rem3.remaining_tokens, 2500 as u64);
    expect_eq(rem3.remaining_cost_micros, 140_000 as u64);
    expect_eq(rem3.remaining_tool_calls, 4 as u32);
    expect_true(rem3.has_deadline);
    expect_eq(rem3.remaining_ms, 80_000 as u64);
}

test("SwarmBudget: parent reservation isolation & unused-quota refund on completeAgent") {
    let ledger = SwarmBudgetLedger.init(1, SwarmBudgetLimits{
        max_tokens: 1000,
        max_cost_micros: 100_000,
        max_tool_calls: 10,
    }) ?? return;

    // Reserve 600 tokens for child 2 -> root free pool drops to 400
    expect_eq(ledger.spawnChild(1, 2, SwarmBudgetLimits{
        max_tokens: 600,
        max_cost_micros: 60_000,
        max_tool_calls: 6,
        reserve_from_parent: true,
    }), SwarmSpawnStatus.ok);

    // Attempting to reserve another 500 tokens for child 3 exceeds root free pool (400)
    expect_eq(ledger.spawnChild(1, 3, SwarmBudgetLimits{
        max_tokens: 500,
        max_cost_micros: 10_000,
        max_tool_calls: 2,
        reserve_from_parent: true,
    }), SwarmSpawnStatus.exceeds_parent_remaining);

    // Child 2 spends 250 tokens -> converts 250 reserved -> 250 used on root; root free remains 400
    expect_true(ledger.charge(2, 250, 20_000, 2, 1000).allowed);
    expect_eq(ledger.remainingBudget(1, 1000).remaining_tokens, 400 as u64);

    // Complete child 2 -> refunds unused 350 tokens back to root free pool (400 + 350 == 750!)
    expect_true(ledger.completeAgent(2));
    expect_eq((ledger.getAgent(2) ?? SwarmAgentNode{}).state, SwarmAgentState.completed);
    expect_eq(ledger.remainingBudget(1, 1000).remaining_tokens, 750 as u64);

    // Now child 3 requesting 500 tokens succeeds!
    expect_eq(ledger.spawnChild(1, 3, SwarmBudgetLimits{
        max_tokens: 500,
        max_cost_micros: 50_000,
        max_tool_calls: 5,
        reserve_from_parent: true,
    }), SwarmSpawnStatus.ok);
}

test("SwarmBudget: ancestor budget breach cascades halted_budget without partial charge") {
    let ledger = SwarmBudgetLedger.init(1, SwarmBudgetLimits{
        max_tokens: 1000,
        max_cost_micros: 100_000,
        max_tool_calls: 10,
    }) ?? return;

    // Parent(2) has 500 tokens; two children (3 and 4) share parent's pool (unreserved)
    * = ledger.spawnChild(1, 2, SwarmBudgetLimits{ max_tokens: 500, max_cost_micros: 50_000, max_tool_calls: 10 });
    * = ledger.spawnChild(2, 3, SwarmBudgetLimits{ max_tokens: 400, max_cost_micros: 40_000, max_tool_calls: 5 });
    * = ledger.spawnChild(2, 4, SwarmBudgetLimits{ max_tokens: 400, max_cost_micros: 40_000, max_tool_calls: 5 });

    // Child 3 spends 350 tokens -> parent(2) has 150 left
    expect_true(ledger.charge(3, 350, 10_000, 1, 1000).allowed);

    // Child 4 tries to spend 200 tokens -> breaches parent(2) remaining 150!
    // Must NOT commit partial charge, and MUST cascade halted_budget across parent(2), child(3), child(4)!
    const fail = ledger.charge(4, 200, 10_000, 1, 1100);
    expect_eq(fail.allowed, false);
    expect_eq(fail.violation, SwarmBudgetViolation.token_limit_exceeded);
    expect_eq(fail.violated_agent_id, 2 as u64);
    expect_eq(fail.cascaded_halts, 3 as usize);

    // Atomic check: root(1) and parent(2) still show 350 used (not 550)
    expect_eq((ledger.getAgent(1) ?? SwarmAgentNode{}).used_tokens, 350 as u64);
    expect_eq((ledger.getAgent(2) ?? SwarmAgentNode{}).used_tokens, 350 as u64);
    expect_eq((ledger.getAgent(2) ?? SwarmAgentNode{}).state, SwarmAgentState.halted_budget);
    expect_eq((ledger.getAgent(3) ?? SwarmAgentNode{}).state, SwarmAgentState.halted_budget);
    expect_eq((ledger.getAgent(4) ?? SwarmAgentNode{}).state, SwarmAgentState.halted_budget);
    // Root(1) remains active and unaffected
    expect_eq((ledger.getAgent(1) ?? SwarmAgentNode{}).state, SwarmAgentState.active);
}

test("SwarmBudget: deadline monotonic tightening & cascading halted_deadline") {
    let ledger = SwarmBudgetLedger.init(1, SwarmBudgetLimits{
        max_tokens: 5000,
        max_cost_micros: 500_000,
        max_tool_calls: 20,
        deadline_ms: 10_000,
    }) ?? return;

    // Child deadline exceeding parent deadline rejected
    expect_eq(ledger.spawnChild(1, 2, SwarmBudgetLimits{
        max_tokens: 1000,
        max_cost_micros: 100_000,
        max_tool_calls: 5,
        deadline_ms: 12_000,
    }), SwarmSpawnStatus.deadline_exceeds_parent);

    // Child 2 with tighter deadline 5000ms, grandchild 3 inherits 5000ms
    expect_eq(ledger.spawnChild(1, 2, SwarmBudgetLimits{
        max_tokens: 1000,
        max_cost_micros: 100_000,
        max_tool_calls: 5,
        deadline_ms: 5000,
    }), SwarmSpawnStatus.ok);
    expect_eq(ledger.spawnChild(2, 3, SwarmBudgetLimits{
        max_tokens: 500,
        max_cost_micros: 50_000,
        max_tool_calls: 3,
        deadline_ms: 0, // inherits 5000
    }), SwarmSpawnStatus.ok);

    // Charge grandchild 3 at t=5001 -> deadline_exceeded cascades halted_deadline
    const dl = ledger.charge(3, 10, 100, 1, 5001);
    expect_eq(dl.allowed, false);
    expect_eq(dl.violation, SwarmBudgetViolation.deadline_exceeded);
    expect_eq((ledger.getAgent(3) ?? SwarmAgentNode{}).state, SwarmAgentState.halted_deadline);
}

test("SwarmBudget: cost & tool-call limits + 64-node deep chain iterative stress") {
    // Cost limit & tool limit enforcement
    let g = SwarmBudgetLedger.init(1, SwarmBudgetLimits{
        max_tokens: 100_000,
        max_cost_micros: 500,
        max_tool_calls: 2,
    }) ?? return;
    expect_true(g.charge(1, 10, 200, 1, 100).allowed);
    const tool_breach = g.charge(1, 10, 100, 2, 200);
    expect_eq(tool_breach.violation, SwarmBudgetViolation.tool_limit_exceeded);

    // Deep 64-node chain (depth 0..63) verifying zero-recursion iterative traversal
    let deep = SwarmBudgetLedger.init(1, SwarmBudgetLimits{
        max_tokens: 10_000,
        max_cost_micros: 10_000,
        max_tool_calls: 100,
    }) ?? return;
    let id: u64 = 2;
    while (id <= (SWARM_BUDGET_MAX_NODES as u64)) : (id += 1) {
        expect_eq(deep.spawnChild(id - 1, id, SwarmBudgetLimits{
            max_tokens: 10_000,
            max_cost_micros: 10_000,
            max_tool_calls: 100,
        }), SwarmSpawnStatus.ok);
    }
    // 65th node rejected as capacity_full
    expect_eq(deep.spawnChild(1, 999, SwarmBudgetLimits{
        max_tokens: 10,
        max_cost_micros: 10,
        max_tool_calls: 1,
    }), SwarmSpawnStatus.capacity_full);

    // Charge leaf node 64 -> propagates all 64 hops to root 1
    expect_true(deep.charge(SWARM_BUDGET_MAX_NODES as u64, 777, 123, 1, 500).allowed);
    expect_eq((deep.getAgent(1) ?? SwarmAgentNode{}).used_tokens, 777 as u64);
    expect_eq(deep.stats().root_used_tokens, 777 as u64);
}

test("SwarmBudget: invalid IDs, telemetry stats & cap.a2a namespace facade parity") {
    expect_true(cap.a2a.SwarmBudgetLedger.init(0, SwarmBudgetLimits{}) == null);
    let ledger = cap.a2a.SwarmBudgetLedger.init(10, SwarmBudgetLimits{
        max_tokens: 1000,
        max_cost_micros: 1000,
        max_tool_calls: 5,
    }) ?? return;

    expect_eq(ledger.spawnChild(10, 0, SwarmBudgetLimits{}), SwarmSpawnStatus.invalid_agent_id);
    expect_eq(ledger.spawnChild(10, 10, SwarmBudgetLimits{}), SwarmSpawnStatus.duplicate_agent_id);
    expect_eq(ledger.spawnChild(999, 11, SwarmBudgetLimits{}), SwarmSpawnStatus.parent_not_found);
    expect_eq(ledger.charge(999, 1, 1, 1, 100).violation, SwarmBudgetViolation.agent_not_found);
    expect_eq(ledger.completeAgent(999), false);

    // Cost breach on root
    const cost_fail = ledger.charge(10, 10, 2000, 1, 100);
    expect_eq(cost_fail.violation, SwarmBudgetViolation.cost_limit_exceeded);

    const st = ledger.stats();
    expect_eq(st.node_count, 1 as usize);
    expect_eq(st.halted_budget_agents, 1 as usize);
    expect_eq(st.total_charges_denied, 2 as u64);
}
