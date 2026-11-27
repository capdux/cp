// cap/test/testCapTrie.cp
// Comprehensive unit tests for @cap/trie

import { test, expect_eq } from "cap:test";
import {
    MAX_TRIE_NODES,
    TrieNode,
    LongestMatch,
    Trie,
} from "../../packages/radix_trie/src/index.cp";

fn main() {
    test("1. @cap/trie — Empty trie & empty key handling", () => {
        let t = Trie.init();
        expect_eq(t.len(), 0 as usize);
        expect_eq(t.nodeCount(), 1 as usize);
        expect_eq(t.contains(""), false);
        expect_eq(t.get(""), -1 as i64);
        expect_eq(t.hasPrefix(""), false);
        expect_eq(t.countWithPrefix(""), 0 as usize);

        expect_eq(t.insert("", 77), true);
        expect_eq(t.len(), 1 as usize);
        expect_eq(t.contains(""), true);
        expect_eq(t.get(""), 77 as i64);
        expect_eq(t.hasPrefix(""), true);
        expect_eq(t.countWithPrefix(""), 1 as usize);
    });

    test("2. @cap/trie — Insert, overwrite update, contains & negative value -1 distinction", () => {
        let t = Trie.init();
        expect_eq(t.insert("apple", 10), true);
        expect_eq(t.insert("banana", -1), true);
        expect_eq(t.len(), 2 as usize);

        expect_eq(t.get("apple"), 10 as i64);
        expect_eq(t.contains("banana"), true);
        expect_eq(t.get("banana"), -1 as i64);
        expect_eq(t.contains("missing"), false);
        expect_eq(t.get("missing"), -1 as i64);

        // Overwrite existing key does not increase len or nodeCount
        const nodes_before = t.nodeCount();
        expect_eq(t.insert("apple", 99), true);
        expect_eq(t.len(), 2 as usize);
        expect_eq(t.nodeCount(), nodes_before);
        expect_eq(t.get("apple"), 99 as i64);
    });

    test("3. @cap/trie — Shared prefixes, hasPrefix & countWithPrefix", () => {
        let t = Trie.init();
        expect_eq(t.insert("cap", 1), true);
        expect_eq(t.insert("cap:io", 2), true);
        expect_eq(t.insert("cap:net", 3), true);
        expect_eq(t.insert("cap:net:http", 4), true);
        expect_eq(t.insert("zig", 5), true);

        expect_eq(t.hasPrefix("cap"), true);
        expect_eq(t.countWithPrefix("cap"), 4 as usize);
        expect_eq(t.countWithPrefix("cap:"), 3 as usize);
        expect_eq(t.countWithPrefix("cap:net"), 2 as usize);
        expect_eq(t.countWithPrefix("zig"), 1 as usize);
        expect_eq(t.hasPrefix("rust"), false);
        expect_eq(t.countWithPrefix("rust"), 0 as usize);
    });

    test("4. @cap/trie — Longest Prefix Match (LPM) for HTTP/RPC routing", () => {
        let router = Trie.init();
        expect_eq(router.insert("/", 1), true);
        expect_eq(router.insert("/api", 2), true);
        expect_eq(router.insert("/api/v1", 3), true);
        expect_eq(router.insert("/api/v1/users", 4), true);

        const m1 = router.longestPrefixMatch("/api/v1/users/42/profile");
        expect_eq(m1.found, true);
        expect_eq(m1.matched_len, 13 as usize);
        expect_eq(m1.value, 4 as i64);

        const m2 = router.longestPrefixMatch("/api/v1/orders");
        expect_eq(m2.found, true);
        expect_eq(m2.matched_len, 7 as usize);
        expect_eq(m2.value, 3 as i64);

        const m3 = router.longestPrefixMatch("/healthz");
        expect_eq(m3.found, true);
        expect_eq(m3.matched_len, 1 as usize);
        expect_eq(m3.value, 1 as i64);

        const m4 = router.longestPrefixMatch("no-leading-slash");
        expect_eq(m4.found, false);
    });

    test("5. @cap/trie — Deletion accurately updates len, hasPrefix, countWithPrefix & LPM", () => {
        let t = Trie.init();
        expect_eq(t.insert("app", 1), true);
        expect_eq(t.insert("apple", 2), true);
        expect_eq(t.insert("apply", 3), true);

        expect_eq(t.remove("unknown"), false);
        expect_eq(t.remove("apple"), true);
        expect_eq(t.remove("apple"), false); // double delete returns false
        expect_eq(t.len(), 2 as usize);
        expect_eq(t.contains("apple"), false);
        expect_eq(t.hasPrefix("appl"), true);
        expect_eq(t.countWithPrefix("appl"), 1 as usize);

        // Remove remaining "apply" -> prefix "appl" has 0 live words now
        expect_eq(t.remove("apply"), true);
        expect_eq(t.hasPrefix("appl"), false);
        expect_eq(t.countWithPrefix("appl"), 0 as usize);
        expect_eq(t.countWithPrefix("app"), 1 as usize);

        // LPM now falls back to "app"
        const m = t.longestPrefixMatch("applesauce");
        expect_eq(m.found, true);
        expect_eq(m.matched_len, 3 as usize);
        expect_eq(m.value, 1 as i64);
    });

    test("6. @cap/trie — Static pool capacity guard & clear() reuse", () => {
        let t = Trie.init();
        // Insert keys of 100 bytes each; 5 keys = 500 nodes (+1 root = 501 <= 512), 6th exceeds 512
        let key: [100]u8 = undefined;
        let i: usize = 0;
        while (i < 100) {
            key[i] = 'a';
            i += 1;
        }
        let k: u8 = 0;
        while (k < 5) {
            key[0] = ('A' as u8) + k;
            expect_eq(t.insert(key[0..100], k as i64), true);
            k += 1;
        }
        expect_eq(t.nodeCount(), 501 as usize);

        // 6th 100-byte key requires 100 new nodes -> 601 > 512 -> rejected atomically
        key[0] = 'Z';
        expect_eq(t.insert(key[0..100], 99), false);
        expect_eq(t.len(), 5 as usize);
        expect_eq(t.nodeCount(), 501 as usize);

        // clear() resets static pool for full reuse
        t.clear();
        expect_eq(t.len(), 0 as usize);
        expect_eq(t.nodeCount(), 1 as usize);
        expect_eq(t.insert("reused", 42), true);
        expect_eq(t.get("reused"), 42 as i64);
    });
}
