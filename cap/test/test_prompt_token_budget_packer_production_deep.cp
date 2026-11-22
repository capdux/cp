// Origin: Pure-CP Stdlib Test Suite (lib/prompt/token_budget_packer.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Priority Prompt Token Budget Packer:
//   1. Ample budget: all items included intact in original input order
//   2. Phase-1 must_keep priority locking for system_pinned & user_query
//   3. Priority descending + recency_seq descending deterministic eviction of older history
//   4. drop_whole vs keep_head / keep_tail truncation & min_tokens + ellipsis lower bound
//   5. keep_middle_out exact head/tail split (even & odd content budgets) & budget conservation
//   6. must_keep exceeding effective budget triggers pinned_overflow=true without budget violation
//   7. Empty input, invalid config, undersized out_decisions & > BUDGET_PACKER_MAX_ITEMS guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BUDGET_PACKER_MAX_ITEMS,
    BudgetItemKind,
    BudgetTruncatePolicy,
    BudgetPackConfig,
    BudgetedPromptItem,
    PackedSlotDecision,
    BudgetPackReport,
    packPromptWithinBudget,
} from "cap:prompt";

// ============================================================================
// Test 1: Ample Budget -> All Included Intact in Original Input Order
// ============================================================================

fn testAmpleBudgetAllIncludedInOrder()! {
    const items: [4]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 10, kind: BudgetItemKind.system_pinned, priority: 100, token_count: 120, min_tokens: 120, policy: BudgetTruncatePolicy.must_keep, recency_seq: 1 },
        BudgetedPromptItem{ item_id: 20, kind: BudgetItemKind.tool_schema,   priority: 80,  token_count: 200, min_tokens: 100, policy: BudgetTruncatePolicy.drop_whole, recency_seq: 2 },
        BudgetedPromptItem{ item_id: 30, kind: BudgetItemKind.rag_context,   priority: 50,  token_count: 300, min_tokens: 50,  policy: BudgetTruncatePolicy.keep_middle_out, recency_seq: 3 },
        BudgetedPromptItem{ item_id: 40, kind: BudgetItemKind.user_query,    priority: 100, token_count: 80,  min_tokens: 80,  policy: BudgetTruncatePolicy.must_keep, recency_seq: 4 },
    ];
    const cfg = BudgetPackConfig{ max_context_tokens: 2000, reserved_output_tokens: 500, ellipsis_overhead_tokens: 4 };
    expect_eq(cfg.isValid(), true);
    expect_eq(cfg.effectiveBudget(), 1500);

    let out_buf: [4]PackedSlotDecision = [PackedSlotDecision{}; 4];
    let out_s: []PackedSlotDecision = out_buf[0..];
    const rep: BudgetPackReport = cap.prompt.packPromptWithinBudget(items[0..], cfg, out_s);

    expect_eq(rep.effective_budget, 1500);
    expect_eq(rep.allocated_total, 700);
    expect_eq(rep.remaining_tokens, 800);
    expect_eq(rep.included_count, 4);
    expect_eq(rep.truncated_count, 0);
    expect_eq(rep.dropped_count, 0);
    expect_eq(rep.pinned_overflow, false);

    // Original input order preserved in out_decisions
    expect_eq(out_buf[0].item_id, 10);
    expect_eq(out_buf[1].item_id, 20);
    expect_eq(out_buf[2].item_id, 30);
    expect_eq(out_buf[3].item_id, 40);
    expect_eq(out_buf[2].included, true);
    expect_eq(out_buf[2].truncated, false);
    expect_eq(out_buf[2].allocated_tokens, 300);
    expect_eq(out_buf[2].head_tokens, 300);
    expect_eq(out_buf[2].tail_tokens, 0);
}

// ============================================================================
// Test 2: Phase-1 must_keep Locking for System & User Query
// ============================================================================

