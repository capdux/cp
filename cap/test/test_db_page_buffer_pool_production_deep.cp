// Origin: Pure-CP Stdlib Test Suite (lib/db/page_buffer_pool.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc CLOCK Second-Chance Page Buffer Pool:
//   1. Page fetch, cache hit, pin reference counting & unpin underflow guard
//   2. CLOCK second-chance algorithm: re-accessed hot page survives while cold page is evicted
//   3. Dirty page eviction reports exact evicted_page_id, evicted_was_dirty & dirty_evictions
//   4. All-pinned pool refuses eviction cleanly (all_pinned=true) without corrupting in-flight pages
//   5. Batched flush_dirty_pages with small caller buffer clears only flushed frames without losing pages
//   6. invalidate_page rejects pinned pages and frees unpinned frames for zero-eviction reuse
//   7. Invalid capacity (0, >128) & BUFFER_POOL_INVALID_PAGE boundary guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BUFFER_POOL_MAX_FRAMES,
    BUFFER_POOL_INVALID_PAGE,
    PageFetchOutcome,
    BufferPoolStats,
    ClockPageBufferPool,
} from "cap:db";

// ============================================================================
// Test 1: Fetch, Hit, Pin Reference Counting & Unpin Underflow Guard
// ============================================================================

fn testFetchHitPinAndUnpinUnderflow()! {
    let pool = cap.db.ClockPageBufferPool.init(4) ?? return error.InitFailed;

    // First fetch -> miss, pin_count = 1
    const r1: PageFetchOutcome = pool.fetch_page(100);
    expect_eq(r1.ok, true);
    expect_eq(r1.cache_hit, false);
    expect_eq(r1.evicted, false);
    expect_eq(r1.frame_idx, 0);

    // Second fetch of same page -> hit, pin_count = 2
    const r2 = pool.fetch_page(100);
    expect_eq(r2.ok, true);
    expect_eq(r2.cache_hit, true);
    expect_eq(r2.frame_idx, 0);

    // Unpin twice succeeds; third unpin must fail (underflow guard)
    expect_eq(pool.unpin_page(100, false), true);
    expect_eq(pool.stats().pinned_frames, 1);
    expect_eq(pool.unpin_page(100, true), true);
    expect_eq(pool.stats().pinned_frames, 0);
    expect_eq(pool.stats().dirty_frames, 1);
    expect_eq(pool.unpin_page(100, false), false); // underflow prevented
    expect_eq(pool.unpin_page(999, false), false); // non-resident page

    const st: BufferPoolStats = pool.stats();
    expect_eq(st.capacity, 4);
    expect_eq(st.occupied_frames, 1);
    expect_eq(st.cache_hits, 1);
    expect_eq(st.cache_misses, 1);
}

// ============================================================================
// Test 2: CLOCK Second-Chance Eviction (Hot Page Survives, Cold Page Evicted)
// ============================================================================

fn testClockSecondChanceEviction()! {
    let pool = ClockPageBufferPool.init(3) ?? return error.InitFailed;

    // Fill frames 0, 1, 2 with pages 10, 20, 30 and unpin all (all have ref_bit=true)
    expect_eq(pool.fetch_page(10).ok, true);
    expect_eq(pool.fetch_page(20).ok, true);
    expect_eq(pool.fetch_page(30).ok, true);
    expect_eq(pool.unpin_page(10, false), true);
    expect_eq(pool.unpin_page(20, false), true);
    expect_eq(pool.unpin_page(30, false), true);

    // Trigger first eviction with page 40:
    //   Pass 1 clears ref_bit on 0(10), 1(20), 2(30); Pass 2 evicts frame 0(10), hand advances to 1.
    //   Now frame 0=40(ref=1,pin=1), frame 1=20(ref=0,pin=0), frame 2=30(ref=0,pin=0), hand=1.
    const r40 = pool.fetch_page(40);
    expect_eq(r40.ok, true);
    expect_eq(r40.evicted, true);
    expect_eq(r40.evicted_page_id, 10);
    expect_eq(r40.frame_idx, 0);
    expect_eq(pool.unpin_page(40, false), true);

    // Re-access hot page 20 (frame 1) -> restores ref_bit=true!
    const r20_hit = pool.fetch_page(20);
    expect_eq(r20_hit.ok, true);
    expect_eq(r20_hit.cache_hit, true);
    expect_eq(pool.unpin_page(20, false), true);

    // Fetch page 50: clock_hand starts at frame 1(page 20, ref=true) -> gives second chance (clears ref_bit),
    // advances to frame 2(page 30, ref=false) -> evicts cold page 30 instead of hot page 20!
    const r50 = pool.fetch_page(50);
    expect_eq(r50.ok, true);
    expect_eq(r50.evicted, true);
    expect_eq(r50.evicted_page_id, 30);
    expect_eq(r50.frame_idx, 2);

    // Confirm hot page 20 is still resident (cache hit!)
    const r20_still_hot = pool.fetch_page(20);
    expect_eq(r20_still_hot.ok, true);
    expect_eq(r20_still_hot.cache_hit, true);
}

