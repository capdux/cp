// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc SGLang RadixAttention Prefix-Shared KV-Cache Tree) | Created: 2026-11-22
// Pure CP zero-heap-allocation SGLang-style RadixAttention Prefix-Shared KV-Cache Tree with Leaf-First LRU Eviction:
//   - Fixed inline 128-node radix tree (`RadixKvCacheTree`, node 0 = virtual root) with 0 dynamic heap allocation
//   - Compressed edge storage up to 16 tokens & 16 KV block IDs per node (`RADIX_KV_MAX_EDGE_TOKENS`)
//   - Longest prefix matching (`match_prefix`) with intra-edge partial match support and LRU timestamp refresh
//   - Automatic radix edge splitting (`insert_prefix`) on mid-edge forks with lock_ref & timestamp inheritance
//   - Ancestor-chain reference locking (`lock_path` / `unlock_path`) with atomic underflow protection
//   - Strict leaf-first LRU eviction (`evict_lru_tokens`) with parent cascading reclamation and slot reuse

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum total node slots (including virtual root at index 0) in `RadixKvCacheTree`.
pub const RADIX_KV_MAX_NODES: usize = 128;

/// Maximum tokens and KV block IDs compressed onto a single radix tree edge.
pub const RADIX_KV_MAX_EDGE_TOKENS: usize = 16;

/// Sentinel index representing no parent, no child, or no sibling node.
pub const RADIX_KV_NO_NODE: u16 = 0xFFFF;

/// Single compressed edge node in the Left-Child Right-Sibling (LCRS) Radix KV-Cache Tree.
pub struct RadixKvNode {
    tokens: [RADIX_KV_MAX_EDGE_TOKENS]u32 = [0; RADIX_KV_MAX_EDGE_TOKENS];
    kv_blocks: [RADIX_KV_MAX_EDGE_TOKENS]u32 = [0; RADIX_KV_MAX_EDGE_TOKENS];
    token_len: u8 = 0;
    parent: u16 = RADIX_KV_NO_NODE;
    first_child: u16 = RADIX_KV_NO_NODE;
    next_sibling: u16 = RADIX_KV_NO_NODE;
    lock_ref: u32 = 0;
    last_access_step: u64 = 0;
    occupied: bool = false;
}

/// Result returned by `RadixKvCacheTree.match_prefix`.
pub struct RadixMatchResult {
    matched_tokens: usize = 0;
    last_node_idx: u16 = 0;
    out_blocks_written: usize = 0;
}

/// Telemetry & capacity snapshot returned by `RadixKvCacheTree.stats`.
pub struct RadixKvStats {
    active_nodes: usize = 0;
    cached_tokens: usize = 0;
    locked_nodes: usize = 0;
    total_hits: u64 = 0;
    total_splits: u64 = 0;
    total_evicted_tokens: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc SGLang RadixAttention KV-Cache Tree
// ============================================================================

/// Zero-heap-allocation SGLang-style RadixAttention KV-Cache prefix tree.
/// Node `0` is the permanent virtual root (`occupied == true`, `token_len == 0`).
/// Slots `1 .. RADIX_KV_MAX_NODES - 1` store compressed token/KV-block edges.
pub struct RadixKvCacheTree {
    nodes: [RADIX_KV_MAX_NODES]RadixKvNode = [RadixKvNode{}; RADIX_KV_MAX_NODES];
    total_hits: u64 = 0;
    total_splits: u64 = 0;
    total_evicted_tokens: u64 = 0;

    /// Initializes an empty `RadixKvCacheTree` with virtual root at index `0`.
    static fn init(): RadixKvCacheTree {
        let tree = RadixKvCacheTree{
            nodes: [RadixKvNode{}; RADIX_KV_MAX_NODES],
            total_hits: 0,
            total_splits: 0,
            total_evicted_tokens: 0,
        };
        tree.nodes[0].occupied = true;
        tree.nodes[0].token_len = 0;
        tree.nodes[0].parent = RADIX_KV_NO_NODE;
        tree.nodes[0].first_child = RADIX_KV_NO_NODE;
        tree.nodes[0].next_sibling = RADIX_KV_NO_NODE;
        return tree;
    }

