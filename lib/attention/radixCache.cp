// Origin: Pure-CP Stdlib (SGLang RadixAttention Zero-Alloc Prefix KV-Cache Radix Tree & LRU Eviction Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation SGLang RadixAttention style prefix KV-cache compressed radix tree:
//   - Fixed-capacity compressed radix node pool (`PRC_MAX_NODES = 128`) + contiguous Token/KV-Page
//     arena pools (`PRC_MAX_POOL_SLOTS = 2048`) with automatic zero-heap in-place pool compaction
//   - Pure iterative (zero recursion) longest-prefix matching (`matchPrefix`) with partial-edge hit support
//   - Prefix insertion (`insertPrefix`) with O(1) offset-based edge splitting (zero data copy on split)
//   - Ancestor-chain reference pinning (`pinNode` / `unpinNode`) protecting in-flight requests from eviction
//   - Bottom-up LRU leaf eviction (`evictPages`) reclaiming unpinned cold leaves & cascading empty parents

/// Maximum radix tree nodes (index 0 is reserved for the virtual root).
pub const PRC_MAX_NODES: usize = 128;

/// Total contiguous token & KV-page slot capacity across all cached edges (zero dynamic heap).
pub const PRC_MAX_POOL_SLOTS: usize = 2048;

/// Sentinel invalid node identifier.
pub const PRC_NO_NODE: u16 = 0xFFFF;

/// Fixed virtual root node identifier.
pub const PRC_ROOT_NODE: u16 = 0;

// ============================================================================
// 1. Public Models
// ============================================================================

/// Compressed radix edge node storing contiguous arena offsets, pinning count, and LRU tick.
pub struct PrefixRadixNode {
    node_id: u16 = 0;
    parent_idx: u16 = PRC_NO_NODE;
    first_child_idx: u16 = PRC_NO_NODE;
    next_sibling_idx: u16 = PRC_NO_NODE;
    child_count: u16 = 0;
    token_offset: u32 = 0;
    token_len: u32 = 0;
    ref_count: u32 = 0;
    last_access_tick: u64 = 0;
    active: bool = false;
}

/// Outcome of `PrefixRadixCache.matchPrefix`.
pub struct PrefixRadixMatch {
    matched_tokens: usize = 0;
    matched_pages: usize = 0;
    leaf_node_id: u16 = PRC_ROOT_NODE;
    /// Non-zero when the match ended midway along `leaf_node_id`'s compressed edge.
    split_offset: u32 = 0;
}

/// Outcome of `PrefixRadixCache.insertPrefix`.
pub struct PrefixRadixInsertResult {
    ok: bool = false;
    leaf_node_id: u16 = PRC_NO_NODE;
    reused_tokens: usize = 0;
    inserted_tokens: usize = 0;
    split_performed: bool = false;
}

/// Outcome of `PrefixRadixCache.evictPages`.
pub struct PrefixRadixEvictReport {
    freed_pages: usize = 0;
    evicted_nodes: usize = 0;
    remaining_unpinned_pages: usize = 0;
}

/// Aggregate capacity, cache utilization, and hit/eviction telemetry for `PrefixRadixCache`.
pub struct PrefixRadixStats {
    active_nodes: usize = 1;
    max_nodes: usize = PRC_MAX_NODES;
    used_pool_slots: usize = 0;
    pool_capacity: usize = PRC_MAX_POOL_SLOTS;
    cached_tokens: usize = 0;
    pinned_nodes: usize = 0;
    total_matches: u64 = 0;
    total_matched_tokens: u64 = 0;
    total_evicted_pages: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc RadixAttention Prefix KV-Cache Engine
// ============================================================================

/// Zero-heap-allocation SGLang RadixAttention prefix KV-cache compressed radix tree.
pub struct PrefixRadixCache {
    nodes: [PRC_MAX_NODES]PrefixRadixNode = [PrefixRadixNode{}; PRC_MAX_NODES];
    token_pool: [PRC_MAX_POOL_SLOTS]u32 = undefined;
    kv_page_pool: [PRC_MAX_POOL_SLOTS]u32 = undefined;
    pool_used: usize = 0;
    active_node_count: usize = 1;
    cached_token_count: usize = 0;
    total_matches: u64 = 0;
    total_matched_tokens: u64 = 0;
    total_evicted_pages: u64 = 0;

