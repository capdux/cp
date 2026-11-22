// Origin: Pure-CP Stdlib Test Suite (lib/agent/context_compactor.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc AI Agent Context Compactor:
//   1. Within-budget fast path & empty message slice boundary
//   2. Atomic tool-pair group integrity (group_id never split on eviction or pinned propagation)
//   3. Two-stage recency + importance scoring (high-importance kept, low-importance drop, mid summarize_drop + span)
//   4. Recent turns protection (min_recent_turns = 2 aligned with atomic tool groups)
//   5. Pinned exceeds max_tokens budget guard (pinned_exceeds_budget keeps all pinned safely)
//   6. Tight budget best-effort guard (budget_tight_best_effort when recent turns exceed max_tokens)
//   7. validateToolPairIntegrity & markOrphanToolMessages (valid, orphan_call, orphan_result, reversed, missing_group_id)
//   8. 128-message capacity cap, actionAt OOB guard & cap.agent namespace access

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    COMPACTOR_MAX_MESSAGES,
    COMPACTOR_NO_GROUP,
    CompactMessageRole,
    CompactionAction,
    CompactionStatus,
    ToolPairIssueKind,
    CompactMessage,
    CompactionConfig,
    validateToolPairIntegrity,
    markOrphanToolMessages,
    planContextCompaction,
} from "cap:agent";

// ============================================================================
// Test 1: Within-Budget Fast Path & Empty Slice Boundary
// ============================================================================

fn testWithinBudgetAndEmptySlice()! {
    let empty: [1]CompactMessage = [CompactMessage{}];
    const empty_plan = planContextCompaction(empty[0..0], CompactionConfig{ max_tokens: 1000, target_tokens: 800 });
    expect_eq(empty_plan.status == CompactionStatus.within_budget, true);
    expect_eq(empty_plan.message_count, 0);
    expect_eq(empty_plan.kept_tokens, 0);
    expect_eq(empty_plan.has_summary_span, false);
    expect_eq(empty_plan.actionAt(0) == null, true);

    let msgs: [3]CompactMessage = [
        CompactMessage{ msg_id: 1, role: CompactMessageRole.system, token_count: 200, pinned: true },
        CompactMessage{ msg_id: 2, role: CompactMessageRole.user, token_count: 150 },
        CompactMessage{ msg_id: 3, role: CompactMessageRole.assistant, token_count: 250 },
    ];
    const plan = planContextCompaction(msgs[0..], CompactionConfig{ max_tokens: 1000, target_tokens: 800 });
    expect_eq(plan.status == CompactionStatus.within_budget, true);
    expect_eq(plan.original_tokens, 600);
    expect_eq(plan.kept_tokens, 600);
    expect_eq(plan.saved_tokens, 0);
    expect_eq(plan.kept_count, 3);
    expect_eq(plan.summarized_count, 0);
    expect_eq(plan.dropped_count, 0);
    expect_eq(plan.has_summary_span, false);
}

// ============================================================================
// Test 2: Atomic Tool-Pair Integrity (Never Split group_id)
// ============================================================================

