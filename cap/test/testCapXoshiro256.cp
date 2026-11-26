import { test, expect_eq } from "cap:test";
import {
    seedFromU64,
    nextU64,
    nextF64,
    nextRange,
    jump,
    shuffleU32,
} from "../../packages/@cap/xoshiro256/src/index.cp";

fn main() {
    test("1. @cap/xoshiro256 — Deterministic reproducibility for identical seeds", () => {
        let a = seedFromU64(42);
        let b = seedFromU64(42);
        let i: usize = 0;
        while (i < 50) {
            expect_eq(nextU64(&a), nextU64(&b));
            i += 1;
        }
    });

    test("2. @cap/xoshiro256 — Distinct seeds produce independent streams", () => {
        let a = seedFromU64(1);
        let b = seedFromU64(2);
        let diff_count: usize = 0;
        let i: usize = 0;
        while (i < 20) {
            if (nextU64(&a) != nextU64(&b)) {
                diff_count += 1;
            }
            i += 1;
        }
        expect_eq(diff_count, 20 as usize);
    });

    test("3. @cap/xoshiro256 — nextF64 1000 samples in [0.0, 1.0) with mean ~0.5", () => {
        let rng = seedFromU64(123456789);
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < 1000) {
            const v = nextF64(&rng);
            expect_eq(v >= 0.0 && v < 1.0, true);
            sum += v;
            i += 1;
        }
        const mean = sum / 1000.0;
        expect_eq(mean > 0.45 && mean < 0.55, true);
    });

    test("4. @cap/xoshiro256 — nextRange(10) bounds, distribution & shuffleU32", () => {
        let rng = seedFromU64(9999);
        expect_eq(nextRange(&rng, 0), 0 as u64);
        expect_eq(nextRange(&rng, 1), 0 as u64);

        let buckets: [10]usize = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let i: usize = 0;
        while (i < 500) {
            const v = nextRange(&rng, 10);
            expect_eq(v < 10, true);
            buckets[v as usize] += 1;
            i += 1;
        }
        let b: usize = 0;
        while (b < 10) {
            expect_eq(buckets[b] > 20, true);
            b += 1;
        }

        // Fisher-Yates shuffle preserves exact sum of 0..9 = 45
        let arr: [10]u32 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
        shuffleU32(&rng, arr[0..10]);
        let total: u32 = 0;
        let k: usize = 0;
        while (k < 10) {
            total += arr[k];
            k += 1;
        }
        expect_eq(total, 45 as u32);
    });

    test("5. @cap/xoshiro256 — jump() creates distinct non-overlapping parallel substream", () => {
        let base = seedFromU64(777);
        let jumped = seedFromU64(777);
        jump(&jumped);

        let diff: usize = 0;
        let i: usize = 0;
        while (i < 20) {
            if (nextU64(&base) != nextU64(&jumped)) {
                diff += 1;
            }
            i += 1;
        }
        expect_eq(diff, 20 as usize);
    });

    test("6. @cap/xoshiro256 — Zero seed yields healthy non-zero 256-bit state", () => {
        let z = seedFromU64(0);
        const non_zero = (z.s[0] != 0) || (z.s[1] != 0) || (z.s[2] != 0) || (z.s[3] != 0);
        expect_eq(non_zero, true);
        const v1 = nextU64(&z);
        const v2 = nextU64(&z);
        expect_eq(v1 != v2, true);
    });
}
