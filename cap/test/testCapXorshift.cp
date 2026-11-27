// cap/test/testCapXorshift.cp
// Comprehensive unit tests for @cap/xorshift

import { test, expect_eq } from "cap:test";
import {
    XorShift128,
    XorShift64Star,
    XorShift32
} from "../../packages/rand_xorshift/src/index.cp";

fn main() {
    test("1. @cap/xorshift — XorShift128 determinism & official seed=[1,0,...,0] state transition", () => {
        let seed: [16]u8 = [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let r1 = XorShift128.fromSeed(seed);
        let r2 = XorShift128.fromSeed(seed);

        // Exact Marsaglia / rand_xorshift sequence for (x=1, y=0, z=0, w=0)
        expect_eq(r1.nextU32(), 2057 as u32);
        expect_eq(r1.nextU32(), 2057 as u32);
        expect_eq(r1.nextU32(), 2057 as u32);
        expect_eq(r1.nextU32(), 2057 as u32);
        expect_eq(r1.nextU32(), 4196416 as u32);

        // Identical seed produces identical sequence
        expect_eq(r2.nextU32(), 2057 as u32);
        expect_eq(r2.nextU32(), 2057 as u32);
        expect_eq(r2.nextU32(), 2057 as u32);
        expect_eq(r2.nextU32(), 2057 as u32);
        expect_eq(r2.nextU32(), 4196416 as u32);
    });

    test("2. @cap/xorshift — XorShift128.fromSeed all-zero fallback avoids stuck zero state", () => {
        let zero_seed: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let rng = XorShift128.fromSeed(zero_seed);
        let v1 = rng.nextU32();
        let v2 = rng.nextU32();
        expect_eq(v1 != (0 as u32), true);
        expect_eq(v2 != (0 as u32), true);
        expect_eq(v1 != v2, true);
    });

    test("3. @cap/xorshift — XorShift128.fromU64, nextU64 & nextF64 [0, 1) distribution", () => {
        let r1 = XorShift128.fromU64(42);
        let r2 = XorShift128.fromU64(42);
        expect_eq(r1.nextU64(), r2.nextU64());

        let i: usize = 0;
        let sum: f64 = 0.0;
        while (i < 100) {
            let f = r1.nextF64();
            expect_eq(f >= 0.0 && f < 1.0, true);
            sum = sum + f;
            i = i + 1;
        }
        // Mean of 100 uniform [0, 1) samples should be comfortably within (0.2, 0.8)
        expect_eq(sum > 20.0 && sum < 80.0, true);
    });

    test("4. @cap/xorshift — XorShift128.fillBytes across non-multiple-of-4 boundary", () => {
        let r1 = XorShift128.fromU64(123456);
        let r2 = XorShift128.fromU64(123456);

        let buf: [11]u8 = undefined;
        r1.fillBytes(buf[0..11]);

        let w0 = r2.nextU32();
        let w1 = r2.nextU32();
        let w2 = r2.nextU32();

        expect_eq(buf[0], (w0 & 0xFF) as u8);
        expect_eq(buf[3], ((w0 >> 24) & 0xFF) as u8);
        expect_eq(buf[4], (w1 & 0xFF) as u8);
        expect_eq(buf[7], ((w1 >> 24) & 0xFF) as u8);
        expect_eq(buf[8], (w2 & 0xFF) as u8);
        expect_eq(buf[9], ((w2 >> 8) & 0xFF) as u8);
        expect_eq(buf[10], ((w2 >> 16) & 0xFF) as u8);
    });

    test("5. @cap/xorshift — XorShift64Star overflow-safe 64-bit multiply & determinism", () => {
        let r1 = XorShift64Star.new(1);
        let r2 = XorShift64Star.new(1);
        let r0 = XorShift64Star.new(0); // zero seed fallback

        let a1 = r1.nextU64();
        let a2 = r2.nextU64();
        expect_eq(a1, a2);
        expect_eq(a1 != (0 as u64), true);

        // Run 64 iterations to stress 64-bit wrapping multiplication without Debug overflow panic
        let i: usize = 0;
        let prev = a1;
        while (i < 64) {
            let cur = r1.nextU64();
            expect_eq(cur != prev, true);
            prev = cur;
            i = i + 1;
        }

        expect_eq(r0.nextU32() != (0 as u32), true);
    });

    test("6. @cap/xorshift — XorShift32 determinism & zero-seed safety", () => {
        let r1 = XorShift32.new(2463534242);
        let r2 = XorShift32.new(2463534242);
        let r0 = XorShift32.new(0);

        let v1 = r1.nextU32();
        let v2 = r2.nextU32();
        expect_eq(v1, v2);
        expect_eq(v1 != (0 as u32), true);
        expect_eq(r1.nextU32() != v1, true);

        let z1 = r0.nextU32();
        let z2 = r0.nextU32();
        expect_eq(z1 != (0 as u32), true);
        expect_eq(z2 != z1, true);
    });
}