fn testAtomicToolPairIntegrityOnCompaction()! {
    // Message 0: pinned system (300)
    // Messages 1..3: group 10 (tool_call 200 + two tool_results 400 + 400 = 1000 tokens, mid importance)
    // Messages 4..5: group 20 (tool_call 200 pinned=true + tool_result 300 pinned=false -> whole group pinned!)
    // Messages 6..7: recent user + assistant (500)
    // Total = 2300 tokens; max_tokens = 1500, target_tokens = 1400
    let msgs: [8]CompactMessage = [
        CompactMessage{ msg_id: 1, role: CompactMessageRole.system, token_count: 300, pinned: true, importance_bps: 10000 },
        CompactMessage{ msg_id: 2, role: CompactMessageRole.tool_call, token_count: 200, group_id: 10, importance_bps: 3000 },
        CompactMessage{ msg_id: 3, role: CompactMessageRole.tool_result, token_count: 400, group_id: 10, importance_bps: 3000 },
        CompactMessage{ msg_id: 4, role: CompactMessageRole.tool_result, token_count: 400, group_id: 10, importance_bps: 3000 },
        CompactMessage{ msg_id: 5, role: CompactMessageRole.tool_call, token_count: 200, group_id: 20, pinned: true, importance_bps: 4000 },
        CompactMessage{ msg_id: 6, role: CompactMessageRole.tool_result, token_count: 300, group_id: 20, pinned: false, importance_bps: 2000 },
        CompactMessage{ msg_id: 7, role: CompactMessageRole.user, token_count: 200, importance_bps: 7000 },
        CompactMessage{ msg_id: 8, role: CompactMessageRole.assistant, token_count: 300, importance_bps: 7000 },
    ];

    const cfg = CompactionConfig{
        max_tokens: 1500,
        target_tokens: 1400,
        summary_reserve_tokens: 100,
        min_recent_turns: 1,
    };
    const plan = planContextCompaction(msgs[0..], cfg);
    expect_eq(plan.status == CompactionStatus.compacted, true);

    // Group 10 (indices 1, 2, 3) must be evicted together atomically as summarize_drop
    const a1 = plan.actionAt(1) ?? return error.MissingA1;
    const a2 = plan.actionAt(2) ?? return error.MissingA2;
    const a3 = plan.actionAt(3) ?? return error.MissingA3;
    expect_eq(a1 == CompactionAction.summarize_drop, true);
    expect_eq(a2 == CompactionAction.summarize_drop, true);
    expect_eq(a3 == CompactionAction.summarize_drop, true);

    // Group 20 (indices 4, 5) must stay kept together because index 4 is pinned
    const a4 = plan.actionAt(4) ?? return error.MissingA4;
    const a5 = plan.actionAt(5) ?? return error.MissingA5;
    expect_eq(a4 == CompactionAction.keep, true);
    expect_eq(a5 == CompactionAction.keep, true);

    expect_eq(plan.kept_tokens, 1300);
    expect_eq(plan.saved_tokens, 1000);
    expect_eq(plan.has_summary_span, true);
    expect_eq(plan.summary_start_idx, 1);
    expect_eq(plan.summary_end_idx, 4);
}

// ============================================================================
// Test 3: Two-Stage Recency + Importance Scoring (keep / summarize_drop / drop)
// ============================================================================

