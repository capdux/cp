// cap/test/testShellRefStr.cp
// Production-grade deep tests for cap:shell refStr.cp (Bun shell/RefCountedStr pool)

import { test, assert, assertEq } from "cap:test";
import {
    REF_STR_MAX_SLOTS,
    REF_STR_MAX_LEN,
    RefStrError,
    RefStrHandle,
    RefStrPool,
} from "cap:shell";

test("RefStrPool: alloc / byteSlice / ref / deref lifecycle", () => {
    let pool = RefStrPool.init();
    assertEq(pool.activeCount(), @as(usize, 0));
    assertEq(pool.capacity(), REF_STR_MAX_SLOTS);

    const h = pool.alloc("NODE_ENV=production") catch RefStrHandle{};
    assert(pool.isAlive(h));
    assertEq(pool.activeCount(), @as(usize, 1));
    assertEq(pool.refCount(h) catch 0, @as(u32, 1));
    assertEq(pool.byteSlice(h) catch "", "NODE_ENV=production");

    assertEq(pool.ref(h) catch 0, @as(u32, 2));
    assertEq(pool.ref(h) catch 0, @as(u32, 3));
    assertEq(pool.refCount(h) catch 0, @as(u32, 3));

    assert(!(pool.deref(h) catch true));
    assertEq(pool.refCount(h) catch 0, @as(u32, 2));
    assert(!(pool.deref(h) catch true));
    assertEq(pool.refCount(h) catch 0, @as(u32, 1));

    // Final deref frees slot
    assert(pool.deref(h) catch false);
    assert(!pool.isAlive(h));
    assertEq(pool.activeCount(), @as(usize, 0));
});

test("RefStrPool: intern deduplicates live strings & handles empty string", () => {
    let pool = RefStrPool.init();

    const h1 = pool.intern("/usr/local/bin:/usr/bin") catch RefStrHandle{};
    const h2 = pool.intern("/usr/local/bin:/usr/bin") catch RefStrHandle{};
    assert(h1.eql(h2));
    assertEq(pool.activeCount(), @as(usize, 1));
    assertEq(pool.refCount(h1) catch 0, @as(u32, 2));

    const he1 = pool.intern("") catch RefStrHandle{};
    const he2 = pool.intern("") catch RefStrHandle{};
    assert(he1.eql(he2));
    assertEq(pool.activeCount(), @as(usize, 2));
    assertEq(pool.byteSlice(he1) catch "x", "");
    assertEq(pool.refCount(he1) catch 0, @as(u32, 2));

    assert(!(pool.deref(h1) catch true));
    assert(pool.deref(h2) catch false);
    assertEq(pool.activeCount(), @as(usize, 1));
});

test("RefStrPool: slot reuse increments generation & blocks ABA stale handle", () => {
    let pool = RefStrPool.init();

    const old_h = pool.alloc("secret_v1") catch RefStrHandle{};
    assert(pool.deref(old_h) catch false);
    assert(!pool.isAlive(old_h));

    // Reallocate into the exact same slot index 0
    const new_h = pool.alloc("secret_v2") catch RefStrHandle{};
    assertEq(new_h.index, old_h.index);
    assert(new_h.generation != old_h.generation);
    assert(!old_h.eql(new_h));
    assert(pool.isAlive(new_h));
    assert(!pool.isAlive(old_h));
    assertEq(pool.byteSlice(new_h) catch "", "secret_v2");

    // Stale handle cannot read, ref, or deref the reused slot
    let got_stale_read = false;
    * = pool.byteSlice(old_h) catch |e| {
        if (e == RefStrError.StaleHandle) {
            got_stale_read = true;
        }
        ""
    };
    assert(got_stale_read);

    let got_stale_deref = false;
    * = pool.deref(old_h) catch |e| {
        if (e == RefStrError.StaleHandle) {
            got_stale_deref = true;
        }
        false
    };
    assert(got_stale_deref);
    // Live slot remains untouched
    assertEq(pool.refCount(new_h) catch 0, @as(u32, 1));
});

test("RefStrPool: InvalidHandle out-of-range index guard", () => {
    let pool = RefStrPool.init();
    const bad = RefStrHandle{ index: 999, generation: 1 };
    assert(!pool.isAlive(bad));

    let got_invalid = false;
    * = pool.refCount(bad) catch |e| {
        if (e == RefStrError.InvalidHandle) {
            got_invalid = true;
        }
        0
    };
    assert(got_invalid);
});

test("RefStrPool: StringTooLong boundary (256 ok, 257 rejected)", () => {
    let pool = RefStrPool.init();
    let max_buf: [256]u8 = ['a'; 256];
    let over_buf: [257]u8 = ['b'; 257];
    assertEq(max_buf.len, REF_STR_MAX_LEN);

    const h = pool.alloc(max_buf[0..]) catch RefStrHandle{};
    assert(pool.isAlive(h));
    assertEq((pool.byteSlice(h) catch "").len, @as(usize, 256));

    let got_too_long = false;
    * = pool.alloc(over_buf[0..]) catch |e| {
        if (e == RefStrError.StringTooLong) {
            got_too_long = true;
        }
        RefStrHandle{}
    };
    assert(got_too_long);
});

test("RefStrPool: PoolFull exhaustion & recovery after deref", () => {
    let pool = RefStrPool.init();
    let handles: [64]RefStrHandle = [RefStrHandle{}; 64];

    let i: usize = 0;
    while (i < REF_STR_MAX_SLOTS) {
        handles[i] = pool.alloc("v") catch RefStrHandle{};
        i += 1;
    }
    assertEq(pool.activeCount(), REF_STR_MAX_SLOTS);

    let got_full = false;
    * = pool.alloc("overflow") catch |e| {
        if (e == RefStrError.PoolFull) {
            got_full = true;
        }
        RefStrHandle{}
    };
    assert(got_full);

    // Free one slot and verify pool recovers cleanly
    assert(pool.deref(handles[10]) catch false);
    assertEq(pool.activeCount(), REF_STR_MAX_SLOTS - 1);
    const recovered = pool.alloc("recovered") catch RefStrHandle{};
    assertEq(recovered.index, @as(u16, 10));
    assertEq(pool.byteSlice(recovered) catch "", "recovered");
    assertEq(pool.activeCount(), REF_STR_MAX_SLOTS);
});
