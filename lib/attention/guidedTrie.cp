// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Guided Decoding Prefix-Trie & Logit Masker) | Created: 2026-11-22
// Pure CP zero-heap-allocation Token-ID Prefix-Trie (Left-Child Right-Sibling) and O(V + B) Logit Masker
// for vLLM / SGLang style constrained LLM decoding (tool-name whitelist, enum constraints, JSON key guards):
//   - Inline fixed-capacity 512-node LCRS trie (`GuidedTokenTrie`) with zero dynamic heap allocation
//   - Atomic two-phase `insert_sequence` (prefix sharing, idempotent duplicates, empty/overflow rollback-free guard)
//   - Immutable value-type `GuidedTrieCursor` supporting speculative branching & dead-state interception
//   - O(V + B) zero-alloc `apply_logit_mask_f32` preserving exact original logits for valid next tokens & EOS

const GUIDED_MASK_LOGIT: f32 = -1.0e30;

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum total nodes (including root at index 0) in `GuidedTokenTrie` (zero dynamic heap).
pub const GUIDED_TRIE_MAX_NODES: usize = 512;

/// Sentinel index indicating no child, sibling, or parent node.
pub const GUIDED_TRIE_NO_NODE: u16 = 0xFFFF;

/// Single node in the Left-Child Right-Sibling (LCRS) token prefix trie.
pub struct GuidedTrieNode {
    token_id: u32 = 0;
    is_terminal: bool = false;
    first_child: u16 = GUIDED_TRIE_NO_NODE;
    next_sibling: u16 = GUIDED_TRIE_NO_NODE;
    parent: u16 = GUIDED_TRIE_NO_NODE;
    depth: u16 = 0;
}

/// Lightweight immutable decoding cursor tracking the current trie position.
pub struct GuidedTrieCursor {
    node_idx: u16 = 0;
    step_count: usize = 0;
    is_complete: bool = false;
    is_dead: bool = false;
}

/// Summary statistics snapshot of a `GuidedTokenTrie`.
pub struct GuidedTrieStats {
    node_count: usize = 1;
    sequence_count: usize = 0;
    max_depth: usize = 0;
}

// ============================================================================
// 2. Zero-Alloc Left-Child Right-Sibling Guided Token Trie
// ============================================================================

/// Zero-heap-allocation Token-ID Prefix-Trie for constrained autoregressive decoding.
/// Node `0` is always the virtual root (`depth == 0`, `is_terminal == false`).
pub struct GuidedTokenTrie {
    nodes: [GUIDED_TRIE_MAX_NODES]GuidedTrieNode = [GuidedTrieNode{}; GUIDED_TRIE_MAX_NODES];
    node_count: usize = 1;
    sequence_count: usize = 0;
    max_depth: usize = 0;

    /// Initializes an empty `GuidedTokenTrie` containing only the virtual root node at index `0`.
    static fn init(): GuidedTokenTrie {
        return GuidedTokenTrie{
            nodes: [GuidedTrieNode{}; GUIDED_TRIE_MAX_NODES],
            node_count: 1,
            sequence_count: 0,
            max_depth: 0,
        };
    }

    /// Internal helper: finds the child node index under `parent_idx` matching `token_id`,
    /// or `GUIDED_TRIE_NO_NODE` if absent.
    fn findChild(parent_idx: u16, token_id: u32): u16 {
        if ((parent_idx as usize) >= _.node_count) return GUIDED_TRIE_NO_NODE;
        let child_idx: u16 = _.nodes[parent_idx as usize].first_child;
        while (child_idx != GUIDED_TRIE_NO_NODE) {
            const c_usize: usize = child_idx as usize;
            if (c_usize >= _.node_count) return GUIDED_TRIE_NO_NODE;
            if (_.nodes[c_usize].token_id == token_id) {
                return child_idx;
            }
            child_idx = _.nodes[c_usize].next_sibling;
        }
        return GUIDED_TRIE_NO_NODE;
    }

    /// Internal helper: appends `new_idx` to the tail of `parent_idx`'s child linked list
    /// to preserve deterministic insertion order during enumeration.
    fn appendChild(*_, parent_idx: u16, new_idx: u16) {
        const p_usize: usize = parent_idx as usize;
        let child_idx: u16 = _.nodes[p_usize].first_child;
        if (child_idx == GUIDED_TRIE_NO_NODE) {
            _.nodes[p_usize].first_child = new_idx;
            return;
        }
        while (true) {
            const c_usize: usize = child_idx as usize;
            const next_sib: u16 = _.nodes[c_usize].next_sibling;
            if (next_sib == GUIDED_TRIE_NO_NODE) {
                _.nodes[c_usize].next_sibling = new_idx;
                return;
            }
            child_idx = next_sib;
        }
    }

