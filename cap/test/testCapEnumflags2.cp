import { test, expect_eq } from "cap:test";
import { BitFlags64 } from "../../packages/@cap/enumflags2/src/index.cp";

const FLAG_A: u64 = 1;       // bit 0
const FLAG_B: u64 = 1 << 1;  // bit 1
const FLAG_C: u64 = 1 << 2;  // bit 2
const FLAG_D: u64 = 1 << 5;  // bit 5
const VALID_MASK: u64 = FLAG_A | FLAG_B | FLAG_C | FLAG_D;

fn main() {
    test("1. @cap/enumflags2 — empty, all, fromBitsTruncate & fromBitsStrict", () => {
        const e = BitFlags64.empty(VALID_MASK);
        expect_eq(e.raw(), 0 as u64);
        expect_eq(e.len(), 0 as usize);
        expect_eq(e.isEmpty(), true);
        expect_eq(e.isAll(), false);

        const a = BitFlags64.all(VALID_MASK);
        expect_eq(a.raw(), VALID_MASK);
        expect_eq(a.len(), 4 as usize);
        expect_eq(a.isEmpty(), false);
        expect_eq(a.isAll(), true);

        const t = BitFlags64.fromBitsTruncate(FLAG_A | FLAG_C | (1 << 3) | (1 << 60), VALID_MASK);
        expect_eq(t.raw(), FLAG_A | FLAG_C);
        expect_eq(t.len(), 2 as usize);

        const s_ok = BitFlags64.fromBitsStrict(FLAG_B | FLAG_D, VALID_MASK);
        expect_eq(s_ok != null, true);
        if (v := s_ok) {
            expect_eq(v.raw(), FLAG_B | FLAG_D);
        }

        const s_bad = BitFlags64.fromBitsStrict(FLAG_A | (1 << 4), VALID_MASK);
        expect_eq(s_bad == null, true);
    });

    test("2. @cap/enumflags2 — insert, remove, toggle, set & clear with valid_mask guard", () => {
        let f = BitFlags64.empty(VALID_MASK);
        f.insert(FLAG_A | FLAG_B | (1 << 7)); // bit 7 is outside VALID_MASK
        expect_eq(f.raw(), FLAG_A | FLAG_B);
        expect_eq(f.contains(FLAG_A), true);
        expect_eq(f.contains(FLAG_A | FLAG_B), true);
        expect_eq(f.contains(1 << 7), false);

        f.toggle(FLAG_B | FLAG_D | (1 << 9));
        expect_eq(f.raw(), FLAG_A | FLAG_D);

        f.set(FLAG_C, true);
        expect_eq(f.raw(), FLAG_A | FLAG_C | FLAG_D);
        f.set(FLAG_A, false);
        expect_eq(f.raw(), FLAG_C | FLAG_D);

        f.remove(FLAG_D);
        expect_eq(f.raw(), FLAG_C);

        f.clear();
        expect_eq(f.isEmpty(), true);
        expect_eq(f.raw(), 0 as u64);
    });

    test("3. @cap/enumflags2 — contains & intersects multi-bit queries", () => {
        const f = BitFlags64.fromBitsTruncate(FLAG_A | FLAG_C, VALID_MASK);
        expect_eq(f.contains(0), true);
        expect_eq(f.contains(FLAG_A), true);
        expect_eq(f.contains(FLAG_A | FLAG_C), true);
        expect_eq(f.contains(FLAG_A | FLAG_B), false);
        expect_eq(f.contains(FLAG_A | (1 << 10)), false);

        expect_eq(f.intersects(FLAG_B | FLAG_C), true);
        expect_eq(f.intersects(FLAG_B | FLAG_D), false);
        expect_eq(f.intersects(1 << 10), false);
    });

    test("4. @cap/enumflags2 — Set algebra: union, intersect, difference, symmetricDifference, complement", () => {
        const x = BitFlags64.fromBitsTruncate(FLAG_A | FLAG_B, VALID_MASK);
        const y = BitFlags64.fromBitsTruncate(FLAG_B | FLAG_C, VALID_MASK);

        expect_eq(x.unionFlags(y).raw(), FLAG_A | FLAG_B | FLAG_C);
        expect_eq(x.intersectFlags(y).raw(), FLAG_B);
        expect_eq(x.differenceFlags(y).raw(), FLAG_A);
        expect_eq(x.symmetricDifferenceFlags(y).raw(), FLAG_A | FLAG_C);
        expect_eq(x.complement().raw(), FLAG_C | FLAG_D);
        expect_eq(BitFlags64.all(VALID_MASK).complement().isEmpty(), true);
        expect_eq(BitFlags64.empty(VALID_MASK).complement().isAll(), true);
    });

    test("5. @cap/enumflags2 — nextFlag bit cursor iteration across 0..63", () => {
        const full_mask: u64 = ~(0 as u64);
        const f = BitFlags64.fromBitsTruncate(
            (1 as u64) | ((1 as u64) << 5) | ((1 as u64) << 33) | ((1 as u64) << 63),
            full_mask
        );
        expect_eq(f.len(), 4 as usize);

        let found: [4]u8 = undefined;
        let count: usize = 0;
        let cur: u8 = 0;
        while (bit := f.nextFlag(cur)) {
            found[count] = bit;
            count += 1;
            if (bit == (63 as u8)) {
                break;
            }
            cur = bit + 1;
        }
        expect_eq(count, 4 as usize);
        expect_eq(found[0], 0 as u8);
        expect_eq(found[1], 5 as u8);
        expect_eq(found[2], 33 as u8);
        expect_eq(found[3], 63 as u8);

        expect_eq(f.nextFlag(64 as u8) == null, true);
        expect_eq(BitFlags64.empty(full_mask).nextFlag(0 as u8) == null, true);
    });

    test("6. @cap/enumflags2 — High-bit popcount & full 64-bit mask edge cases", () => {
        const full_mask: u64 = ~(0 as u64);
        const all64 = BitFlags64.all(full_mask);
        expect_eq(all64.len(), 64 as usize);
        expect_eq(all64.isAll(), true);
        expect_eq(all64.contains((1 as u64) << 63), true);

        const alt_bits: u64 = ((0xAAAAAAAA as u64) << 32) | (0xAAAAAAAA as u64);
        const alt = BitFlags64.fromBitsStrict(alt_bits, full_mask);
        expect_eq(alt != null, true);
        if (v := alt) {
            expect_eq(v.len(), 32 as usize);
            expect_eq(v.complement().raw(), 0x5555555555555555 as u64);
            expect_eq(v.complement().len(), 32 as usize);
        }
    });
}
