import { test, expect_eq } from "cap:test";
import { Flags32, Flags64 } from "../../packages/@cap/bitflags-crate/src/index.cp";

fn main() {
    test("1. @cap/bitflags-crate — Flags32 empty, all, insert, remove & toggle", () => {
        let mask: u32 = 0b1111;
        let f = Flags32.empty(mask);
        expect_eq(f.isEmpty(), true);
        expect_eq(f.isAll(), false);
        f.insert(0b0011);
        expect_eq(f.contains(0b0001), true);
        expect_eq(f.contains(0b0100), false);
        expect_eq(f.intersects(0b0110), true);
        expect_eq(f.count(), 2);
        f.toggle(0b0110);
        expect_eq(f.bits(), 0b0101);
        f.remove(0b0001);
        expect_eq(f.bits(), 0b0100);
    });

    test("2. @cap/bitflags-crate — fromBits strict vs truncate vs retain", () => {
        let mask: u32 = 0x0f;
        expect_eq(Flags32.fromBits(0x1f, mask) == null, true);
        let strict = Flags32.fromBits(0x05, mask) ?? Flags32.empty(mask);
        expect_eq(strict.bits(), 0x05);
        let trunc = Flags32.fromBitsTruncate(0xff, mask);
        expect_eq(trunc.bits(), 0x0f);
        let ret = Flags32.fromBitsRetain(0x15, mask);
        expect_eq(ret.containsUnknownBits(), true);
        ret.truncate();
        expect_eq(ret.containsUnknownBits(), false);
        expect_eq(ret.bits(), 0x05);
    });

    test("3. @cap/bitflags-crate — Set algebra (union, intersection, difference, symDiff, complement)", () => {
        let mask: u32 = 0b1111;
        let a = Flags32.fromBitsTruncate(0b0110, mask);
        let b = Flags32.fromBitsTruncate(0b1100, mask);
        expect_eq(a.unionWith(b).bits(), 0b1110);
        expect_eq(a.intersection(b).bits(), 0b0100);
        expect_eq(a.difference(b).bits(), 0b0010);
        expect_eq(a.symmetricDifference(b).bits(), 0b1010);
        expect_eq(a.complement().bits(), 0b1001);
    });

    test("4. @cap/bitflags-crate — set(flag, bool) & clear", () => {
        let mask: u32 = 0b1111;
        let f = Flags32.all(mask);
        expect_eq(f.isAll(), true);
        f.set(0b0010, false);
        expect_eq(f.bits(), 0b1101);
        f.set(0b0010, true);
        expect_eq(f.isAll(), true);
        f.clear();
        expect_eq(f.isEmpty(), true);
    });

    test("5. @cap/bitflags-crate — Flags64 high-bit flags (bit 40 & bit 60)", () => {
        let b40: u64 = (1 as u64) << 40;
        let b60: u64 = (1 as u64) << 60;
        let mask = b40 | b60;
        let f = Flags64.empty(mask);
        f.insert(b40);
        expect_eq(f.contains(b40), true);
        expect_eq(f.contains(b60), false);
        expect_eq(f.count(), 1);
        expect_eq(f.complement().bits(), b60);
    });

    test("6. @cap/bitflags-crate — Flags64 fromBits rejects undefined bits", () => {
        let mask: u64 = 0xff;
        expect_eq(Flags64.fromBits(0x100, mask) == null, true);
        let ok = Flags64.fromBits(0xaa, mask) ?? Flags64.empty(mask);
        expect_eq(ok.count(), 4);
    });
}
