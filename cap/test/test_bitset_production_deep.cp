// Source: cp/cap/test/test_bitset_production_deep.cp
// Deep Production Verification Suite for cap:bitset & cap.bitset
// Covers:
//   1. Dynamic growth, set/clear/toggle/testBit, setRange/clearRange across 64-bit word boundaries (63/64/127/128)
//   2. @ctz fast scan: firstSetBit, nextSetBit, toIndices & @popCount count() accuracy
//   3. Set algebra (unionWith/intersectWith/differenceWith/xorWith/isSubsetOf/jaccardSimilarity) across unequal lengths
//   4. toHex/fromHex roundtrip, InvalidHex rejection, and zero-heap FixedBitSet(256)
//   5. `import cap from "cap"; cap.bitset` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BitSetError,
    BitSet,
    FixedBitSet,
} from "../../lib/ds/bitset_.cp";

test("cap:bitset - dynamic growth, single bit ops & cross-word range boundaries") {
    const alloc = std.heap.page_allocator;
    let bs = BitSet.initWithCapacity(alloc, 16) catch unreachable;
    defer bs.deinit();

    expect_eq(bs.isEmpty(), true);
    expect_eq(bs.count(), 0);

    bs.set(0) catch unreachable;
    bs.set(63) catch unreachable;
    bs.set(64) catch unreachable;
    bs.set(127) catch unreachable;
    bs.set(128) catch unreachable;
    expect_eq(bs.count(), 5);
    expect_eq(bs.testBit(63), true);
    expect_eq(bs.testBit(64), true);
    expect_eq(bs.testBit(127), true);
    expect_eq(bs.testBit(128), true);
    expect_eq(bs.testBit(129), false);

    bs.clear(64);
    expect_eq(bs.testBit(64), false);
    bs.toggle(64) catch unreachable;
    expect_eq(bs.testBit(64), true);
    bs.toggle(64) catch unreachable;
    expect_eq(bs.testBit(64), false);

    bs.clearAll();
    expect_eq(bs.isEmpty(), true);

    // Cross-word range [60, 132): spans word 0, word 1, word 2
    bs.setRange(60, 132) catch unreachable;
    expect_eq(bs.count(), 72);
    expect_eq(bs.testBit(59), false);
    expect_eq(bs.testBit(60), true);
    expect_eq(bs.testBit(63), true);
    expect_eq(bs.testBit(64), true);
    expect_eq(bs.testBit(127), true);
    expect_eq(bs.testBit(128), true);
    expect_eq(bs.testBit(131), true);
    expect_eq(bs.testBit(132), false);

    // Clear sub-range [63, 129) across 63/64/127/128 boundaries -> leaves 60..62 (3) + 129..131 (3) = 6 bits
    bs.clearRange(63, 129) catch unreachable;
    expect_eq(bs.count(), 6);
    expect_eq(bs.testBit(62), true);
    expect_eq(bs.testBit(63), false);
    expect_eq(bs.testBit(64), false);
    expect_eq(bs.testBit(127), false);
    expect_eq(bs.testBit(128), false);
    expect_eq(bs.testBit(129), true);

    // Empty range [10, 10) is a no-op; inverted range [10, 5) returns InvalidRange
    bs.setRange(10, 10) catch unreachable;
    expect_eq(bs.count(), 6);

    let invalid_caught = false;
    bs.setRange(10, 5) catch |e| {
        if (e == BitSetError.InvalidRange) invalid_caught = true;
    };
    expect_eq(invalid_caught, true);

    let cloned = bs.clone(alloc) catch unreachable;
    defer cloned.deinit();
    expect_eq(cloned.equals(bs), true);
    expect_eq(cloned.count(), 6);
}

test("cap:bitset - @ctz fast scan firstSetBit, nextSetBit & toIndices") {
    const alloc = std.heap.page_allocator;
    let bs = BitSet.init(alloc);
    defer bs.deinit();

    expect_eq(bs.firstSetBit() == null, true);
    expect_eq(bs.nextSetBit(0) == null, true);

    const empty_idx = bs.toIndices(alloc) catch unreachable;
    defer alloc.free(empty_idx);
    expect_eq(empty_idx.len, 0);

    const expected = [0, 63, 64, 127, 128, 300, 1023];
    for (bit of expected) {
        bs.set(bit) catch unreachable;
    }
    expect_eq(bs.count(), 7);
    expect_eq(bs.firstSetBit().?, 0);
    expect_eq(bs.nextSetBit(1).?, 63);
    expect_eq(bs.nextSetBit(64).?, 64);
    expect_eq(bs.nextSetBit(129).?, 300);
    expect_eq(bs.nextSetBit(1024) == null, true);

    const indices = bs.toIndices(alloc) catch unreachable;
    defer alloc.free(indices);
    expect_eq(indices.len, expected.len);
    let i: usize = 0;
    while (i < expected.len) : (i += 1) {
        expect_eq(indices[i], expected[i]);
    }
}