fn testMustKeepLockedBeforeNonPinned()! {
    // Non-pinned item has higher numeric priority (999), yet must_keep items MUST still lock first
    const items: [3]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 1, kind: BudgetItemKind.rag_context,   priority: 999, token_count: 250, min_tokens: 200, policy: BudgetTruncatePolicy.drop_whole, recency_seq: 10 },
        BudgetedPromptItem{ item_id: 2, kind: BudgetItemKind.system_pinned, priority: 10,  token_count: 150, min_tokens: 150, policy: BudgetTruncatePolicy.must_keep,  recency_seq: 1 },
        BudgetedPromptItem{ item_id: 3, kind: BudgetItemKind.user_query,    priority: 10,  token_count: 120, min_tokens: 120, policy: BudgetTruncatePolicy.must_keep,  recency_seq: 2 },
    ];
    const cfg = BudgetPackConfig{ max_context_tokens: 400, reserved_output_tokens: 100, ellipsis_overhead_tokens: 4 }; // effective = 300

    let out_buf: [3]PackedSlotDecision = [PackedSlotDecision{}; 3];
    let out_s: []PackedSlotDecision = out_buf[0..];
    const rep = packPromptWithinBudget(items[0..], cfg, out_s);

    expect_eq(rep.pinned_overflow, false);
    expect_eq(rep.included_count, 2);
    expect_eq(rep.dropped_count, 1);
    expect_eq(rep.allocated_total, 270);
    expect_eq(rep.remaining_tokens, 30);

    expect_eq(out_buf[0].included, false); // rag (250) dropped because only 30 left
    expect_eq(out_buf[1].included, true);  // system locked
    expect_eq(out_buf[2].included, true);  // user query locked
}

// ============================================================================
// Test 3: Priority & Recency Ranking Evicts Older History Deterministically
// ============================================================================

fn testPriorityAndRecencyEviction()! {
    const items: [4]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 1, kind: BudgetItemKind.history_turn, priority: 50, token_count: 100, min_tokens: 100, policy: BudgetTruncatePolicy.drop_whole, recency_seq: 1 }, // oldest
        BudgetedPromptItem{ item_id: 2, kind: BudgetItemKind.history_turn, priority: 50, token_count: 100, min_tokens: 100, policy: BudgetTruncatePolicy.drop_whole, recency_seq: 3 }, // newest
        BudgetedPromptItem{ item_id: 3, kind: BudgetItemKind.history_turn, priority: 50, token_count: 100, min_tokens: 100, policy: BudgetTruncatePolicy.drop_whole, recency_seq: 2 }, // middle
        BudgetedPromptItem{ item_id: 4, kind: BudgetItemKind.tool_schema,  priority: 90, token_count: 100, min_tokens: 100, policy: BudgetTruncatePolicy.drop_whole, recency_seq: 0 }, // highest priority
    ];
    // Effective budget = 200 -> keeps item 4 (prio 90) and item 2 (prio 50, recency 3)
    const cfg = BudgetPackConfig{ max_context_tokens: 300, reserved_output_tokens: 100, ellipsis_overhead_tokens: 4 };

    let out_buf: [4]PackedSlotDecision = [PackedSlotDecision{}; 4];
    let out_s: []PackedSlotDecision = out_buf[0..];
    const rep = packPromptWithinBudget(items[0..], cfg, out_s);

    expect_eq(rep.allocated_total, 200);
    expect_eq(rep.included_count, 2);
    expect_eq(rep.dropped_count, 2);
    expect_eq(out_buf[0].included, false);
    expect_eq(out_buf[1].included, true);
    expect_eq(out_buf[2].included, false);
    expect_eq(out_buf[3].included, true);
}

// ============================================================================
// Test 4: drop_whole vs keep_head / keep_tail & min_tokens Guard
// ============================================================================

