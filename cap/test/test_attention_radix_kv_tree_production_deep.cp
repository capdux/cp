// Origin: Pure-CP Stdlib Test Suite (lib/attention/radix_kv_tree.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc SGLang RadixAttention KV-Cache Tree:
//   1. Insert, exact full-edge & intra-edge partial prefix matching, out_kv_blocks cap & miss guard
//   2. Mid-edge fork automatic Radix Edge Split (single & nested) with independent KV block verification
//   3. Mid-edge split under active lock_path (lock_ref & LRU timestamp inheritance onto suffix)
//   4. Long sequence (> 16 tokens) multi-node chunked storage, full match & deep mid-chunk split
//   5. lock_path in-flight protection vs strict leaf-first LRU eviction ordering across sibling branches
//   6. unlock_path atomic underflow guard, leaf-to-parent cascading reclamation & inline slot reuse
//   7. Idempotent duplicate / strict-prefix re-insertion & empty/mismatched input boundary guards
//   8. Full 127-data-node pool atomic overflow guard, eviction & multi-node slot reuse

import { test, expect_eq } from "cap:test";
import {
    RADIX_KV_MAX_NODES,
    RADIX_KV_MAX_EDGE_TOKENS,
    RADIX_KV_NO_NODE,
    RadixMatchResult,
    RadixKvStats,
    RadixKvCacheTree,
} from "cap:attention";

// ============================================================================
// Test 1: Insert, Full/Partial Prefix Match, Output Cap & Miss Guard
// ============================================================================
test("radix_kv_tree: insert, exact full & intra-edge partial prefix match", () => {
    let tree = RadixKvCacheTree.init();
    const s0: RadixKvStats = tree.stats();
    expect_eq(s0.active_nodes, 0);
    expect_eq(s0.cached_tokens, 0);
    expect_eq(s0.locked_nodes, 0);
    expect_eq(s0.total_hits, 0);
    expect_eq(s0.total_splits, 0);
    expect_eq(s0.total_evicted_tokens, 0);

    const toks: [6]u32 = [10, 20, 30, 40, 50, 60];
    const kvs: [6]u32 = [100, 101, 102, 103, 104, 105];
    const inserted = tree.insert_prefix(toks[0..], kvs[0..], 1);
    expect_eq(inserted, 6);

    const s1 = tree.stats();
    expect_eq(s1.active_nodes, 1);
    expect_eq(s1.cached_tokens, 6);

    // Exact full match
    let out_buf: [8]u32 = [0; 8];
    let out_s: []u32 = out_buf[0..];
    const m_full: RadixMatchResult = tree.match_prefix(toks[0..], 2, out_s);
    expect_eq(m_full.matched_tokens, 6);
    expect_eq(m_full.out_blocks_written, 6);
    expect_eq(m_full.last_node_idx > 0, true);
    expect_eq(out_buf[0], 100);
    expect_eq(out_buf[5], 105);

    // Intra-edge partial match (shorter query)
    const short_q: [3]u32 = [10, 20, 30];
    let part_buf: [8]u32 = [0; 8];
    let part_s: []u32 = part_buf[0..];
    const m_part = tree.match_prefix(short_q[0..], 3, part_s);
    expect_eq(m_part.matched_tokens, 3);
    expect_eq(m_part.out_blocks_written, 3);
    expect_eq(m_part.last_node_idx, m_full.last_node_idx);
    expect_eq(part_buf[0], 100);
    expect_eq(part_buf[2], 102);

    // Intra-edge partial match (diverging mid-edge)
    const div_q: [5]u32 = [10, 20, 30, 999, 888];
    let div_buf: [8]u32 = [0; 8];
    let div_s: []u32 = div_buf[0..];
    const m_div = tree.match_prefix(div_q[0..], 4, div_s);
    expect_eq(m_div.matched_tokens, 3);
    expect_eq(m_div.out_blocks_written, 3);
    expect_eq(div_buf[1], 101);

    // Caller output buffer smaller than matched tokens
    let tiny_buf: [2]u32 = [0; 2];
    let tiny_s: []u32 = tiny_buf[0..];
    const m_cap = tree.match_prefix(toks[0..], 5, tiny_s);
    expect_eq(m_cap.matched_tokens, 6);
    expect_eq(m_cap.out_blocks_written, 2);
    expect_eq(tiny_buf[0], 100);
    expect_eq(tiny_buf[1], 101);

    // Complete miss does not increment total_hits
    const miss_q: [2]u32 = [777, 888];
    let miss_buf: [4]u32 = [0; 4];
    let miss_s: []u32 = miss_buf[0..];
    const m_miss = tree.match_prefix(miss_q[0..], 6, miss_s);
    expect_eq(m_miss.matched_tokens, 0);
    expect_eq(m_miss.last_node_idx, 0);
    expect_eq(m_miss.out_blocks_written, 0);

    expect_eq(tree.stats().total_hits, 4);
});

