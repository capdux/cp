// cap/test/testCapFastrand.cp
// Comprehensive unit tests for @cap/fastrand

import { test, expect_eq } from "cap:test";
import { Rng } from "../../packages/fastrand/src/index.cp";

fn isAlphaNum(c: u8) -> bool {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn main() {
    test("1. @cap/fastrand — Deterministic seed reproducibility & fork divergence", () => {
        let r1 = Rng.withSeed(123456789);
        let r2 = Rng.withSeed(123456789);
        let i: usize = 0;
        while (i < 16) {
            expect_eq(r1.nextU64(), r2.nextU64());
            i += 1;
        }

        // Different seeds & forked streams diverge
        let child = r1.fork();
        expect_eq(r1.nextU64() != child.nextU64(), true);

        let r0 = Rng.withSeed(0);
        expect_eq(r0.nextU64() != 0 as u64, true);
    });

    test("2. @cap/fastrand — u64Range & usizeRange bounds and uniform spread", () => {
        let rng = Rng.withSeed(42);
        let buckets: [5]usize = [0, 0, 0, 0, 0];
        let i: usize = 0;
        while (i < 500) {
            const v = rng.u64Range(10, 15);
            expect_eq(v >= (10 as u64) && v < (15 as u64), true);
            buckets[(v - 10) as usize] += 1;
            i += 1;
        }
        // Every bucket should receive a healthy count out of 500 draws
        let b: usize = 0;
        while (b < 5) {
            expect_eq(buckets[b] > (50 as usize), true);
            b += 1;
        }
        expect_eq(rng.usizeRange(7, 7), 7 as usize);
    });

    test("3. @cap/fastrand — i64Range signed bounds (negative to positive) & nextChance", () => {
        let rng = Rng.withSeed(999);
        let saw_neg = false;
        let saw_pos = false;
        let i: usize = 0;
        while (i < 200) {
            const v = rng.i64Range(-10, 10);
            expect_eq(v >= -10 && v <= 10, true);
            if (v < 0) saw_neg = true;
            if (v > 0) saw_pos = true;
            i += 1;
        }
        expect_eq(saw_neg, true);
        expect_eq(saw_pos, true);

        expect_eq(rng.nextChance(0, 10), false);
        expect_eq(rng.nextChance(10, 10), true);
        expect_eq(rng.nextChance(1, 0), false);
    });

    test("4. @cap/fastrand — f64Unit strictly within [0.0, 1.0)", () => {
        let rng = Rng.withSeed(2026);
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < 200) {
            const f = rng.f64Unit();
            expect_eq(f >= 0.0 && f < 1.0, true);
            sum += f;
            i += 1;
        }
        const mean = sum / 200.0;
        expect_eq(mean > 0.35 && mean < 0.65, true);
    });

    test("5. @cap/fastrand — fillBytes & Fisher-Yates shuffleU64 permutation conservation", () => {
        let rng = Rng.withSeed(7777);
        let bytes: [27]u8 = undefined;
        rng.fillBytes(bytes[0..27]);
        let non_zero: usize = 0;
        let k: usize = 0;
        while (k < 27) {
            if (bytes[k] != 0) non_zero += 1;
            k += 1;
        }
        expect_eq(non_zero > (10 as usize), true);

        let arr: [10]u64 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
        rng.shuffleU64(arr[0..10]);
        let sum: u64 = 0;
        let seen: [10]bool = [false, false, false, false, false, false, false, false, false, false];
        let moved = false;
        let idx: usize = 0;
        while (idx < 10) {
            const val = arr[idx];
            expect_eq(val < (10 as u64), true);
            seen[val as usize] = true;
            sum += val;
            if (val != (idx as u64)) moved = true;
            idx += 1;
        }
        expect_eq(sum, 45 as u64);
        expect_eq(moved, true);
        let s_i: usize = 0;
        while (s_i < 10) {
            expect_eq(seen[s_i], true);
            s_i += 1;
        }
    });

    test("6. @cap/fastrand — alphanumeric token generation", () => {
        let rng = Rng.withSeed(314159);
        let buf: [32]u8 = undefined;
        const tok = rng.alphanumeric(buf[0..32]);
        expect_eq(tok.len, 32 as usize);
        let i: usize = 0;
        while (i < tok.len) {
            expect_eq(isAlphaNum(tok[i]), true);
            i += 1;
        }
    });
}
