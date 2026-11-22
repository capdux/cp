// Origin: Pure-CP Stdlib (Zero-Alloc Path-Compressed Byte Prefix Trie / Patricia-Radix KV) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation path-compressed byte prefix trie:
//   - Fixed 256-node pool + 4 KiB inline edge-byte pool, strictly iterative (zero recursion)
//   - O(L) exact insert (with atomic pool pre-check & edge splitting), get, remove (with single-child coalescing)
//   - O(L) longestPrefixMatch (LPM) for routing tables / CIDR / policy prefixes
//   - Strictly lexicographical iterative prefix range scan with truncation telemetry

/// Maximum fixed node count in `PrefixTrie` (slot 0 is reserved for root).
pub const PREFIX_TRIE_MAX_NODES: usize = 256;

/// Inline byte pool capacity (4 KiB) storing compressed edge labels.
pub const PREFIX_TRIE_BYTE_POOL_SIZE: usize = 4096;

/// Maximum supported key length in bytes.
pub const PREFIX_TRIE_MAX_KEY_LEN: usize = 128;

/// Sentinel index representing no node.
pub const PREFIX_TRIE_NO_NODE: u16 = 0xFFFF;

// ============================================================================
// Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Result of `PrefixTrie.longestPrefixMatch`.
pub struct PrefixMatch {
    matched_len: usize = 0;
    value: u64 = 0;
}

/// Single key-value entry emitted by `PrefixTrie.scanPrefix`.
pub struct PrefixScanHit {
    key_buf: [PREFIX_TRIE_MAX_KEY_LEN]u8 = [0; PREFIX_TRIE_MAX_KEY_LEN];
    key_len: usize = 0;
    value: u64 = 0;

    /// Returns a read-only byte slice view of the reconstructed key.
    fn keySlice(): []const u8 {
        const n: usize = if (_.key_len <= PREFIX_TRIE_MAX_KEY_LEN) _.key_len else PREFIX_TRIE_MAX_KEY_LEN;
        return _.key_buf[0..n];
    }
}

/// Summary report returned by `PrefixTrie.scanPrefix` including truncation stats.
pub struct PrefixScanReport {
    written: usize = 0;
    total_matched: usize = 0;
    truncated: bool = false;
}

/// Telemetry snapshot for `PrefixTrie`.
pub struct PrefixTrieStats {
    entry_count: usize = 0;
    active_nodes: usize = 0;
    max_nodes: usize = PREFIX_TRIE_MAX_NODES;
    used_edge_bytes: usize = 0;
    max_edge_bytes: usize = PREFIX_TRIE_BYTE_POOL_SIZE;
}

/// Internal compressed trie node stored in the fixed node pool.
struct PrefixTrieNode {
    edge_off: u16 = 0;
    edge_len: u16 = 0;
    first_child: u16 = PREFIX_TRIE_NO_NODE;
    next_sibling: u16 = PREFIX_TRIE_NO_NODE;
    parent: u16 = PREFIX_TRIE_NO_NODE;
    has_value: bool = false;
    value: u64 = 0;
    in_use: bool = false;

    static fn empty(): PrefixTrieNode {
        return PrefixTrieNode{};
    }
}

// ============================================================================
// Core Path-Compressed Byte Prefix Trie Engine
// ============================================================================

/// Zero-heap-allocation path-compressed byte prefix trie (Patricia/Radix KV).
pub struct PrefixTrie {
    nodes: [PREFIX_TRIE_MAX_NODES]PrefixTrieNode = [PrefixTrieNode.empty(); PREFIX_TRIE_MAX_NODES];
    byte_pool: [PREFIX_TRIE_BYTE_POOL_SIZE]u8 = [0; PREFIX_TRIE_BYTE_POOL_SIZE];
    byte_pool_used: usize = 0;
    active_nodes: usize = 1;
    entry_count: usize = 0;

    /// Initializes an empty `PrefixTrie` with root node at slot `0`.
    static fn init(): PrefixTrie {
        let t = PrefixTrie{};
        t.nodes[0].in_use = true;
        t.active_nodes = 1;
        return t;
    }

