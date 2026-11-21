// Source: native-cp-stdlib (Generational O(1) Slab Object Pool)
const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import { SlabPool, Handle, SlabError } from "cap:slab_pool";

test "slab_pool O(1) sequential insert and PoolFull" {
    let pool = SlabPool(u64).init(std.testing.allocator, 3)?;
    defer pool.deinit();

    expect_eq(pool.capacity(), @as(usize, 3));
    expect_eq(pool.len(), @as(usize, 0));

    const h0 = pool.insert(100)?;
    const h1 = pool.insert(200)?;
    const h2 = pool.insert(300)?;

    expect_eq(pool.len(), @as(usize, 3));
    expect_eq(pool.getConst(h0)?, @as(u64, 100));
    expect_eq(pool.getConst(h1)?, @as(u64, 200));
    expect_eq(pool.getConst(h2)?, @as(u64, 300));

    let ptr1 = pool.get(h1)?;
    ptr1.* = 250;
    expect_eq(pool.getConst(h1)?, @as(u64, 250));

    let full_hit = false;
    if (_ := pool.insert(400)) {
        full_hit = false;
    } else (err) {
        expect_eq(err, SlabError.PoolFull);
        full_hit = true;
    }
    expect_eq(full_hit, true);
}

test "slab_pool middle slot remove and reuse increments generation" {
    let pool = SlabPool(i32).init(std.testing.allocator, 4)?;
    defer pool.deinit();

    const h0 = pool.insert(10)?;
    const h1 = pool.insert(20)?;
    const h2 = pool.insert(30)?;

    const removed = pool.remove(h1)?;
    expect_eq(removed, @as(i32, 20));
    expect_eq(pool.len(), @as(usize, 2));
    expect_eq(pool.isValid(h1), false);
    expect_eq(pool.isValid(h0), true);
    expect_eq(pool.isValid(h2), true);

    const reused = pool.insert(99)?;
    expect_eq(reused.index, h1.index);
    expect_eq(reused.generation, h1.generation + 1);
    expect_eq(pool.len(), @as(usize, 3));
    expect_eq(pool.getConst(reused)?, @as(i32, 99));
}

test "slab_pool StaleHandle blocks ABA access and InvalidHandle checks" {
    let pool = SlabPool(u32).init(std.testing.allocator, 2)?;
    defer pool.deinit();

    const old_h = pool.insert(11)?;
    const removed = pool.remove(old_h)?;
    expect_eq(removed, @as(u32, 11));

    const new_h = pool.insert(22)?;
    expect_eq(new_h.index, old_h.index);
    expect_eq(old_h.eql(new_h), false);

    let stale_get_blocked = false;
    if (_ := pool.get(old_h)) {
        stale_get_blocked = false;
    } else (err) {
        expect_eq(err, SlabError.StaleHandle);
        stale_get_blocked = true;
    }
    expect_eq(stale_get_blocked, true);

    let stale_remove_blocked = false;
    if (_ := pool.remove(old_h)) {
        stale_remove_blocked = false;
    } else (err) {
        expect_eq(err, SlabError.StaleHandle);
        stale_remove_blocked = true;
    }
    expect_eq(stale_remove_blocked, true);

    const bad_h = Handle { index: 99, generation: 1 };
    expect_eq(pool.isValid(bad_h), false);
    let invalid_blocked = false;
    if (_ := pool.getConst(bad_h)) {
        invalid_blocked = false;
    } else (err) {
        expect_eq(err, SlabError.InvalidHandle);
        invalid_blocked = true;
    }
    expect_eq(invalid_blocked, true);
}

test "slab_pool forEachActive and reset" {
    let pool = SlabPool(u64).init(std.testing.allocator, 5)?;
    defer pool.deinit();

    const h0 = pool.insert(10)?;
    const h1 = pool.insert(20)?;
    const h2 = pool.insert(30)?;
    * = pool.remove(h1)?;

    const SumCtx = struct {
        total: u64,
        count: usize,
    };
    let ctx = SumCtx { total: 0, count: 0 };

    const visitor = struct {
        fn visit(c: *SumCtx, _: Handle, val: *u64) {
            c.total += val.*;
            c.count += 1;
        }
    }.visit;

    pool.forEachActive(&ctx, visitor);
    expect_eq(ctx.count, @as(usize, 2));
    expect_eq(ctx.total, @as(u64, 40));

    pool.reset();
    expect_eq(pool.len(), @as(usize, 0));
    expect_eq(pool.isValid(h0), false);
    expect_eq(pool.isValid(h2), false);

    const h_after = pool.insert(777)?;
    expect_eq(pool.len(), @as(usize, 1));
    expect_eq(pool.getConst(h_after)?, @as(u64, 777));
}

test "slab_pool dynamic cap.slab_pool mount and zero-leak" {
    let pool = cap.slab_pool.SlabPool(i64).init(std.testing.allocator, 4)?;
    defer pool.deinit();

    const h0 = pool.insert(1234)?;
    const h1 = pool.insert(5678)?;
    expect_eq(pool.getConst(h0)?, @as(i64, 1234));
    expect_eq(pool.getConst(h1)?, @as(i64, 5678));

    * = pool.remove(h0)?;
    const h2 = pool.insert(9012)?;
    expect_eq(h2.index, h0.index);
    expect_eq(pool.getConst(h2)?, @as(i64, 9012));
    expect_eq(pool.len(), @as(usize, 2));
}