    /// Internal helper: counts unoccupied data slots in `1 .. RADIX_KV_MAX_NODES`.
    fn countFreeSlots(): usize {
        let free_count: usize = 0;
        let i: usize = 1;
        while (i < RADIX_KV_MAX_NODES) : (i += 1) {
            if (!_.nodes[i].occupied) {
                free_count += 1;
            }
        }
        return free_count;
    }

    /// Internal helper: allocates the lowest free data slot in `1 .. RADIX_KV_MAX_NODES`,
    /// or returns `RADIX_KV_NO_NODE` if full.
    fn allocSlot(*_): u16 {
        let i: usize = 1;
        while (i < RADIX_KV_MAX_NODES) : (i += 1) {
            if (!_.nodes[i].occupied) {
                _.nodes[i] = RadixKvNode{};
                _.nodes[i].occupied = true;
                return @intCast(i);
            }
        }
        return RADIX_KV_NO_NODE;
    }

    /// Internal helper: finds child of `parent_idx` whose edge starts with `first_token`.
    fn findChildByFirstToken(parent_idx: u16, first_token: u32): u16 {
        if ((parent_idx as usize) >= RADIX_KV_MAX_NODES) return RADIX_KV_NO_NODE;
        let child_idx: u16 = _.nodes[parent_idx as usize].first_child;
        let guard: usize = 0;
        while (child_idx != RADIX_KV_NO_NODE && guard < RADIX_KV_MAX_NODES) : (guard += 1) {
            const c_usize: usize = child_idx as usize;
            if (c_usize >= RADIX_KV_MAX_NODES || !_.nodes[c_usize].occupied) {
                return RADIX_KV_NO_NODE;
            }
            if (_.nodes[c_usize].token_len > 0 && _.nodes[c_usize].tokens[0] == first_token) {
                return child_idx;
            }
            child_idx = _.nodes[c_usize].next_sibling;
        }
        return RADIX_KV_NO_NODE;
    }

    /// Internal helper: appends `child_idx` to the tail of `parent_idx`'s child linked list.
    fn appendChild(*_, parent_idx: u16, child_idx: u16) {
        const p_usize: usize = parent_idx as usize;
        _.nodes[child_idx as usize].parent = parent_idx;
        _.nodes[child_idx as usize].next_sibling = RADIX_KV_NO_NODE;

        let cur: u16 = _.nodes[p_usize].first_child;
        if (cur == RADIX_KV_NO_NODE) {
            _.nodes[p_usize].first_child = child_idx;
            return;
        }
        let guard: usize = 0;
        while (guard < RADIX_KV_MAX_NODES) : (guard += 1) {
            const c_usize: usize = cur as usize;
            const next_sib: u16 = _.nodes[c_usize].next_sibling;
            if (next_sib == RADIX_KV_NO_NODE) {
                _.nodes[c_usize].next_sibling = child_idx;
                return;
            }
            cur = next_sib;
        }
    }

    /// Internal helper: unlinks `child_idx` from `parent_idx`'s child linked list.
    fn detachChild(*_, parent_idx: u16, child_idx: u16) {
        if (parent_idx == RADIX_KV_NO_NODE || (parent_idx as usize) >= RADIX_KV_MAX_NODES) return;
        const p_usize: usize = parent_idx as usize;
        let cur: u16 = _.nodes[p_usize].first_child;
        if (cur == RADIX_KV_NO_NODE) return;

        if (cur == child_idx) {
            _.nodes[p_usize].first_child = _.nodes[child_idx as usize].next_sibling;
            _.nodes[child_idx as usize].next_sibling = RADIX_KV_NO_NODE;
            return;
        }

        let guard: usize = 0;
        while (cur != RADIX_KV_NO_NODE && guard < RADIX_KV_MAX_NODES) : (guard += 1) {
            const c_usize: usize = cur as usize;
            const next_sib: u16 = _.nodes[c_usize].next_sibling;
            if (next_sib == child_idx) {
                _.nodes[c_usize].next_sibling = _.nodes[child_idx as usize].next_sibling;
                _.nodes[child_idx as usize].next_sibling = RADIX_KV_NO_NODE;
                return;
            }
            cur = next_sib;
        }
    }