    fn allocNode(*_): u16? {
        let i: usize = 1;
        while (i < PREFIX_TRIE_MAX_NODES) : (i += 1) {
            if (!_.nodes[i].in_use) {
                _.nodes[i] = PrefixTrieNode.empty();
                _.nodes[i].in_use = true;
                _.active_nodes += 1;
                return i as u16;
            }
        }
        return null;
    }

    fn freeNode(*_, idx: u16) {
        if (idx == 0 || (idx as usize) >= PREFIX_TRIE_MAX_NODES) return;
        if (_.nodes[idx as usize].in_use) {
            _.nodes[idx as usize] = PrefixTrieNode.empty();
            if (_.active_nodes > 1) {
                _.active_nodes -= 1;
            }
        }
    }

    fn allocBytes(*_, src: []const u8): u16? {
        if (src.len == 0) return 0;
        if (_.byte_pool_used + src.len > PREFIX_TRIE_BYTE_POOL_SIZE) {
            return null;
        }
        const off: usize = _.byte_pool_used;
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            _.byte_pool[off + i] = src[i];
        }
        _.byte_pool_used += src.len;
        return off as u16;
    }

    fn edgeSlice(node_idx: u16): []const u8 {
        const n = _.nodes[node_idx as usize];
        const off: usize = n.edge_off as usize;
        const elen: usize = n.edge_len as usize;
        return _.byte_pool[off .. off + elen];
    }

    fn firstEdgeByte(node_idx: u16): u8 {
        const n = _.nodes[node_idx as usize];
        return _.byte_pool[n.edge_off as usize];
    }

    /// Finds direct child of `parent_idx` whose edge starts with `first_b`.
    fn findChildByByte(parent_idx: u16, first_b: u8): u16 {
        let cur: u16 = _.nodes[parent_idx as usize].first_child;
        while (cur != PREFIX_TRIE_NO_NODE) {
            const b: u8 = _.firstEdgeByte(cur);
            if (b == first_b) return cur;
            if (b > first_b) return PREFIX_TRIE_NO_NODE; // children are sorted ascending
            cur = _.nodes[cur as usize].next_sibling;
        }
        return PREFIX_TRIE_NO_NODE;
    }

    /// Inserts `child_idx` into `parent_idx`'s sibling list keeping edge first bytes strictly ascending.
    fn linkChildSorted(*_, parent_idx: u16, child_idx: u16) {
        const child_b: u8 = _.firstEdgeByte(child_idx);
        _.nodes[child_idx as usize].parent = parent_idx;

        const head: u16 = _.nodes[parent_idx as usize].first_child;
        if (head == PREFIX_TRIE_NO_NODE || child_b < _.firstEdgeByte(head)) {
            _.nodes[child_idx as usize].next_sibling = head;
            _.nodes[parent_idx as usize].first_child = child_idx;
            return;
        }

        let prev: u16 = head;
        let cur: u16 = _.nodes[head as usize].next_sibling;
        while (cur != PREFIX_TRIE_NO_NODE && _.firstEdgeByte(cur) < child_b) {
            prev = cur;
            cur = _.nodes[cur as usize].next_sibling;
        }
        _.nodes[child_idx as usize].next_sibling = cur;
        _.nodes[prev as usize].next_sibling = child_idx;
    }

    /// Replaces `old_child` with `new_child` in `parent_idx`'s sibling list (same first byte).
    fn replaceChildInPlace(*_, parent_idx: u16, old_child: u16, new_child: u16) {
        _.nodes[new_child as usize].parent = parent_idx;
        _.nodes[new_child as usize].next_sibling = _.nodes[old_child as usize].next_sibling;
        if (_.nodes[parent_idx as usize].first_child == old_child) {
            _.nodes[parent_idx as usize].first_child = new_child;
            return;
        }
        let prev: u16 = _.nodes[parent_idx as usize].first_child;
        while (prev != PREFIX_TRIE_NO_NODE) {
            if (_.nodes[prev as usize].next_sibling == old_child) {
                _.nodes[prev as usize].next_sibling = new_child;
                return;
            }
            prev = _.nodes[prev as usize].next_sibling;
        }
    }

    /// Unlinks `child_idx` from `parent_idx`'s sibling list.
    fn unlinkChild(*_, parent_idx: u16, child_idx: u16) {
        if (_.nodes[parent_idx as usize].first_child == child_idx) {
            _.nodes[parent_idx as usize].first_child = _.nodes[child_idx as usize].next_sibling;
            _.nodes[child_idx as usize].next_sibling = PREFIX_TRIE_NO_NODE;
            _.nodes[child_idx as usize].parent = PREFIX_TRIE_NO_NODE;
            return;
        }
        let prev: u16 = _.nodes[parent_idx as usize].first_child;
        while (prev != PREFIX_TRIE_NO_NODE) {
            if (_.nodes[prev as usize].next_sibling == child_idx) {
                _.nodes[prev as usize].next_sibling = _.nodes[child_idx as usize].next_sibling;
                _.nodes[child_idx as usize].next_sibling = PREFIX_TRIE_NO_NODE;
                _.nodes[child_idx as usize].parent = PREFIX_TRIE_NO_NODE;
                return;
            }
            prev = _.nodes[prev as usize].next_sibling;
        }
    }

    /// Attempts to coalesce an empty internal node `node_idx` that has exactly one child.
    fn tryCoalesceSingleChild(*_, node_idx: u16) {
        if (node_idx == 0) return;
        const n = _.nodes[node_idx as usize];
        if (!n.in_use || n.has_value) return;

        const child_idx: u16 = n.first_child;
        if (child_idx == PREFIX_TRIE_NO_NODE) return;
        if (_.nodes[child_idx as usize].next_sibling != PREFIX_TRIE_NO_NODE) return;

        const parent_idx: u16 = n.parent;
        if (parent_idx == PREFIX_TRIE_NO_NODE) return;

        const c = _.nodes[child_idx as usize];
        const merged_len: usize = (n.edge_len as usize) + (c.edge_len as usize);
        if (merged_len > PREFIX_TRIE_MAX_KEY_LEN) return;
        if (_.byte_pool_used + merged_len > PREFIX_TRIE_BYTE_POOL_SIZE) return;

        const new_off: usize = _.byte_pool_used;
        let i: usize = 0;
        while (i < (n.edge_len as usize)) : (i += 1) {
            _.byte_pool[new_off + i] = _.byte_pool[(n.edge_off as usize) + i];
        }
        let j: usize = 0;
        while (j < (c.edge_len as usize)) : (j += 1) {
            _.byte_pool[new_off + (n.edge_len as usize) + j] = _.byte_pool[(c.edge_off as usize) + j];
        }
        _.byte_pool_used += merged_len;

        _.nodes[child_idx as usize].edge_off = new_off as u16;
        _.nodes[child_idx as usize].edge_len = merged_len as u16;
        _.replaceChildInPlace(parent_idx, node_idx, child_idx);
        _.freeNode(node_idx);
    }

    // ========================================================================
    // Public Operations
    // ========================================================================

    /// Inserts or updates `(key, value)` with automatic compressed edge splitting.
    /// Rejects empty keys, keys longer than `PREFIX_TRIE_MAX_KEY_LEN`, or pool exhaustion atomically.
    fn insert(*_, key: []const u8, value: u64): bool {
        if (key.len == 0 || key.len > PREFIX_TRIE_MAX_KEY_LEN) {
            return false;
        }

        let cur: u16 = 0;
        let pos: usize = 0;
        const free_nodes: usize = PREFIX_TRIE_MAX_NODES - _.active_nodes;

        while (pos < key.len) {
            const child: u16 = _.findChildByByte(cur, key[pos]);
            if (child == PREFIX_TRIE_NO_NODE) {
                // Need 1 new leaf node + remaining key bytes
                const rem_len: usize = key.len - pos;
                if (free_nodes < 1 || _.byte_pool_used + rem_len > PREFIX_TRIE_BYTE_POOL_SIZE) {
                    return false;
                }
                const edge_off: u16 = _.allocBytes(key[pos..]) ?? return false;
                const leaf_idx: u16 = _.allocNode() ?? return false;
                _.nodes[leaf_idx as usize].edge_off = edge_off;
                _.nodes[leaf_idx as usize].edge_len = rem_len as u16;
                _.nodes[leaf_idx as usize].has_value = true;
                _.nodes[leaf_idx as usize].value = value;
                _.linkChildSorted(cur, leaf_idx);
                _.entry_count += 1;
                return true;
            }

            // Compare key[pos..] with child's compressed edge
            const edge_off: usize = _.nodes[child as usize].edge_off as usize;
            const edge_len: usize = _.nodes[child as usize].edge_len as usize;
            const rem_len: usize = key.len - pos;
            const cmp_limit: usize = if (edge_len < rem_len) edge_len else rem_len;

            let lcp: usize = 0;
            while (lcp < cmp_limit && _.byte_pool[edge_off + lcp] == key[pos + lcp]) : (lcp += 1) {}

            if (lcp == edge_len) {
                // Full edge matched; descend into child
                cur = child;
                pos += edge_len;
                continue;
            }

            // Split edge at `lcp`:
            //   - if pos + lcp == key.len: need 1 node (split node holds value), 0 new bytes
            //   - else: need 2 nodes (split node + new leaf), (key.len - pos - lcp) new bytes
            const tail_len: usize = key.len - (pos + lcp);
            const needed_nodes: usize = if (tail_len == 0) 1 else 2;
            if (free_nodes < needed_nodes || _.byte_pool_used + tail_len > PREFIX_TRIE_BYTE_POOL_SIZE) {
                return false;
            }

            const split_idx: u16 = _.allocNode() ?? return false;
            _.nodes[split_idx as usize].edge_off = edge_off as u16;
            _.nodes[split_idx as usize].edge_len = lcp as u16;
            _.replaceChildInPlace(cur, child, split_idx);

            // Shorten existing child edge in place (reuses byte_pool slice, zero allocation!)
            _.nodes[child as usize].edge_off = (edge_off + lcp) as u16;
            _.nodes[child as usize].edge_len = (edge_len - lcp) as u16;
            _.nodes[child as usize].next_sibling = PREFIX_TRIE_NO_NODE;
            _.linkChildSorted(split_idx, child);

            if (tail_len == 0) {
                _.nodes[split_idx as usize].has_value = true;
                _.nodes[split_idx as usize].value = value;
                _.entry_count += 1;
                return true;
            }

            const leaf_off: u16 = _.allocBytes(key[pos + lcp ..]) ?? return false;
            const leaf_idx: u16 = _.allocNode() ?? return false;
            _.nodes[leaf_idx as usize].edge_off = leaf_off;
            _.nodes[leaf_idx as usize].edge_len = tail_len as u16;
            _.nodes[leaf_idx as usize].has_value = true;
            _.nodes[leaf_idx as usize].value = value;
            _.linkChildSorted(split_idx, leaf_idx);
            _.entry_count += 1;
            return true;
        }

        // Exact match on existing node `cur`
        if (!_.nodes[cur as usize].has_value) {
            _.nodes[cur as usize].has_value = true;
            _.entry_count += 1;
        }
        _.nodes[cur as usize].value = value;
        return true;
    }

    /// O(L) exact lookup for `key`. Returns `null` if absent or empty.
    fn get(key: []const u8): u64? {
        if (key.len == 0 || key.len > PREFIX_TRIE_MAX_KEY_LEN) {
            return null;
        }
        let cur: u16 = 0;
        let pos: usize = 0;
        while (pos < key.len) {
            const child: u16 = _.findChildByByte(cur, key[pos]);
            if (child == PREFIX_TRIE_NO_NODE) return null;

            const edge = _.edgeSlice(child);
            if (key.len - pos < edge.len) return null;
            let i: usize = 0;
            while (i < edge.len) : (i += 1) {
                if (edge[i] != key[pos + i]) return null;
            }
            pos += edge.len;
            cur = child;
        }
        if (_.nodes[cur as usize].has_value) {
            return _.nodes[cur as usize].value;
        }
        return null;
    }

    /// O(L) exact removal of `key`. Automatically removes empty leaves and coalesces
    /// single-child internal nodes to keep the trie compact.
    fn remove(*_, key: []const u8): bool {
        if (key.len == 0 || key.len > PREFIX_TRIE_MAX_KEY_LEN) {
            return false;
        }
        let cur: u16 = 0;
        let pos: usize = 0;
        while (pos < key.len) {
            const child: u16 = _.findChildByByte(cur, key[pos]);
            if (child == PREFIX_TRIE_NO_NODE) return false;

            const edge = _.edgeSlice(child);
            if (key.len - pos < edge.len) return false;
            let i: usize = 0;
            while (i < edge.len) : (i += 1) {
                if (edge[i] != key[pos + i]) return false;
            }
            pos += edge.len;
            cur = child;
        }

        if (!_.nodes[cur as usize].has_value) {
            return false;
        }

        _.nodes[cur as usize].has_value = false;
        _.nodes[cur as usize].value = 0;
        if (_.entry_count > 0) {
            _.entry_count -= 1;
        }

        if (cur == 0) {
            return true;
        }

        if (_.nodes[cur as usize].first_child == PREFIX_TRIE_NO_NODE) {
            // Leaf node: unlink from parent, free node, then coalesce parent if needed
            const parent_idx: u16 = _.nodes[cur as usize].parent;
            _.unlinkChild(parent_idx, cur);
            _.freeNode(cur);
            if (parent_idx != 0) {
                _.tryCoalesceSingleChild(parent_idx);
            }
        } else {
            // Internal node: coalesce if it now has only one child
            _.tryCoalesceSingleChild(cur);
        }
        return true;
    }

    /// O(L) Longest Prefix Match (LPM): returns the longest stored key that is a prefix of `query`.
    fn longestPrefixMatch(query: []const u8): PrefixMatch? {
        if (query.len == 0) {
            return null;
        }
        let cur: u16 = 0;
        let pos: usize = 0;
        let best: PrefixMatch? = null;

        while (pos < query.len) {
            const child: u16 = _.findChildByByte(cur, query[pos]);
            if (child == PREFIX_TRIE_NO_NODE) break;

            const edge = _.edgeSlice(child);
            if (query.len - pos < edge.len) break;
            let matched: bool = true;
            let i: usize = 0;
            while (i < edge.len) : (i += 1) {
                if (edge[i] != query[pos + i]) {
                    matched = false;
                    break;
                }
            }
            if (!matched) break;

            pos += edge.len;
            cur = child;
            if (_.nodes[cur as usize].has_value) {
                best = PrefixMatch{
                    matched_len: pos,
                    value: _.nodes[cur as usize].value,
                };
            }
        }
        return best;
    }

    /// Strictly iterative, lexicographically ordered prefix range scan.
    /// Enumerates all stored keys starting with `prefix` (empty `prefix` scans all entries)
    /// into `out_hits`, returning written count, total matches, and truncation flag.
    fn scanPrefix(prefix: []const u8, out_hits: *[]PrefixScanHit): PrefixScanReport {
        if (prefix.len > PREFIX_TRIE_MAX_KEY_LEN) {
            return PrefixScanReport{};
        }

        // Step 1: Walk down to the subtree root matching `prefix`, reconstructing its full path
        let path_buf: [PREFIX_TRIE_MAX_KEY_LEN]u8 = [0; PREFIX_TRIE_MAX_KEY_LEN];
        let path_len: usize = 0;
        let cur: u16 = 0;
        let pos: usize = 0;

        while (pos < prefix.len) {
            const child: u16 = _.findChildByByte(cur, prefix[pos]);
            if (child == PREFIX_TRIE_NO_NODE) {
                return PrefixScanReport{};
            }
            const edge = _.edgeSlice(child);
            const rem: usize = prefix.len - pos;
            const cmp_len: usize = if (edge.len < rem) edge.len else rem;

            let i: usize = 0;
            while (i < cmp_len) : (i += 1) {
                if (edge[i] != prefix[pos + i]) {
                    return PrefixScanReport{};
                }
            }

            // Append full edge of `child` to `path_buf`
            if (path_len + edge.len > PREFIX_TRIE_MAX_KEY_LEN) {
                return PrefixScanReport{};
            }
            let e: usize = 0;
            while (e < edge.len) : (e += 1) {
                path_buf[path_len + e] = edge[e];
            }
            path_len += edge.len;
            cur = child;
            pos += cmp_len;
        }

        // Step 2: Iterative pre-order DFS over sorted sibling lists -> guaranteed lexicographical order
        let stack_node: [PREFIX_TRIE_MAX_NODES]u16 = [0; PREFIX_TRIE_MAX_NODES];
        let stack_next_child: [PREFIX_TRIE_MAX_NODES]u16 = [PREFIX_TRIE_NO_NODE; PREFIX_TRIE_MAX_NODES];
        let stack_path_len: [PREFIX_TRIE_MAX_NODES]usize = [0; PREFIX_TRIE_MAX_NODES];
        let sp: usize = 0;

        let report = PrefixScanReport{};

        // Visit subtree root `cur`
        if (_.nodes[cur as usize].has_value) {
            report.total_matched += 1;
            if (report.written < out_hits.len) {
                let hit = PrefixScanHit{ key_len: path_len, value: _.nodes[cur as usize].value };
                let k: usize = 0;
                while (k < path_len) : (k += 1) {
                    hit.key_buf[k] = path_buf[k];
                }
                out_hits[report.written] = hit;
                report.written += 1;
            } else {
                report.truncated = true;
            }
        }

        stack_node[0] = cur;
        stack_next_child[0] = _.nodes[cur as usize].first_child;
        stack_path_len[0] = path_len;
        sp = 1;

        while (sp > 0) {
            const top: usize = (sp - 1) as usize;
            const child: u16 = stack_next_child[top];
            if (child == PREFIX_TRIE_NO_NODE) {
                sp -= 1;
                continue;
            }
            // Advance parent's next-child cursor
            stack_next_child[top] = _.nodes[child as usize].next_sibling;

            const base_len: usize = stack_path_len[top];
            const edge = _.edgeSlice(child);
            if (base_len + edge.len > PREFIX_TRIE_MAX_KEY_LEN) {
                continue;
            }
            let e: usize = 0;
            while (e < edge.len) : (e += 1) {
                path_buf[base_len + e] = edge[e];
            }
            const child_path_len: usize = base_len + edge.len;

            if (_.nodes[child as usize].has_value) {
                report.total_matched += 1;
                if (report.written < out_hits.len) {
                    let hit = PrefixScanHit{ key_len: child_path_len, value: _.nodes[child as usize].value };
                    let k: usize = 0;
                    while (k < child_path_len) : (k += 1) {
                        hit.key_buf[k] = path_buf[k];
                    }
                    out_hits[report.written] = hit;
                    report.written += 1;
                } else {
                    report.truncated = true;
                }
            }

            if (_.nodes[child as usize].first_child != PREFIX_TRIE_NO_NODE && sp < PREFIX_TRIE_MAX_NODES) {
                stack_node[sp] = child;
                stack_next_child[sp] = _.nodes[child as usize].first_child;
                stack_path_len[sp] = child_path_len;
                sp += 1;
            }
        }

        return report;
    }

    /// Returns the number of stored key-value entries.
    fn len(): usize {
        return _.entry_count;
    }

    /// Returns `true` when no entries are stored.
    fn isEmpty(): bool {
        return _.entry_count == 0;
    }

    /// Clears all entries and resets the node and byte pools.
    fn clear(*_) {
        let i: usize = 0;
        while (i < PREFIX_TRIE_MAX_NODES) : (i += 1) {
            _.nodes[i] = PrefixTrieNode.empty();
        }
        _.nodes[0].in_use = true;
        _.byte_pool_used = 0;
        _.active_nodes = 1;
        _.entry_count = 0;
    }

    /// Returns a telemetry snapshot of node pool and edge byte pool usage.
    fn stats(): PrefixTrieStats {
        return PrefixTrieStats{
            entry_count: _.entry_count,
            active_nodes: _.active_nodes,
            max_nodes: PREFIX_TRIE_MAX_NODES,
            used_edge_bytes: _.byte_pool_used,
            max_edge_bytes: PREFIX_TRIE_BYTE_POOL_SIZE,
        };
    }
}
