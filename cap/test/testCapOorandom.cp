import { test, expect_eq } from "cap:test";
import { Rand32, Rand64 } from "../../packages/oorandom/src/index.cp";

fn main() {
    test("Rand32 PCG32 standard reference vector (seed=42, inc=54)", () => {
        // Official PCG32 pcg32-demo vector: pcg32_srandom(42, 54)
        let rng = Rand32.newInc(42, 54);
        expect_eq(rng.randU32(), 0xA15C02B7);
        expect_eq(rng.randU32(), 0x7B47F409);
        expect_eq(rng.randU32(), 0xBA1D3330);
        expect_eq(rng.randU32(), 0x83D2F293);
        expect_eq(rng.randU32(), 0xBFA4784B);
        expect_eq(rng.randU32() != 0, true);
    });

    test("Rand32 determinism and state progression", () => {
        let r1 = Rand32.new(12345);
        let r2 = Rand32.new(12345);
        expect_eq(r1.state(), r2.state());

        let i: usize = 0;
        while (i < 16) {
            expect_eq(r1.randU32(), r2.randU32());
            i += 1;
        }
        expect_eq(r1.state(), r2.state());

        let r3 = Rand32.new(12346);
        expect_eq(r1.state() != r3.state(), true);
    });

    test("Rand32 randI32 and randFloat bounds", () => {
        let rng = Rand32.new(999);
        let neg_count: usize = 0;
        let pos_count: usize = 0;
        let i: usize = 0;
        while (i < 100) {
            const v = rng.randI32();
            if (v < 0) {
                neg_count += 1;
            } else {
                pos_count += 1;
            }
            const f = rng.randFloat();
            expect_eq(f >= 0.0 && f < 1.0, true);
            i += 1;
        }
        expect_eq(neg_count > 10 && pos_count > 10, true);
    });

    test("Rand32 randRange unbiased bounds and edge cases", () => {
        let rng = Rand32.new(777);
        expect_eq(rng.randRange(10, 10), 10);
        expect_eq(rng.randRange(20, 10), 20);
        expect_eq(rng.randRange(5, 6), 5);

        let i: usize = 0;
        while (i < 200) {
            const v = rng.randRange(10, 25);
            expect_eq(v >= 10 && v < 25, true);
            i += 1;
        }
    });

    test("Rand64 determinism and full 64-bit entropy", () => {
        let r1 = Rand64.new(2026);
        let r2 = Rand64.new(2026);
        let saw_high_bit: bool = false;
        let i: usize = 0;
        while (i < 32) {
            const v1 = r1.randU64();
            const v2 = r2.randU64();
            expect_eq(v1, v2);
            if ((v1 >> 32) != 0) {
                saw_high_bit = true;
            }
            i += 1;
        }
        expect_eq(saw_high_bit, true);
    });

    test("Rand64 randFloat stays strictly in [0.0, 1.0)", () => {
        let rng = Rand64.new(314159);
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < 100) {
            const f = rng.randFloat();
            expect_eq(f >= 0.0 && f < 1.0, true);
            sum = sum + f;
            i += 1;
        }
        expect_eq(sum > 20.0 && sum < 80.0, true);
    });
}