    /// Internal helper: refreshes `last_access_step` along `node_idx` up to root (excluding root).
    fn touchPath(*_, node_idx: u16, now_step: u64) {
        let cur: u16 = node_idx;
        let guard: usize = 0;
        while (cur != RADIX_KV_NO_NODE && cur != 0 && guard < RADIX_KV_MAX_NODES) : (guard += 1) {
            const c_usize: usize = cur as usize;
            if (c_usize >= RADIX_KV_MAX_NODES || !_.nodes[c_usize].occupied) break;
            _.nodes[c_usize].last_access_step = now_step;
            cur = _.nodes[c_usize].parent;
        }
    }

    /// Matches the longest cached token prefix starting from the virtual root.
    /// - Supports both full-edge traversal and intra-edge partial prefix matching.
    /// - Refreshes `last_access_step = now_step` on every visited node along the matched path.
    /// - Writes matched KV block IDs into `out_kv_blocks` (capped by `out_kv_blocks.len`).
    /// - Increments `total_hits` when `matched_tokens > 0`.
    fn match_prefix(
        *_,
        query_tokens: []const u32,
        now_step: u64,
        out_kv_blocks: *[]u32,
    ): RadixMatchResult {
        if (query_tokens.len == 0) {
            return RadixMatchResult{
                matched_tokens: 0,
                last_node_idx: 0,
                out_blocks_written: 0,
            };
        }

        let cur_idx: u16 = 0;
        let matched: usize = 0;
        let written: usize = 0;

        while (matched < query_tokens.len) {
            const child_idx: u16 = _.findChildByFirstToken(cur_idx, query_tokens[matched]);
            if (child_idx == RADIX_KV_NO_NODE) break;

            const c_usize: usize = child_idx as usize;
            const edge_len: usize = _.nodes[c_usize].token_len as usize;
            const rem_query: usize = query_tokens.len - matched;
            const cmp_limit: usize = if (edge_len < rem_query) edge_len else rem_query;

            let common: usize = 0;
            while (common < cmp_limit) : (common += 1) {
                if (_.nodes[c_usize].tokens[common] != query_tokens[matched + common]) {
                    break;
                }
            }

            if (common == 0) break;

            _.nodes[c_usize].last_access_step = now_step;

            let k: usize = 0;
            while (k < common) : (k += 1) {
                if (written < out_kv_blocks.len) {
                    out_kv_blocks[written] = _.nodes[c_usize].kv_blocks[k];
                    written += 1;
                }
            }

            matched += common;
            cur_idx = child_idx;

            if (common < edge_len) {
                break;
            }
        }

        if (matched > 0) {
            _.total_hits += 1;
        }

        return RadixMatchResult{
            matched_tokens: matched,
            last_node_idx: cur_idx,
            out_blocks_written: written,
        };
    }