// ============================================================================
// Test 2: Mid-Edge Fork Automatic Edge Split & Multi-Branch KV Verification
// ============================================================================
test("radix_kv_tree: mid-edge fork auto-split & dual/triple branch sharing", () => {
    let tree = RadixKvCacheTree.init();

    // Branch A: [10, 20, 30, 40, 50, 60]
    const seq_a: [6]u32 = [10, 20, 30, 40, 50, 60];
    const kv_a: [6]u32 = [100, 101, 102, 103, 104, 105];
    expect_eq(tree.insert_prefix(seq_a[0..], kv_a[0..], 1), 6);

    // Branch B forks after [10, 20, 30] -> triggers Edge Split!
    const seq_b: [5]u32 = [10, 20, 30, 77, 88];
    const kv_b: [5]u32 = [900, 901, 902, 200, 201];
    expect_eq(tree.insert_prefix(seq_b[0..], kv_b[0..], 2), 2);

    const st1 = tree.stats();
    expect_eq(st1.total_splits, 1);
    expect_eq(st1.active_nodes, 3); // [10,20,30], [40,50,60], [77,88]
    expect_eq(st1.cached_tokens, 8);

    // Branch C forks mid-edge inside A's suffix [40, 50, 60] -> nested Edge Split!
    const seq_c: [5]u32 = [10, 20, 30, 40, 99];
    const kv_c: [5]u32 = [900, 901, 902, 903, 300];
    expect_eq(tree.insert_prefix(seq_c[0..], kv_c[0..], 3), 1);

    const st2 = tree.stats();
    expect_eq(st2.total_splits, 2);
    expect_eq(st2.active_nodes, 5); // [10,20,30], [40], [50,60], [77,88], [99]
    expect_eq(st2.cached_tokens, 9);

    // Verify all 3 branches share prefix KV blocks & retain independent tail KV blocks
    let out_a: [8]u32 = [0; 8];
    let out_b: [8]u32 = [0; 8];
    let out_c: [8]u32 = [0; 8];
    let sa: []u32 = out_a[0..];
    let sb: []u32 = out_b[0..];
    let sc: []u32 = out_c[0..];

    const ma = tree.match_prefix(seq_a[0..], 10, sa);
    const mb = tree.match_prefix(seq_b[0..], 11, sb);
    const mc = tree.match_prefix(seq_c[0..], 12, sc);

    expect_eq(ma.matched_tokens, 6);
    expect_eq(out_a[0], 100);
    expect_eq(out_a[2], 102);
    expect_eq(out_a[3], 103);
    expect_eq(out_a[5], 105);

    expect_eq(mb.matched_tokens, 5);
    expect_eq(out_b[0], 100); // shared from A
    expect_eq(out_b[2], 102); // shared from A
    expect_eq(out_b[3], 200); // B's own tail
    expect_eq(out_b[4], 201);

    expect_eq(mc.matched_tokens, 5);
    expect_eq(out_c[0], 100);
    expect_eq(out_c[3], 103); // shared [40] from A
    expect_eq(out_c[4], 300); // C's own tail
});

