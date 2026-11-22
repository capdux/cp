// Source: cp/lib/search/trie.cp (Prefix Autocomplete Trie & Search Error Types)
// Pure CP prefix autocomplete trie with ASCII case normalization, frequency aggregation,
// and Top-K suggestion ranking (frequency descending, lexicographical ascending).

const std = @import("std");

/// Error set for search, indexing, and query parsing operations.
pub err SearchError {
    InvalidQuery,
    DocumentNotFound,
    DuplicateDocId,
    OutOfMemory,
}

/// Autocomplete suggestion entry returned by `PrefixTrie.suggest` and `SearchIndex.autocomplete`.
/// `term` borrows from the trie's internal storage and remains valid until `deinit()`.
pub struct PrefixSuggestion {
    term: string = "";
    freq: u32 = 0;
}

struct TrieEdge {
    ch: u8 = 0;
    child: usize = 0;
}

struct TrieNode {
    edges: std.ArrayList(TrieEdge) = .empty;
    freq: u32 = 0;
    term: []u8 = [];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn suggestionBetter(a: PrefixSuggestion, b: PrefixSuggestion): bool {
    if (a.freq != b.freq) return a.freq > b.freq;
    return std.mem.order(u8, a.term, b.term) == .lt;
}

fn siftDownWorstSuggestion(items: []PrefixSuggestion, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && suggestionBetter(items[worst], items[right])) {
            worst = right;
        }
        if (!suggestionBetter(items[idx], items[worst])) break;
        const tmp = items[idx];
        items[idx] = items[worst];
        items[worst] = tmp;
        idx = worst;
    }
}

fn sortSuggestionsDesc(items: []PrefixSuggestion) {
    if (items.len <= 1) return;
    const n = items.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        siftDownWorstSuggestion(items, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = items[0];
        items[0] = items[end];
        items[end] = tmp;
        siftDownWorstSuggestion(items, 0, end);
    }
}

/// Pure CP prefix trie for fast case-insensitive term autocomplete.
pub struct PrefixTrie {
    alloc: std.mem.Allocator = undefined;
    nodes: std.ArrayList(TrieNode) = .empty;
    unique_terms: usize = 0;

    /// Initializes an empty `PrefixTrie` with a root node.
    pub static fn init(alloc: std.mem.Allocator): PrefixTrie {
        let trie = PrefixTrie{
            alloc: alloc,
            nodes: .empty,
            unique_terms: 0,
        };
        trie.nodes.append(alloc, TrieNode{}) catch {};
        return trie;
    }

    /// Releases all nodes, edge lists, and stored term buffers owned by the trie.
    pub fn deinit(*_) {
        for (*node of _.nodes.items) {
            node.edges.deinit(_.alloc);
            if (node.term.len > 0) {
                _.alloc.free(node.term);
            }
        }
        _.nodes.deinit(_.alloc);
        _.unique_terms = 0;
    }

    /// Returns the number of active unique terms with `freq > 0` in the trie.
    pub fn termCount(): usize {
        return _.unique_terms;
    }

    fn ensureRoot(*_): ! {
        if (_.nodes.items.len == 0) {
            _.nodes.append(_.alloc, TrieNode{})?;
        }
    }

    fn findChild(node_idx: usize, ch: u8): usize? {
        const edges = _.nodes.items[node_idx].edges.items;
        for (edge of edges) {
            if (edge.ch == ch) return edge.child;
        }
        return null;
    }

    fn walkLower(raw: string): usize? {
        if (_.nodes.items.len == 0) return null;
        let cur: usize = 0;
        for (b of raw) {
            const lc = toLowerAscii(b);
            if (next := _.findChild(cur, lc)) {
                cur = next;
            } else {
                return null;
            }
        }
        return cur;
    }

    /// Inserts `term` (normalized to ASCII lowercase) and adds `weight_delta` to its frequency.
    pub fn insert(*_, term: string, weight_delta: u32 = 1): ! {
        if (term.len == 0) return;
        _.ensureRoot()?;

        let lower_buf = _.alloc.alloc(u8, term.len)?;
        defer _.alloc.free(lower_buf);
        for (i of 0..term.len) {
            lower_buf[i] = toLowerAscii(term[i]);
        }

        let cur: usize = 0;
        for (ch of lower_buf) {
            if (next := _.findChild(cur, ch)) {
                cur = next;
            } else {
                const new_idx = _.nodes.items.len;
                _.nodes.append(_.alloc, TrieNode{})?;
                _.nodes.items[cur].edges.append(_.alloc, TrieEdge{
                    ch: ch,
                    child: new_idx,
                })?;
                cur = new_idx;
            }
        }

        if (_.nodes.items[cur].term.len == 0) {
            _.nodes.items[cur].term = _.alloc.dupe(u8, lower_buf)?;
        }

        const prev_freq = _.nodes.items[cur].freq;
        const delta = if (weight_delta == 0 && prev_freq == 0) 1 else weight_delta;
        _.nodes.items[cur].freq = _.nodes.items[cur].freq +| delta;
        if (prev_freq == 0 && _.nodes.items[cur].freq > 0) {
            _.unique_terms += 1;
        }
    }

    /// Decrements frequency of `term` by `weight_delta` (used when removing/updating documents).
    pub fn decrement(*_, term: string, weight_delta: u32 = 1) {
        if (term.len == 0 || weight_delta == 0) return;
        if (node_idx := _.walkLower(term)) {
            const cur_freq = _.nodes.items[node_idx].freq;
            if (cur_freq == 0) return;
            if (cur_freq <= weight_delta) {
                _.nodes.items[node_idx].freq = 0;
                if (_.unique_terms > 0) _.unique_terms -= 1;
            } else {
                _.nodes.items[node_idx].freq = cur_freq - weight_delta;
            }
        }
    }

    /// Returns `true` if `term` (case-insensitive) exists in the trie with `freq > 0`.
    pub fn contains(term: string): bool {
        if (term.len == 0) return false;
        if (node_idx := _.walkLower(term)) {
            return _.nodes.items[node_idx].freq > 0;
        }
        return false;
    }

    /// Returns up to `limit` autocomplete candidates matching `prefix`,
    /// ordered by frequency descending then lexicographically ascending.
    /// Caller owns the returned slice and must free it with `alloc.free(results)`.
    pub fn suggest(
        alloc: std.mem.Allocator,
        prefix: string,
        limit: usize,
    ): ([]PrefixSuggestion)! {
        if (limit == 0) return [];
        const start_opt = _.walkLower(prefix);
        if (start_opt == null) return [];
        const start_idx = start_opt.?;

        let collected: std.ArrayList(PrefixSuggestion) = .empty;
        defer collected.deinit(alloc);

        let stack: std.ArrayList(usize) = .empty;
        defer stack.deinit(alloc);
        stack.append(alloc, start_idx)?;

        while (stack.items.len > 0) {
            const node_idx = stack.pop().?;
            const node = _.nodes.items[node_idx];
            if (node.freq > 0 && node.term.len > 0) {
                collected.append(alloc, PrefixSuggestion{
                    term: node.term,
                    freq: node.freq,
                })?;
            }
            for (edge of node.edges.items) {
                stack.append(alloc, edge.child)?;
            }
        }

        if (collected.items.len == 0) return [];
        sortSuggestionsDesc(collected.items);

        const out_len = @min(limit, collected.items.len);
        const out = alloc.alloc(PrefixSuggestion, out_len)?;
        @memcpy(out, collected.items[0..out_len]);
        return out;
    }
}