    /// Initializes an empty `PrefixRadixCache` with virtual root node `0`.
    static fn init(): PrefixRadixCache {
        let cache = PrefixRadixCache{
            nodes: [PrefixRadixNode{}; PRC_MAX_NODES],
            token_pool: undefined,
            kv_page_pool: undefined,
            pool_used: 0,
            active_node_count: 1,
            cached_token_count: 0,
            total_matches: 0,
            total_matched_tokens: 0,
            total_evicted_pages: 0,
        };
        cache.nodes[PRC_ROOT_NODE as usize] = PrefixRadixNode{
            node_id: PRC_ROOT_NODE,
            parent_idx: PRC_NO_NODE,
            first_child_idx: PRC_NO_NODE,
            next_sibling_idx: PRC_NO_NODE,
            child_count: 0,
            token_offset: 0,
            token_len: 0,
            ref_count: 0,
            last_access_tick: 0,
            active: true,
        };
        return cache;
    }

    /// Returns metadata for `node_id`, or `null` if out of bounds or inactive.
    fn getNode(node_id: u16): PrefixRadixNode? {
        const idx: usize = node_id as usize;
        if (idx >= PRC_MAX_NODES || !_.nodes[idx].active) return null;
        return _.nodes[idx];
    }

    fn allocNodeSlot(*_): u16? {
        let i: usize = 1; // slot 0 is permanent root
        while (i < PRC_MAX_NODES) : (i += 1) {
            if (!_.nodes[i].active) return i as u16;
        }
        return null;
    }

    fn attachChild(*_, parent_id: u16, child_id: u16) {
        const p_idx: usize = parent_id as usize;
        const c_idx: usize = child_id as usize;
        _.nodes[c_idx].parent_idx = parent_id;
        _.nodes[c_idx].next_sibling_idx = _.nodes[p_idx].first_child_idx;
        _.nodes[p_idx].first_child_idx = child_id;
        _.nodes[p_idx].child_count += 1;
    }

    fn detachChild(*_, parent_id: u16, child_id: u16) {
        const p_idx: usize = parent_id as usize;
        let prev: u16 = PRC_NO_NODE;
        let cur: u16 = _.nodes[p_idx].first_child_idx;
        let guard: usize = 0;
        while (cur != PRC_NO_NODE && guard < PRC_MAX_NODES) : (guard += 1) {
            const next_sib: u16 = _.nodes[cur as usize].next_sibling_idx;
            if (cur == child_id) {
                if (prev == PRC_NO_NODE) {
                    _.nodes[p_idx].first_child_idx = next_sib;
                } else {
                    _.nodes[prev as usize].next_sibling_idx = next_sib;
                }
                if (_.nodes[p_idx].child_count > 0) {
                    _.nodes[p_idx].child_count -= 1;
                }
                _.nodes[child_id as usize].parent_idx = PRC_NO_NODE;
                _.nodes[child_id as usize].next_sibling_idx = PRC_NO_NODE;
                return;
            }
            prev = cur;
            cur = next_sib;
        }
    }

    fn replaceChildLink(*_, parent_id: u16, old_child: u16, new_child: u16) {
        const p_idx: usize = parent_id as usize;
        let prev: u16 = PRC_NO_NODE;
        let cur: u16 = _.nodes[p_idx].first_child_idx;
        let guard: usize = 0;
        while (cur != PRC_NO_NODE && guard < PRC_MAX_NODES) : (guard += 1) {
            if (cur == old_child) {
                const next_sib: u16 = _.nodes[old_child as usize].next_sibling_idx;
                _.nodes[new_child as usize].parent_idx = parent_id;
                _.nodes[new_child as usize].next_sibling_idx = next_sib;
                if (prev == PRC_NO_NODE) {
                    _.nodes[p_idx].first_child_idx = new_child;
                } else {
                    _.nodes[prev as usize].next_sibling_idx = new_child;
                }
                _.nodes[old_child as usize].parent_idx = PRC_NO_NODE;
                _.nodes[old_child as usize].next_sibling_idx = PRC_NO_NODE;
                return;
            }
            prev = cur;
            cur = _.nodes[cur as usize].next_sibling_idx;
        }
    }