fn testRecencyImportanceScoringAndDropThreshold()! {
    let msgs: [7]CompactMessage = [
        CompactMessage{ msg_id: 1, role: CompactMessageRole.system, token_count: 200, pinned: true, importance_bps: 10000 },
        // Noise message: very low importance (500 < 1500) -> should be dropped directly
        CompactMessage{ msg_id: 2, role: CompactMessageRole.assistant, token_count: 400, importance_bps: 500 },
        // Normal mid-history message (4000 >= 1500) -> should be summarize_drop
        CompactMessage{ msg_id: 3, role: CompactMessageRole.user, token_count: 400, importance_bps: 4000 },
        // High-importance anchor (9500) -> should survive compaction!
        CompactMessage{ msg_id: 4, role: CompactMessageRole.summary_anchor, token_count: 300, importance_bps: 9500 },
        // Normal mid-history message (3500) -> evicted as summarize_drop
        CompactMessage{ msg_id: 5, role: CompactMessageRole.assistant, token_count: 400, importance_bps: 3500 },
        // Recent turn (protected)
        CompactMessage{ msg_id: 6, role: CompactMessageRole.user, token_count: 200, importance_bps: 6000 },
        CompactMessage{ msg_id: 7, role: CompactMessageRole.assistant, token_count: 200, importance_bps: 6000 },
    ];
    // Total = 2100. Keep system(200) + anchor(300) + recent(400) = 900 <= target(1200) - reserve(200) = 1000
    const cfg = CompactionConfig{
        max_tokens: 1500,
        target_tokens: 1200,
        summary_reserve_tokens: 200,
        min_recent_turns: 1,
        recency_weight_bps: 4000,
        importance_weight_bps: 6000,
        drop_instead_of_summarize_below_bps: 1500,
    };

    const plan = planContextCompaction(msgs[0..], cfg);
    expect_eq(plan.status == CompactionStatus.compacted, true);
    expect_eq((plan.actionAt(0) ?? CompactionAction.drop) == CompactionAction.keep, true);
    expect_eq((plan.actionAt(1) ?? CompactionAction.keep) == CompactionAction.drop, true);
    expect_eq((plan.actionAt(2) ?? CompactionAction.keep) == CompactionAction.summarize_drop, true);
    expect_eq((plan.actionAt(3) ?? CompactionAction.drop) == CompactionAction.keep, true);
    expect_eq((plan.actionAt(4) ?? CompactionAction.keep) == CompactionAction.summarize_drop, true);
    expect_eq((plan.actionAt(5) ?? CompactionAction.drop) == CompactionAction.keep, true);
    expect_eq((plan.actionAt(6) ?? CompactionAction.drop) == CompactionAction.keep, true);

    expect_eq(plan.dropped_count, 1);
    expect_eq(plan.summarized_count, 2);
    expect_eq(plan.kept_count, 4);
    expect_eq(plan.kept_tokens, 900);
    expect_eq(plan.saved_tokens, 1200);
    expect_eq(plan.has_summary_span, true);
    expect_eq(plan.summary_start_idx, 2);
    expect_eq(plan.summary_end_idx, 5);
}

// ============================================================================
// Test 4: Recent Turns Protection Aligned With Atomic Groups
// ============================================================================

fn testRecentTurnsProtection()! {
    let msgs: [7]CompactMessage = [
        CompactMessage{ msg_id: 1, role: CompactMessageRole.user, token_count: 500, importance_bps: 5000 },
        CompactMessage{ msg_id: 2, role: CompactMessageRole.assistant, token_count: 500, importance_bps: 5000 },
        // Turn N-1 (must be protected when min_recent_turns == 2)
        CompactMessage{ msg_id: 3, role: CompactMessageRole.user, token_count: 300, importance_bps: 1000 },
        CompactMessage{ msg_id: 4, role: CompactMessageRole.tool_call, token_count: 200, group_id: 77, importance_bps: 1000 },
        CompactMessage{ msg_id: 5, role: CompactMessageRole.tool_result, token_count: 200, group_id: 77, importance_bps: 1000 },
        // Turn N (must be protected)
        CompactMessage{ msg_id: 6, role: CompactMessageRole.user, token_count: 200, importance_bps: 1000 },
        CompactMessage{ msg_id: 7, role: CompactMessageRole.assistant, token_count: 200, importance_bps: 1000 },
    ];

    const cfg = CompactionConfig{
        max_tokens: 1500,
        target_tokens: 1200,
        summary_reserve_tokens: 0,
        min_recent_turns: 2,
    };
    const plan = planContextCompaction(msgs[0..], cfg);
    expect_eq(plan.status == CompactionStatus.compacted, true);

    // Only oldest turn (indices 0, 1) evicted; last 2 turns (indices 2..6) strictly kept
    expect_eq((plan.actionAt(0) ?? CompactionAction.keep) == CompactionAction.summarize_drop, true);
    expect_eq((plan.actionAt(1) ?? CompactionAction.keep) == CompactionAction.summarize_drop, true);
    let idx: usize = 2;
    while (idx < 7) : (idx += 1) {
        expect_eq((plan.actionAt(idx) ?? CompactionAction.drop) == CompactionAction.keep, true);
    }
    expect_eq(plan.kept_tokens, 1100);
}

