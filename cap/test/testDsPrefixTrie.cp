// Origin: Pure-CP Stdlib Test Suite (lib/ds/prefixTrie.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Path-Compressed Byte Prefix Trie (Patricia/Radix KV):
//   1. Edge splitting (mid-edge prefix & branch splits), exact lookup & in-place value overwrite
//   2. Exact removal with empty-leaf reclamation & single-child internal node coalescing
//   3. O(L) Longest Prefix Match (LPM) for hierarchical routing/policy tables
//   4. Strictly lexicographical iterative prefix range scan (mid-edge prefix & full-tree scan)
//   5. Output buffer truncation telemetry (written, total_matched, truncated)
//   6. Atomic pool-exhaustion guard (no partial corruption; existing key overwrite still succeeds)
//   7. Empty/oversized key guards, clear() reset & cap.ds.PrefixTrie facade verification

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    PREFIX_TRIE_MAX_KEY_LEN,
    PrefixScanHit,
    PrefixTrie,
} from "cap:ds";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Compressed Edge Splitting, Exact Lookup & Overwrite Update
// ============================================================================
test("prefixTrie: edge splitting, prefix insertion, exact get & overwrite update", () => {
    let trie = PrefixTrie.init();
    expect_true(trie.isEmpty());

    expect_true(trie.insert("apple", 1));
    expect_true(trie.insert("apply", 2));    // splits "appl" -> "e", "y"
    expect_true(trie.insert("app", 3));      // splits "appl" -> "app" (val=3) -> "l"
    expect_true(trie.insert("apricot", 4));  // splits "app" -> "ap" -> "p", "ricot"
    expect_true(trie.insert("banana", 5));
    expect_eq(trie.len(), 5);

    expect_eq(trie.get("apple") ?? 0, 1);
    expect_eq(trie.get("apply") ?? 0, 2);
    expect_eq(trie.get("app") ?? 0, 3);
    expect_eq(trie.get("apricot") ?? 0, 4);
    expect_eq(trie.get("banana") ?? 0, 5);
    expect_true(trie.get("ap") == null);
    expect_true(trie.get("appl") == null);
    expect_true(trie.get("apples") == null);

    // Overwrite existing key does not change entry count
    expect_true(trie.insert("app", 333));
    expect_eq(trie.len(), 5);
    expect_eq(trie.get("app") ?? 0, 333);
});

// ============================================================================
// Test 2: Exact Removal & Single-Child Internal Node Coalescing
// ============================================================================
test("prefixTrie: remove deletes keys and coalesces single-child internal nodes", () => {
    let trie = PrefixTrie.init();
    expect_true(trie.insert("foobar", 10));
    expect_true(trie.insert("foobaz", 20));
    expect_true(trie.insert("fooqux", 30));
    const nodes_before: usize = trie.stats().active_nodes;

    // Removing non-existent key returns false
    expect_true(!trie.remove("foo"));
    expect_true(!trie.remove("unknown"));

    // Remove "foobaz": leaf freed + "fooba" internal node coalesced into "foobar"
    expect_true(trie.remove("foobaz"));
    expect_eq(trie.len(), 2);
    expect_true(trie.get("foobaz") == null);
    expect_eq(trie.get("foobar") ?? 0, 10);
    expect_eq(trie.get("fooqux") ?? 0, 30);
    expect_true(trie.stats().active_nodes < nodes_before);

    // Remove remaining keys -> back to only root node (active_nodes == 1)
    expect_true(trie.remove("foobar"));
    expect_true(trie.remove("fooqux"));
    expect_true(trie.isEmpty());
    expect_eq(trie.stats().active_nodes, 1);
});

// ============================================================================
// Test 3: O(L) Longest Prefix Match (LPM) for Routing / Policy Tables
// ============================================================================
test("prefixTrie: longestPrefixMatch resolves hierarchical route prefixes accurately", () => {
    let trie = PrefixTrie.init();
    expect_true(trie.insert("/", 1));
    expect_true(trie.insert("/api", 2));
    expect_true(trie.insert("/api/v1", 3));
    expect_true(trie.insert("/api/v1/users", 4));
    expect_true(trie.insert("/api/v2", 5));

    if (m1 := trie.longestPrefixMatch("/api/v1/users/12345/profile")) {
        expect_eq(m1.value, 4);
        expect_eq(m1.matched_len, 13);
    } else {
        expect_true(false);
    }

    if (m2 := trie.longestPrefixMatch("/api/v1/orders")) {
        expect_eq(m2.value, 3);
        expect_eq(m2.matched_len, 7);
    } else {
        expect_true(false);
    }

    if (m3 := trie.longestPrefixMatch("/api/v3/health")) {
        expect_eq(m3.value, 2);
        expect_eq(m3.matched_len, 4);
    } else {
        expect_true(false);
    }

    if (m4 := trie.longestPrefixMatch("/static/logo.png")) {
        expect_eq(m4.value, 1);
        expect_eq(m4.matched_len, 1);
    } else {
        expect_true(false);
    }

    // No matching prefix at all
    expect_true(trie.longestPrefixMatch("http://example.com") == null);
    expect_true(trie.longestPrefixMatch("") == null);
});