// ============================================================================
// Test 3: Mid-Edge Split Under Active Lock (lock_ref & Timestamp Inheritance)
// ============================================================================
test("radix_kv_tree: edge split inherits lock_ref onto suffix node", () => {
    let tree = RadixKvCacheTree.init();

    const seq_a: [6]u32 = [1, 2, 3, 4, 5, 6];
    const kv_a: [6]u32 = [11, 12, 13, 14, 15, 16];
    expect_eq(tree.insert_prefix(seq_a[0..], kv_a[0..], 10), 6);

    let out_buf: [6]u32 = [0; 6];
    let out_s: []u32 = out_buf[0..];
    const m = tree.match_prefix(seq_a[0..], 10, out_s);
    expect_eq(tree.lock_path(m.last_node_idx), true);
    expect_eq(tree.stats().locked_nodes, 1);

    // Fork mid-edge of the locked node -> prefix & suffix must both stay locked!
    const seq_b: [5]u32 = [1, 2, 3, 9, 10];
    const kv_b: [5]u32 = [11, 12, 13, 29, 30];
    expect_eq(tree.insert_prefix(seq_b[0..], kv_b[0..], 20), 2);

    const st = tree.stats();
    expect_eq(st.total_splits, 1);
    expect_eq(st.active_nodes, 3);
    expect_eq(st.locked_nodes, 2); // prefix [1,2,3] + suffix [4,5,6]

    // Eviction must only remove unlocked branch B ([9,10]), never locked suffix [4,5,6]
    let freed_buf: [8]u32 = [0; 8];
    let freed_s: []u32 = freed_buf[0..];
    const freed = tree.evict_lru_tokens(10, freed_s);
    expect_eq(freed, 2);
    expect_eq(freed_buf[0], 29);
    expect_eq(freed_buf[1], 30);

    // Original locked sequence A remains 100% intact
    let verify_buf: [6]u32 = [0; 6];
    let verify_s: []u32 = verify_buf[0..];
    const mv = tree.match_prefix(seq_a[0..], 25, verify_s);
    expect_eq(mv.matched_tokens, 6);
    expect_eq(verify_buf[5], 16);
});

// ============================================================================
// Test 4: Long Sequence (> 16 tokens) Chunked Insertion & Deep Mid-Chunk Split
// ============================================================================
test("radix_kv_tree: long sequence >16 tokens chunked storage & deep split", () => {
    let tree = RadixKvCacheTree.init();

    // 37 tokens -> 16 + 16 + 5 = 3 chained nodes
    let seq_long: [37]u32 = [0; 37];
    let kv_long: [37]u32 = [0; 37];
    let i: usize = 0;
    while (i < 37) : (i += 1) {
        seq_long[i] = @intCast(1000 + i);
        kv_long[i] = @intCast(5000 + i);
    }

    expect_eq(tree.insert_prefix(seq_long[0..], kv_long[0..], 1), 37);
    const st1 = tree.stats();
    expect_eq(st1.active_nodes, 3);
    expect_eq(st1.cached_tokens, 37);
    expect_eq(RADIX_KV_MAX_EDGE_TOKENS, 16);

    // Match full 37-token sequence
    let out_long: [40]u32 = [0; 40];
    let out_long_s: []u32 = out_long[0..];
    const ml = tree.match_prefix(seq_long[0..], 2, out_long_s);
    expect_eq(ml.matched_tokens, 37);
    expect_eq(ml.out_blocks_written, 37);
    let j: usize = 0;
    while (j < 37) : (j += 1) {
        expect_eq(out_long[j], @intCast(5000 + j));
    }

    // Fork at index 20 (inside the second 16-token chunk) with 25-token branch
    let seq_fork: [25]u32 = [0; 25];
    let kv_fork: [25]u32 = [0; 25];
    let k: usize = 0;
    while (k < 25) : (k += 1) {
        if (k < 20) {
            seq_fork[k] = @intCast(1000 + k);
            kv_fork[k] = @intCast(5000 + k);
        } else {
            seq_fork[k] = @intCast(9000 + k);
            kv_fork[k] = @intCast(8000 + k);
        }
    }

    expect_eq(tree.insert_prefix(seq_fork[0..], kv_fork[0..], 3), 5);
    const st2 = tree.stats();
    expect_eq(st2.total_splits, 1);
    expect_eq(st2.active_nodes, 5); // 16 + 4 + 12 + 5 + 5(fork)
    expect_eq(st2.cached_tokens, 42);

    // Verify both full 37-token path and 25-token forked path match accurately
    let out_a: [40]u32 = [0; 40];
    let out_b: [40]u32 = [0; 40];
    let oa_s: []u32 = out_a[0..];
    let ob_s: []u32 = out_b[0..];
    expect_eq(tree.match_prefix(seq_long[0..], 4, oa_s).matched_tokens, 37);
    expect_eq(tree.match_prefix(seq_fork[0..], 5, ob_s).matched_tokens, 25);
    expect_eq(out_a[36], 5036);
    expect_eq(out_b[19], 5019);
    expect_eq(out_b[20], 8020);
    expect_eq(out_b[24], 8024);
});

