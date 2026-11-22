// Origin: Pure-CP Stdlib Test Suite (lib/attention/spec_tree.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Medusa / Eagle tree-structured speculative decoding:
// multi-branch N x N tree causal attention mask (sibling isolation + ancestor visibility),
// single/multi-branch greedy longest accepted path + bonus token, all-rejected fallback,
// invalid non-topological/broken/cyclic tree rejection, and undersized buffer guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SPEC_TREE_NO_PARENT,
    DraftTreeNode,
    TreeVerifyResult,
    validateDraftTree,
    buildTreeAttentionMask,
    verifyDraftTreeGreedy,
} from "cap:attention";

// ============================================================================
// Test 1: Multi-Branch N x N Tree Causal Attention Mask Exactness
// ============================================================================

fn testMultiBranchTreeCausalAttentionMask()! {
    // 7-node multi-branch tree:
    //        0
    //       / \
    //      1   2
    //     / \   \
    //    3   4   5
    //             \
    //              6
    const nodes: [7]DraftTreeNode = [
        DraftTreeNode{ token_id: 100, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
        DraftTreeNode{ token_id: 101, parent_idx: 0, depth: 1 },
        DraftTreeNode{ token_id: 102, parent_idx: 0, depth: 1 },
        DraftTreeNode{ token_id: 103, parent_idx: 1, depth: 2 },
        DraftTreeNode{ token_id: 104, parent_idx: 1, depth: 2 },
        DraftTreeNode{ token_id: 105, parent_idx: 2, depth: 2 },
        DraftTreeNode{ token_id: 106, parent_idx: 5, depth: 3 },
    ];

    expect_eq(cap.attention.validateDraftTree(nodes[0..]), true);

    let mask: [49]u8 = [9; 49];
    let mask_s: []u8 = mask[0..];
    expect_eq(cap.attention.buildTreeAttentionMask(nodes[0..], mask_s), true);

    // Strictly lower-triangular (no future token j > i visible).
    let i: usize = 0;
    while (i < 7) : (i += 1) {
        expect_eq(mask[i * 7 + i], 1 as u8);
        let j: usize = i + 1;
        while (j < 7) : (j += 1) {
            expect_eq(mask[i * 7 + j], 0 as u8);
        }
    }

    // Siblings 1 and 2 are mutually invisible.
    expect_eq(mask[2 * 7 + 0], 1 as u8);
    expect_eq(mask[2 * 7 + 1], 0 as u8);

    // Node 3 sees {0, 1, 3} only.
    expect_eq(mask[3 * 7 + 0], 1 as u8);
    expect_eq(mask[3 * 7 + 1], 1 as u8);
    expect_eq(mask[3 * 7 + 2], 0 as u8);
    expect_eq(mask[3 * 7 + 3], 1 as u8);

    // Node 4 sees {0, 1, 4}; sibling 3 and cousin branch 2 are invisible.
    expect_eq(mask[4 * 7 + 0], 1 as u8);
    expect_eq(mask[4 * 7 + 1], 1 as u8);
    expect_eq(mask[4 * 7 + 2], 0 as u8);
    expect_eq(mask[4 * 7 + 3], 0 as u8);
    expect_eq(mask[4 * 7 + 4], 1 as u8);

    // Node 6 sees {0, 2, 5, 6}; entire left subtree {1, 3, 4} is invisible.
    expect_eq(mask[6 * 7 + 0], 1 as u8);
    expect_eq(mask[6 * 7 + 1], 0 as u8);
    expect_eq(mask[6 * 7 + 2], 1 as u8);
    expect_eq(mask[6 * 7 + 3], 0 as u8);
    expect_eq(mask[6 * 7 + 4], 0 as u8);
    expect_eq(mask[6 * 7 + 5], 1 as u8);
    expect_eq(mask[6 * 7 + 6], 1 as u8);
}

// ============================================================================
// Test 2: Single-Branch & Multi-Branch Greedy Verification + Bonus Token
// ============================================================================

fn testSingleAndMultiBranchGreedyVerificationAndBonusToken()! {
    // Part A: Single linear chain 0 -> 1 -> 2 -> 3 (all accepted + bonus)
    {
        const chain: [4]DraftTreeNode = [
            DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
            DraftTreeNode{ token_id: 10, parent_idx: 0, depth: 1 },
            DraftTreeNode{ token_id: 20, parent_idx: 1, depth: 2 },
            DraftTreeNode{ token_id: 30, parent_idx: 2, depth: 3 },
        ];
        const argmax: [4]u32 = [10, 20, 30, 40];
        let accepted: [4]u32 = [0; 4];
        let accepted_s: []u32 = accepted[0..];

        if (res := cap.attention.verifyDraftTreeGreedy(chain[0..], argmax[0..], accepted_s)) {
            expect_eq(res.accepted_len, 3 as usize);
            expect_eq(res.bonus_token_id, 40 as u32);
            expect_eq(res.last_accepted_node, 3 as u16);
            expect_eq(accepted[0], 10 as u32);
            expect_eq(accepted[1], 20 as u32);
            expect_eq(accepted[2], 30 as u32);
        } else {
            expect_eq(true, false);
        }
    }

    // Part B: 7-node multi-branch tree — target selects right deep branch 0 -> 2 -> 5 -> 6
    {
        const nodes: [7]DraftTreeNode = [
            DraftTreeNode{ token_id: 100, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
            DraftTreeNode{ token_id: 101, parent_idx: 0, depth: 1 },
            DraftTreeNode{ token_id: 102, parent_idx: 0, depth: 1 },
            DraftTreeNode{ token_id: 103, parent_idx: 1, depth: 2 },
            DraftTreeNode{ token_id: 104, parent_idx: 1, depth: 2 },
            DraftTreeNode{ token_id: 105, parent_idx: 2, depth: 2 },
            DraftTreeNode{ token_id: 106, parent_idx: 5, depth: 3 },
        ];
        // Node 0 predicts 102 (node 2); node 2 predicts 105 (node 5); node 5 predicts 106 (node 6); node 6 predicts bonus 888.
        const argmax: [7]u32 = [102, 103, 105, 999, 999, 106, 888];
        let accepted: [4]u32 = [0; 4];
        let accepted_s: []u32 = accepted[0..];

        if (res := verifyDraftTreeGreedy(nodes[0..], argmax[0..], accepted_s)) {
            expect_eq(res.accepted_len, 3 as usize);
            expect_eq(res.bonus_token_id, 888 as u32);
            expect_eq(res.last_accepted_node, 6 as u16);
            expect_eq(accepted[0], 102 as u32);
            expect_eq(accepted[1], 105 as u32);
            expect_eq(accepted[2], 106 as u32);
        } else {
            expect_eq(true, false);
        }

        // Part C: Mid-branch divergence at node 1 (predicts 555 which matches neither child 3 nor 4).
        const argmax_div: [7]u32 = [101, 555, 105, 999, 999, 106, 888];
        if (res2 := verifyDraftTreeGreedy(nodes[0..], argmax_div[0..], accepted_s)) {
            expect_eq(res2.accepted_len, 1 as usize);
            expect_eq(res2.bonus_token_id, 555 as u32);
            expect_eq(res2.last_accepted_node, 1 as u16);
            expect_eq(accepted[0], 101 as u32);
        } else {
            expect_eq(true, false);
        }
    }
}

// ============================================================================
// Test 3: All-Rejected Draft Returns Root Bonus Token Safely
// ============================================================================

fn testAllRejectedReturnsRootBonusToken()! {
    const nodes: [3]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
        DraftTreeNode{ token_id: 10, parent_idx: 0, depth: 1 },
        DraftTreeNode{ token_id: 20, parent_idx: 0, depth: 1 },
    ];
    const argmax: [3]u32 = [777, 11, 22]; // 777 matches neither child 10 nor 20
    let empty_buf: [1]u32 = [0];
    let empty_s: []u32 = empty_buf[0..0];

    if (res := verifyDraftTreeGreedy(nodes[0..], argmax[0..], empty_s)) {
        expect_eq(res.accepted_len, 0 as usize);
        expect_eq(res.bonus_token_id, 777 as u32);
        expect_eq(res.last_accepted_node, 0 as u16);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 4: Invalid Non-Topological, Broken-Link & Cyclic Tree Rejection
// ============================================================================

fn testInvalidNonTopologicalBrokenAndCyclicTreesRejected()! {
    const empty_arr: [1]DraftTreeNode = [DraftTreeNode{}];
    expect_eq(validateDraftTree(empty_arr[0..0]), false);

    // Root has parent_idx != SPEC_TREE_NO_PARENT
    const bad_root_parent: [1]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: 0, depth: 0 },
    ];
    expect_eq(validateDraftTree(bad_root_parent[0..]), false);

    // Root has depth != 0
    const bad_root_depth: [1]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 1 },
    ];
    expect_eq(validateDraftTree(bad_root_depth[0..]), false);

    // Disconnected second root
    const broken_link: [3]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
        DraftTreeNode{ token_id: 2, parent_idx: 0, depth: 1 },
        DraftTreeNode{ token_id: 3, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
    ];
    expect_eq(validateDraftTree(broken_link[0..]), false);

    // Non-topological / cycle: node 1 -> parent 2, node 2 -> parent 1
    const cyclic: [3]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
        DraftTreeNode{ token_id: 2, parent_idx: 2, depth: 1 },
        DraftTreeNode{ token_id: 3, parent_idx: 1, depth: 2 },
    ];
    expect_eq(validateDraftTree(cyclic[0..]), false);

    // Self-loop on node 1
    const self_loop: [2]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
        DraftTreeNode{ token_id: 2, parent_idx: 1, depth: 1 },
    ];
    expect_eq(validateDraftTree(self_loop[0..]), false);

    // Inconsistent depth
    const bad_depth: [2]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
        DraftTreeNode{ token_id: 2, parent_idx: 0, depth: 2 },
    ];
    expect_eq(validateDraftTree(bad_depth[0..]), false);

    // Downstream APIs must also reject invalid trees.
    let mask: [9]u8 = [0; 9];
    let mask_s: []u8 = mask[0..];
    const argmax: [3]u32 = [2, 3, 4];
    let acc: [3]u32 = [0; 3];
    let acc_s: []u32 = acc[0..];
    expect_eq(buildTreeAttentionMask(cyclic[0..], mask_s), false);
    expect_eq(verifyDraftTreeGreedy(cyclic[0..], argmax[0..], acc_s) == null, true);
}