fn testDropWholeVsKeepHeadAndKeepTail()! {
    // 4a. keep_head fits remaining 84 (content=80 + ellipsis=4), drop_whole is skipped
    const items_head: [3]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 1, kind: BudgetItemKind.system_pinned, priority: 100, token_count: 116, min_tokens: 116, policy: BudgetTruncatePolicy.must_keep,  recency_seq: 1 },
        BudgetedPromptItem{ item_id: 2, kind: BudgetItemKind.rag_context,   priority: 80,  token_count: 200, min_tokens: 50,  policy: BudgetTruncatePolicy.drop_whole, recency_seq: 2 },
        BudgetedPromptItem{ item_id: 3, kind: BudgetItemKind.rag_context,   priority: 70,  token_count: 200, min_tokens: 50,  policy: BudgetTruncatePolicy.keep_head,  recency_seq: 3 },
    ];
    const cfg = BudgetPackConfig{ max_context_tokens: 300, reserved_output_tokens: 100, ellipsis_overhead_tokens: 4 }; // effective = 200

    let out_h: [3]PackedSlotDecision = [PackedSlotDecision{}; 3];
    let out_hs: []PackedSlotDecision = out_h[0..];
    const rep_h = packPromptWithinBudget(items_head[0..], cfg, out_hs);
    expect_eq(rep_h.allocated_total, 200);
    expect_eq(rep_h.remaining_tokens, 0);
    expect_eq(rep_h.truncated_count, 1);
    expect_eq(out_h[1].included, false);
    expect_eq(out_h[2].included, true);
    expect_eq(out_h[2].truncated, true);
    expect_eq(out_h[2].allocated_tokens, 84);
    expect_eq(out_h[2].head_tokens, 80);
    expect_eq(out_h[2].tail_tokens, 0);

    // 4b. keep_tail + min_tokens lower bound:
    //     item 2 needs min_tokens=90 + 4 = 94 > remaining 84 -> dropped;
    //     item 3 needs min_tokens=40 + 4 = 44 <= 84 -> keep_tail gets tail_tokens=80, head_tokens=0.
    const items_tail: [3]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 1, kind: BudgetItemKind.system_pinned, priority: 100, token_count: 116, min_tokens: 116, policy: BudgetTruncatePolicy.must_keep, recency_seq: 1 },
        BudgetedPromptItem{ item_id: 2, kind: BudgetItemKind.history_turn,  priority: 80,  token_count: 200, min_tokens: 90,  policy: BudgetTruncatePolicy.keep_tail, recency_seq: 2 },
        BudgetedPromptItem{ item_id: 3, kind: BudgetItemKind.history_turn,  priority: 70,  token_count: 200, min_tokens: 40,  policy: BudgetTruncatePolicy.keep_tail, recency_seq: 3 },
    ];
    let out_t: [3]PackedSlotDecision = [PackedSlotDecision{}; 3];
    let out_ts: []PackedSlotDecision = out_t[0..];
    const rep_t = packPromptWithinBudget(items_tail[0..], cfg, out_ts);
    expect_eq(rep_t.allocated_total, 200);
    expect_eq(out_t[1].included, false);
    expect_eq(out_t[2].included, true);
    expect_eq(out_t[2].truncated, true);
    expect_eq(out_t[2].head_tokens, 0);
    expect_eq(out_t[2].tail_tokens, 80);
    expect_eq(out_t[2].allocated_tokens, 84);
}

// ============================================================================
// Test 5: keep_middle_out Exact Head/Tail Split & Ellipsis Budget Conservation
// ============================================================================