    /// Inserts a token sequence and its corresponding KV block IDs into the radix tree.
    /// - Rejects empty input or length mismatch (`tokens.len != kv_blocks.len`), returning `0`.
    /// - Shares existing common prefixes; idempotent on already-cached sequences (returns `0`).
    /// - Automatically splits a radix edge when diverging mid-edge (`total_splits += 1`),
    ///   inheriting `lock_ref` and `last_access_step` onto the split suffix node.
    /// - Atomically checks free node capacity before mutating; returns `0` with zero side effects on overflow.
    /// - Returns the number of newly inserted cached tokens.
    fn insert_prefix(
        *_,
        tokens: []const u32,
        kv_blocks: []const u32,
        now_step: u64,
    ): usize {
        if (tokens.len == 0 || tokens.len != kv_blocks.len) {
            return 0;
        }

        // Phase 1: walk longest common prefix and detect mid-edge divergence.
        let cur_idx: u16 = 0;
        let pos: usize = 0;
        let split_node_idx: u16 = RADIX_KV_NO_NODE;
        let split_offset: usize = 0;
        let touched_leaf: u16 = 0;

        while (pos < tokens.len) {
            const child_idx: u16 = _.findChildByFirstToken(cur_idx, tokens[pos]);
            if (child_idx == RADIX_KV_NO_NODE) break;

            const c_usize: usize = child_idx as usize;
            const edge_len: usize = _.nodes[c_usize].token_len as usize;
            const rem: usize = tokens.len - pos;
            const cmp_limit: usize = if (edge_len < rem) edge_len else rem;

            let common: usize = 0;
            while (common < cmp_limit) : (common += 1) {
                if (_.nodes[c_usize].tokens[common] != tokens[pos + common]) {
                    break;
                }
            }

            if (common < edge_len) {
                if (pos + common == tokens.len) {
                    // Input is an exact prefix of an already-cached edge: idempotent, no split required.
                    pos = tokens.len;
                    touched_leaf = child_idx;
                    break;
                }
                // Diverged mid-edge: schedule edge split at `common`.
                split_node_idx = child_idx;
                split_offset = common;
                pos += common;
                break;
            }

            // Full edge matched.
            cur_idx = child_idx;
            touched_leaf = child_idx;
            pos += edge_len;
        }

        // Already completely cached: refresh timestamps and return 0 newly cached tokens.
        if (pos >= tokens.len) {
            _.touchPath(touched_leaf, now_step);
            return 0;
        }

        // Phase 2: atomic capacity pre-flight guard.
        const rem_tokens: usize = tokens.len - pos;
        const branch_nodes: usize = (rem_tokens + RADIX_KV_MAX_EDGE_TOKENS - 1) / RADIX_KV_MAX_EDGE_TOKENS;
        const split_extra: usize = if (split_node_idx != RADIX_KV_NO_NODE) 1 else 0;
        const needed_nodes: usize = branch_nodes + split_extra;
        if (_.countFreeSlots() < needed_nodes) {
            return 0;
        }

        // Phase 3: refresh LRU timestamps on shared ancestor path.
        _.touchPath(cur_idx, now_step);

        // Phase 4: perform radix edge split if diverging mid-edge.
        if (split_node_idx != RADIX_KV_NO_NODE) {
            const p_usize: usize = split_node_idx as usize;
            const old_len: usize = _.nodes[p_usize].token_len as usize;
            const suffix_len: usize = old_len - split_offset;
            const old_first_child: u16 = _.nodes[p_usize].first_child;
            const old_lock_ref: u32 = _.nodes[p_usize].lock_ref;
            const old_access_step: u64 = _.nodes[p_usize].last_access_step;

            const suffix_idx: u16 = _.allocSlot();
            const s_usize: usize = suffix_idx as usize;

            let k: usize = 0;
            while (k < suffix_len) : (k += 1) {
                _.nodes[s_usize].tokens[k] = _.nodes[p_usize].tokens[split_offset + k];
                _.nodes[s_usize].kv_blocks[k] = _.nodes[p_usize].kv_blocks[split_offset + k];
                _.nodes[p_usize].tokens[split_offset + k] = 0;
                _.nodes[p_usize].kv_blocks[split_offset + k] = 0;
            }

            _.nodes[s_usize].token_len = @intCast(suffix_len);
            _.nodes[s_usize].parent = split_node_idx;
            _.nodes[s_usize].first_child = old_first_child;
            _.nodes[s_usize].next_sibling = RADIX_KV_NO_NODE;
            _.nodes[s_usize].lock_ref = old_lock_ref;
            _.nodes[s_usize].last_access_step = old_access_step;

            // Re-parent all former children of `split_node_idx` to `suffix_idx`.
            let reparent_cur: u16 = old_first_child;
            let guard: usize = 0;
            while (reparent_cur != RADIX_KV_NO_NODE && guard < RADIX_KV_MAX_NODES) : (guard += 1) {
                const r_usize: usize = reparent_cur as usize;
                if (r_usize >= RADIX_KV_MAX_NODES || !_.nodes[r_usize].occupied) break;
                _.nodes[r_usize].parent = suffix_idx;
                reparent_cur = _.nodes[r_usize].next_sibling;
            }

            // Truncate `split_node_idx` to the shared prefix and attach `suffix_idx` as its sole child.
            _.nodes[p_usize].token_len = @intCast(split_offset);
            _.nodes[p_usize].first_child = suffix_idx;
            _.nodes[p_usize].last_access_step = now_step;

            _.total_splits += 1;
            cur_idx = split_node_idx;
        }

        // Phase 5: chunk remaining suffix into nodes of at most `RADIX_KV_MAX_EDGE_TOKENS`.
        let inserted: usize = 0;
        let attach_parent: u16 = cur_idx;
        while (pos < tokens.len) {
            const left: usize = tokens.len - pos;
            const chunk_len: usize = if (left > RADIX_KV_MAX_EDGE_TOKENS) RADIX_KV_MAX_EDGE_TOKENS else left;

            const new_idx: u16 = _.allocSlot();
            const n_usize: usize = new_idx as usize;
            let k: usize = 0;
            while (k < chunk_len) : (k += 1) {
                _.nodes[n_usize].tokens[k] = tokens[pos + k];
                _.nodes[n_usize].kv_blocks[k] = kv_blocks[pos + k];
            }
            _.nodes[n_usize].token_len = @intCast(chunk_len);
            _.nodes[n_usize].lock_ref = 0;
            _.nodes[n_usize].last_access_step = now_step;

            _.appendChild(attach_parent, new_idx);
            attach_parent = new_idx;
            pos += chunk_len;
            inserted += chunk_len;
        }

        return inserted;
    }

