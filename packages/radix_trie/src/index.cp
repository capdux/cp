// @cap/trie — Zero-allocation LCRS static-pool byte prefix tree (Trie) for CP
// Clean-room MIT implementation inspired by Rust `radix_trie` / `qp-trie` and npm `trie-search`.

pub const MAX_TRIE_NODES: usize = 512;

pub struct TrieNode {
    pub byte: u8,
    pub is_end: bool,
    pub value: i64,
    pub first_child: u16,
    pub next_sibling: u16,
    pub subtree_words: u16,

    pub static fn empty() -> TrieNode {
        return TrieNode{
            byte: 0,
            is_end: false,
            value: 0,
            first_child: 0,
            next_sibling: 0,
            subtree_words: 0,
        };
    }
}

pub struct LongestMatch {
    pub matched_len: usize,
    pub value: i64,
    pub found: bool,

    pub static fn none() -> LongestMatch {
        return LongestMatch{
            matched_len: 0,
            value: 0,
            found: false,
        };
    }
}

pub struct Trie {
    pub nodes: [512]TrieNode,
    pub used_nodes: usize,
    pub word_count: usize,

    /// Initializes an empty `Trie` with node 0 as root.
    pub static fn init() -> Trie {
        let nodes: [512]TrieNode = undefined;
        nodes[0] = TrieNode.empty();
        return Trie{
            nodes: nodes,
            used_nodes: 1,
            word_count: 0,
        };
    }

    pub fn clear(*_) {
        _.nodes[0] = TrieNode.empty();
        _.used_nodes = 1;
        _.word_count = 0;
    }

    /// Returns the total number of distinct live keys stored in the trie.
    pub fn len() -> usize {
        return _.word_count;
    }

    /// Returns the total number of allocated nodes in the static pool (including root).
    pub fn nodeCount() -> usize {
        return _.used_nodes;
    }

    fn findChild(parent: u16, b: u8) -> u16 {
        let cur = _.nodes[parent as usize].first_child;
        while (cur != 0) {
            if (_.nodes[cur as usize].byte == b) {
                return cur;
            }
            cur = _.nodes[cur as usize].next_sibling;
        }
        return 0;
    }

    fn walkPrefix(prefix: []const u8) -> u16? {
        let cur: u16 = 0;
        let i: usize = 0;
        while (i < prefix.len) {
            const next = _.findChild(cur, prefix[i]);
            if (next == 0) {
                return null;
            }
            cur = next;
            i += 1;
        }
        return cur;
    }

    /// Inserts or updates `key` with `value`.
    /// Returns `false` (leaving the trie unchanged) if the static node pool cannot fit the new nodes.
    pub fn insert(*_, key: []const u8, value: i64) -> bool {
        // Phase 1: walk existing prefix to check capacity before mutating
        let cur: u16 = 0;
        let matched: usize = 0;
        while (matched < key.len) {
            const next = _.findChild(cur, key[matched]);
            if (next == 0) {
                break;
            }
            cur = next;
            matched += 1;
        }

        const needed = key.len - matched;
        if (_.used_nodes + needed > MAX_TRIE_NODES) {
            return false;
        }

        // Update existing key in-place without changing counts
        if (matched == key.len && _.nodes[cur as usize].is_end) {
            _.nodes[cur as usize].value = value;
            return true;
        }

        // Phase 2: allocate missing suffix nodes
        let i = matched;
        while (i < key.len) {
            const new_idx = _.used_nodes as u16;
            _.used_nodes += 1;
            let node = TrieNode.empty();
            node.byte = key[i];
            node.next_sibling = _.nodes[cur as usize].first_child;
            _.nodes[new_idx as usize] = node;
            _.nodes[cur as usize].first_child = new_idx;
            cur = new_idx;
            i += 1;
        }

        _.nodes[cur as usize].is_end = true;
        _.nodes[cur as usize].value = value;
        _.word_count += 1;

        // Phase 3: increment `subtree_words` along root..terminal path
        let walk: u16 = 0;
        _.nodes[0].subtree_words += 1;
        let k: usize = 0;
        while (k < key.len) {
            walk = _.findChild(walk, key[k]);
            _.nodes[walk as usize].subtree_words += 1;
            k += 1;
        }

        return true;
    }

    /// Returns `true` if `key` is currently present in the trie.
    pub fn contains(key: []const u8) -> bool {
        const node_opt = _.walkPrefix(key);
        if (node_opt == null) {
            return false;
        }
        return _.nodes[node_opt.? as usize].is_end;
    }

    /// Returns the value associated with `key`, or `-1` if not found.
    /// Use `contains(key)` to distinguish a missing key from a key legitimately mapped to `-1`.
    pub fn get(key: []const u8) -> i64 {
        const node_opt = _.walkPrefix(key);
        if (node_opt == null) {
            return -1;
        }
        const n = _.nodes[node_opt.? as usize];
        if (!n.is_end) {
            return -1;
        }
        return n.value;
    }

    /// Logically removes `key` and updates `len` and `subtree_words` counts.
    /// Returns `true` if `key` existed and was removed, `false` otherwise.
    pub fn remove(*_, key: []const u8) -> bool {
        const node_opt = _.walkPrefix(key);
        if (node_opt == null) {
            return false;
        }
        const target = node_opt.? as usize;
        if (!_.nodes[target].is_end) {
            return false;
        }

        _.nodes[target].is_end = false;
        _.nodes[target].value = 0;
        _.word_count -= 1;

        let walk: u16 = 0;
        _.nodes[0].subtree_words -= 1;
        let k: usize = 0;
        while (k < key.len) {
            walk = _.findChild(walk, key[k]);
            _.nodes[walk as usize].subtree_words -= 1;
            k += 1;
        }
        return true;
    }

    /// Returns `true` if at least one live key begins with `prefix`.
    pub fn hasPrefix(prefix: []const u8) -> bool {
        const node_opt = _.walkPrefix(prefix);
        if (node_opt == null) {
            return false;
        }
        return _.nodes[node_opt.? as usize].subtree_words > 0;
    }

    /// Returns the number of live keys starting with `prefix`.
    pub fn countWithPrefix(prefix: []const u8) -> usize {
        const node_opt = _.walkPrefix(prefix);
        if (node_opt == null) {
            return 0;
        }
        return _.nodes[node_opt.? as usize].subtree_words as usize;
    }

    /// Finds the longest registered prefix key that matches the beginning of `text` (LPM).
    pub fn longestPrefixMatch(text: []const u8) -> LongestMatch {
        let best = LongestMatch.none();
        if (_.nodes[0].is_end) {
            best = LongestMatch{
                matched_len: 0,
                value: _.nodes[0].value,
                found: true,
            };
        }

        let cur: u16 = 0;
        let i: usize = 0;
        while (i < text.len) {
            const next = _.findChild(cur, text[i]);
            if (next == 0) {
                break;
            }
            cur = next;
            if (_.nodes[cur as usize].is_end) {
                best = LongestMatch{
                    matched_len: i + 1,
                    value: _.nodes[cur as usize].value,
                    found: true,
                };
            }
            i += 1;
        }
        return best;
    }
}