    fn findChildByFirstToken(parent_id: u16, first_token: u32): u16 {
        let cur: u16 = _.nodes[parent_id as usize].first_child_idx;
        let guard: usize = 0;
        while (cur != PRC_NO_NODE && guard < PRC_MAX_NODES) : (guard += 1) {
            const node = _.nodes[cur as usize];
            if (node.active && node.token_len > 0 && _.token_pool[node.token_offset as usize] == first_token) {
                return cur;
            }
            cur = node.next_sibling_idx;
        }
        return PRC_NO_NODE;
    }

    /// Zero-heap in-place compaction of `token_pool` and `kv_page_pool` reclaiming gaps from evicted leaves.
    fn compactPools(*_) {
        let new_tokens: [PRC_MAX_POOL_SLOTS]u32 = undefined;
        let new_pages: [PRC_MAX_POOL_SLOTS]u32 = undefined;
        let cursor: usize = 0;

        let i: usize = 1;
        while (i < PRC_MAX_NODES) : (i += 1) {
            if (!_.nodes[i].active || _.nodes[i].token_len == 0) continue;
            const old_off: usize = _.nodes[i].token_offset as usize;
            const len: usize = _.nodes[i].token_len as usize;
            if (len > 0) {
                @memcpy(new_tokens[cursor..cursor + len], _.token_pool[old_off..old_off + len]);
                @memcpy(new_pages[cursor..cursor + len], _.kv_page_pool[old_off..old_off + len]);
            }
            _.nodes[i].token_offset = cursor as u32;
            cursor += len;
        }

        if (cursor > 0) {
            @memcpy(_.token_pool[0..cursor], new_tokens[0..cursor]);
            @memcpy(_.kv_page_pool[0..cursor], new_pages[0..cursor]);
        }
        _.pool_used = cursor;
    }

    fn allocPoolSpan(*_, tokens: []const u32, kv_pages: []const u32): u32? {
        const need: usize = tokens.len;
        if (need == 0) return 0;
        if (_.pool_used + need > PRC_MAX_POOL_SLOTS) {
            _.compactPools();
            if (_.pool_used + need > PRC_MAX_POOL_SLOTS) return null;
        }
        const offset: usize = _.pool_used;
        @memcpy(_.token_pool[offset..offset + need], tokens);
        @memcpy(_.kv_page_pool[offset..offset + need], kv_pages);
        _.pool_used += need;
        return offset as u32;
    }

    /// Pure iterative longest-prefix match against cached token sequences.
    /// Copies matched KV page IDs into `out_matched_pages` and refreshes `last_access_tick` along the matched path.
    /// Supports partial-edge hits (`split_offset > 0`) without mutating tree topology.
    fn matchPrefix(
        *_,
        tokens: []const u32,
        now_tick: u64,
        out_matched_pages: *[]u32,
    ): PrefixRadixMatch {
        let res = PrefixRadixMatch{
            matched_tokens: 0,
            matched_pages: 0,
            leaf_node_id: PRC_ROOT_NODE,
            split_offset: 0,
        };
        _.total_matches += 1;
        if (tokens.len == 0) return res;

        let cur: u16 = PRC_ROOT_NODE;
        let pos: usize = 0;

        let depth_guard: usize = 0;
        while (pos < tokens.len && depth_guard < PRC_MAX_NODES) : (depth_guard += 1) {
            const child: u16 = _.findChildByFirstToken(cur, tokens[pos]);
            if (child == PRC_NO_NODE) break;

            const c_idx: usize = child as usize;
            const edge_off: usize = _.nodes[c_idx].token_offset as usize;
            const edge_len: usize = _.nodes[c_idx].token_len as usize;
            const remain: usize = tokens.len - pos;
            const max_cmp: usize = if (edge_len < remain) edge_len else remain;

            let lcp: usize = 0;
            while (lcp < max_cmp && _.token_pool[edge_off + lcp] == tokens[pos + lcp]) : (lcp += 1) {}

            if (lcp == 0) break;

            // Refresh LRU tick on matched node
            if (now_tick > _.nodes[c_idx].last_access_tick) {
                _.nodes[c_idx].last_access_tick = now_tick;
            }

            // Copy matched KV pages into caller's output slice
            let k: usize = 0;
            while (k < lcp && res.matched_pages < out_matched_pages.len) : (k += 1) {
                out_matched_pages[res.matched_pages] = _.kv_page_pool[edge_off + k];
                res.matched_pages += 1;
            }

            pos += lcp;
            res.matched_tokens = pos;
            res.leaf_node_id = child;

            if (lcp < edge_len) {
                // Partial match midway through compressed edge
                res.split_offset = lcp as u32;
                break;
            }

            // Full edge matched; descend further
            res.split_offset = 0;
            cur = child;
        }

        _.total_matched_tokens += res.matched_tokens as u64;
        return res;
    }

