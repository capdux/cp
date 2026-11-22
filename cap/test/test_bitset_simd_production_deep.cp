// Source: native-cp-stdlib (Word-Accelerated BitSet)

import * as std from "std";
import cap from "cap";
import { test, expect_eq } from "cap:test";
import { BitSet, BitSetError } from "cap:ds";

test "bitset_simd: cross-word set/unset/toggle and setRange" {
    const alloc = std.testing.allocator;
    let bs = BitSet.initEmpty(alloc, 200)?;
    defer bs.deinit();

    expect_eq(bs.bitLen(), 200);
    expect_eq(bs.count(), 0);

    bs.set(0)?;
    bs.set(63)?;
    bs.set(64)?;
    bs.set(199)?;
    expect_eq(bs.count(), 4);
    expect_eq(bs.isSet(63), true);
    expect_eq(bs.isSet(64), true);

    bs.unset(63)?;
    expect_eq(bs.isSet(63), false);
    bs.toggle(63)?;
    expect_eq(bs.isSet(63), true);
    bs.toggle(64)?;
    expect_eq(bs.isSet(64), false);

    // Cross multiple 64-bit words: [60, 132) => 72 bits
    bs.setRange(60, 132)?;
    expect_eq(bs.isSet(59), false);
    expect_eq(bs.isSet(60), true);
    expect_eq(bs.isSet(131), true);
    expect_eq(bs.isSet(132), false);

    // Single-word setRange: [150, 160) => 10 bits
    bs.setRange(150, 160)?;
    expect_eq(bs.isSet(149), false);
    expect_eq(bs.isSet(150), true);
    expect_eq(bs.isSet(159), true);
    expect_eq(bs.isSet(160), false);

    // Total set bits: {0}, [60, 132) (72 bits), [150, 160) (10 bits), {199} => 84
    expect_eq(bs.count(), 84);
}

test "bitset_simd: popCount and ctz sparse nextSetBit iteration" {
    const alloc = std.testing.allocator;
    let bs = BitSet.initEmpty(alloc, 300)?;
    defer bs.deinit();

    const targets: [6]usize = [3, 63, 64, 129, 255, 299];
    for (idx of targets) {
        bs.set(idx)?;
    }
    expect_eq(bs.count(), 6);

    let collected: [6]usize = undefined;
    let count_found: usize = 0;
    let cursor: usize = 0;
    while (bit_idx := bs.nextSetBit(cursor)) {
        collected[count_found] = bit_idx;
        count_found = count_found + 1;
        cursor = bit_idx + 1;
    }

    expect_eq(count_found, 6);
    let i: usize = 0;
    while (i < 6) {
        expect_eq(collected[i], targets[i]);
        i = i + 1;
    }
    expect_eq(bs.nextSetBit(300) == null, true);
}

test "bitset_simd: unionWith, intersectWith, differenceWith, xorWith" {
    const alloc = std.testing.allocator;
    let a = BitSet.initEmpty(alloc, 130)?;
    defer a.deinit();
    let b = BitSet.initEmpty(alloc, 130)?;
    defer b.deinit();

    a.setRange(10, 70)?;
    b.setRange(50, 110)?;

    // Union: [10, 110) => 100 bits
    let u = BitSet.initEmpty(alloc, 130)?;
    defer u.deinit();
    u.unionWith(a)?;
    u.unionWith(b)?;
    expect_eq(u.count(), 100);

    // Intersection: [50, 70) => 20 bits
    let inter = BitSet.initEmpty(alloc, 130)?;
    defer inter.deinit();
    inter.unionWith(a)?;
    inter.intersectWith(b)?;
    expect_eq(inter.count(), 20);
    expect_eq(inter.isSet(49), false);
    expect_eq(inter.isSet(50), true);
    expect_eq(inter.isSet(69), true);
    expect_eq(inter.isSet(70), false);

    // Difference: a \ b = [10, 50) => 40 bits
    let diff = BitSet.initEmpty(alloc, 130)?;
    defer diff.deinit();
    diff.unionWith(a)?;
    diff.differenceWith(b)?;
    expect_eq(diff.count(), 40);
    expect_eq(diff.isSet(10), true);
    expect_eq(diff.isSet(49), true);
    expect_eq(diff.isSet(50), false);

    // Symmetric difference (xor): [10, 50) U [70, 110) => 80 bits
    let x = BitSet.initEmpty(alloc, 130)?;
    defer x.deinit();
    x.unionWith(a)?;
    x.xorWith(b)?;
    expect_eq(x.count(), 80);
    expect_eq(x.isSet(49), true);
    expect_eq(x.isSet(50), false);
    expect_eq(x.isSet(70), true);
}

test "bitset_simd: initFull tail masking, equals and bounds checks" {
    const alloc = std.testing.allocator;
    let full = BitSet.initFull(alloc, 67)?;
    defer full.deinit();
    expect_eq(full.count(), 67);
    expect_eq(full.isSet(66), true);
    expect_eq(full.isSet(67), false);

    let manual = BitSet.initEmpty(alloc, 67)?;
    defer manual.deinit();
    manual.setRange(0, 67)?;
    expect_eq(full.equals(manual), true);

    let other_len = BitSet.initFull(alloc, 68)?;
    defer other_len.deinit();
    expect_eq(full.equals(other_len), false);

    let oob_hit = false;
    if (_ := full.set(67)) {} else {
        oob_hit = true;
    }
    expect_eq(oob_hit, true);

    let invalid_range_hit = false;
    if (_ := full.setRange(10, 5)) {} else {
        invalid_range_hit = true;
    }
    expect_eq(invalid_range_hit, true);

    let mismatch_hit = false;
    if (_ := full.unionWith(other_len)) {} else {
        mismatch_hit = true;
    }
    expect_eq(mismatch_hit, true);
}

test "bitset_simd: dynamic mount cap.bitset_simd and zero-leak check" {
    const alloc = std.testing.allocator;
    let dyn_bs = cap.ds.BitSet.initEmpty(alloc, 128)?;
    defer dyn_bs.deinit();

    dyn_bs.set(5)?;
    dyn_bs.set(127)?;
    expect_eq(dyn_bs.count(), 2);
    expect_eq(dyn_bs.nextSetBit(6), 127);
}
