// Origin: Pure-CP Stdlib (AI Inference Tree-Structured Speculative Decoding Mask & Verifier) | Created: 2026-11-22
// Pure CP zero-heap-allocation Medusa / Eagle tree-structured speculative decoding engine:
// strict topological draft tree validation, O(N^2) ancestor causal attention mask construction
// for single-pass parallel tree verification, and greedy longest-accepted-path + bonus token verifier.

const std = @import("std");

/// Sentinel `parent_idx` marking the root node of a speculative draft tree.
pub const SPEC_TREE_NO_PARENT: u16 = 0xFFFF;

/// Single node in a topologically ordered speculative decoding draft tree.
/// Node `0` is always the committed root token (`parent_idx = SPEC_TREE_NO_PARENT`, `depth = 0`).
pub struct DraftTreeNode {
    token_id: u32 = 0;
    parent_idx: u16 = SPEC_TREE_NO_PARENT;
    depth: u16 = 0;
}

/// Output summary returned by `verifyDraftTreeGreedy`.
pub struct TreeVerifyResult {
    /// Number of draft tokens accepted along the longest matching path (excluding the bonus token).
    accepted_len: usize = 0;
    /// Free bonus token predicted by the target model at `last_accepted_node`.
    bonus_token_id: u32 = 0;
    /// Node index of the deepest accepted node (`0` if all draft children were rejected).
    last_accepted_node: u16 = 0;
}

// ============================================================================
// Public Tree-Structured Speculative Decoding API
// ============================================================================

/// Validates that `nodes` forms a well-formed, acyclic, single-root draft tree in strict
/// topological order:
/// - `nodes.len >= 1` and `< SPEC_TREE_NO_PARENT`
/// - `nodes[0].parent_idx == SPEC_TREE_NO_PARENT` and `nodes[0].depth == 0`
/// - For every `i >= 1`: `nodes[i].parent_idx < i` and `nodes[i].depth == nodes[parent].depth + 1`
pub fn validateDraftTree(nodes: []const DraftTreeNode): bool {
    const n: usize = nodes.len;
    if (n == 0 || n >= (SPEC_TREE_NO_PARENT as usize)) return false;

    if (nodes[0].parent_idx != SPEC_TREE_NO_PARENT || nodes[0].depth != 0) {
        return false;
    }

    let i: usize = 1;
    while (i < n) : (i += 1) {
        const p: u16 = nodes[i].parent_idx;
        if (p == SPEC_TREE_NO_PARENT) return false;
        const p_idx: usize = p as usize;
        if (p_idx >= i) return false;
        const p_depth: u16 = nodes[p_idx].depth;
        if (p_depth == std.math.maxInt(u16)) return false;
        if (nodes[i].depth != p_depth + (1 as u16)) return false;
    }

    return true;
}

/// Constructs the `N x N` row-major tree causal attention mask in `out_mask_u8`:
/// `out_mask_u8[i * N + j] == 1` iff node `j` is an ancestor of node `i` or `j == i`, else `0`.
/// Enables single-forward-pass parallel verification of an entire multi-branch draft tree
/// while keeping sibling branches mutually invisible.
///
/// Returns `false` if `nodes` fails `validateDraftTree` or `out_mask_u8.len < N * N`.
pub fn buildTreeAttentionMask(
    nodes: []const DraftTreeNode,
    out_mask_u8: *[]u8,
): bool {
    if (!validateDraftTree(nodes)) return false;
    const n: usize = nodes.len;
    if (n > std.math.maxInt(usize) / n) return false;
    const total: usize = n * n;
    if (out_mask_u8.len < total) return false;

    let z: usize = 0;
    while (z < total) : (z += 1) {
        out_mask_u8[z] = 0;
    }

    // Root attends only to itself.
    out_mask_u8[0] = 1;

    // Topological DP: node `i` inherits its parent's ancestor row (`parent < i`) plus itself.
    let i: usize = 1;
    while (i < n) : (i += 1) {
        const p_idx: usize = nodes[i].parent_idx as usize;
        const p_row: usize = p_idx * n;
        const cur_row: usize = i * n;
        let j: usize = 0;
        while (j < i) : (j += 1) {
            out_mask_u8[cur_row + j] = out_mask_u8[p_row + j];
        }
        out_mask_u8[cur_row + i] = 1;
    }

    return true;
}

/// Greedily verifies a draft tree against the target model's per-node `target_argmax` predictions.
///
/// Starting at root node `0`, repeatedly advances to the child whose `token_id` matches
/// `target_argmax[cur]`, appending each accepted draft token into `out_accepted_tokens`.
/// Stops when no child matches and returns `TreeVerifyResult` containing `accepted_len`,
/// the free `bonus_token_id = target_argmax[last_accepted_node]`, and `last_accepted_node`.
///
/// Returns `null` if `nodes` is invalid, `target_argmax.len < nodes.len`, or `out_accepted_tokens`
/// cannot hold the accepted path length.
pub fn verifyDraftTreeGreedy(
    nodes: []const DraftTreeNode,
    target_argmax: []const u32,
    out_accepted_tokens: *[]u32,
): TreeVerifyResult? {
    if (!validateDraftTree(nodes)) return null;
    const n: usize = nodes.len;
    if (target_argmax.len < n) return null;

    let cur: u16 = 0;
    let accepted_len: usize = 0;

    while (true) {
        const want_tok: u32 = target_argmax[cur as usize];
        let matched_child: u16 = SPEC_TREE_NO_PARENT;

        let c: usize = (cur as usize) + 1;
        while (c < n) : (c += 1) {
            if (nodes[c].parent_idx == cur && nodes[c].token_id == want_tok) {
                matched_child = c as u16;
                break;
            }
        }

        if (matched_child == SPEC_TREE_NO_PARENT) {
            return TreeVerifyResult{
                accepted_len: accepted_len,
                bonus_token_id: want_tok,
                last_accepted_node: cur,
            };
        }

        if (accepted_len >= out_accepted_tokens.len) {
            return null;
        }
        out_accepted_tokens[accepted_len] = want_tok;
        accepted_len += 1;
        cur = matched_child;
    }
}