    /// Increments `lock_ref` on every node along the path from `leaf_node_idx` up to (excluding) root.
    /// Protects an in-flight inference request's KV prefix from LRU eviction.
    /// Returns `false` if `leaf_node_idx` is invalid, unoccupied, or the virtual root `0`.
    fn lock_path(*_, leaf_node_idx: u16): bool {
        if (leaf_node_idx == 0 || leaf_node_idx == RADIX_KV_NO_NODE) return false;
        if ((leaf_node_idx as usize) >= RADIX_KV_MAX_NODES) return false;
        if (!_.nodes[leaf_node_idx as usize].occupied) return false;

        let cur: u16 = leaf_node_idx;
        let guard: usize = 0;
        while (cur != RADIX_KV_NO_NODE && cur != 0 && guard < RADIX_KV_MAX_NODES) : (guard += 1) {
            const c_usize: usize = cur as usize;
            if (c_usize >= RADIX_KV_MAX_NODES || !_.nodes[c_usize].occupied) return false;
            if (_.nodes[c_usize].lock_ref < 0xFFFFFFFF) {
                _.nodes[c_usize].lock_ref += 1;
            }
            cur = _.nodes[c_usize].parent;
        }
        return true;
    }

    /// Atomically decrements `lock_ref` along the path from `leaf_node_idx` up to (excluding) root.
    /// Prevents underflow: if any node along the path currently has `lock_ref == 0`, returns `false`
    /// without mutating any node.
    fn unlock_path(*_, leaf_node_idx: u16): bool {
        if (leaf_node_idx == 0 || leaf_node_idx == RADIX_KV_NO_NODE) return false;
        if ((leaf_node_idx as usize) >= RADIX_KV_MAX_NODES) return false;
        if (!_.nodes[leaf_node_idx as usize].occupied) return false;

        // Pass 1: verify every node on the path has lock_ref > 0.
        let check_cur: u16 = leaf_node_idx;
        let guard1: usize = 0;
        while (check_cur != RADIX_KV_NO_NODE && check_cur != 0 && guard1 < RADIX_KV_MAX_NODES) : (guard1 += 1) {
            const c_usize: usize = check_cur as usize;
            if (c_usize >= RADIX_KV_MAX_NODES || !_.nodes[c_usize].occupied) return false;
            if (_.nodes[c_usize].lock_ref == 0) return false;
            check_cur = _.nodes[c_usize].parent;
        }

        // Pass 2: decrement lock_ref along the verified path.
        let cur: u16 = leaf_node_idx;
        let guard2: usize = 0;
        while (cur != RADIX_KV_NO_NODE && cur != 0 && guard2 < RADIX_KV_MAX_NODES) : (guard2 += 1) {
            const c_usize: usize = cur as usize;
            _.nodes[c_usize].lock_ref -= 1;
            cur = _.nodes[c_usize].parent;
        }
        return true;
    }

