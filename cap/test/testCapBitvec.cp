import { test, expect_eq } from "cap:test";
import { BitVec512, BITVEC_MAX_BITS } from "../../packages/@cap/bitvec/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/bitvec — push, pop, get, set, toggle", () => {
        let bv = BitVec512.new();
        expect_eq(bv.isEmpty(), true);
        expect_eq(bv.push(true), true);
        expect_eq(bv.push(false), true);
        expect_eq(bv.push(true), true);
        expect_eq(bv.len(), 3);
        expect_eq(bv.get(0), true);
        expect_eq(bv.get(1), false);
        expect_eq(bv.get(2), true);

        expect_eq(bv.toggle(1), true);
        expect_eq(bv.get(1), true);
        expect_eq(bv.all(), true);

        expect_eq(bv.pop(), 1);
        expect_eq(bv.len(), 2);
    });

    test("2. @cap/bitvec — withLen, countOnes, countZeros, any, all", () => {
        let bv = BitVec512.withLen(130, true);
        expect_eq(bv.len(), 130);
        expect_eq(bv.countOnes(), 130);
        expect_eq(bv.countZeros(), 0);
        expect_eq(bv.all(), true);
        expect_eq(bv.any(), true);

        * = bv.set(64, false);
        * = bv.set(129, false);
        expect_eq(bv.countOnes(), 128);
        expect_eq(bv.countZeros(), 2);
        expect_eq(bv.all(), false);
    });

    test("3. @cap/bitvec — firstOne and lastOne across 64-bit word boundaries", () => {
        let bv = BitVec512.withLen(200, false);
        expect_eq(bv.firstOne(), -1);
        expect_eq(bv.lastOne(), -1);
        * = bv.set(70, true);
        * = bv.set(195, true);
        expect_eq(bv.firstOne(), 70);
        expect_eq(bv.lastOne(), 195);
    });

    test("4. @cap/bitvec — Bitwise AND, OR, XOR, and NOT", () => {
        let a = BitVec512.withLen(4, false);
        * = a.set(0, true);
        * = a.set(2, true); // 1010

        let b = BitVec512.withLen(4, false);
        * = b.set(1, true);
        * = b.set(2, true); // 0110

        let c = a;
        c.andWith(b); // 0010
        expect_eq(c.countOnes(), 1);
        expect_eq(c.get(2), true);

        let d = a;
        d.orWith(b); // 1110
        expect_eq(d.countOnes(), 3);

        let e = a;
        e.xorWith(b); // 1100
        expect_eq(e.countOnes(), 2);
        expect_eq(e.get(0), true);
        expect_eq(e.get(1), true);

        e.notAll(); // 0011
        expect_eq(e.countOnes(), 2);
        expect_eq(e.get(2), true);
        expect_eq(e.get(3), true);
    });

    test("5. @cap/bitvec — toBinStr formatting", () => {
        let bv = BitVec512.withLen(5, false);
        * = bv.set(0, true);
        * = bv.set(3, true);
        * = bv.set(4, true);
        let buf: [16]u8 = undefined;
        let n = bv.toBinStr(buf[0..16]);
        expect_eq(eqBytes(buf[0..16], n, "10011"), true);
    });

    test("6. @cap/bitvec — Max capacity 512 and out-of-bounds safety", () => {
        let bv = BitVec512.withLen(BITVEC_MAX_BITS, true);
        expect_eq(bv.countOnes(), 512);
        expect_eq(bv.push(true), false);
        expect_eq(bv.set(512, false), false);
        expect_eq(bv.get(512), false);
        bv.clear();
        expect_eq(bv.isEmpty(), true);
        expect_eq(bv.pop(), -1);
    });
}