fn testKeepMiddleOutSplitAndConservation()! {
    // 5a. Even content budget: effective=400, pinned=300, remaining=100, ellipsis=4 -> content=96 -> head=48, tail=48
    const items_even: [2]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 1, kind: BudgetItemKind.system_pinned, priority: 100, token_count: 300, min_tokens: 300, policy: BudgetTruncatePolicy.must_keep,       recency_seq: 1 },
        BudgetedPromptItem{ item_id: 2, kind: BudgetItemKind.rag_context,   priority: 60,  token_count: 500, min_tokens: 40,  policy: BudgetTruncatePolicy.keep_middle_out, recency_seq: 2 },
    ];
    const cfg_even = BudgetPackConfig{ max_context_tokens: 500, reserved_output_tokens: 100, ellipsis_overhead_tokens: 4 };
    let out_e: [2]PackedSlotDecision = [PackedSlotDecision{}; 2];
    let out_es: []PackedSlotDecision = out_e[0..];
    const rep_e = packPromptWithinBudget(items_even[0..], cfg_even, out_es);

    expect_eq(rep_e.allocated_total, 400);
    expect_eq(rep_e.remaining_tokens, 0);
    expect_eq(rep_e.truncated_count, 1);
    expect_eq(out_e[1].included, true);
    expect_eq(out_e[1].truncated, true);
    expect_eq(out_e[1].head_tokens, 48);
    expect_eq(out_e[1].tail_tokens, 48);
    expect_eq(out_e[1].head_tokens + out_e[1].tail_tokens + cfg_even.ellipsis_overhead_tokens, out_e[1].allocated_tokens);

    // 5b. Odd content budget: effective=401, pinned=300, remaining=101, ellipsis=4 -> content=97 -> head=49, tail=48
    const cfg_odd = BudgetPackConfig{ max_context_tokens: 501, reserved_output_tokens: 100, ellipsis_overhead_tokens: 4 };
    let out_o: [2]PackedSlotDecision = [PackedSlotDecision{}; 2];
    let out_os: []PackedSlotDecision = out_o[0..];
    const rep_o = packPromptWithinBudget(items_even[0..], cfg_odd, out_os);

    expect_eq(rep_o.allocated_total, 401);
    expect_eq(out_o[1].head_tokens, 49);
    expect_eq(out_o[1].tail_tokens, 48);
    expect_eq(out_o[1].head_tokens + out_o[1].tail_tokens + cfg_odd.ellipsis_overhead_tokens, 101);
}

// ============================================================================
// Test 6: must_keep Overflow Sets pinned_overflow Without Violating Budget
// ============================================================================

fn testMustKeepOverflowGuard()! {
    const items: [3]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 1, kind: BudgetItemKind.system_pinned, priority: 100, token_count: 180, min_tokens: 180, policy: BudgetTruncatePolicy.must_keep, recency_seq: 1 },
        BudgetedPromptItem{ item_id: 2, kind: BudgetItemKind.user_query,    priority: 90,  token_count: 150, min_tokens: 150, policy: BudgetTruncatePolicy.must_keep, recency_seq: 2 },
        BudgetedPromptItem{ item_id: 3, kind: BudgetItemKind.tool_schema,   priority: 50,  token_count: 50,  min_tokens: 50,  policy: BudgetTruncatePolicy.must_keep, recency_seq: 3 },
    ];
    // Effective budget = 250: item 1 (180) fits, item 2 (150) overflows -> pinned_overflow=true, item 3 (50) still fits!
    const cfg = BudgetPackConfig{ max_context_tokens: 300, reserved_output_tokens: 50, ellipsis_overhead_tokens: 4 };
    let out_buf: [3]PackedSlotDecision = [PackedSlotDecision{}; 3];
    let out_s: []PackedSlotDecision = out_buf[0..];
    const rep = packPromptWithinBudget(items[0..], cfg, out_s);

    expect_eq(rep.pinned_overflow, true);
    expect_eq(rep.allocated_total <= rep.effective_budget, true);
    expect_eq(rep.allocated_total, 230);
    expect_eq(rep.remaining_tokens, 20);
    expect_eq(out_buf[0].included, true);
    expect_eq(out_buf[1].included, false);
    expect_eq(out_buf[2].included, true);
}

// ============================================================================
// Test 7: Empty Input, Invalid Config, Undersized Buffer & Max Items Guards
// ============================================================================

