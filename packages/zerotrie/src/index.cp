// @cap/zerotrie — Zero-allocation compact left-child right-sibling prefix trie
// Inspired by ICU4X / Rust `zerotrie`
// MIT License

pub struct TrieNode {
    pub ch: u8,
    pub has_value: bool,
    pub value: u32,
    pub first_child: i32,
    pub next_sibling: i32,

    pub static fn empty(): TrieNode {
        return TrieNode {
            ch: 0,
            has_value: false,
            value: 0,
            first_child: -1,
            next_sibling: -1,
        };
    }
}

pub struct PrefixMatch {
    pub matched_len: usize,
    pub value: u32,
    pub found: bool,
}

fn findChild(nodes: []const TrieNode, parent: usize, ch: u8): i32 {
    let cur: i32 = nodes[parent].first_child;
    while (cur >= 0) {
        let idx: usize = cur as usize;
        if (nodes[idx].ch == ch) {
            return cur;
        }
        cur = nodes[idx].next_sibling;
    }
    return -1;
}

pub struct ZeroTrie {
    pub node_count: usize,
    pub key_count: usize,

    pub static fn new(): ZeroTrie {
        return ZeroTrie { node_count: 0, key_count: 0 };
    }

    pub fn init(*_, nodes: []TrieNode): bool {
        if (nodes.len == 0) {
            _.node_count = 0;
            _.key_count = 0;
            return false;
        }
        nodes[0] = TrieNode.empty();
        _.node_count = 1;
        _.key_count = 0;
        return true;
    }

    pub fn insert(*_, nodes: []TrieNode, key: string, value: u32): bool {
        if (_.node_count == 0 || nodes.len == 0) {
            return false;
        }
        // Walk existing prefix first to check atomic capacity before allocating
        let cur: usize = 0;
        let i: usize = 0;
        while (i < key.len) {
            let child: i32 = findChild(nodes, cur, key[i]);
            if (child < 0) {
                break;
            }
            cur = child as usize;
            i = i + 1;
        }

        let needed: usize = key.len - i;
        if (needed > nodes.len - _.node_count) {
            return false;
        }

        while (i < key.len) {
            let new_idx: usize = _.node_count;
            _.node_count = _.node_count + 1;
            let parent_node: TrieNode = nodes[cur];
            nodes[new_idx] = TrieNode {
                ch: key[i],
                has_value: false,
                value: 0,
                first_child: -1,
                next_sibling: parent_node.first_child,
            };
            parent_node.first_child = new_idx as i32;
            nodes[cur] = parent_node;
            cur = new_idx;
            i = i + 1;
        }

        let end_node: TrieNode = nodes[cur];
        if (!end_node.has_value) {
            end_node.has_value = true;
            _.key_count = _.key_count + 1;
        }
        end_node.value = value;
        nodes[cur] = end_node;
        return true;
    }

    pub fn get(nodes: []const TrieNode, key: string): i64 {
        if (_.node_count == 0 || nodes.len == 0) {
            return -1;
        }
        let cur: usize = 0;
        let i: usize = 0;
        while (i < key.len) {
            let child: i32 = findChild(nodes, cur, key[i]);
            if (child < 0) {
                return -1;
            }
            cur = child as usize;
            i = i + 1;
        }
        if (!nodes[cur].has_value) {
            return -1;
        }
        return nodes[cur].value as i64;
    }

    pub fn containsKey(nodes: []const TrieNode, key: string): bool {
        return _.get(nodes, key) >= 0;
    }

    pub fn hasPrefix(nodes: []const TrieNode, prefix: string): bool {
        if (_.node_count == 0 || nodes.len == 0 || _.key_count == 0) {
            return false;
        }
        let cur: usize = 0;
        let i: usize = 0;
        while (i < prefix.len) {
            let child: i32 = findChild(nodes, cur, prefix[i]);
            if (child < 0) {
                return false;
            }
            cur = child as usize;
            i = i + 1;
        }
        return true;
    }

    pub fn longestPrefixMatch(nodes: []const TrieNode, input: string): PrefixMatch {
        if (_.node_count == 0 || nodes.len == 0) {
            return PrefixMatch { matched_len: 0, value: 0, found: false };
        }
        let best_found: bool = false;
        let best_len: usize = 0;
        let best_val: u32 = 0;
        if (nodes[0].has_value) {
            best_found = true;
            best_len = 0;
            best_val = nodes[0].value;
        }
        let cur: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            let child: i32 = findChild(nodes, cur, input[i]);
            if (child < 0) {
                break;
            }
            cur = child as usize;
            if (nodes[cur].has_value) {
                best_found = true;
                best_len = i + 1;
                best_val = nodes[cur].value;
            }
            i = i + 1;
        }
        return PrefixMatch {
            matched_len: best_len,
            value: best_val,
            found: best_found,
        };
    }
}
