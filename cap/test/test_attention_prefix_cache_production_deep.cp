// Origin: Pure-CP Stdlib Test Suite (lib/attention/prefix_cache.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation block-level KV Prefix Cache:
// multi-agent shared system prompt prefix hits, chained FNV-1a anti-false-hit across diverged
// prefixes, ref-counted pin protection, LRU eviction with tick refresh, all-pinned refusal,
// and telemetry/boundary guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PREFIX_INVALID_BLOCK,
    computePrefixBlockHash,
    PrefixBlockEntry,
    PrefixMatchResult,
    PrefixCacheStats,
    KvPrefixCache,
} from "cap:attention";

fn testMultiAgentSharedSystemPromptPrefixHits()! {
    let entries: [8]PrefixBlockEntry = undefined;
    let entry_s: []PrefixBlockEntry = entries[0..];

    const cache_opt = cap.attention.KvPrefixCache.init(entry_s, 4);
    expect_eq(cache_opt != null, true);
    if (init_cache := cache_opt) {
        let cache = init_cache;

        // Agent A: 2 shared system blocks (8 tokens) + 1 agent-A user block (4 tokens) = 12 tokens.
        const agent_a: [12]u32 = [1, 2, 3, 4, 5, 6, 7, 8, 100, 101, 102, 103];
        const blocks_a: [3]u32 = [10, 11, 12];
        expect_eq(cache.insertPrefixBlocks(agent_a[0..], blocks_a[0..], 1), 3);

        // Agent B shares the first 8 system prompt tokens, diverges on block 2, plus 2 trailing partial tokens.
        const agent_b: [14]u32 = [1, 2, 3, 4, 5, 6, 7, 8, 200, 201, 202, 203, 999, 998];
        let matched_ids: [4]u32 = [0, 0, 0, 0];
        let matched_s: []u32 = matched_ids[0..];

        const res = cache.matchLongestPrefix(agent_b[0..], 2, matched_s);
        expect_eq(res.matched_blocks, 2);
        expect_eq(res.matched_tokens, 8);
        expect_eq(matched_ids[0], 10);
        expect_eq(matched_ids[1], 11);

        // Verify chained last_prefix_hash equals manual 2-step chain.
        const h0: u64 = computePrefixBlockHash(0, agent_a[0..4]);
        const h1: u64 = computePrefixBlockHash(h0, agent_a[4..8]);
        expect_eq(res.last_prefix_hash, h1);

        // Full hit on Agent A's own prompt (all 3 blocks).
        const res_a = cache.matchLongestPrefix(agent_a[0..], 3, matched_s);
        expect_eq(res_a.matched_blocks, 3);
        expect_eq(res_a.matched_tokens, 12);
        expect_eq(matched_ids[2], 12);
    }
}

fn testChainedHashPreventsDivergedSuffixFalseHits()! {
    const blk_a: [4]u32 = [10, 11, 12, 13];
    const blk_b: [4]u32 = [20, 21, 22, 23];
    const blk_shared_tail: [4]u32 = [77, 78, 79, 80];

    const ha: u64 = cap.attention.computePrefixBlockHash(0, blk_a[0..]);
    const hb: u64 = computePrefixBlockHash(0, blk_b[0..]);
    const h_ac: u64 = computePrefixBlockHash(ha, blk_shared_tail[0..]);
    const h_bc: u64 = computePrefixBlockHash(hb, blk_shared_tail[0..]);

    expect_eq(ha != hb, true);
    expect_eq(h_ac != h_bc, true);
    expect_eq(ha != 0 && h_ac != 0, true);

    let entries: [4]PrefixBlockEntry = undefined;
    let entry_s: []PrefixBlockEntry = entries[0..];
    if (init_cache := KvPrefixCache.init(entry_s, 4)) {
        let cache = init_cache;

        // Insert [A, C] into cache.
        const seq_ac: [8]u32 = [10, 11, 12, 13, 77, 78, 79, 80];
        const ids_ac: [2]u32 = [30, 31];
        expect_eq(cache.insertPrefixBlocks(seq_ac[0..], ids_ac[0..], 1), 2);

        // Query [B, C]: block 0 diverges, so block 1 (even though tokens are identical) must NOT match.
        const seq_bc: [8]u32 = [20, 21, 22, 23, 77, 78, 79, 80];
        let out_ids: [2]u32 = [0, 0];
        let out_s: []u32 = out_ids[0..];
        const res = cache.matchLongestPrefix(seq_bc[0..], 2, out_s);
        expect_eq(res.matched_blocks, 0);
        expect_eq(res.matched_tokens, 0);
        expect_eq(res.last_prefix_hash, 0);
    } else {
        expect_eq(true, false);
    }
}