    /// Strict leaf-first LRU eviction: repeatedly evicts the coldest unlocked leaf node
    /// (`first_child == RADIX_KV_NO_NODE && lock_ref == 0`, lowest `last_access_step`)
    /// until at least `needed_tokens` KV blocks are freed or no evictable leaves remain.
    /// Removing a leaf naturally exposes its parent for subsequent rounds if the parent
    /// becomes childless and unlocked.
    /// Writes freed KV block IDs into `out_freed_blocks` (capped by `out_freed_blocks.len`)
    /// and returns the total number of KV blocks freed.
    fn evict_lru_tokens(*_, needed_tokens: usize, out_freed_blocks: *[]u32): usize {
        if (needed_tokens == 0) return 0;

        let total_freed: usize = 0;
        let out_written: usize = 0;

        while (total_freed < needed_tokens) {
            let best_idx: u16 = RADIX_KV_NO_NODE;
            let best_step: u64 = 0xFFFFFFFFFFFFFFFF;

            let i: usize = 1;
            while (i < RADIX_KV_MAX_NODES) : (i += 1) {
                if (_.nodes[i].occupied &&
                    _.nodes[i].first_child == RADIX_KV_NO_NODE &&
                    _.nodes[i].lock_ref == 0)
                {
                    if (best_idx == RADIX_KV_NO_NODE || _.nodes[i].last_access_step < best_step) {
                        best_idx = @intCast(i);
                        best_step = _.nodes[i].last_access_step;
                    }
                }
            }

            if (best_idx == RADIX_KV_NO_NODE) {
                break;
            }

            const v_usize: usize = best_idx as usize;
            const parent_idx: u16 = _.nodes[v_usize].parent;
            const v_len: usize = _.nodes[v_usize].token_len as usize;

            _.detachChild(parent_idx, best_idx);

            let k: usize = 0;
            while (k < v_len) : (k += 1) {
                if (out_written < out_freed_blocks.len) {
                    out_freed_blocks[out_written] = _.nodes[v_usize].kv_blocks[k];
                    out_written += 1;
                }
            }

            total_freed += v_len;
            _.total_evicted_tokens += v_len as u64;

            // Release slot back to the inline pool for immediate reuse.
            _.nodes[v_usize] = RadixKvNode{};
        }

        return total_freed;
    }

    /// Returns a telemetry & capacity snapshot of the radix KV-cache tree.
    fn stats(): RadixKvStats {
        let active: usize = 0;
        let cached: usize = 0;
        let locked: usize = 0;

        let i: usize = 1;
        while (i < RADIX_KV_MAX_NODES) : (i += 1) {
            if (_.nodes[i].occupied) {
                active += 1;
                cached += _.nodes[i].token_len as usize;
                if (_.nodes[i].lock_ref > 0) {
                    locked += 1;
                }
            }
        }

        return RadixKvStats{
            active_nodes: active,
            cached_tokens: cached,
            locked_nodes: locked,
            total_hits: _.total_hits,
            total_splits: _.total_splits,
            total_evicted_tokens: _.total_evicted_tokens,
        };
    }
}
