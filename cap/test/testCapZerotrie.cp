import { test, expect_eq } from "cap:test";
import {
    TrieNode,
    PrefixMatch,
    ZeroTrie,
} from "../../packages/@cap/zerotrie/src/index.cp";

fn main() {
    test("1. @cap/zerotrie — multi-key insert & exact lookup", () => {
        let pool: [64]TrieNode = [TrieNode.empty(); 64];
        let trie: ZeroTrie = ZeroTrie.new();
        expect_eq(trie.init(pool[0..64]), true);

        expect_eq(trie.insert(pool[0..64], "GET", 1), true);
        expect_eq(trie.insert(pool[0..64], "POST", 2), true);
        expect_eq(trie.insert(pool[0..64], "PUT", 3), true);
        expect_eq(trie.insert(pool[0..64], "DELETE", 4), true);

        expect_eq(trie.key_count, 4 as usize);
        expect_eq(trie.get(pool[0..64], "GET"), 1 as i64);
        expect_eq(trie.get(pool[0..64], "POST"), 2 as i64);
        expect_eq(trie.get(pool[0..64], "PUT"), 3 as i64);
        expect_eq(trie.get(pool[0..64], "DELETE"), 4 as i64);
        expect_eq(trie.containsKey(pool[0..64], "PATCH"), false);
        expect_eq(trie.get(pool[0..64], "PATCH"), -1 as i64);
    });

    test("2. @cap/zerotrie — shared prefix keys a / ab / abc stay independent", () => {
        let pool: [32]TrieNode = [TrieNode.empty(); 32];
        let trie: ZeroTrie = ZeroTrie.new();
        * = trie.init(pool[0..32]);

        expect_eq(trie.insert(pool[0..32], "a", 10), true);
        expect_eq(trie.insert(pool[0..32], "ab", 20), true);
        expect_eq(trie.insert(pool[0..32], "abc", 30), true);

        expect_eq(trie.key_count, 3 as usize);
        expect_eq(trie.node_count, 4 as usize); // root + a + b + c
        expect_eq(trie.get(pool[0..32], "a"), 10 as i64);
        expect_eq(trie.get(pool[0..32], "ab"), 20 as i64);
        expect_eq(trie.get(pool[0..32], "abc"), 30 as i64);
        expect_eq(trie.get(pool[0..32], "abcd"), -1 as i64);
    });

    test("3. @cap/zerotrie — key overwrite updates value without increasing key_count", () => {
        let pool: [32]TrieNode = [TrieNode.empty(); 32];
        let trie: ZeroTrie = ZeroTrie.new();
        * = trie.init(pool[0..32]);

        expect_eq(trie.insert(pool[0..32], "en-US", 100), true);
        expect_eq(trie.get(pool[0..32], "en-US"), 100 as i64);
        let nodes_before: usize = trie.node_count;

        expect_eq(trie.insert(pool[0..32], "en-US", 200), true);
        expect_eq(trie.get(pool[0..32], "en-US"), 200 as i64);
        expect_eq(trie.key_count, 1 as usize);
        expect_eq(trie.node_count, nodes_before);
    });

    test("4. @cap/zerotrie — hasPrefix detection", () => {
        let pool: [64]TrieNode = [TrieNode.empty(); 64];
        let trie: ZeroTrie = ZeroTrie.new();
        * = trie.init(pool[0..64]);

        expect_eq(trie.hasPrefix(pool[0..64], ""), false);
        * = trie.insert(pool[0..64], "/api/v1/users", 1);
        * = trie.insert(pool[0..64], "/api/v2/items", 2);

        expect_eq(trie.hasPrefix(pool[0..64], ""), true);
        expect_eq(trie.hasPrefix(pool[0..64], "/api/"), true);
        expect_eq(trie.hasPrefix(pool[0..64], "/api/v1"), true);
        expect_eq(trie.hasPrefix(pool[0..64], "/api/v3"), false);
        expect_eq(trie.containsKey(pool[0..64], "/api/"), false);
    });

    test("5. @cap/zerotrie — longestPrefixMatch for routing & tokenization", () => {
        let pool: [64]TrieNode = [TrieNode.empty(); 64];
        let trie: ZeroTrie = ZeroTrie.new();
        * = trie.init(pool[0..64]);

        * = trie.insert(pool[0..64], "/", 1);
        * = trie.insert(pool[0..64], "/api", 2);
        * = trie.insert(pool[0..64], "/api/v1", 3);

        let m1: PrefixMatch = trie.longestPrefixMatch(pool[0..64], "/api/v1/users/42");
        expect_eq(m1.found, true);
        expect_eq(m1.matched_len, 7 as usize);
        expect_eq(m1.value, 3 as u32);

        let m2: PrefixMatch = trie.longestPrefixMatch(pool[0..64], "/api/v2/health");
        expect_eq(m2.found, true);
        expect_eq(m2.matched_len, 4 as usize);
        expect_eq(m2.value, 2 as u32);

        let m3: PrefixMatch = trie.longestPrefixMatch(pool[0..64], "nomatch");
        expect_eq(m3.found, false);
        expect_eq(m3.matched_len, 0 as usize);
    });

    test("6. @cap/zerotrie — empty key, missing key & pool-full atomic boundary", () => {
        let pool: [5]TrieNode = [TrieNode.empty(); 5];
        let trie: ZeroTrie = ZeroTrie.new();
        * = trie.init(pool[0..5]);

        // Empty key lives on root node (0 extra nodes)
        expect_eq(trie.insert(pool[0..5], "", 77), true);
        expect_eq(trie.get(pool[0..5], ""), 77 as i64);

        // "abcd" uses 4 nodes -> total 5/5 nodes (full)
        expect_eq(trie.insert(pool[0..5], "abcd", 88), true);
        expect_eq(trie.node_count, 5 as usize);

        // Next new key "xyz" cannot fit -> rejected atomically, existing keys intact
        expect_eq(trie.insert(pool[0..5], "xyz", 99), false);
        expect_eq(trie.get(pool[0..5], "xyz"), -1 as i64);
        expect_eq(trie.hasPrefix(pool[0..5], "x"), false);
        expect_eq(trie.get(pool[0..5], "abcd"), 88 as i64);
        expect_eq(trie.key_count, 2 as usize);
    });
}