fn testBoundaryAndInvalidGuards()! {
    const valid_cfg = BudgetPackConfig{ max_context_tokens: 1000, reserved_output_tokens: 200, ellipsis_overhead_tokens: 4 };
    const one_item: [1]BudgetedPromptItem = [
        BudgetedPromptItem{ item_id: 1, kind: BudgetItemKind.user_query, priority: 10, token_count: 50, min_tokens: 50, policy: BudgetTruncatePolicy.must_keep, recency_seq: 1 },
    ];

    // 7a. Empty items
    const empty_items: [0]BudgetedPromptItem = [];
    let out1: [1]PackedSlotDecision = [PackedSlotDecision{}; 1];
    let out1_s: []PackedSlotDecision = out1[0..];
    const rep_empty = packPromptWithinBudget(empty_items[0..], valid_cfg, out1_s);
    expect_eq(rep_empty.included_count, 0);
    expect_eq(rep_empty.dropped_count, 0);
    expect_eq(rep_empty.effective_budget, 800);

    // 7b. Invalid configs (reserved >= max, max == 0)
    const bad_cfg1 = BudgetPackConfig{ max_context_tokens: 500, reserved_output_tokens: 500 };
    const bad_cfg2 = BudgetPackConfig{ max_context_tokens: 0, reserved_output_tokens: 0 };
    expect_eq(bad_cfg1.isValid(), false);
    expect_eq(bad_cfg1.effectiveBudget(), 0);
    expect_eq(bad_cfg2.isValid(), false);
    const rep_bad = packPromptWithinBudget(one_item[0..], bad_cfg1, out1_s);
    expect_eq(rep_bad.included_count, 0);
    expect_eq(rep_bad.dropped_count, 1);

    // 7c. Undersized out_decisions slice
    let out0: [0]PackedSlotDecision = [];
    let out0_s: []PackedSlotDecision = out0[0..];
    const rep_small = packPromptWithinBudget(one_item[0..], valid_cfg, out0_s);
    expect_eq(rep_small.included_count, 0);
    expect_eq(rep_small.dropped_count, 1);

    // 7d. Exceeding BUDGET_PACKER_MAX_ITEMS (65 items)
    const too_many: [BUDGET_PACKER_MAX_ITEMS + 1]BudgetedPromptItem = [BudgetedPromptItem{}; BUDGET_PACKER_MAX_ITEMS + 1];
    let out_many: [BUDGET_PACKER_MAX_ITEMS + 1]PackedSlotDecision = [PackedSlotDecision{}; BUDGET_PACKER_MAX_ITEMS + 1];
    let out_many_s: []PackedSlotDecision = out_many[0..];
    const rep_many = packPromptWithinBudget(too_many[0..], valid_cfg, out_many_s);
    expect_eq(rep_many.included_count, 0);
    expect_eq(rep_many.dropped_count, BUDGET_PACKER_MAX_ITEMS + 1);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("prompt token_budget_packer: ample budget includes all items intact in original order", testAmpleBudgetAllIncludedInOrder);
test("prompt token_budget_packer: phase-1 locks must_keep system & user_query before others", testMustKeepLockedBeforeNonPinned);
test("prompt token_budget_packer: priority & recency_seq deterministically evict older history", testPriorityAndRecencyEviction);
test("prompt token_budget_packer: drop_whole vs keep_head/keep_tail truncation & min_tokens guard", testDropWholeVsKeepHeadAndKeepTail);
test("prompt token_budget_packer: keep_middle_out exact head/tail split & ellipsis conservation", testKeepMiddleOutSplitAndConservation);
test("prompt token_budget_packer: must_keep overflow sets pinned_overflow without exceeding budget", testMustKeepOverflowGuard);
test("prompt token_budget_packer: empty input, invalid config, undersized buffer & max items guards", testBoundaryAndInvalidGuards);
