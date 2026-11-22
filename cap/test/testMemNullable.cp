// cap/test/testMemNullable.cp
// Production-grade deep tests for cap:mem nullable.cp (Bun bun_alloc/NullableAllocator)

import { test, assert, assertEq } from "cap:test";
import {
    NullableAllocError,
    AllocOrigin,
    NullableBump,
    OwnedOrBorrowed,
} from "cap:mem";

test("NullableBump: empty null allocator is safe no-op on free & rejects alloc", () => {
    let empty = NullableBump.empty();
    assert(empty.isNull());
    assertEq(empty.capacity(), @as(usize, 0));
    assertEq(empty.usedBytes(), @as(usize, 0));
    assertEq(empty.remainingBytes(), @as(usize, 0));
    assert(!empty.isCustomTag(1));

    // Free operations are safe no-ops on null allocator
    empty.freeBytes(16);
    assert(!empty.freeTail(8));
    assertEq(empty.freeCount(), @as(usize, 0));

    let fallback: [0]u8 = [];
    let got_null = false;
    * = empty.alloc(8) catch |e| {
        if (e == NullableAllocError.NullAllocator) {
            got_null = true;
        }
        fallback[0..]
    };
    assert(got_null);
});

test("NullableBump: init / alloc / allocAligned / dupe / reset", () => {
    let storage: [64]u8 = [0; 64];
    let alloc = NullableBump.init(storage[0..]);
    assert(!alloc.isNull());
    assertEq(alloc.capacity(), @as(usize, 64));
    assertEq(alloc.remainingBytes(), @as(usize, 64));

    const s1 = alloc.dupe("abc") catch storage[0..0];
    assertEq(s1.len, @as(usize, 3));
    assertEq(s1[0], @as(u8, 'a'));
    assertEq(s1[2], @as(u8, 'c'));
    assertEq(alloc.usedBytes(), @as(usize, 3));

    // Align next allocation to 8-byte boundary (pos 3 -> 8)
    const s2 = alloc.allocAligned(10, 8) catch storage[0..0];
    assertEq(s2.len, @as(usize, 10));
    assertEq(alloc.usedBytes(), @as(usize, 18));
    assertEq(alloc.remainingBytes(), @as(usize, 46));
    assertEq(alloc.allocCount(), @as(usize, 2));

    alloc.reset();
    assertEq(alloc.usedBytes(), @as(usize, 0));
    assertEq(alloc.remainingBytes(), @as(usize, 64));
});

test("NullableBump: freeTail LIFO reclaim & custom allocator tag", () => {
    let storage: [32]u8 = [0; 32];
    let alloc = NullableBump.initWithTag(storage[0..], 42);
    assertEq(alloc.customTag(), @as(u16, 42));
    assert(alloc.isCustomTag(42));
    assert(!alloc.isCustomTag(99));
    assert(!alloc.isCustomTag(0));

    const a = alloc.alloc(12) catch storage[0..0];
    assertEq(a.len, @as(usize, 12));
    assertEq(alloc.usedBytes(), @as(usize, 12));

    // Cannot reclaim more than used
    assert(!alloc.freeTail(20));
    assertEq(alloc.usedBytes(), @as(usize, 12));

    // Reclaim exact tail
    assert(alloc.freeTail(12));
    assertEq(alloc.usedBytes(), @as(usize, 0));
    assertEq(alloc.freeCount(), @as(usize, 1));
});

test("OwnedOrBorrowed: borrowed vs BumpOwned vs CustomTag release semantics", () => {
    let storage: [64]u8 = [0; 64];
    let alloc = NullableBump.init(storage[0..]);

    let b = OwnedOrBorrowed.borrowed("static_literal");
    assert(!b.isOwned());
    assertEq(b.origin(), AllocOrigin.Borrowed);
    assertEq(b.slice(), "static_literal");
    assertEq(b.len(), @as(usize, 14));
    assert(!b.isEmpty());
    // Releasing borrowed does not increment allocator freeCount
    b.release(&alloc);
    assertEq(alloc.freeCount(), @as(usize, 0));
    assert(b.isEmpty());

    let owned = OwnedOrBorrowed.fromAlloc(&alloc, "copied_buf") catch OwnedOrBorrowed.borrowed("");
    assert(owned.isOwned());
    assertEq(owned.origin(), AllocOrigin.BumpOwned);
    assertEq(owned.slice(), "copied_buf");
    owned.release(&alloc);
    assertEq(alloc.freeCount(), @as(usize, 1));
    assert(!owned.isOwned());
    assert(owned.isEmpty());

    let tagged_storage: [32]u8 = [0; 32];
    let tagged_alloc = NullableBump.initWithTag(tagged_storage[0..], 7);
    let tagged = OwnedOrBorrowed.fromAlloc(&tagged_alloc, "wtf") catch OwnedOrBorrowed.borrowed("");
    assertEq(tagged.origin(), AllocOrigin.CustomTag);
    assertEq(tagged.slice(), "wtf");
    tagged.release(&tagged_alloc);
    assertEq(tagged_alloc.freeCount(), @as(usize, 1));
});

test("NullableBump: OutOfMemory & InvalidAlignment guards", () => {
    let storage: [16]u8 = [0; 16];
    let alloc = NullableBump.init(storage[0..]);

    let got_bad_align = false;
    * = alloc.allocAligned(4, 3) catch |e| {
        if (e == NullableAllocError.InvalidAlignment) {
            got_bad_align = true;
        }
        storage[0..0]
    };
    assert(got_bad_align);

    let got_zero_align = false;
    * = alloc.allocAligned(4, 0) catch |e| {
        if (e == NullableAllocError.InvalidAlignment) {
            got_zero_align = true;
        }
        storage[0..0]
    };
    assert(got_zero_align);

    * = alloc.alloc(12) catch storage[0..0];
    let got_oom = false;
    * = alloc.alloc(8) catch |e| {
        if (e == NullableAllocError.OutOfMemory) {
            got_oom = true;
        }
        storage[0..0]
    };
    assert(got_oom);
    assertEq(alloc.usedBytes(), @as(usize, 12));
});

test("OwnedOrBorrowed: fromAlloc on empty NullableBump propagates NullAllocator", () => {
    let empty = NullableBump.empty();
    let got_null = false;
    * = OwnedOrBorrowed.fromAlloc(&empty, "nope") catch |e| {
        if (e == NullableAllocError.NullAllocator) {
            got_null = true;
        }
        OwnedOrBorrowed.borrowed("")
    };
    assert(got_null);
});
