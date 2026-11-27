// cap/test/testCapPcg.cp
// Comprehensive unit tests for @cap/pcg

import { test, expect_eq } from "cap:test";
import { Pcg32 } from "../../packages/rand_pcg/src/index.cp";

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
}