// ============================================================================
// Test 5: Pinned Exceeds Budget Guard (pinned_exceeds_budget)
// ============================================================================

fn testPinnedExceedsBudgetGuard()! {
    let msgs: [4]CompactMessage = [
        CompactMessage{ msg_id: 1, role: CompactMessageRole.system, token_count: 1200, pinned: true, importance_bps: 10000 },
        CompactMessage{ msg_id: 2, role: CompactMessageRole.summary_anchor, token_count: 600, pinned: true, importance_bps: 9000 },
        CompactMessage{ msg_id: 3, role: CompactMessageRole.user, token_count: 400, pinned: false, importance_bps: 3000 },
        CompactMessage{ msg_id: 4, role: CompactMessageRole.assistant, token_count: 400, pinned: false, importance_bps: 3000 },
    ];
    const cfg = CompactionConfig{
        max_tokens: 1500,
        target_tokens: 1200,
        min_recent_turns: 0,
    };
    const plan = planContextCompaction(msgs[0..], cfg);
    expect_eq(plan.status == CompactionStatus.pinned_exceeds_budget, true);
    expect_eq((plan.actionAt(0) ?? CompactionAction.drop) == CompactionAction.keep, true);
    expect_eq((plan.actionAt(1) ?? CompactionAction.drop) == CompactionAction.keep, true);
    expect_eq((plan.actionAt(2) ?? CompactionAction.keep) == CompactionAction.summarize_drop, true);
    expect_eq((plan.actionAt(3) ?? CompactionAction.keep) == CompactionAction.summarize_drop, true);
    expect_eq(plan.kept_tokens, 1800);
    expect_eq(plan.saved_tokens, 800);
}

// ============================================================================
// Test 6: Budget Tight Best-Effort Guard (Protected Recent Turns > max_tokens)
// ============================================================================

fn testBudgetTightBestEffortGuard()! {
    let msgs: [4]CompactMessage = [
        CompactMessage{ msg_id: 1, role: CompactMessageRole.user, token_count: 300, importance_bps: 2000 },
        CompactMessage{ msg_id: 2, role: CompactMessageRole.assistant, token_count: 300, importance_bps: 2000 },
        CompactMessage{ msg_id: 3, role: CompactMessageRole.user, token_count: 700, importance_bps: 8000 },
        CompactMessage{ msg_id: 4, role: CompactMessageRole.assistant, token_count: 700, importance_bps: 8000 },
    ];
    const cfg = CompactionConfig{
        max_tokens: 1000,
        target_tokens: 900,
        min_recent_turns: 1,
    };
    const plan = planContextCompaction(msgs[0..], cfg);
    expect_eq(plan.status == CompactionStatus.budget_tight_best_effort, true);
    expect_eq(plan.kept_tokens, 1400);
    expect_eq(plan.saved_tokens, 600);
}

// ============================================================================
// Test 7: Tool-Pair Integrity Validation & Orphan Flagging
// ============================================================================

