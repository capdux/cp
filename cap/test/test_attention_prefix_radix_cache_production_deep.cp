// Production Deep Test Suite for cap:attention PrefixRadixCache (lib/attention/prefix_radix_cache.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    PRC_MAX_NODES,
    PRC_MAX_POOL_SLOTS,
    PRC_NO_NODE,
    PRC_ROOT_NODE,
    PrefixRadixCache,
} from "cap:attention";

// ============================================================================
// Test 1: Insert, exact match & partial-edge prefix match
// ============================================================================
test("prefix_radix_cache: insert, exact match & partial-edge prefix match", fn() {
    let cache = PrefixRadixCache.init();

    const seq1: [5]u32 = [1, 2, 3, 4, 5];
    const pages1: [5]u32 = [10, 11, 12, 13, 14];
    const ins = cache.insertPrefix(seq1[0..], pages1[0..], 100);
    expect_true(ins.ok);
    expect_eq(ins.reused_tokens, 0 as usize);
    expect_eq(ins.inserted_tokens, 5 as usize);
    expect_true(!ins.split_performed);

    // Exact match
    let out: [8]u32 = [0; 8];
    let out_s: []u32 = out[0..];
    const m_full = cache.matchPrefix(seq1[0..], 110, *out_s);
    expect_eq(m_full.matched_tokens, 5 as usize);
    expect_eq(m_full.matched_pages, 5 as usize);
    expect_eq(m_full.leaf_node_id, ins.leaf_node_id);
    expect_eq(m_full.split_offset, 0 as u32);
    expect_eq(out[0], 10 as u32);
    expect_eq(out[4], 14 as u32);

    // Partial-edge prefix match (matches [1, 2, 3] then diverges at token 99)
    const partial_q: [4]u32 = [1, 2, 3, 99];
    const m_part = cache.matchPrefix(partial_q[0..], 120, *out_s);
    expect_eq(m_part.matched_tokens, 3 as usize);
    expect_eq(m_part.matched_pages, 3 as usize);
    expect_eq(m_part.leaf_node_id, ins.leaf_node_id);
    expect_eq(m_part.split_offset, 3 as u32);
    expect_eq(out[0], 10 as u32);
    expect_eq(out[2], 12 as u32);
});

// ============================================================================
// Test 2: O(1) offset edge split preserving shared prefix & divergent suffixes
// ============================================================================
test("prefix_radix_cache: O(1) offset edge split preserving shared prefix & suffixes", fn() {
    let cache = PrefixRadixCache.init();

    const seq_a: [5]u32 = [10, 20, 30, 40, 50];
    const pages_a: [5]u32 = [100, 101, 102, 103, 104];
    const r_a = cache.insertPrefix(seq_a[0..], pages_a[0..], 1);
    expect_true(r_a.ok);

    // Diverge after 2 shared tokens [10, 20] -> splits into internal [10, 20] + leaves [30,40,50] & [99,100]
    const seq_b: [4]u32 = [10, 20, 99, 100];
    const pages_b: [4]u32 = [100, 101, 202, 203];
    const r_b = cache.insertPrefix(seq_b[0..], pages_b[0..], 2);
    expect_true(r_b.ok);
    expect_true(r_b.split_performed);
    expect_eq(r_b.reused_tokens, 2 as usize);
    expect_eq(r_b.inserted_tokens, 2 as usize);

    // Total cached tokens = 5 + 2 = 7 (shared prefix [10, 20] not duplicated!)
    expect_eq(cache.stats().cached_tokens, 7 as usize);
    expect_eq(cache.stats().active_nodes, 4 as usize); // root + mid + leaf_a + leaf_b

    // Verify both branches match cleanly with exact KV pages
    let out: [8]u32 = [0; 8];
    let out_s: []u32 = out[0..];
    const ma = cache.matchPrefix(seq_a[0..], 5, *out_s);
    expect_eq(ma.matched_tokens, 5 as usize);
    expect_eq(out[0], 100 as u32);
    expect_eq(out[2], 102 as u32);
    expect_eq(out[4], 104 as u32);

    const mb = cache.matchPrefix(seq_b[0..], 6, *out_s);
    expect_eq(mb.matched_tokens, 4 as usize);
    expect_eq(out[0], 100 as u32);
    expect_eq(out[2], 202 as u32);
    expect_eq(out[3], 203 as u32);

    // Inserting a shorter prefix [10] splits mid [10, 20] cleanly with 0 inserted tokens
    const seq_short: [1]u32 = [10];
    const pages_short: [1]u32 = [100];
    const r_s = cache.insertPrefix(seq_short[0..], pages_short[0..], 7);
    expect_true(r_s.ok);
    expect_true(r_s.split_performed);
    expect_eq(r_s.reused_tokens, 1 as usize);
    expect_eq(r_s.inserted_tokens, 0 as usize);
    expect_eq(cache.stats().cached_tokens, 7 as usize);
});