fn testRefCountPinProtectionAgainstEviction()! {
    let entries: [2]PrefixBlockEntry = undefined;
    let entry_s: []PrefixBlockEntry = entries[0..];
    if (init_cache := KvPrefixCache.init(entry_s, 4)) {
        let cache = init_cache;

        const tok1: [4]u32 = [1, 1, 1, 1];
        const tok2: [4]u32 = [2, 2, 2, 2];
        const tok3: [4]u32 = [3, 3, 3, 3];
        const id1: [1]u32 = [101];
        const id2: [1]u32 = [102];
        const id3: [1]u32 = [103];

        expect_eq(cache.insertPrefixBlocks(tok1[0..], id1[0..], 1), 1);
        expect_eq(cache.insertPrefixBlocks(tok2[0..], id2[0..], 2), 1);

        // Pin older block tok1 twice (simulating 2 concurrent in-flight agent requests).
        const h1: u64 = computePrefixBlockHash(0, tok1[0..]);
        expect_eq(cache.pinBlock(h1), true);
        expect_eq(cache.pinBlock(h1), true);
        expect_eq(cache.unpinBlock(h1), true); // ref_count == 1 -> still protected!

        // Inserting tok3 into full 2-slot cache must skip pinned tok1 (tick=1) and evict tok2 (tick=2).
        expect_eq(cache.insertPrefixBlocks(tok3[0..], id3[0..], 3), 1);

        let out_ids: [1]u32 = [0];
        let out_s: []u32 = out_ids[0..];
        expect_eq(cache.matchLongestPrefix(tok1[0..], 4, out_s).matched_blocks, 1);
        expect_eq(out_ids[0], 101);
        expect_eq(cache.matchLongestPrefix(tok2[0..], 5, out_s).matched_blocks, 0);
        expect_eq(cache.matchLongestPrefix(tok3[0..], 6, out_s).matched_blocks, 1);
        expect_eq(out_ids[0], 103);

        // Release final ref on tok1 -> now tok1 becomes eligible for eviction.
        expect_eq(cache.unpinBlock(h1), true);
        expect_eq(cache.unpinBlock(h1), false); // underflow guard
        expect_eq(cache.stats().pinned_blocks, 0);
        if (evicted := cache.evictLruUnpinned()) {
            expect_eq(evicted, 101); // tok1 had tick=4 < tok3's tick=6
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testLruEvictionWithTickRefresh()! {
    let entries: [3]PrefixBlockEntry = undefined;
    let entry_s: []PrefixBlockEntry = entries[0..];
    if (init_cache := KvPrefixCache.init(entry_s, 4)) {
        let cache = init_cache;

        const b0: [4]u32 = [10, 0, 0, 0];
        const b1: [4]u32 = [20, 0, 0, 0];
        const b2: [4]u32 = [30, 0, 0, 0];
        const b3: [4]u32 = [40, 0, 0, 0];
        const id0: [1]u32 = [200];
        const id1: [1]u32 = [201];
        const id2: [1]u32 = [202];
        const id3: [1]u32 = [203];

        expect_eq(cache.insertPrefixBlocks(b0[0..], id0[0..], 1), 1);
        expect_eq(cache.insertPrefixBlocks(b1[0..], id1[0..], 2), 1);
        expect_eq(cache.insertPrefixBlocks(b2[0..], id2[0..], 3), 1);

        // Access b0 at tick=10 so b1 (tick=2) becomes the true LRU victim.
        let out_ids: [1]u32 = [0];
        let out_s: []u32 = out_ids[0..];
        expect_eq(cache.matchLongestPrefix(b0[0..], 10, out_s).matched_blocks, 1);

        // Insert b3 at tick=11 -> must evict b1, retaining b0, b2, b3.
        expect_eq(cache.insertPrefixBlocks(b3[0..], id3[0..], 11), 1);
        expect_eq(cache.matchLongestPrefix(b1[0..], 12, out_s).matched_blocks, 0);
        expect_eq(cache.matchLongestPrefix(b0[0..], 13, out_s).matched_blocks, 1);
        expect_eq(cache.matchLongestPrefix(b2[0..], 14, out_s).matched_blocks, 1);
        expect_eq(cache.matchLongestPrefix(b3[0..], 15, out_s).matched_blocks, 1);
        expect_eq(cache.stats().evictions, 1);
    } else {
        expect_eq(true, false);
    }
}

fn testAllPinnedRefusalStatsAndBoundaryGuards()! {
    let entries: [2]PrefixBlockEntry = undefined;
    let entry_s: []PrefixBlockEntry = entries[0..];

    // Invalid init guards: empty pool or block_size == 0.
    expect_eq(KvPrefixCache.init(entries[0..0], 4) == null, true);
    expect_eq(KvPrefixCache.init(entry_s, 0) == null, true);

    if (init_cache := KvPrefixCache.init(entry_s, 4)) {
        let cache = init_cache;
        const p1: [4]u32 = [1, 2, 3, 4];
        const p2: [4]u32 = [5, 6, 7, 8];
        const p3: [4]u32 = [9, 10, 11, 12];
        const id1: [1]u32 = [501];
        const id2: [1]u32 = [502];
        const id3: [1]u32 = [503];
        const bad_id: [1]u32 = [PREFIX_INVALID_BLOCK];

        // PREFIX_INVALID_BLOCK rejected cleanly.
        expect_eq(cache.insertPrefixBlocks(p1[0..], bad_id[0..], 1), 0);

        // Fill both slots and pin both.
        expect_eq(cache.insertPrefixBlocks(p1[0..], id1[0..], 1), 1);
        expect_eq(cache.insertPrefixBlocks(p2[0..], id2[0..], 2), 1);
        const h1: u64 = computePrefixBlockHash(0, p1[0..]);
        const h2: u64 = computePrefixBlockHash(0, p2[0..]);
        expect_eq(cache.pinBlock(h1), true);
        expect_eq(cache.pinBlock(h2), true);

        // All slots pinned -> eviction returns null, insert refuses to overwrite pinned blocks.
        expect_eq(cache.evictLruUnpinned() == null, true);
        expect_eq(cache.insertPrefixBlocks(p3[0..], id3[0..], 3), 0);

        // Hit p1, miss p3 -> verify exact telemetry accounting.
        let out_ids: [1]u32 = [0];
        let out_s: []u32 = out_ids[0..];
        expect_eq(cache.matchLongestPrefix(p1[0..], 4, out_s).matched_blocks, 1);
        expect_eq(cache.matchLongestPrefix(p3[0..], 5, out_s).matched_blocks, 0);

        const st: PrefixCacheStats = cache.stats();
        expect_eq(st.active_blocks, 2);
        expect_eq(st.pinned_blocks, 2);
        expect_eq(st.hits, 1);
        expect_eq(st.misses, 1);
        expect_eq(st.evictions, 0);
        expect_eq(st.tokens_saved, 4);
    } else {
        expect_eq(true, false);
    }
}

fn main() {
    test("1. attention/prefix_cache: multi-agent shared system prompt prefix hits", () => {
        testMultiAgentSharedSystemPromptPrefixHits()?;
    });

    test("2. attention/prefix_cache: chained FNV-1a hash prevents diverged suffix false hits", () => {
        testChainedHashPreventsDivergedSuffixFalseHits()?;
    });

    test("3. attention/prefix_cache: ref-counted pin protection against eviction", () => {
        testRefCountPinProtectionAgainstEviction()?;
    });

    test("4. attention/prefix_cache: LRU eviction of oldest unpinned block with tick refresh", () => {
        testLruEvictionWithTickRefresh()?;
    });

    test("5. attention/prefix_cache: all-pinned refusal, telemetry accounting & boundary guards", () => {
        testAllPinnedRefusalStatsAndBoundaryGuards()?;
    });
}