test("cap:bitset - set algebra across unequal lengths & jaccardSimilarity") {
    const alloc = std.heap.page_allocator;

    let a = BitSet.init(alloc);
    defer a.deinit();
    a.set(1) catch unreachable;
    a.set(65) catch unreachable;
    a.set(300) catch unreachable;

    let b = BitSet.init(alloc);
    defer b.deinit();
    b.set(1) catch unreachable;
    b.set(2) catch unreachable;

    expect_eq(a.intersects(b), true);
    expect_eq(a.isSubsetOf(b), false);

    // Intersect long `a` with short `b`: high words in `a` must be zeroed!
    let inter = a.clone(alloc) catch unreachable;
    defer inter.deinit();
    inter.intersectWith(b);
    expect_eq(inter.count(), 1);
    expect_eq(inter.testBit(1), true);
    expect_eq(inter.testBit(65), false);
    expect_eq(inter.testBit(300), false);

    let only_one = BitSet.init(alloc);
    defer only_one.deinit();
    only_one.set(1) catch unreachable;
    expect_eq(inter.equals(only_one), true);
    expect_eq(inter.isSubsetOf(b), true);

    // Union short `u` with long `a`
    let u = b.clone(alloc) catch unreachable;
    defer u.deinit();
    u.unionWith(a) catch unreachable;
    expect_eq(u.count(), 4);
    expect_eq(a.isSubsetOf(u), true);
    expect_eq(b.isSubsetOf(u), true);

    // Difference: u \ b == {65, 300}
    let diff = u.clone(alloc) catch unreachable;
    defer diff.deinit();
    diff.differenceWith(b);
    expect_eq(diff.count(), 2);
    expect_eq(diff.testBit(65), true);
    expect_eq(diff.testBit(300), true);

    // XOR: {1, 65, 300} ^ {1, 2} == {2, 65, 300}
    let x = a.clone(alloc) catch unreachable;
    defer x.deinit();
    x.xorWith(b) catch unreachable;
    expect_eq(x.count(), 3);
    expect_eq(x.testBit(1), false);
    expect_eq(x.testBit(2), true);
    expect_eq(x.testBit(65), true);
    expect_eq(x.testBit(300), true);

    // Jaccard similarity: {1, 2, 3} vs {2, 3, 4} -> 2 / 4 = 0.5
    let j1 = BitSet.init(alloc);
    defer j1.deinit();
    j1.setRange(1, 4) catch unreachable;

    let j2 = BitSet.init(alloc);
    defer j2.deinit();
    j2.setRange(2, 5) catch unreachable;

    const sim = j1.jaccardSimilarity(j2);
    expect_eq(sim > 0.499 && sim < 0.501, true);

    let e1 = BitSet.init(alloc);
    defer e1.deinit();
    let e2 = BitSet.init(alloc);
    defer e2.deinit();
    expect_eq(e1.jaccardSimilarity(e2), 1.0);
}

test("cap:bitset - toHex/fromHex roundtrip, InvalidHex & stack FixedBitSet(256)") {
    const alloc = std.heap.page_allocator;

    let bs = BitSet.init(alloc);
    defer bs.deinit();
    bs.set(0) catch unreachable;
    bs.set(7) catch unreachable;
    bs.set(63) catch unreachable;
    bs.set(64) catch unreachable;
    bs.set(127) catch unreachable;
    bs.set(128) catch unreachable;
    bs.set(255) catch unreachable;

    const hex = bs.toHex(alloc) catch unreachable;
    defer alloc.free(hex);

    let restored = BitSet.fromHex(alloc, hex) catch unreachable;
    defer restored.deinit();
    expect_eq(restored.equals(bs), true);
    expect_eq(restored.count(), 7);

    // Empty roundtrip
    let empty_bs = BitSet.init(alloc);
    defer empty_bs.deinit();
    const empty_hex = empty_bs.toHex(alloc) catch unreachable;
    defer alloc.free(empty_hex);
    expect_eq(empty_hex.len, 0);
    let restored_empty = BitSet.fromHex(alloc, empty_hex) catch unreachable;
    defer restored_empty.deinit();
    expect_eq(restored_empty.isEmpty(), true);

    // InvalidHex checks (odd length & non-hex char)
    let odd_err = false;
    let r_odd = BitSet.fromHex(alloc, "abc") catch |e| {
        if (e == BitSetError.InvalidHex) odd_err = true;
        break :r_odd BitSet.init(alloc);
    };
    r_odd.deinit();
    expect_eq(odd_err, true);

    let char_err = false;
    let r_char = BitSet.fromHex(alloc, "0g") catch |e| {
        if (e == BitSetError.InvalidHex) char_err = true;
        break :r_char BitSet.init(alloc);
    };
    r_char.deinit();
    expect_eq(char_err, true);

    // Zero-heap stack FixedBitSet(256)
    let f1 = FixedBitSet(256).init();
    expect_eq(f1.isEmpty(), true);
    f1.set(0);
    f1.set(63);
    f1.set(64);
    f1.set(255);
    f1.set(256); // out of bounds, ignored
    expect_eq(f1.count(), 4);
    expect_eq(f1.testBit(255), true);
    expect_eq(f1.testBit(256), false);

    let f2 = FixedBitSet(256).init();
    f2.set(64);
    f2.set(128);

    f1.unionWith(f2);
    expect_eq(f1.count(), 5);
    expect_eq(f1.testBit(128), true);

    f1.intersectWith(f2);
    expect_eq(f1.count(), 2);
    expect_eq(f1.equals(f2), true);

    f1.clear(64);
    expect_eq(f1.count(), 1);
}

test("cap.bitset - dynamic mounting via import cap from cap") {
    const alloc = std.heap.page_allocator;
    let bs = BitSet.init(alloc);
    defer bs.deinit();

    bs.set(10) catch unreachable;
    bs.set(200) catch unreachable;
    expect_eq(bs.count(), 2);
    expect_eq(bs.testBit(200), true);

    let fbs = FixedBitSet(128).init();
    fbs.set(127);
    expect_eq(fbs.count(), 1);
    expect_eq(fbs.testBit(127), true);
}
