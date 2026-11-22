// Origin: Pure-CP Stdlib Test Suite (PagedAttention KV-Cache Block Manager, CoW & Prefix Sharing) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/kv_cache.cp (PagedKvAllocator & BlockTable)
// Covers:
//   1. Prefix hash zero-copy sharing across concurrent requests & post-release cached reuse
//   2. LRU eviction of ref_count == 0 cached prefix blocks when pool is full (never evicting active blocks)
//   3. Beam Search retainBlock + copyOnWrite (shared split vs exclusive zero-copy & full-pool atomicity)
//   4. BlockTable.mapTokenToPhys exact logical-to-physical [phys_block, slot_offset] mapping
//   5. Invalid block_id, double-release, full pool & zero block_size boundary defenses

import { test, expect_eq } from "cap:test";
import {
    KV_INVALID_BLOCK,
    KvBlockMeta,
    KvAllocStats,
    PagedKvAllocator,
    BlockTable,
} from "../../lib/attention/index.cp";

fn testPrefixSharingAndPostReleaseCacheReuse()! {
    let blocks_buf: [4]KvBlockMeta = [KvBlockMeta{}; 4];
    let blocks: []KvBlockMeta = blocks_buf[0..];
    let alloc = PagedKvAllocator.init(blocks, 16) ?? return;

    const sys_hash: u64 = 0xCAFE0001;

    // Request 1 misses cache and allocates block 0
    const b1 = alloc.allocateOrSharePrefix(sys_hash, 10) ?? KV_INVALID_BLOCK;
    expect_eq(b1, 0);
    expect_eq(blocks_buf[0].ref_count, 1);

    // Request 2 hits active prefix block 0 with zero copy
    const b2 = alloc.allocateOrSharePrefix(sys_hash, 20) ?? KV_INVALID_BLOCK;
    expect_eq(b2, 0);
    expect_eq(blocks_buf[0].ref_count, 2);

    const st1 = alloc.stats();
    expect_eq(st1.prefix_misses, 1);
    expect_eq(st1.prefix_hits, 1);
    expect_eq(st1.free_blocks, 3);
    expect_eq(st1.cached_evictable_blocks, 0);

    // Both requests release block 0 -> ref_count drops to 0, block stays cached for prefix reuse
    expect_eq(alloc.releaseBlock(b1, 30), true);
    expect_eq(alloc.releaseBlock(b2, 31), true);
    expect_eq(blocks_buf[0].ref_count, 0);

    const st2 = alloc.stats();
    expect_eq(st2.free_blocks, 4);
    expect_eq(st2.cached_evictable_blocks, 1);

    // Request 3 arrives with same system prompt hash -> revives cached block 0 with zero copy!
    const b3 = alloc.allocateOrSharePrefix(sys_hash, 40) ?? KV_INVALID_BLOCK;
    expect_eq(b3, 0);
    expect_eq(blocks_buf[0].ref_count, 1);
    expect_eq(alloc.stats().prefix_hits, 2);
    expect_eq(alloc.stats().cached_evictable_blocks, 0);
}

fn testLruEvictionOfCachedBlocksProtectsActive()! {
    let blocks_buf: [3]KvBlockMeta = [KvBlockMeta{}; 3];
    let blocks: []KvBlockMeta = blocks_buf[0..];
    let alloc = PagedKvAllocator.init(blocks, 16) ?? return;

    // Block 0: prefix A (stays ACTIVE)
    const ba = alloc.allocateOrSharePrefix(0x1111, 10) ?? KV_INVALID_BLOCK;
    // Block 1: prefix B (released at tick 25 -> oldest cached)
    const bb = alloc.allocateOrSharePrefix(0x2222, 20) ?? KV_INVALID_BLOCK;
    expect_eq(alloc.releaseBlock(bb, 25), true);
    // Block 2: prefix C (released at tick 35 -> newer cached)
    const bc = alloc.allocateOrSharePrefix(0x3333, 30) ?? KV_INVALID_BLOCK;
    expect_eq(alloc.releaseBlock(bc, 35), true);

    expect_eq(ba, 0);
    expect_eq(bb, 1);
    expect_eq(bc, 2);
    expect_eq(alloc.stats().cached_evictable_blocks, 2);

    // Allocate prefix D at tick 40 -> must evict LRU cached block 1 (B, tick 25), never active block 0 (A)
    const bd = alloc.allocateOrSharePrefix(0x4444, 40) ?? KV_INVALID_BLOCK;
    expect_eq(bd, 1);
    expect_eq(blocks_buf[0].prefix_hash, 0x1111);
    expect_eq(blocks_buf[0].ref_count, 1);

    // Prefix C (block 2) is still cached -> reviving it hits cache!
    const bc_hit = alloc.allocateOrSharePrefix(0x3333, 45) ?? KV_INVALID_BLOCK;
    expect_eq(bc_hit, 2);
    expect_eq(alloc.stats().prefix_hits, 1);
    expect_eq(alloc.stats().free_blocks, 0);

    // All 3 blocks are now actively referenced -> allocating prefix E must return null
    expect_eq(alloc.allocateOrSharePrefix(0x5555, 50) == null, true);
    expect_eq(alloc.allocateFresh(50) == null, true);
}

