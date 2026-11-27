// cap/test/testCapPcg32.cp
// Comprehensive unit tests for @cap/pcg32 (PCG-XSH-RR 32-bit PRNG)

import { test, expect_eq } from "cap:test";
import { Pcg32 } from "../../packages/pcg-random/src/index.cp";

fn main() {
    test("1. @cap/pcg32 — deterministic reproducible sequence from same seed & stream", () => {
        let a: Pcg32 = Pcg32.new(42, 54);
        let b: Pcg32 = Pcg32.new(42, 54);

        // Official PCG32 reference vector for seed=42, seq=54:
        // first output is 0xa15c02b7 (2707161783)
        const v0: u32 = a.nextU32();
        expect_eq(v0, 0xA15C02B7 as u32);
        expect_eq(b.nextU32(), v0);

        let i: usize = 0;
        while (i < 16) {
            expect_eq(a.nextU32(), b.nextU32());
            i += 1;
        }
    });

    test("2. @cap/pcg32 — different streams produce independent sequences", () => {
        let s1: Pcg32 = Pcg32.new(12345, 1);
        let s2: Pcg32 = Pcg32.new(12345, 2);

        let diffs: usize = 0;
        let i: usize = 0;
        while (i < 10) {
            if (s1.nextU32() != s2.nextU32()) {
                diffs += 1;
            }
            i += 1;
        }
        expect_eq(diffs, 10 as usize);
    });

    test("3. @cap/pcg32 — nextBounded stays strictly within [0, bound)", () => {
        let rng: Pcg32 = Pcg32.fromSeed(999);
        expect_eq(rng.nextBounded(0), 0 as u32);
        expect_eq(rng.nextBounded(1), 0 as u32);

        let seen: [10]bool = [false, false, false, false, false, false, false, false, false, false];
        let i: usize = 0;
        while (i < 200) {
            const v: u32 = rng.nextBounded(10);
            expect_eq(v < 10, true);
            seen[v as usize] = true;
            i += 1;
        }
        let k: usize = 0;
        while (k < 10) {
            expect_eq(seen[k], true);
            k += 1;
        }
    });

    test("4. @cap/pcg32 — advance(delta) matches stepping delta times in O(log n)", () => {
        let fast: Pcg32 = Pcg32.new(777, 888);
        let slow: Pcg32 = Pcg32.new(777, 888);

        fast.advance(1000);
        let i: usize = 0;
        while (i < 1000) {
            * = slow.nextU32();
            i += 1;
        }
        expect_eq(fast.nextU32(), slow.nextU32());
        expect_eq(fast.nextU64(), slow.nextU64());
    });

    test("5. @cap/pcg32 — fillBytes deterministic & non-trivial output", () => {
        let r1: Pcg32 = Pcg32.fromSeed(2026);
        let r2: Pcg32 = Pcg32.fromSeed(2026);
        let b1: [23]u8 = undefined;
        let b2: [23]u8 = undefined;

        r1.fillBytes(b1[0..23]);
        r2.fillBytes(b2[0..23]);

        let nonzero: usize = 0;
        let i: usize = 0;
        while (i < 23) {
            expect_eq(b1[i], b2[i]);
            if (b1[i] != 0) {
                nonzero += 1;
            }
            i += 1;
        }
        expect_eq(nonzero > 10, true);
    });

    test("6. @cap/pcg32 — nextF64 produces uniform values in [0.0, 1.0)", () => {
        let rng: Pcg32 = Pcg32.fromSeed(314159);
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < 200) {
            const f: f64 = rng.nextF64();
            expect_eq(f >= 0.0 && f < 1.0, true);
            sum = sum + f;
            i += 1;
        }
        const mean: f64 = sum / 200.0;
        expect_eq(mean > 0.35 && mean < 0.65, true);
    });
}