// ============================================================================
// Test 5: lock_path Protection vs Strict Leaf-First LRU Eviction Ordering
// ============================================================================
test("radix_kv_tree: lock_path protects hot/in-flight branch while cold leaves evict in LRU order", () => {
    let tree = RadixKvCacheTree.init();

    // Shared prompt [1, 2, 3] + 3 sibling branches at steps 5, 10, 20
    const seq_a: [5]u32 = [1, 2, 3, 101, 102];
    const kv_a: [5]u32 = [10, 11, 12, 21, 22];
    const seq_b: [5]u32 = [1, 2, 3, 201, 202];
    const kv_b: [5]u32 = [10, 11, 12, 31, 32];
    const seq_c: [5]u32 = [1, 2, 3, 301, 302];
    const kv_c: [5]u32 = [10, 11, 12, 41, 42];

    expect_eq(tree.insert_prefix(seq_a[0..], kv_a[0..], 5), 5);
    expect_eq(tree.insert_prefix(seq_b[0..], kv_b[0..], 10), 2);
    expect_eq(tree.insert_prefix(seq_c[0..], kv_c[0..], 20), 2);

    // Match & lock coldest branch A (step 5) to simulate an active in-flight request
    let tmp: [5]u32 = [0; 5];
    let tmp_s: []u32 = tmp[0..];
    const ma = tree.match_prefix(seq_a[0..], 5, tmp_s);
    expect_eq(ma.matched_tokens, 5);
    expect_eq(tree.lock_path(ma.last_node_idx), true);
    expect_eq(tree.stats().locked_nodes, 2); // shared [1,2,3] + branch A [101,102]

    // First eviction of 2 tokens must choose branch B (step 10 < step 20, A is locked)
    let freed1: [4]u32 = [0; 4];
    let f1_s: []u32 = freed1[0..];
    expect_eq(tree.evict_lru_tokens(2, f1_s), 2);
    expect_eq(freed1[0], 31);
    expect_eq(freed1[1], 32);

    // Second eviction of 2 tokens must choose branch C (step 20)
    let freed2: [4]u32 = [0; 4];
    let f2_s: []u32 = freed2[0..];
    expect_eq(tree.evict_lru_tokens(2, f2_s), 2);
    expect_eq(freed2[0], 41);
    expect_eq(freed2[1], 42);

    // Third eviction must yield 0 because only locked branch A + shared parent remain
    let freed3: [4]u32 = [0; 4];
    let f3_s: []u32 = freed3[0..];
    expect_eq(tree.evict_lru_tokens(10, f3_s), 0);

    const st = tree.stats();
    expect_eq(st.active_nodes, 2);
    expect_eq(st.cached_tokens, 5);
    expect_eq(st.total_evicted_tokens, 4);

    // Locked branch A still matches completely
    let verify: [5]u32 = [0; 5];
    let v_s: []u32 = verify[0..];
    expect_eq(tree.match_prefix(seq_a[0..], 30, v_s).matched_tokens, 5);
    expect_eq(verify[3], 21);
    expect_eq(verify[4], 22);
});

