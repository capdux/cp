// Type definitions for CP

export class TrieNode {
    byte: u8,
    is_end: bool,
    value: i64,
    first_child: u16,
    next_sibling: u16,
    subtree_words: u16,
    fn empty(): TrieNode;
}

export class LongestMatch {
    matched_len: usize,
    value: i64,
    found: bool,
    fn none(): LongestMatch;
}

export class Trie {
    nodes: [512]TrieNode,
    used_nodes: usize,
    word_count: usize,
    fn init(): Trie;
    fn clear(_: auto);
    fn len(): usize;
    fn nodeCount(): usize;
    fn findChild(parent: u16, b: u8): u16;
    fn walkPrefix(prefix: &[]u8): u16?;
    fn insert(_: auto, key: &[]u8, value: i64): bool;
    fn contains(key: &[]u8): bool;
    fn get(key: &[]u8): i64;
    fn remove(_: auto, key: &[]u8): bool;
    fn hasPrefix(prefix: &[]u8): bool;
    fn countWithPrefix(prefix: &[]u8): usize;
    fn longestPrefixMatch(text: &[]u8): LongestMatch;
}