// ============================================================================
// Test 4: Lexicographical Iterative Prefix Range Scan (Mid-Edge & Full-Tree)
// ============================================================================
test("prefixTrie: scanPrefix enumerates matching keys in strict lexicographical order", () => {
    let trie = PrefixTrie.init();
    // Insert deliberately out of lexicographical order
    expect_true(trie.insert("zebra", 9));
    expect_true(trie.insert("apply", 3));
    expect_true(trie.insert("apple", 2));
    expect_true(trie.insert("apricot", 4));
    expect_true(trie.insert("app", 1));
    expect_true(trie.insert("banana", 5));

    // Scan mid-edge prefix "appl" -> ["apple", "apply"]
    let hits: [8]PrefixScanHit = [PrefixScanHit{}; 8];
    let hits_s: []PrefixScanHit = hits[0..];
    const r_mid = trie.scanPrefix("appl", *hits_s);
    expect_eq(r_mid.written, 2);
    expect_eq(r_mid.total_matched, 2);
    expect_true(!r_mid.truncated);
    expect_true(bytesEq(hits[0].keySlice(), "apple"));
    expect_eq(hits[0].value, 2);
    expect_true(bytesEq(hits[1].keySlice(), "apply"));
    expect_eq(hits[1].value, 3);

    // Scan empty prefix "" -> entire trie in strict lexicographical order
    const r_all = trie.scanPrefix("", *hits_s);
    expect_eq(r_all.written, 6);
    expect_eq(r_all.total_matched, 6);
    expect_true(!r_all.truncated);
    expect_true(bytesEq(hits[0].keySlice(), "app"));
    expect_true(bytesEq(hits[1].keySlice(), "apple"));
    expect_true(bytesEq(hits[2].keySlice(), "apply"));
    expect_true(bytesEq(hits[3].keySlice(), "apricot"));
    expect_true(bytesEq(hits[4].keySlice(), "banana"));
    expect_true(bytesEq(hits[5].keySlice(), "zebra"));
});

// ============================================================================
// Test 5: Output Buffer Truncation Telemetry
// ============================================================================
test("prefixTrie: scanPrefix reports written, total_matched & truncated when buffer is small", () => {
    let trie = PrefixTrie.init();
    expect_true(trie.insert("k/5", 5));
    expect_true(trie.insert("k/1", 1));
    expect_true(trie.insert("k/4", 4));
    expect_true(trie.insert("k/2", 2));
    expect_true(trie.insert("k/3", 3));

    let small: [2]PrefixScanHit = [PrefixScanHit{}; 2];
    let small_s: []PrefixScanHit = small[0..];
    const rep = trie.scanPrefix("k/", *small_s);
    expect_eq(rep.written, 2);
    expect_eq(rep.total_matched, 5);
    expect_true(rep.truncated);
    expect_true(bytesEq(small[0].keySlice(), "k/1"));
    expect_true(bytesEq(small[1].keySlice(), "k/2"));
});

// ============================================================================
// Test 6: Atomic Pool-Exhaustion Guard
// ============================================================================
test("prefixTrie: byte-pool exhaustion rejects atomically while preserving existing entries", () => {
    let trie = PrefixTrie.init();
    // Each unique 64-byte key consumes 64 bytes -> 64 keys fill the 4096-byte pool completely
    let key_buf: [64]u8 = ['a'; 64];
    let inserted: usize = 0;
    let i: usize = 0;
    while (i < 70) : (i += 1) {
        key_buf[0] = ('!' as usize + i) as u8;
        if (trie.insert(key_buf[0..], (i + 1) as u64)) {
            inserted += 1;
        }
    }
    expect_eq(inserted, 64);
    expect_eq(trie.len(), 64);
    expect_eq(trie.stats().used_edge_bytes, 4096);

    // Overwriting an existing key still succeeds even with 0 bytes left in byte pool!
    key_buf[0] = '!';
    expect_true(trie.insert(key_buf[0..], 9999));
    expect_eq(trie.get(key_buf[0..]) ?? 0, 9999);
});

// ============================================================================
// Test 7: Boundary Guards (Empty / Oversized Keys), clear() & cap.ds Facade
// ============================================================================
test("prefixTrie: rejects empty & oversized keys, clear() resets pools & cap.ds facade works", () => {
    let trie = cap.ds.PrefixTrie.init();

    // Empty key rejected
    expect_true(!trie.insert("", 1));
    expect_true(trie.get("") == null);
    expect_true(!trie.remove(""));

    // Oversized key (> PREFIX_TRIE_MAX_KEY_LEN == 128) rejected
    const oversized: [129]u8 = ['x'; 129];
    expect_eq(PREFIX_TRIE_MAX_KEY_LEN, 128);
    expect_true(!trie.insert(oversized[0..], 1));
    expect_true(trie.get(oversized[0..]) == null);

    // Max-length 128-byte key accepted
    const max_key: [128]u8 = ['z'; 128];
    expect_true(trie.insert(max_key[0..], 77));
    expect_eq(trie.get(max_key[0..]) ?? 0, 77);

    // clear() resets entries, nodes & byte pool
    trie.clear();
    expect_true(trie.isEmpty());
    expect_eq(trie.stats().active_nodes, 1);
    expect_eq(trie.stats().used_edge_bytes, 0);
    expect_true(trie.insert("fresh", 88));
    expect_eq(trie.get("fresh") ?? 0, 88);
});