    /// Inserts a valid token sequence into the prefix trie.
    /// - Rejects empty sequences (`tokens.len == 0`), returning `false`.
    /// - Shares existing common prefixes without allocating duplicate nodes.
    /// - Idempotent on already-registered sequences (returns `true` without inflating `sequence_count`).
    /// - Atomically checks remaining capacity against `GUIDED_TRIE_MAX_NODES` before mutating,
    ///   returning `false` with zero partial nodes if capacity is insufficient.
    fn insert_sequence(*_, tokens: []const u32): bool {
        if (tokens.len == 0) return false;

        // Phase 1: walk the longest matching existing prefix.
        let cur_idx: u16 = 0;
        let matched: usize = 0;
        while (matched < tokens.len) {
            const next_idx: u16 = _.findChild(cur_idx, tokens[matched]);
            if (next_idx == GUIDED_TRIE_NO_NODE) break;
            cur_idx = next_idx;
            matched += 1;
        }

        const needed: usize = tokens.len - matched;
        if (needed == 0) {
            // Sequence path already exists in trie.
            if (!_.nodes[cur_idx as usize].is_terminal) {
                _.nodes[cur_idx as usize].is_terminal = true;
                _.sequence_count += 1;
                if (tokens.len > _.max_depth) {
                    _.max_depth = tokens.len;
                }
            }
            return true;
        }

        // Atomic capacity pre-flight guard (prevents partial insertion on full trie).
        if (_.node_count + needed > GUIDED_TRIE_MAX_NODES) {
            return false;
        }

        // Phase 2: allocate and link remaining suffix nodes.
        let i: usize = matched;
        while (i < tokens.len) : (i += 1) {
            const new_usize: usize = _.node_count;
            const new_idx: u16 = @intCast(new_usize);
            const depth_u16: u16 = @intCast(i + 1);
            _.nodes[new_usize] = GuidedTrieNode{
                token_id: tokens[i],
                is_terminal: false,
                first_child: GUIDED_TRIE_NO_NODE,
                next_sibling: GUIDED_TRIE_NO_NODE,
                parent: cur_idx,
                depth: depth_u16,
            };
            _.node_count += 1;
            _.appendChild(cur_idx, new_idx);
            cur_idx = new_idx;
        }

        _.nodes[cur_idx as usize].is_terminal = true;
        _.sequence_count += 1;
        if (tokens.len > _.max_depth) {
            _.max_depth = tokens.len;
        }
        return true;
    }

    /// Creates a fresh cursor positioned at the virtual root node (`node_idx = 0`).
    fn new_cursor(): GuidedTrieCursor {
        return GuidedTrieCursor{
            node_idx: 0,
            step_count: 0,
            is_complete: false,
            is_dead: false,
        };
    }

    /// Advances `cursor` by `token_id`.
    /// - If `token_id` matches a child of `cursor.node_idx`, returns the next cursor with
    ///   `step_count + 1` and `is_complete = child.is_terminal`.
    /// - Otherwise (or if `cursor` is already dead / out of range), returns a dead cursor
    ///   with `is_dead = true`, `is_complete = false`, `node_idx = GUIDED_TRIE_NO_NODE`.
    fn advance(cursor: GuidedTrieCursor, token_id: u32): GuidedTrieCursor {
        if (cursor.is_dead || (cursor.node_idx as usize) >= _.node_count) {
            return GuidedTrieCursor{
                node_idx: GUIDED_TRIE_NO_NODE,
                step_count: cursor.step_count,
                is_complete: false,
                is_dead: true,
            };
        }
        const next_idx: u16 = _.findChild(cursor.node_idx, token_id);
        if (next_idx == GUIDED_TRIE_NO_NODE) {
            return GuidedTrieCursor{
                node_idx: GUIDED_TRIE_NO_NODE,
                step_count: cursor.step_count,
                is_complete: false,
                is_dead: true,
            };
        }
        return GuidedTrieCursor{
            node_idx: next_idx,
            step_count: cursor.step_count + 1,
            is_complete: _.nodes[next_idx as usize].is_terminal,
            is_dead: false,
        };
    }

