// cap/test/testCapCuid2.cp
// Comprehensive unit tests for @cap/cuid2

import { test, expect_eq } from "cap:test";
import {
    Cuid2Generator,
    isCuid2,
    isCuid2WithLen,
} from "../../packages/@cap/cuid2/src/index.cp";

fn bytesEqual(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/cuid2 — Default 24-char generation, leading letter & Base36 compliance", () => {
        let gen = Cuid2Generator.init(0xCAFEBABE as u64, 0x12345678 as u64, 24 as usize);
        let out: [32]u8 = undefined;
        const n = gen.next(1700000000000 as u64, out[0..32]);
        expect_eq(n, 24 as usize);
        expect_eq(gen.counterValue(), 1 as u64);
        expect_eq(isCuid2(out[0..n]), true);
        expect_eq(isCuid2WithLen(out[0..n], 24 as usize), true);
        expect_eq(out[0] >= 'a' && out[0] <= 'z', true);
    });

    test("2. @cap/cuid2 — 100 IDs in the same millisecond are 100% collision-free", () => {
        let gen = Cuid2Generator.init(1001 as u64, 9999 as u64, 24 as usize);
        let ids: [100][24]u8 = undefined;
        let i: usize = 0;
        while (i < 100) {
            const n = gen.next(1700000000000 as u64, ids[i][0..24]);
            expect_eq(n, 24 as usize);
            expect_eq(isCuid2(ids[i][0..24]), true);
            i += 1;
        }
        expect_eq(gen.counterValue(), 100 as u64);

        let a: usize = 0;
        while (a < 100) {
            let b = a + (1 as usize);
            while (b < 100) {
                expect_eq(bytesEqual(ids[a][0..24], ids[b][0..24]), false);
                b += 1;
            }
            a += 1;
        }
    });

    test("3. @cap/cuid2 — Distinct fingerprint & seed produce isolated IDs", () => {
        let g1 = Cuid2Generator.init(1 as u64, 100 as u64, 24 as usize);
        let g2 = Cuid2Generator.init(2 as u64, 100 as u64, 24 as usize);
        let g3 = Cuid2Generator.init(1 as u64, 200 as u64, 24 as usize);

        let b1: [32]u8 = undefined;
        let b2: [32]u8 = undefined;
        let b3: [32]u8 = undefined;
        * = g1.next(1700000000000 as u64, b1[0..32]);
        * = g2.next(1700000000000 as u64, b2[0..32]);
        * = g3.next(1700000000000 as u64, b3[0..32]);

        expect_eq(bytesEqual(b1[0..24], b2[0..24]), false);
        expect_eq(bytesEqual(b1[0..24], b3[0..24]), false);
        expect_eq(bytesEqual(b2[0..24], b3[0..24]), false);
    });

    test("4. @cap/cuid2 — Custom lengths (10, 32) and clamping (<8 -> 8, >32 -> 32)", () => {
        let g10 = Cuid2Generator.init(5 as u64, 6 as u64, 10 as usize);
        let b10: [32]u8 = undefined;
        expect_eq(g10.next(1 as u64, b10[0..32]), 10 as usize);
        expect_eq(isCuid2WithLen(b10[0..10], 10 as usize), true);

        let g32 = Cuid2Generator.init(5 as u64, 6 as u64, 32 as usize);
        let b32: [32]u8 = undefined;
        expect_eq(g32.next(1 as u64, b32[0..32]), 32 as usize);
        expect_eq(isCuid2WithLen(b32[0..32], 32 as usize), true);

        let gSmall = Cuid2Generator.init(5 as u64, 6 as u64, 3 as usize);
        let bSmall: [32]u8 = undefined;
        expect_eq(gSmall.next(1 as u64, bSmall[0..32]), 8 as usize);

        let gBig = Cuid2Generator.init(5 as u64, 6 as u64, 99 as usize);
        let bBig: [40]u8 = undefined;
        expect_eq(gBig.next(1 as u64, bBig[0..40]), 32 as usize);
    });

    test("5. @cap/cuid2 — isCuid2 rejects uppercase, leading digit, hyphen & empty", () => {
        expect_eq(isCuid2("clh3am123000008l4abcd567"), true);
        expect_eq(isCuid2("a1"), true);
        expect_eq(isCuid2(""), false);
        expect_eq(isCuid2("a"), false);
        expect_eq(isCuid2("1abcde"), false);
        expect_eq(isCuid2("Abcde123"), false);
        expect_eq(isCuid2("abc-def123"), false);
        expect_eq(isCuid2("abc_def123"), false);
    });

    test("6. @cap/cuid2 — Deterministic reproducibility & small buffer guard", () => {
        let gA = Cuid2Generator.init(77 as u64, 88 as u64, 24 as usize);
        let gB = Cuid2Generator.init(77 as u64, 88 as u64, 24 as usize);

        let outA: [32]u8 = undefined;
        let outB: [32]u8 = undefined;
        expect_eq(gA.next(123456789 as u64, outA[0..32]), 24 as usize);
        expect_eq(gB.next(123456789 as u64, outB[0..32]), 24 as usize);
        expect_eq(bytesEqual(outA[0..24], outB[0..24]), true);

        let tiny: [10]u8 = undefined;
        expect_eq(gA.next(123456789 as u64, tiny[0..10]), 0 as usize);
    });
}
