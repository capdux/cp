// Source: cp/cap/test/test_bun_alloc_production_deep.cp

const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import { BumpAllocator, PoolAllocator, TrackingAllocator, ScratchArena } from "cap:bun_alloc";

const Item = struct {
    id: u64,
    score: u32,
};

fn testBumpAllocator()! {
    let stack_buf: [64]u8 = undefined;
    let bump = BumpAllocator.init(&stack_buf);
    const alloc = bump.allocator();

    // Aligned allocation for u64
    let p1 = alloc.alloc(u64, 2)?;
    p1[0] = 42;
    p1[1] = 99;
    expect_eq(p1[0], 42);
    expect_eq(p1[1], 99);
    expect_eq(@intFromPtr(p1.ptr) % @alignOf(u64), 0);
    expect_eq(bump.usedBytes() >= 16, true);
    expect_eq(bump.remainingBytes() <= 48, true);

    // Exceeding capacity returns null / OutOfMemory
    const huge = bump.allocBytes(128);
    expect_eq(huge == null, true);
    let got_oom = false;
    * = alloc.alloc(u8, 128) catch e => {
        if (e == error.OutOfMemory) {
            got_oom = true;
        }
    };
    expect_eq(got_oom, true);

    // Reset allows full buffer reuse
    bump.reset();
    expect_eq(bump.usedBytes(), 0);
    expect_eq(bump.remainingBytes(), 64);

    let p2 = alloc.alloc(u8, 64)?;
    p2[0] = 7;
    p2[63] = 8;
    expect_eq(p2[0], 7);
    expect_eq(p2[63], 8);
    expect_eq(bump.remainingBytes(), 0);
}

fn testPoolAllocator()! {
    let gpa = std.heap.DebugAllocator(.{}).init;
    const backing = gpa.allocator();

    let pool = PoolAllocator(Item).initWithOptions(backing, 4, 8);

    pool.prewarm(4)?;
    expect_eq(pool.capacity(), 4);
    expect_eq(pool.activeCount(), 0);

    let a = pool.create()?;
    a.id = 101;
    a.score = 10;
    let b = pool.create()?;
    b.id = 102;
    b.score = 20;
    expect_eq(pool.activeCount(), 2);

    const a_addr = @intFromPtr(a);
    pool.recycle(a);
    expect_eq(pool.activeCount(), 1);

    // Next create must reuse the recycled slot address in O(1)
    let c = pool.create()?;
    expect_eq(@intFromPtr(c), a_addr);
    expect_eq(pool.activeCount(), 2);

    pool.recycle(b);
    pool.recycle(c);
    expect_eq(pool.activeCount(), 0);

    pool.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testTrackingAllocator()! {
    let gpa = std.heap.DebugAllocator(.{}).init;
    const backing = gpa.allocator();

    let tracker = TrackingAllocator.init(backing);
    const alloc = tracker.allocator();

    const buf1 = alloc.alloc(u8, 128)?;
    expect_eq(tracker.alloc_count, 1);
    expect_eq(tracker.live_bytes, 128);
    expect_eq(tracker.peak_bytes, 128);
    expect_eq(tracker.hasLeaks(), true);

    const buf2 = alloc.alloc(u8, 256)?;
    expect_eq(tracker.alloc_count, 2);
    expect_eq(tracker.live_bytes, 384);
    expect_eq(tracker.peak_bytes, 384);

    alloc.free(buf1);
    expect_eq(tracker.free_count, 1);
    expect_eq(tracker.live_bytes, 256);
    expect_eq(tracker.peak_bytes, 384);
    expect_eq(tracker.hasLeaks(), true);

    const stats_str = tracker.formatStats(backing)?;
    expect_eq(stats_str.len > 0, true);
    backing.free(stats_str);

    alloc.free(buf2);
    expect_eq(tracker.free_count, 2);
    expect_eq(tracker.live_bytes, 0);
    expect_eq(tracker.hasLeaks(), false);
    expect_eq(gpa.deinit() == .ok, true);
}

fn testScratchArena()! {
    let gpa = std.heap.DebugAllocator(.{}).init;
    const backing = gpa.allocator();

    let scratch = ScratchArena.initWithChunkSize(backing, 128);

    const alloc = scratch.allocator();
    let base_slice = alloc.alloc(u8, 32)?;
    base_slice[0] = 1;
    const used_after_base = scratch.usedBytes();

    const cp = scratch.mark();
    // Force multi-chunk expansion
    let s1 = alloc.alloc(u8, 96)?;
    let s2 = alloc.alloc(u8, 96)?;
    s1[0] = 2;
    s2[0] = 3;
    expect_eq(scratch.chunkCount() >= 2, true);
    expect_eq(scratch.usedBytes() > used_after_base, true);

    // Rewind back to checkpoint
    scratch.rewind(cp);
    expect_eq(scratch.usedBytes(), used_after_base);

    // Subsequent allocations reuse existing chunks without losing base_slice
    let s3 = alloc.alloc(u8, 64)?;
    s3[0] = 4;
    expect_eq(base_slice[0], 1);

    scratch.resetRetainingCapacity();
    expect_eq(scratch.usedBytes(), 0);
    expect_eq(scratch.capacityBytes() >= 256, true);

    scratch.deinit();
    expect_eq(gpa.deinit() == .ok, true);
}

fn testCapBunAllocDynamicMount()! {
    let gpa = std.heap.DebugAllocator(.{}).init;
    const backing = gpa.allocator();

    const ba = cap.bun_alloc;
    let tracker = ba.TrackingAllocator.init(backing);
    const tracked = tracker.allocator();

    let scratch = ba.ScratchArena.initWithChunkSize(tracked, 128);
    const cp = scratch.mark();
    let tmp = scratch.allocator().alloc(u64, 8)?;
    tmp[0] = 777;
    expect_eq(tmp[0], 777);
    scratch.rewind(cp);
    scratch.deinit();

    expect_eq(tracker.hasLeaks(), false);
    expect_eq(tracker.live_bytes, 0);
    expect_eq(gpa.deinit() == .ok, true);
}

fn main()! {
    test("1. BumpAllocator aligned allocation, OOM boundary, and reset reuse", () => {
        testBumpAllocator()?;
    });
    test("2. PoolAllocator(T) O(1) create/recycle address reuse and zero-leak deinit", () => {
        testPoolAllocator()?;
    });
    test("3. TrackingAllocator peak/live bytes, counts, and leak detection", () => {
        testTrackingAllocator()?;
    });
    test("4. ScratchArena mark/rewind partial rollback and multi-chunk expansion", () => {
        testScratchArena()?;
    });
    test("5. cap.bun_alloc dynamic mount and DebugAllocator zero-leak integration", () => {
        testCapBunAllocDynamicMount()?;
    });
}