fn testBeamSearchRetainAndCopyOnWrite()! {
    let blocks_buf: [3]KvBlockMeta = [KvBlockMeta{}; 3];
    let blocks: []KvBlockMeta = blocks_buf[0..];
    let alloc = PagedKvAllocator.init(blocks, 16) ?? return;

    const b0 = alloc.allocateOrSharePrefix(0xAAAA, 10) ?? KV_INVALID_BLOCK;
    expect_eq(b0, 0);
    blocks_buf[0].filled_tokens = 12;

    // Beam Search forks shared prefix
    expect_eq(alloc.retainBlock(b0, 15), true);
    expect_eq(blocks_buf[0].ref_count, 2);

    // Beam 1 mutates -> shared (ref_count == 2) triggers CoW to fresh block 1
    const cow1 = alloc.copyOnWrite(b0, 20) ?? KV_INVALID_BLOCK;
    expect_eq(cow1, 1);
    expect_eq(blocks_buf[0].ref_count, 1);
    expect_eq(blocks_buf[1].ref_count, 1);
    expect_eq(blocks_buf[1].filled_tokens, 12);
    expect_eq(alloc.stats().cow_copies, 1);

    // Beam 2 is now sole owner of block 0 (ref_count == 1) -> CoW returns block 0 in-place (zero-copy!)
    const cow2 = alloc.copyOnWrite(b0, 25) ?? KV_INVALID_BLOCK;
    expect_eq(cow2, 0);
    expect_eq(blocks_buf[0].ref_count, 1);
    expect_eq(blocks_buf[0].has_hash, false);
    expect_eq(alloc.stats().cow_copies, 1);

    // Full-pool CoW failure must not corrupt original ref_count
    const b2 = alloc.allocateFresh(30) ?? KV_INVALID_BLOCK;
    expect_eq(b2, 2);
    expect_eq(alloc.retainBlock(b2, 31), true);
    expect_eq(blocks_buf[2].ref_count, 2);
    expect_eq(alloc.copyOnWrite(b2, 32) == null, true);
    expect_eq(blocks_buf[2].ref_count, 2); // intact!
}

fn testBlockTableMapping()! {
    let phys_buf: [3]u32 = [0; 3];
    let phys: []u32 = phys_buf[0..];
    let bt = BlockTable.init(phys, 16) ?? return;

    expect_eq(bt.appendBlock(KV_INVALID_BLOCK), false);
    expect_eq(bt.appendBlock(7), true);
    expect_eq(bt.appendBlock(3), true);
    expect_eq(bt.appendBlock(12), true);
    expect_eq(bt.appendBlock(99), false); // table full

    const invalid_pair: [2]u32 = [KV_INVALID_BLOCK, 0];
    const m0 = bt.mapTokenToPhys(0) ?? invalid_pair;
    expect_eq(m0[0], 7);
    expect_eq(m0[1], 0);

    const m15 = bt.mapTokenToPhys(15) ?? invalid_pair;
    expect_eq(m15[0], 7);
    expect_eq(m15[1], 15);

    const m16 = bt.mapTokenToPhys(16) ?? invalid_pair;
    expect_eq(m16[0], 3);
    expect_eq(m16[1], 0);

    const m35 = bt.mapTokenToPhys(35) ?? invalid_pair;
    expect_eq(m35[0], 12);
    expect_eq(m35[1], 3);

    // Out-of-bounds logical position
    expect_eq(bt.mapTokenToPhys(48) == null, true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    let blocks_buf: [2]KvBlockMeta = [KvBlockMeta{}; 2];
    let blocks: []KvBlockMeta = blocks_buf[0..];
    let empty_blocks: []KvBlockMeta = blocks_buf[0..0];

    expect_eq(PagedKvAllocator.init(empty_blocks, 16) == null, true);
    expect_eq(PagedKvAllocator.init(blocks, 0) == null, true);

    let phys_buf: [2]u32 = [0; 2];
    let phys: []u32 = phys_buf[0..];
    expect_eq(BlockTable.init(phys, 0) == null, true);

    let alloc = PagedKvAllocator.init(blocks, 16) ?? return;
    // Invalid block_id, out-of-range block_id, and unallocated block operations
    expect_eq(alloc.retainBlock(KV_INVALID_BLOCK, 1), false);
    expect_eq(alloc.retainBlock(99, 1), false);
    expect_eq(alloc.retainBlock(0, 1), false); // ref_count == 0
    expect_eq(alloc.releaseBlock(KV_INVALID_BLOCK, 1), false);
    expect_eq(alloc.releaseBlock(99, 1), false);
    expect_eq(alloc.releaseBlock(0, 1), false); // double-release / unallocated
    expect_eq(alloc.copyOnWrite(KV_INVALID_BLOCK, 1) == null, true);
    expect_eq(alloc.copyOnWrite(99, 1) == null, true);
    expect_eq(alloc.copyOnWrite(0, 1) == null, true);

    // Allocate and double-release defense
    const b0 = alloc.allocateFresh(5) ?? KV_INVALID_BLOCK;
    expect_eq(alloc.releaseBlock(b0, 6), true);
    expect_eq(alloc.releaseBlock(b0, 7), false); // second release rejected
}

fn main() {
    test("1. attention kv_cache: prefix hash zero-copy sharing & post-release reuse", () => {
        testPrefixSharingAndPostReleaseCacheReuse()?;
    });
    test("2. attention kv_cache: LRU eviction of cached blocks protects active blocks", () => {
        testLruEvictionOfCachedBlocksProtectsActive()?;
    });
    test("3. attention kv_cache: Beam Search retainBlock & CoW (split vs zero-copy)", () => {
        testBeamSearchRetainAndCopyOnWrite()?;
    });
    test("4. attention kv_cache: BlockTable.mapTokenToPhys exact logical->physical mapping", () => {
        testBlockTableMapping()?;
    });
    test("5. attention kv_cache: invalid block_id, double-release, full pool & zero block_size", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