// ============================================================================
// Test 5: Undersized Buffer Defensive Guards
// ============================================================================

fn testUndersizedBuffersDefensiveGuards()! {
    const nodes: [4]DraftTreeNode = [
        DraftTreeNode{ token_id: 1, parent_idx: SPEC_TREE_NO_PARENT, depth: 0 },
        DraftTreeNode{ token_id: 10, parent_idx: 0, depth: 1 },
        DraftTreeNode{ token_id: 20, parent_idx: 1, depth: 2 },
        DraftTreeNode{ token_id: 30, parent_idx: 2, depth: 3 },
    ];

    // Mask requires 4 * 4 = 16 bytes; 15 bytes must fail.
    let short_mask: [15]u8 = [0; 15];
    let short_mask_s: []u8 = short_mask[0..];
    expect_eq(buildTreeAttentionMask(nodes[0..], short_mask_s), false);

    // target_argmax shorter than nodes.len must fail.
    const short_argmax: [3]u32 = [10, 20, 30];
    let acc_ok: [4]u32 = [0; 4];
    let acc_ok_s: []u32 = acc_ok[0..];
    expect_eq(verifyDraftTreeGreedy(nodes[0..], short_argmax[0..], acc_ok_s) == null, true);

    // out_accepted_tokens shorter than accepted path length (needs 3, given 2) must return null.
    const full_argmax: [4]u32 = [10, 20, 30, 40];
    let short_acc: [2]u32 = [0; 2];
    let short_acc_s: []u32 = short_acc[0..];
    expect_eq(verifyDraftTreeGreedy(nodes[0..], full_argmax[0..], short_acc_s) == null, true);
}

fn main() {
    test("1. attention.spec_tree: multi-branch N x N tree causal attention mask exactness", () => {
        testMultiBranchTreeCausalAttentionMask()?;
    });
    test("2. attention.spec_tree: single & multi-branch greedy verification + bonus token", () => {
        testSingleAndMultiBranchGreedyVerificationAndBonusToken()?;
    });
    test("3. attention.spec_tree: all-rejected draft returns root bonus token safely", () => {
        testAllRejectedReturnsRootBonusToken()?;
    });
    test("4. attention.spec_tree: invalid non-topological, broken-link & cyclic tree rejection", () => {
        testInvalidNonTopologicalBrokenAndCyclicTreesRejected()?;
    });
    test("5. attention.spec_tree: undersized mask/argmax/output buffer defensive guards", () => {
        testUndersizedBuffersDefensiveGuards()?;
    });
}