    /// Enumerates all valid next token IDs from `cursor` into caller-provided `out_tokens`.
    /// - If `cursor.is_complete` is `true`, automatically includes `eos_token_id` (deduplicated).
    /// - Returns the number of tokens written to `out_tokens` (capped by `out_tokens.len`).
    /// - Returns `0` for dead or invalid cursors.
    fn allowed_next_tokens(
        cursor: GuidedTrieCursor,
        eos_token_id: u32,
        out_tokens: *[]u32,
    ): usize {
        if (cursor.is_dead || (cursor.node_idx as usize) >= _.node_count) {
            return 0;
        }

        let written: usize = 0;
        let has_eos: bool = false;
        let child_idx: u16 = _.nodes[cursor.node_idx as usize].first_child;
        while (child_idx != GUIDED_TRIE_NO_NODE) {
            const c_usize: usize = child_idx as usize;
            if (c_usize >= _.node_count) break;
            const tid: u32 = _.nodes[c_usize].token_id;
            if (tid == eos_token_id) {
                has_eos = true;
            }
            if (written < out_tokens.len) {
                out_tokens[written] = tid;
                written += 1;
            }
            child_idx = _.nodes[c_usize].next_sibling;
        }

        if (cursor.is_complete && !has_eos) {
            if (written < out_tokens.len) {
                out_tokens[written] = eos_token_id;
                written += 1;
            }
        }
        return written;
    }

    /// Masks `logits` in place in O(V + B) time with zero dynamic heap allocation:
    /// - Sets all disallowed vocabulary entries to `-1.0e30`.
    /// - Preserves exact original logit values for every in-bounds valid next token
    ///   (and `eos_token_id` when `cursor.is_complete` is `true`).
    /// - Out-of-bounds token IDs (`token_id >= logits.len`) are safely ignored.
    /// - Dead or invalid cursors mask every entry in `logits` and return `0`.
    /// - Returns the count of valid in-bounds tokens preserved in `logits`.
    fn apply_logit_mask_f32(
        cursor: GuidedTrieCursor,
        eos_token_id: u32,
        logits: *[]f32,
    ): usize {
        const vocab_len: usize = logits.len;
        if (vocab_len == 0) return 0;

        if (cursor.is_dead || (cursor.node_idx as usize) >= _.node_count) {
            let i: usize = 0;
            while (i < vocab_len) : (i += 1) {
                logits[i] = GUIDED_MASK_LOGIT;
            }
            return 0;
        }

        // Gather in-bounds allowed token IDs and snapshot their original logits on stack.
        // At most (GUIDED_TRIE_MAX_NODES - 1) children + 1 EOS = GUIDED_TRIE_MAX_NODES entries.
        let saved_ids: [GUIDED_TRIE_MAX_NODES]usize = undefined;
        let saved_vals: [GUIDED_TRIE_MAX_NODES]f32 = undefined;
        let allowed_count: usize = 0;
        let has_eos: bool = false;

        let child_idx: u16 = _.nodes[cursor.node_idx as usize].first_child;
        while (child_idx != GUIDED_TRIE_NO_NODE) {
            const c_usize: usize = child_idx as usize;
            if (c_usize >= _.node_count) break;
            const tid: u32 = _.nodes[c_usize].token_id;
            if (tid == eos_token_id) {
                has_eos = true;
            }
            const tid_usize: usize = tid as usize;
            if (tid_usize < vocab_len && allowed_count < GUIDED_TRIE_MAX_NODES) {
                saved_ids[allowed_count] = tid_usize;
                saved_vals[allowed_count] = logits[tid_usize];
                allowed_count += 1;
            }
            child_idx = _.nodes[c_usize].next_sibling;
        }

        if (cursor.is_complete && !has_eos) {
            const eos_usize: usize = eos_token_id as usize;
            if (eos_usize < vocab_len && allowed_count < GUIDED_TRIE_MAX_NODES) {
                saved_ids[allowed_count] = eos_usize;
                saved_vals[allowed_count] = logits[eos_usize];
                allowed_count += 1;
            }
        }

        // Linear fill all vocabulary logits with mask sentinel.
        let i: usize = 0;
        while (i < vocab_len) : (i += 1) {
            logits[i] = GUIDED_MASK_LOGIT;
        }

        // Restore exact original logits for valid next tokens.
        let k: usize = 0;
        while (k < allowed_count) : (k += 1) {
            logits[saved_ids[k]] = saved_vals[k];
        }

        return allowed_count;
    }

    /// Returns a snapshot of current trie node count, registered sequences, and max depth.
    fn stats(): GuidedTrieStats {
        return GuidedTrieStats{
            node_count: _.node_count,
            sequence_count: _.sequence_count,
            max_depth: _.max_depth,
        };
    }
}