    /// Inserts a token sequence and its corresponding KV page IDs into the compressed radix tree,
    /// performing O(1) offset-based edge splitting when diverging midway along an existing edge.
    fn insertPrefix(
        *_,
        tokens: []const u32,
        kv_pages: []const u32,
        now_tick: u64,
    ): PrefixRadixInsertResult {
        let fail = PrefixRadixInsertResult{};
        if (tokens.len == 0 || tokens.len != kv_pages.len) return fail;

        let cur: u16 = PRC_ROOT_NODE;
        let pos: usize = 0;
        let split_done: bool = false;

        let depth_guard: usize = 0;
        while (pos < tokens.len && depth_guard < PRC_MAX_NODES) : (depth_guard += 1) {
            const child: u16 = _.findChildByFirstToken(cur, tokens[pos]);
            if (child == PRC_NO_NODE) {
                // Append new leaf edge for remaining tokens[pos..]
                let new_id: u16 = PRC_NO_NODE;
                if (nid := _.allocNodeSlot()) {
                    new_id = nid;
                } else {
                    return fail;
                }

                let span_off: u32 = 0;
                if (off := _.allocPoolSpan(tokens[pos..], kv_pages[pos..])) {
                    span_off = off;
                } else {
                    return fail;
                }

                const add_len: usize = tokens.len - pos;
                _.nodes[new_id as usize] = PrefixRadixNode{
                    node_id: new_id,
                    parent_idx: PRC_NO_NODE,
                    first_child_idx: PRC_NO_NODE,
                    next_sibling_idx: PRC_NO_NODE,
                    child_count: 0,
                    token_offset: span_off,
                    token_len: add_len as u32,
                    ref_count: 0,
                    last_access_tick: now_tick,
                    active: true,
                };
                _.attachChild(cur, new_id);
                _.active_node_count += 1;
                _.cached_token_count += add_len;

                return PrefixRadixInsertResult{
                    ok: true,
                    leaf_node_id: new_id,
                    reused_tokens: pos,
                    inserted_tokens: add_len,
                    split_performed: split_done,
                };
            }

            const c_idx: usize = child as usize;
            const edge_off: usize = _.nodes[c_idx].token_offset as usize;
            const edge_len: usize = _.nodes[c_idx].token_len as usize;
            const remain: usize = tokens.len - pos;
            const max_cmp: usize = if (edge_len < remain) edge_len else remain;

            let lcp: usize = 0;
            while (lcp < max_cmp && _.token_pool[edge_off + lcp] == tokens[pos + lcp]) : (lcp += 1) {}

            if (lcp == edge_len) {
                // Entire edge matches; refresh tick and descend
                if (now_tick > _.nodes[c_idx].last_access_tick) {
                    _.nodes[c_idx].last_access_tick = now_tick;
                }
                pos += lcp;
                cur = child;
                continue;
            }

            // Partial edge match (0 < lcp < edge_len): perform O(1) offset-based edge split!
            const tail_after_split: usize = tokens.len - (pos + lcp);
            // Pre-check node capacity (need 1 node for mid, plus 1 more if inserting a divergent suffix)
            const nodes_needed: usize = if (tail_after_split > 0) 2 else 1;
            if (_.active_node_count + nodes_needed > PRC_MAX_NODES) return fail;

            // Pre-check pool space if suffix must be stored (before mutating tree!)
            let suffix_off: u32 = 0;
            if (tail_after_split > 0) {
                if (off := _.allocPoolSpan(tokens[pos + lcp ..], kv_pages[pos + lcp ..])) {
                    suffix_off = off;
                } else {
                    return fail;
                }
            }

            // Re-read child's token_offset after potential pool compaction!
            const cur_child_off: u32 = _.nodes[c_idx].token_offset;
            const mid_id: u16 = _.allocNodeSlot() ?? return fail;
            const child_tick: u64 = _.nodes[c_idx].last_access_tick;
            const mid_tick: u64 = if (now_tick > child_tick) now_tick else child_tick;

            _.nodes[mid_id as usize] = PrefixRadixNode{
                node_id: mid_id,
                parent_idx: PRC_NO_NODE,
                first_child_idx: PRC_NO_NODE,
                next_sibling_idx: PRC_NO_NODE,
                child_count: 0,
                token_offset: cur_child_off,
                token_len: lcp as u32,
                ref_count: _.nodes[c_idx].ref_count,
                last_access_tick: mid_tick,
                active: true,
            };
            _.active_node_count += 1;

            // Splice `mid_id` in place of `child` under `cur`, then shrink `child`'s offset/len and attach under `mid_id`.
            _.replaceChildLink(cur, child, mid_id);
            _.nodes[c_idx].token_offset = cur_child_off + (lcp as u32);
            _.nodes[c_idx].token_len = (edge_len - lcp) as u32;
            _.attachChild(mid_id, child);
            split_done = true;

            if (tail_after_split == 0) {
                // Inserted sequence ended exactly at the split boundary `mid_id`.
                return PrefixRadixInsertResult{
                    ok: true,
                    leaf_node_id: mid_id,
                    reused_tokens: pos + lcp,
                    inserted_tokens: 0,
                    split_performed: true,
                };
            }

            const leaf_id: u16 = _.allocNodeSlot() ?? return fail;
            _.nodes[leaf_id as usize] = PrefixRadixNode{
                node_id: leaf_id,
                parent_idx: PRC_NO_NODE,
                first_child_idx: PRC_NO_NODE,
                next_sibling_idx: PRC_NO_NODE,
                child_count: 0,
                token_offset: suffix_off,
                token_len: tail_after_split as u32,
                ref_count: 0,
                last_access_tick: now_tick,
                active: true,
            };
            _.attachChild(mid_id, leaf_id);
            _.active_node_count += 1;
            _.cached_token_count += tail_after_split;

            return PrefixRadixInsertResult{
                ok: true,
                leaf_node_id: leaf_id,
                reused_tokens: pos + lcp,
                inserted_tokens: tail_after_split,
                split_performed: true,
            };
        }

        // Exact prefix already existed completely in tree
        return PrefixRadixInsertResult{
            ok: true,
            leaf_node_id: cur,
            reused_tokens: pos,
            inserted_tokens: 0,
            split_performed: split_done,
        };
    }