// ============================================================================
// Test 3: Ancestor-chain pinNode/unpinNode & eviction protection
// ============================================================================
test("prefix_radix_cache: ancestor-chain pinNode/unpinNode & eviction protection", fn() {
    let cache = PrefixRadixCache.init();

    const seq_a: [4]u32 = [1, 2, 3, 4];
    const pages_a: [4]u32 = [10, 11, 12, 13];
    const seq_b: [4]u32 = [1, 2, 8, 9];
    const pages_b: [4]u32 = [10, 11, 80, 81];

    const ra = cache.insertPrefix(seq_a[0..], pages_a[0..], 1); // older tick = 1
    const rb = cache.insertPrefix(seq_b[0..], pages_b[0..], 10); // newer tick = 10

    // Pin branch A (older): pins leaf A AND shared parent [1, 2]
    expect_true(cache.pinNode(ra.leaf_node_id));
    expect_eq(cache.stats().pinned_nodes, 2 as usize);

    // Request eviction of 10 pages -> ONLY unpinned leaf B (80, 81) can be evicted; A & shared parent stay intact!
    let freed: [8]u32 = [0; 8];
    let freed_s: []u32 = freed[0..];
    const ev1 = cache.evictPages(10, *freed_s);
    expect_eq(ev1.freed_pages, 2 as usize);
    expect_eq(ev1.evicted_nodes, 1 as usize);
    expect_eq(ev1.remaining_unpinned_pages, 0 as usize);
    expect_eq(freed[0], 80 as u32);
    expect_eq(freed[1], 81 as u32);

    // Unpin branch A -> now leaf A and shared parent can be evicted bottom-up!
    expect_true(cache.unpinNode(ra.leaf_node_id));
    expect_eq(cache.stats().pinned_nodes, 0 as usize);

    const ev2 = cache.evictPages(10, *freed_s);
    expect_eq(ev2.freed_pages, 4 as usize); // leaf A (2 pages) + now-leaf shared parent (2 pages)
    expect_eq(cache.stats().cached_tokens, 0 as usize);
    expect_eq(cache.stats().active_nodes, 1 as usize); // only root remains
});

// ============================================================================
// Test 4: Bottom-up LRU leaf eviction order & tick refresh on matchPrefix
// ============================================================================
test("prefix_radix_cache: bottom-up LRU leaf eviction order & tick refresh on match", fn() {
    let cache = PrefixRadixCache.init();

    const s1: [2]u32 = [100, 1];
    const p1: [2]u32 = [1000, 1001];
    const s2: [2]u32 = [200, 2];
    const p2: [2]u32 = [2000, 2001];
    const s3: [2]u32 = [300, 3];
    const p3: [2]u32 = [3000, 3001];

    * = cache.insertPrefix(s1[0..], p1[0..], 10);
    * = cache.insertPrefix(s2[0..], p2[0..], 20);
    * = cache.insertPrefix(s3[0..], p3[0..], 30);

    // Touch s1 at tick=50 -> now s2 (tick=20) is the coldest LRU leaf!
    let out: [4]u32 = [0; 4];
    let out_s: []u32 = out[0..];
    * = cache.matchPrefix(s1[0..], 50, *out_s);

    let freed: [4]u32 = [0; 4];
    let freed_s: []u32 = freed[0..];
    const ev = cache.evictPages(2, *freed_s);
    expect_eq(ev.freed_pages, 2 as usize);
    expect_eq(freed[0], 2000 as u32);
    expect_eq(freed[1], 2001 as u32);

    // Verify s2 is gone while s1 and s3 still match
    expect_eq(cache.matchPrefix(s2[0..], 60, *out_s).matched_tokens, 0 as usize);
    expect_eq(cache.matchPrefix(s1[0..], 60, *out_s).matched_tokens, 2 as usize);
    expect_eq(cache.matchPrefix(s3[0..], 60, *out_s).matched_tokens, 2 as usize);
});

