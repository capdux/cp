// cap/test/testCapPcg.cp
// Comprehensive unit tests for @cap/pcg

import { test, expect_eq } from "cap:test";
import { Pcg32, Pcg64Mcg } from "../../packages/rand_pcg/src/index.cp";

fn main() {
    test("1. @cap/pcg — official PCG32 reference vector (seed=42, stream=54)", () => {
        // Melissa O'Neill's canonical PCG32 demo vector: pcg32_srandom(42u, 54u)
        let rng = Pcg32.init(42, 54);
        expect_eq(rng.nextU32(), 0xa15c02b7 as u32);
        expect_eq(rng.nextU32(), 0x7b47f409 as u32);
        expect_eq(rng.nextU32(), 0xba1d3330 as u32);
        expect_eq(rng.nextU32(), 0x83d2f293 as u32);
        expect_eq(rng.nextU32(), 0xbfa4784b as u32);
        expect_eq(rng.nextU32(), 0xcbed606e as u32);
    });

    test("2. @cap/pcg — deterministic reproducibility & independent streams", () => {
        let a = Pcg32.init(12345, 1);
        let b = Pcg32.init(12345, 1);
        let c = Pcg32.init(12345, 2);
        expect_eq(a.nextU64(), b.nextU64());
        expect_eq(a.nextU32() != c.nextU32(), true);
    });

    test("3. @cap/pcg — O(log N) advance matches sequential stepping exactly", () => {
        let stepRng = Pcg32.init(999, 7);
        let jumpRng = Pcg32.init(999, 7);

        let i: usize = 0;
        while (i < 500) {
            * = stepRng.nextU32();
            i += 1;
        }
        jumpRng.advance(500);

        expect_eq(jumpRng.nextU32(), stepRng.nextU32());
        expect_eq(jumpRng.nextU32(), stepRng.nextU32());
        expect_eq(jumpRng.nextU64(), stepRng.nextU64());
    });

    test("4. @cap/pcg — unbiased bounded sampling range", () => {
        let rng = Pcg32.init(777, 3);
        let i: usize = 0;
        while (i < 200) {
            const v = rng.nextBoundedU32(10);
            expect_eq(v < (10 as u32), true);
            i += 1;
        }
        expect_eq(rng.nextBoundedU32(1), 0 as u32);
        expect_eq(rng.nextBoundedU32(0), 0 as u32);
    });

    test("5. @cap/pcg — nextF64 strictly in [0.0, 1.0)", () => {
        let rng = Pcg32.init(2026, 11);
        let i: usize = 0;
        while (i < 100) {
            const f = rng.nextF64();
            expect_eq(f >= 0.0 && f < 1.0, true);
            i += 1;
        }
    });

    test("6. @cap/pcg — advance(0) is no-op", () => {
        let a = Pcg32.init(55, 9);
        let b = Pcg32.init(55, 9);
        a.advance(0);
        expect_eq(a.nextU32(), b.nextU32());
    });

    test("7. @cap/pcg — Pcg32 new, seedFromU64, nextF32, nextBool & fillBytes", () => {
        let a = Pcg32.new(42, 54);
        let b = Pcg32.init(42, 54);
        expect_eq(a.nextU32(), b.nextU32());

        let s1 = Pcg32.seedFromU64(123456);
        let s2 = Pcg32.seedFromU64(123456);
        expect_eq(s1.nextU64(), s2.nextU64());

        const f32v = s1.nextF32();
        expect_eq(f32v >= 0.0 && f32v < 1.0, true);
        * = s1.nextBool();

        let buf1: [11]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let buf2: [11]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let r1 = Pcg32.seedFromU64(99);
        let r2 = Pcg32.seedFromU64(99);
        r1.fillBytes(buf1[0..11]);
        r2.fillBytes(buf2[0..11]);
        let i: usize = 0;
        let nonzero: bool = false;
        while (i < 11) {
            expect_eq(buf1[i], buf2[i]);
            if (buf1[i] != 0) { nonzero = true; }
            i += 1;
        }
        expect_eq(nonzero, true);
    });

    test("8. @cap/pcg — Pcg64Mcg determinism, fillBytes, nextF64 & O(log N) advance", () => {
        let step = Pcg64Mcg.new(0x123456789ABCDEF0, 0x0FEDCBA987654321);
        let jump = Pcg64Mcg.new(0x123456789ABCDEF0, 0x0FEDCBA987654321);
        let i: usize = 0;
        while (i < 250) {
            * = step.nextU64();
            i += 1;
        }
        jump.advance(250);
        expect_eq(jump.nextU64(), step.nextU64());
        expect_eq(jump.nextU32(), step.nextU32());

        let seeded = Pcg64Mcg.seedFromU64(2026);
        const fv = seeded.nextF64();
        expect_eq(fv >= 0.0 && fv < 1.0, true);
        let bytes: [13]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        seeded.fillBytes(bytes[0..13]);
        expect_eq(bytes[0] != 0 || bytes[1] != 0 || bytes[2] != 0, true);
    });
}