// ============================================================================
// Test 6: unlock_path Underflow Guard, Cascading Parent Reclamation & Slot Reuse
// ============================================================================
test("radix_kv_tree: unlock_path cascading reclamation, underflow guard & slot reuse", () => {
    let tree = RadixKvCacheTree.init();

    // Build a 3-node chain: 16 + 16 + 4 = 36 tokens
    let seq: [36]u32 = [0; 36];
    let kv: [36]u32 = [0; 36];
    let i: usize = 0;
    while (i < 36) : (i += 1) {
        seq[i] = @intCast(100 + i);
        kv[i] = @intCast(200 + i);
    }
    expect_eq(tree.insert_prefix(seq[0..], kv[0..], 1), 36);

    let out: [36]u32 = [0; 36];
    let out_s: []u32 = out[0..];
    const m = tree.match_prefix(seq[0..], 2, out_s);
    expect_eq(m.matched_tokens, 36);

    // Invalid lock/unlock guards
    expect_eq(tree.lock_path(0), false);
    expect_eq(tree.lock_path(RADIX_KV_NO_NODE), false);
    expect_eq(tree.unlock_path(0), false);
    expect_eq(tree.unlock_path(m.last_node_idx), false); // not locked yet -> underflow guard

    // Lock entire 3-node chain, verify blocked eviction, then unlock
    expect_eq(tree.lock_path(m.last_node_idx), true);
    expect_eq(tree.stats().locked_nodes, 3);

    let freed_blocked: [4]u32 = [0; 4];
    let fb_s: []u32 = freed_blocked[0..];
    expect_eq(tree.evict_lru_tokens(4, fb_s), 0);

    expect_eq(tree.unlock_path(m.last_node_idx), true);
    expect_eq(tree.unlock_path(m.last_node_idx), false); // second unlock prevented
    expect_eq(tree.stats().locked_nodes, 0);

    // Cascading eviction of all 36 tokens: leaf (4) -> middle (16) -> top (16)
    let freed_all: [36]u32 = [0; 36];
    let fa_s: []u32 = freed_all[0..];
    const n_freed = tree.evict_lru_tokens(36, fa_s);
    expect_eq(n_freed, 36);
    // First evicted node is the tail leaf (kv 232..235)
    expect_eq(freed_all[0], 232);
    expect_eq(freed_all[3], 235);
    // Last evicted node is the top ancestor (kv 200..215)
    expect_eq(freed_all[20], 200);
    expect_eq(freed_all[35], 215);

    const st_empty = tree.stats();
    expect_eq(st_empty.active_nodes, 0);
    expect_eq(st_empty.cached_tokens, 0);
    expect_eq(st_empty.total_evicted_tokens, 36);

    // Reuse reclaimed slots cleanly
    const fresh_tok: [3]u32 = [7, 8, 9];
    const fresh_kv: [3]u32 = [70, 80, 90];
    expect_eq(tree.insert_prefix(fresh_tok[0..], fresh_kv[0..], 50), 3);
    expect_eq(tree.stats().active_nodes, 1);
    expect_eq(tree.stats().cached_tokens, 3);
});

// ============================================================================
// Test 7: Idempotent Re-Insertion & Empty / Length-Mismatch Input Guards
// ============================================================================
test("radix_kv_tree: idempotence & empty/mismatched boundary guards", () => {
    let tree = RadixKvCacheTree.init();

    const empty_u32: [0]u32 = [];
    const one_u32: [1]u32 = [42];
    const two_u32: [2]u32 = [42, 43];

    // Empty & mismatched inputs rejected cleanly
    expect_eq(tree.insert_prefix(empty_u32[0..], empty_u32[0..], 1), 0);
    expect_eq(tree.insert_prefix(one_u32[0..], two_u32[0..], 1), 0);
    expect_eq(tree.insert_prefix(two_u32[0..], one_u32[0..], 1), 0);
    expect_eq(tree.stats().active_nodes, 0);

    let out_buf: [4]u32 = [0; 4];
    let out_s: []u32 = out_buf[0..];
    const m_empty = tree.match_prefix(empty_u32[0..], 1, out_s);
    expect_eq(m_empty.matched_tokens, 0);
    expect_eq(m_empty.last_node_idx, 0);
    expect_eq(tree.evict_lru_tokens(0, out_s), 0);

    // Insert [10, 20, 30, 40]
    const seq: [4]u32 = [10, 20, 30, 40];
    const kv: [4]u32 = [1, 2, 3, 4];
    expect_eq(tree.insert_prefix(seq[0..], kv[0..], 10), 4);

    // Exact duplicate insert is idempotent (0 new tokens, 0 splits)
    expect_eq(tree.insert_prefix(seq[0..], kv[0..], 20), 0);
    // Strict prefix re-insert is idempotent (0 new tokens, 0 splits)
    expect_eq(tree.insert_prefix(seq[0..2], kv[0..2], 30), 0);

    const st = tree.stats();
    expect_eq(st.active_nodes, 1);
    expect_eq(st.cached_tokens, 4);
    expect_eq(st.total_splits, 0);
});