    /// Pins `node_id` and every ancestor up to (excluding) the virtual root, preventing LRU eviction.
    fn pinNode(*_, node_id: u16): bool {
        const idx: usize = node_id as usize;
        if (idx == (PRC_ROOT_NODE as usize) || idx >= PRC_MAX_NODES || !_.nodes[idx].active) {
            return false;
        }

        let cur: u16 = node_id;
        let guard: usize = 0;
        while (cur != PRC_NO_NODE && cur != PRC_ROOT_NODE && guard < PRC_MAX_NODES) : (guard += 1) {
            const c_idx: usize = cur as usize;
            if (!_.nodes[c_idx].active) return false;
            _.nodes[c_idx].ref_count += 1;
            cur = _.nodes[c_idx].parent_idx;
        }
        return true;
    }

    /// Unpins `node_id` and its ancestors up to the virtual root (atomic pre-check prevents underflow).
    fn unpinNode(*_, node_id: u16): bool {
        const idx: usize = node_id as usize;
        if (idx == (PRC_ROOT_NODE as usize) || idx >= PRC_MAX_NODES || !_.nodes[idx].active) {
            return false;
        }

        // Pre-validate entire ancestor chain has ref_count > 0
        let chk: u16 = node_id;
        let g1: usize = 0;
        while (chk != PRC_NO_NODE && chk != PRC_ROOT_NODE && g1 < PRC_MAX_NODES) : (g1 += 1) {
            const c_idx: usize = chk as usize;
            if (!_.nodes[c_idx].active || _.nodes[c_idx].ref_count == 0) return false;
            chk = _.nodes[c_idx].parent_idx;
        }

        let cur: u16 = node_id;
        let g2: usize = 0;
        while (cur != PRC_NO_NODE && cur != PRC_ROOT_NODE && g2 < PRC_MAX_NODES) : (g2 += 1) {
            const c_idx: usize = cur as usize;
            _.nodes[c_idx].ref_count -= 1;
            cur = _.nodes[c_idx].parent_idx;
        }
        return true;
    }