// ============================================================================
// Test 3: Dirty Page Eviction Reporting & Clean vs Dirty Counters
// ============================================================================

fn testDirtyPageEvictionReporting()! {
    let pool = ClockPageBufferPool.init(2) ?? return error.InitFailed;

    expect_eq(pool.fetch_page(1).ok, true);
    expect_eq(pool.fetch_page(2).ok, true);
    // Mark page 1 dirty, keep page 2 clean
    expect_eq(pool.unpin_page(1, true), true);
    expect_eq(pool.unpin_page(2, false), true);

    // Evicts dirty page 1 first
    const r3 = pool.fetch_page(3);
    expect_eq(r3.ok, true);
    expect_eq(r3.evicted, true);
    expect_eq(r3.evicted_page_id, 1);
    expect_eq(r3.evicted_was_dirty, true);
    expect_eq(pool.unpin_page(3, false), true);

    // Evicts clean page 2 next
    const r4 = pool.fetch_page(4);
    expect_eq(r4.ok, true);
    expect_eq(r4.evicted, true);
    expect_eq(r4.evicted_page_id, 2);
    expect_eq(r4.evicted_was_dirty, false);

    const st = pool.stats();
    expect_eq(st.evictions, 2);
    expect_eq(st.dirty_evictions, 1);
}

// ============================================================================
// Test 4: All-Pinned Pool Refuses Eviction Safely
// ============================================================================

fn testAllPinnedPoolRefusesEviction()! {
    let pool = ClockPageBufferPool.init(2) ?? return error.InitFailed;

    expect_eq(pool.fetch_page(11).ok, true);
    expect_eq(pool.fetch_page(22).ok, true);
    // Both frames remain pinned (pin_count == 1)

    const r_fail = pool.fetch_page(33);
    expect_eq(r_fail.ok, false);
    expect_eq(r_fail.all_pinned, true);
    expect_eq(r_fail.evicted, false);
    expect_eq(pool.stats().evictions, 0);

    // Both pinned pages are completely intact
    expect_eq(pool.fetch_page(11).cache_hit, true);
    expect_eq(pool.unpin_page(11, false), true);
    expect_eq(pool.unpin_page(11, false), true); // now page 11 has pin_count == 0

    // Now fetching page 33 succeeds by evicting unpinned page 11
    const r_ok = pool.fetch_page(33);
    expect_eq(r_ok.ok, true);
    expect_eq(r_ok.evicted, true);
    expect_eq(r_ok.evicted_page_id, 11);
}

// ============================================================================
// Test 5: Batched flush_dirty_pages Without Losing Remaining Dirty Pages
// ============================================================================