// ============================================================================
// Test 8: Full 127-Node Pool Atomic Overflow Guard & Evict-Then-Reuse
// ============================================================================
test("radix_kv_tree: full node pool atomic overflow guard & slot reuse", () => {
    let tree = RadixKvCacheTree.init();
    const max_data_nodes: usize = RADIX_KV_MAX_NODES - 1; // 127

    // Fill all 127 data node slots with distinct 1-token root children
    let i: usize = 0;
    while (i < max_data_nodes) : (i += 1) {
        const t: [1]u32 = [@intCast(i + 1)];
        const b: [1]u32 = [@intCast(5000 + i + 1)];
        expect_eq(tree.insert_prefix(t[0..], b[0..], @intCast(i + 1)), 1);
    }

    const st_full = tree.stats();
    expect_eq(st_full.active_nodes, max_data_nodes);
    expect_eq(st_full.cached_tokens, max_data_nodes);

    // Overflow insert (1 node needed) must fail atomically returning 0
    const extra_t: [1]u32 = [9999];
    const extra_b: [1]u32 = [8888];
    expect_eq(tree.insert_prefix(extra_t[0..], extra_b[0..], 200), 0);
    expect_eq(tree.stats().active_nodes, max_data_nodes);

    // Evict 1 coldest leaf (step 1 -> kv 5001), leaving only 1 free slot
    let freed1: [2]u32 = [0; 2];
    let f1_s: []u32 = freed1[0..];
    expect_eq(tree.evict_lru_tokens(1, f1_s), 1);
    expect_eq(freed1[0], 5001);
    expect_eq(tree.stats().active_nodes, max_data_nodes - 1);

    // Attempt 20-token insert (needs 2 nodes, only 1 free) -> atomic rollback-free failure!
    let seq20: [20]u32 = [0; 20];
    let kv20: [20]u32 = [0; 20];
    let k: usize = 0;
    while (k < 20) : (k += 1) {
        seq20[k] = @intCast(7000 + k);
        kv20[k] = @intCast(9000 + k);
    }
    expect_eq(tree.insert_prefix(seq20[0..], kv20[0..], 201), 0);
    expect_eq(tree.stats().active_nodes, max_data_nodes - 1); // zero partial nodes leaked

    // Evict 1 more coldest leaf (step 2 -> kv 5002), now 2 free slots available
    let freed2: [2]u32 = [0; 2];
    let f2_s: []u32 = freed2[0..];
    expect_eq(tree.evict_lru_tokens(1, f2_s), 1);
    expect_eq(freed2[0], 5002);

    // Now the 20-token (2-node) insert succeeds cleanly!
    expect_eq(tree.insert_prefix(seq20[0..], kv20[0..], 202), 20);
    expect_eq(tree.stats().active_nodes, max_data_nodes);
    expect_eq(tree.stats().cached_tokens, (max_data_nodes - 2) + 20);

    let out20: [20]u32 = [0; 20];
    let out20_s: []u32 = out20[0..];
    const m20 = tree.match_prefix(seq20[0..], 203, out20_s);
    expect_eq(m20.matched_tokens, 20);
    expect_eq(out20[0], 9000);
    expect_eq(out20[19], 9019);
});