fn testValidateToolPairIntegrityAndOrphans()! {
    // 1. Valid conversation with 1 tool_call + 2 parallel tool_results under group 5
    let good: [4]CompactMessage = [
        CompactMessage{ msg_id: 1, role: CompactMessageRole.user, token_count: 50 },
        CompactMessage{ msg_id: 2, role: CompactMessageRole.tool_call, token_count: 40, group_id: 5 },
        CompactMessage{ msg_id: 3, role: CompactMessageRole.tool_result, token_count: 60, group_id: 5 },
        CompactMessage{ msg_id: 4, role: CompactMessageRole.tool_result, token_count: 60, group_id: 5 },
    ];
    const good_rep = validateToolPairIntegrity(good[0..]);
    expect_eq(good_rep.valid, true);
    expect_eq(good_rep.total_tool_calls, 1);
    expect_eq(good_rep.total_tool_results, 2);
    expect_eq(good_rep.orphan_tool_calls, 0);
    expect_eq(good_rep.orphan_tool_results, 0);

    // 2. Broken conversation: orphan tool_result before call, orphan tool_call, missing group_id
    let bad: [5]CompactMessage = [
        CompactMessage{ msg_id: 1, role: CompactMessageRole.tool_result, token_count: 50, group_id: 9 }, // orphan result (before call)
        CompactMessage{ msg_id: 2, role: CompactMessageRole.tool_call, token_count: 50, group_id: 9 },   // orphan call (no result after)
        CompactMessage{ msg_id: 3, role: CompactMessageRole.tool_call, token_count: 50, group_id: COMPACTOR_NO_GROUP }, // missing group_id
        CompactMessage{ msg_id: 4, role: CompactMessageRole.tool_call, token_count: 50, group_id: 12 },  // valid pair
        CompactMessage{ msg_id: 5, role: CompactMessageRole.tool_result, token_count: 50, group_id: 12 },
    ];
    const bad_rep = validateToolPairIntegrity(bad[0..]);
    expect_eq(bad_rep.valid, false);
    expect_eq(bad_rep.orphan_tool_results, 1);
    expect_eq(bad_rep.orphan_tool_calls, 1);
    expect_eq(bad_rep.missing_group_id_count, 1);
    expect_eq(bad_rep.first_issue_index, 0);
    expect_eq(bad_rep.first_issue_kind == ToolPairIssueKind.orphan_tool_result, true);

    let flags: [5]bool = [false; 5];
    let flags_s: []bool = flags[0..];
    const orphan_cnt = markOrphanToolMessages(bad[0..], *flags_s);
    expect_eq(orphan_cnt, 3);
    expect_eq(flags[0], true);
    expect_eq(flags[1], true);
    expect_eq(flags[2], true);
    expect_eq(flags[3], false);
    expect_eq(flags[4], false);
}

// ============================================================================
// Test 8: 128-Message Capacity Cap & cap.agent Namespace Invocation
// ============================================================================

fn testCapacityCapAndNamespaceAccess()! {
    let big: [140]CompactMessage = [CompactMessage{}; 140];
    let i: usize = 0;
    while (i < 140) : (i += 1) {
        big[i] = CompactMessage{
            msg_id: (i as u32) + 1,
            role: if (i % 2 == 0) CompactMessageRole.user else CompactMessageRole.assistant,
            token_count: 100,
            importance_bps: 3000,
        };
    }

    const plan = cap.agent.planContextCompaction(big[0..], CompactionConfig{
        max_tokens: 4000,
        target_tokens: 3000,
        summary_reserve_tokens: 200,
        min_recent_turns: 2,
    });
    expect_eq(plan.message_count, COMPACTOR_MAX_MESSAGES);
    expect_eq(plan.status == CompactionStatus.compacted, true);
    expect_eq(plan.kept_tokens <= 3000, true);
    expect_eq(plan.actionAt(COMPACTOR_MAX_MESSAGES) == null, true);
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("context_compactor: within-budget fast path & empty slice boundary", testWithinBudgetAndEmptySlice);
test("context_compactor: atomic tool-pair integrity (never split group_id)", testAtomicToolPairIntegrityOnCompaction);
test("context_compactor: two-stage recency + importance scoring (keep/summarize_drop/drop)", testRecencyImportanceScoringAndDropThreshold);
test("context_compactor: recent turns protection aligned with atomic groups", testRecentTurnsProtection);
test("context_compactor: pinned exceeds budget guard (pinned_exceeds_budget)", testPinnedExceedsBudgetGuard);
test("context_compactor: tight budget best-effort guard (budget_tight_best_effort)", testBudgetTightBestEffortGuard);
test("context_compactor: validateToolPairIntegrity & markOrphanToolMessages", testValidateToolPairIntegrityAndOrphans);
test("context_compactor: 128-message capacity cap & cap.agent namespace access", testCapacityCapAndNamespaceAccess);