fn testBatchedFlushDirtyPages()! {
    let pool = ClockPageBufferPool.init(4) ?? return error.InitFailed;

    let p: u64 = 1;
    while (p <= 4) : (p += 1) {
        expect_eq(pool.fetch_page(p * 10).ok, true);
        expect_eq(pool.unpin_page(p * 10, true), true);
    }
    expect_eq(pool.stats().dirty_frames, 4);

    // Small caller buffer of size 2 flushes in 2 clean batches
    let batch: [2]u64 = [0, 0];
    let batch_s: []u64 = batch[0..];

    const n1 = pool.flush_dirty_pages(batch_s);
    expect_eq(n1, 2);
    expect_eq(batch[0], 10);
    expect_eq(batch[1], 20);
    expect_eq(pool.stats().dirty_frames, 2);

    const n2 = pool.flush_dirty_pages(batch_s);
    expect_eq(n2, 2);
    expect_eq(batch[0], 30);
    expect_eq(batch[1], 40);
    expect_eq(pool.stats().dirty_frames, 0);

    // Third flush returns 0
    expect_eq(pool.flush_dirty_pages(batch_s), 0);
}

// ============================================================================
// Test 6: invalidate_page Protects Pinned Frames & Frees Unpinned Frames
// ============================================================================

fn testInvalidatePageSafetyAndReuse()! {
    let pool = ClockPageBufferPool.init(2) ?? return error.InitFailed;

    expect_eq(pool.fetch_page(7).ok, true);
    expect_eq(pool.fetch_page(8).ok, true);

    // Cannot invalidate while pinned
    expect_eq(pool.invalidate_page(7), false);
    expect_eq(pool.stats().occupied_frames, 2);

    // Unpin and invalidate page 7 -> frame freed cleanly
    expect_eq(pool.unpin_page(7, true), true);
    expect_eq(pool.invalidate_page(7), true);
    expect_eq(pool.stats().occupied_frames, 1);
    expect_eq(pool.stats().dirty_frames, 0);

    // Next fetch reuses freed frame without triggering eviction
    const r9 = pool.fetch_page(9);
    expect_eq(r9.ok, true);
    expect_eq(r9.evicted, false);
    expect_eq(r9.frame_idx, 0);
}

// ============================================================================
// Test 7: Invalid Capacity & BUFFER_POOL_INVALID_PAGE Boundary Guards
// ============================================================================

fn testInvalidCapacityAndSentinelGuards()! {
    expect_eq(ClockPageBufferPool.init(0) == null, true);
    expect_eq(ClockPageBufferPool.init(BUFFER_POOL_MAX_FRAMES + 1) == null, true);

    let max_pool = ClockPageBufferPool.init(BUFFER_POOL_MAX_FRAMES) ?? return error.MaxInitFailed;
    expect_eq(max_pool.stats().capacity, BUFFER_POOL_MAX_FRAMES);

    // Reject BUFFER_POOL_INVALID_PAGE across all mutating entry points
    const inv = max_pool.fetch_page(BUFFER_POOL_INVALID_PAGE);
    expect_eq(inv.ok, false);
    expect_eq(inv.all_pinned, false);
    expect_eq(max_pool.unpin_page(BUFFER_POOL_INVALID_PAGE, true), false);
    expect_eq(max_pool.invalidate_page(BUFFER_POOL_INVALID_PAGE), false);
    expect_eq(max_pool.stats().occupied_frames, 0);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("db page_buffer_pool: fetch, hit, pin reference counting & unpin underflow guard", testFetchHitPinAndUnpinUnderflow);
test("db page_buffer_pool: CLOCK second-chance spares re-accessed hot page and evicts cold page", testClockSecondChanceEviction);
test("db page_buffer_pool: dirty page eviction accurately reports evicted_page_id and dirty flag", testDirtyPageEvictionReporting);
test("db page_buffer_pool: all-pinned pool returns all_pinned=true without corrupting in-flight pages", testAllPinnedPoolRefusesEviction);
test("db page_buffer_pool: batched flush_dirty_pages clears only written frames without losing pages", testBatchedFlushDirtyPages);
test("db page_buffer_pool: invalidate_page blocks pinned pages and frees unpinned frames for reuse", testInvalidatePageSafetyAndReuse);
test("db page_buffer_pool: rejects invalid capacity (0, >128) and BUFFER_POOL_INVALID_PAGE sentinel", testInvalidCapacityAndSentinelGuards);
