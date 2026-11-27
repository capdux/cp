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

    test("Rand32 statePair, fromState, randBool, randRangeI32, fillBytes & advance", () => {
        let r1 = Rand32.new(4242);
        * = r1.randU32();
        const saved = r1.statePair();
        let restored = Rand32.fromState(saved[0], saved[1]);
        expect_eq(r1.randU32(), restored.randU32());
        expect_eq(r1.randU32(), restored.randU32());

        let i: usize = 0;
        while (i < 100) {
            const v = r1.randRangeI32(-50, 50);
            expect_eq(v >= -50 && v < 50, true);
            i += 1;
        }
        * = r1.randBool();

        let step = Rand32.new(888);
        let jump = Rand32.new(888);
        let k: usize = 0;
        while (k < 120) {
            * = step.randU32();
            k += 1;
        }
        jump.advance(120);
        expect_eq(jump.randU32(), step.randU32());

        let buf: [9]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0];
        jump.fillBytes(buf[0..9]);
        expect_eq(buf[0] != 0 || buf[1] != 0 || buf[2] != 0, true);
    });

    test("Rand64 newInc, randI64, randRange, randRangeI64, state, fillBytes & advance", () => {
        let r1 = Rand64.newInc(11, 22, 33, 44);
        let r2 = Rand64.newInc(11, 22, 33, 44);
        const s1 = r1.state();
        const s2 = r2.state();
        expect_eq(s1[0], s2[0]);
        expect_eq(s1[1], s2[1]);
        expect_eq(r1.randI64(), r2.randI64());

        let i: usize = 0;
        while (i < 100) {
            const u = r1.randRange(100, 200);
            expect_eq(u >= 100 && u < 200, true);
            const sg = r1.randRangeI64(-1000, 1000);
            expect_eq(sg >= -1000 && sg < 1000, true);
            i += 1;
        }
        * = r1.randBool();

        let step = Rand64.new(555);
        let jump = Rand64.new(555);
        let k: usize = 0;
        while (k < 80) {
            * = step.randU64();
            k += 1;
        }
        jump.advance(80);
        expect_eq(jump.randU64(), step.randU64());

        let buf: [12]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        jump.fillBytes(buf[0..12]);
        expect_eq(buf[0] != 0 || buf[1] != 0 || buf[2] != 0, true);
    });
}