    fn countUnpinnedPages(): usize {
        let sum: usize = 0;
        let i: usize = 1;
        while (i < PRC_MAX_NODES) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].ref_count == 0) {
                sum += _.nodes[i].token_len as usize;
            }
        }
        return sum;
    }

    /// Bottom-up LRU leaf eviction: iteratively evicts the least-recently-used unpinned (`ref_count == 0`)
    /// leaf nodes until at least `needed_pages` KV pages are freed or no evictable leaves remain.
    /// Automatically cascades reclamation of empty unpinned parent nodes.
    fn evictPages(
        *_,
        needed_pages: usize,
        out_freed_pages: *[]u32,
    ): PrefixRadixEvictReport {
        let report = PrefixRadixEvictReport{};
        if (needed_pages == 0) {
            report.remaining_unpinned_pages = _.countUnpinnedPages();
            return report;
        }

        let out_pos: usize = 0;
        let iter_guard: usize = 0;
        while (report.freed_pages < needed_pages && iter_guard < PRC_MAX_NODES) : (iter_guard += 1) {
            // Find cold unpinned leaf (child_count == 0 && ref_count == 0) with smallest last_access_tick
            let victim: u16 = PRC_NO_NODE;
            let best_tick: u64 = 0xFFFFFFFFFFFFFFFF;

            let i: usize = 1;
            while (i < PRC_MAX_NODES) : (i += 1) {
                const n = _.nodes[i];
                if (!n.active || n.child_count != 0 || n.ref_count != 0) continue;
                if (victim == PRC_NO_NODE || n.last_access_tick < best_tick ||
                    (n.last_access_tick == best_tick && (i as u16) < victim)) {
                    victim = i as u16;
                    best_tick = n.last_access_tick;
                }
            }
            if (victim == PRC_NO_NODE) break;

            // Collect freed KV page IDs & detach victim
            let cur_evict: u16 = victim;
            let cascade_guard: usize = 0;
            while (cur_evict != PRC_NO_NODE && cur_evict != PRC_ROOT_NODE && cascade_guard < PRC_MAX_NODES) : (cascade_guard += 1) {
                const v_idx: usize = cur_evict as usize;
                const parent_id: u16 = _.nodes[v_idx].parent_idx;
                const off: usize = _.nodes[v_idx].token_offset as usize;
                const len: usize = _.nodes[v_idx].token_len as usize;

                let k: usize = 0;
                while (k < len) : (k += 1) {
                    if (out_pos < out_freed_pages.len) {
                        out_freed_pages[out_pos] = _.kv_page_pool[off + k];
                        out_pos += 1;
                    }
                }

                if (parent_id != PRC_NO_NODE) {
                    _.detachChild(parent_id, cur_evict);
                }
                _.nodes[v_idx] = PrefixRadixNode{};
                if (_.active_node_count > 1) {
                    _.active_node_count -= 1;
                }
                if (len <= _.cached_token_count) {
                    _.cached_token_count -= len;
                } else {
                    _.cached_token_count = 0;
                }
                report.freed_pages += len;
                report.evicted_nodes += 1;
                _.total_evicted_pages += len as u64;

                // Cascade reclaim empty parent node if it has no remaining children, zero tokens, and ref_count == 0
                if (parent_id != PRC_NO_NODE && parent_id != PRC_ROOT_NODE) {
                    const p_node = _.nodes[parent_id as usize];
                    if (p_node.active && p_node.child_count == 0 && p_node.token_len == 0 && p_node.ref_count == 0) {
                        cur_evict = parent_id;
                        continue;
                    }
                }
                break;
            }
        }

        report.remaining_unpinned_pages = _.countUnpinnedPages();
        return report;
    }

    /// Returns aggregate cache utilization, pinning, match, and eviction statistics.
    fn stats(): PrefixRadixStats {
        let pinned: usize = 0;
        let i: usize = 1;
        while (i < PRC_MAX_NODES) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].ref_count > 0) {
                pinned += 1;
            }
        }
        return PrefixRadixStats{
            active_nodes: _.active_node_count,
            max_nodes: PRC_MAX_NODES,
            used_pool_slots: _.pool_used,
            pool_capacity: PRC_MAX_POOL_SLOTS,
            cached_tokens: _.cached_token_count,
            pinned_nodes: pinned,
            total_matches: _.total_matches,
            total_matched_tokens: _.total_matched_tokens,
            total_evicted_pages: _.total_evicted_pages,
        };
    }
}
