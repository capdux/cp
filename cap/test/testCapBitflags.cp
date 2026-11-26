import { test, expect_eq } from "cap:test";
import {
    Flags64,
    empty,
    fromBits,
    fromBitIndex,
    isEmpty,
    contains,
    intersects,
    unionFlags,
    intersection,
    difference,
    symmetricDifference,
    insert,
    remove,
    toggle,
    setCond,
    truncate,
    countBits,
    lowestBitIndex,
    formatHex,
} from "../../packages/@cap/bitflags/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/bitflags — construction, contains & intersects", () => {
        let READ: Flags64 = fromBitIndex(0);
        let WRITE: Flags64 = fromBitIndex(1);
        let EXEC: Flags64 = fromBitIndex(2);

        let rw: Flags64 = unionFlags(READ, WRITE);
        expect_eq(isEmpty(empty()), true);
        expect_eq(contains(rw, READ), true);
        expect_eq(contains(rw, WRITE), true);
        expect_eq(contains(rw, EXEC), false);
        expect_eq(intersects(rw, unionFlags(WRITE, EXEC)), true);
        expect_eq(intersects(rw, EXEC), false);
    });

    test("2. @cap/bitflags — set algebra (intersection, difference, symmetricDifference)", () => {
        let a: Flags64 = fromBits(0b1101);
        let b: Flags64 = fromBits(0b1011);
        expect_eq(intersection(a, b).bits, 0b1001);
        expect_eq(difference(a, b).bits, 0b0100);
        expect_eq(symmetricDifference(a, b).bits, 0b0110);
    });

    test("3. @cap/bitflags — in-place insert, remove, toggle & setCond", () => {
        let f: Flags64 = empty();
        let A: Flags64 = fromBitIndex(0);
        let B: Flags64 = fromBitIndex(3);
        insert(&f, A);
        insert(&f, B);
        expect_eq(f.bits, 0b1001);
        toggle(&f, A);
        expect_eq(f.bits, 0b1000);
        remove(&f, B);
        expect_eq(isEmpty(f), true);
        setCond(&f, B, true);
        expect_eq(contains(f, B), true);
        setCond(&f, B, false);
        expect_eq(isEmpty(f), true);
    });

    test("4. @cap/bitflags — truncate mask & countBits & lowestBitIndex", () => {
        let t: Flags64 = truncate(0xff, 0x0f);
        expect_eq(t.bits, 0x0f);
        expect_eq(countBits(t), 4);
        expect_eq(lowestBitIndex(empty()), -1);
        expect_eq(lowestBitIndex(fromBitIndex(10)), 10);
        expect_eq(lowestBitIndex(fromBits(0b10100)), 2);
    });

    test("5. @cap/bitflags — 63rd high bit & out-of-range index guard", () => {
        let hi: Flags64 = fromBitIndex(63);
        expect_eq(countBits(hi), 1);
        expect_eq(lowestBitIndex(hi), 63);
        expect_eq(isEmpty(fromBitIndex(64)), true);
    });

    test("6. @cap/bitflags — formatHex & small buffer protection", () => {
        let buf: [32]u8 = undefined;
        let n0: usize = formatHex(empty(), buf[0..]);
        expect_eq(eqStr(buf[0..n0], "0x0"), true);

        let n1: usize = formatHex(fromBits(0xdeadbeef), buf[0..]);
        expect_eq(eqStr(buf[0..n1], "0xdeadbeef"), true);

        let tiny: [2]u8 = undefined;
        expect_eq(formatHex(fromBits(0xff), tiny[0..]), 0);
    });
}
