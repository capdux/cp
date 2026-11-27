// Type definitions for CP

export class TrieNode {
    ch: u8,
    has_value: bool,
    value: u32,
    first_child: i32,
    next_sibling: i32,
    fn empty(): TrieNode;
}

export class PrefixMatch {
    matched_len: usize,
    value: u32,
    found: bool,
}

export fn findChild(nodes: &[]TrieNode, parent: usize, ch: u8): i32;
export class ZeroTrie {
    node_count: usize,
    key_count: usize,
    fn new(): ZeroTrie;
    fn init(_: auto, nodes: []TrieNode): bool;
    fn insert(_: auto, nodes: []TrieNode, key: string, value: u32): bool;
    fn get(nodes: &[]TrieNode, key: string): i64;
    fn containsKey(nodes: &[]TrieNode, key: string): bool;
    fn hasPrefix(nodes: &[]TrieNode, prefix: string): bool;
    fn longestPrefixMatch(nodes: &[]TrieNode, input: string): PrefixMatch;
}

