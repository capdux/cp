// cap/test/testCapXorFilter.cp
// Comprehensive unit tests for @cap/xor-filter (Graf & Lemire Xor8 / Rust xorf parity)

import { test, expect_eq } from "cap:test";
import { Xor8Filter } from "../../packages/xorf/src/index.cp";

fn main() {
    test("1. @cap/xor-filter — empty set & single-key filter", () => {
        let dummy: [1]u64 = [0];
        const empty = Xor8Filter.build(dummy[0..0]);
        expect_eq(empty.isValid(), true);
        expect_eq(empty.len(), 0 as usize);
        expect_eq(empty.fingerprintBytes(), 0 as usize);
        expect_eq(empty.contains(42 as u64), false);

        let one: [1]u64 = [999999];
        const f1 = Xor8Filter.build(one[0..1]);
        expect_eq(f1.isValid(), true);
        expect_eq(f1.len(), 1 as usize);
        expect_eq(f1.contains(999999 as u64), true);
    });

    test("2. @cap/xor-filter — 100 sequential & pseudo-random keys have 0% false negatives", () => {
        let keys: [100]u64 = undefined;
        let i: usize = 0;
        while (i < 50) {
            keys[i] = (i as u64) + 1000;
            i += 1;
        }
        let st: u64 = 1234567;
        while (i < 100) {
            st = (st * (1103515245 as u64) + (12345 as u64)) & (0x7FFFFFFF as u64);
            keys[i] = st + (100000 as u64);
            i += 1;
        }

        const filter = Xor8Filter.build(keys[0..100]);
        expect_eq(filter.isValid(), true);
        expect_eq(filter.len(), 100 as usize);

        i = 0;
        while (i < 100) {
            expect_eq(filter.contains(keys[i]), true);
            i += 1;
        }
    });

    test("3. @cap/xor-filter — duplicate keys automatically deduplicated during build", () => {
        let dups: [8]u64 = [7, 7, 42, 42, 42, 100, 7, 100];
        const filter = Xor8Filter.build(dups[0..8]);
        expect_eq(filter.isValid(), true);
        expect_eq(filter.len(), 3 as usize);
        expect_eq(filter.contains(7 as u64), true);
        expect_eq(filter.contains(42 as u64), true);
        expect_eq(filter.contains(100 as u64), true);
    });

    test("4. @cap/xor-filter — false-positive rate < 2% over 500 absent keys (theory ~0.39%)", () => {
        let keys: [100]u64 = undefined;
        let i: usize = 0;
        while (i < 100) {
            keys[i] = (i as u64) + 1;
            i += 1;
        }
        const filter = Xor8Filter.build(keys[0..100]);
        expect_eq(filter.isValid(), true);

        let false_positives: usize = 0;
        let q: u64 = 10000;
        while (q < 10500) {
            if (filter.contains(q)) {
                false_positives += 1;
            }
            q += 1;
        }
        // Expected ~2 out of 500; strictly assert < 10 (< 2%)
        expect_eq(false_positives < (10 as usize), true);
    });

    test("5. @cap/xor-filter — full 256-key capacity build & > 256 overflow rejection", () => {
        let keys: [256]u64 = undefined;
        let i: usize = 0;
        while (i < 256) {
            keys[i] = (i as u64) * 13 + 7;
            i += 1;
        }
        const filter = Xor8Filter.build(keys[0..256]);
        expect_eq(filter.isValid(), true);
        expect_eq(filter.len(), 256 as usize);

        i = 0;
        while (i < 256) {
            expect_eq(filter.contains(keys[i]), true);
            i += 1;
        }

        // 257 keys exceeds MAX_KEYS -> valid == false
        let over: [257]u64 = undefined;
        i = 0;
        while (i < 257) {
            over[i] = i as u64;
            i += 1;
        }
        const bad = Xor8Filter.build(over[0..257]);
        expect_eq(bad.isValid(), false);
        expect_eq(bad.contains(0 as u64), false);
    });

    test("6. @cap/xor-filter — compact fingerprintBytes() footprint check", () => {
        let keys: [200]u64 = undefined;
        let i: usize = 0;
        while (i < 200) {
            keys[i] = (i as u64) + 500;
            i += 1;
        }
        const filter = Xor8Filter.build(keys[0..200]);
        expect_eq(filter.isValid(), true);
        // 1.23 * 200 + 32 = 278 -> rounded up to multiple of 3 = 279 bytes (<= 384)
        const bytes = filter.fingerprintBytes();
        expect_eq(bytes >= (246 as usize) && bytes <= (300 as usize), true);
        expect_eq((bytes % 3), 0 as usize);
    });
}