// ============================================================================
// Test 5: Zero-heap in-place pool compaction under repeated insert/evict churn
// ============================================================================
test("prefix_radix_cache: zero-heap in-place pool compaction under insert/evict churn", fn() {
    let cache = PrefixRadixCache.init();

    // Repeatedly insert 256-token chunks and evict them across 12 rounds (3072 total > 2048 pool!)
    let toks: [256]u32 = [0; 256];
    let pgs: [256]u32 = [0; 256];
    let freed: [256]u32 = [0; 256];
    let freed_s: []u32 = freed[0..];

    let round: u32 = 0;
    while (round < 12) : (round += 1) {
        let i: usize = 0;
        while (i < 256) : (i += 1) {
            toks[i] = round * 1000 + (i as u32);
            pgs[i] = round * 1000 + (i as u32);
        }
        const r = cache.insertPrefix(toks[0..], pgs[0..], (round + 1) as u64);
        expect_true(r.ok);

        // Keep only the latest 2 rounds cached by evicting older round once > 512 tokens
        if (cache.stats().cached_tokens > 512) {
            const ev = cache.evictPages(256, *freed_s);
            expect_eq(ev.freed_pages, 256 as usize);
        }
    }

    // Compaction kept pool_used within capacity and latest round still matches intact
    expect_true(cache.stats().used_pool_slots <= PRC_MAX_POOL_SLOTS);
    let out: [256]u32 = [0; 256];
    let out_s: []u32 = out[0..];
    const m = cache.matchPrefix(toks[0..], 100, *out_s);
    expect_eq(m.matched_tokens, 256 as usize);
    expect_eq(out[255], 11255 as u32);
});

// ============================================================================
// Test 6: Full node pool capacity exhaustion guard
// ============================================================================
test("prefix_radix_cache: full node pool capacity exhaustion guard", fn() {
    let cache = PrefixRadixCache.init();

    // Root uses slot 0; remaining PRC_MAX_NODES - 1 = 127 distinct single-token leaves fill node pool
    let i: u32 = 1;
    while ((i as usize) < PRC_MAX_NODES) : (i += 1) {
        const t: [1]u32 = [i];
        const p: [1]u32 = [i * 10];
        const r = cache.insertPrefix(t[0..], p[0..], i as u64);
        expect_true(r.ok);
    }
    expect_eq(cache.stats().active_nodes, PRC_MAX_NODES);

    // 128th distinct leaf must fail cleanly without corrupting tree
    const extra_t: [1]u32 = [9999];
    const extra_p: [1]u32 = [99990];
    expect_true(!cache.insertPrefix(extra_t[0..], extra_p[0..], 999).ok);
});

// ============================================================================
// Test 7: Strict boundary guards (empty tokens, mismatch, invalid node IDs)
// ============================================================================
test("prefix_radix_cache: strict boundary guards (empty, mismatch, invalid IDs, underflow)", fn() {
    let cache = PrefixRadixCache.init();

    const empty_u32: [0]u32 = [];
    const one_t: [1]u32 = [42];
    const two_p: [2]u32 = [1, 2];

    expect_true(!cache.insertPrefix(empty_u32[0..], empty_u32[0..], 1).ok);
    expect_true(!cache.insertPrefix(one_t[0..], two_p[0..], 1).ok);

    let out: [4]u32 = [0; 4];
    let out_s: []u32 = out[0..];
    const m_empty = cache.matchPrefix(empty_u32[0..], 1, *out_s);
    expect_eq(m_empty.matched_tokens, 0 as usize);
    expect_eq(m_empty.leaf_node_id, PRC_ROOT_NODE);

    // Invalid pin/unpin on root, out-of-bounds, and inactive nodes
    expect_true(!cache.pinNode(PRC_ROOT_NODE));
    expect_true(!cache.unpinNode(PRC_ROOT_NODE));
    expect_true(!cache.pinNode(PRC_NO_NODE));
    expect_true(!cache.pinNode(5)); // inactive
    expect_true(cache.getNode(5) == null);
    expect_true(cache.getNode(PRC_NO_NODE) == null);

    // Insert 1 node then verify unpin without pin fails safely (no underflow)
    const p1: [1]u32 = [77];
    const ins = cache.insertPrefix(one_t[0..], p1[0..], 1);
    expect_true(ins.ok);
    expect_true(!cache.unpinNode(ins.leaf_node_id));

    // Evicting 0 pages is a safe no-op
    const ev0 = cache.evictPages(0, *out_s);
    expect_eq(ev0.freed_pages, 0 as usize);
    expect_eq(ev0.remaining_unpinned_pages, 1 as usize);
});
