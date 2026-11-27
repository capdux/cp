import { test, expect_eq } from "cap:test";
import {
    BloomFilter,
    popcount,
    fillRatio,
    optimalNumHashes,
} from "../../packages/bloom-filters/src/index.cp";

fn formatKey(prefix: u8, n: usize, out: []u8): []u8 {
    out[0] = prefix;
    out[1] = 45; // '-'
    let v: usize = n;
    let d3: u8 = ((v / 1000) % 10) as u8;
    let d2: u8 = ((v / 100) % 10) as u8;
    let d1: u8 = ((v / 10) % 10) as u8;
    let d0: u8 = (v % 10) as u8;
    out[2] = d3 + 48;
    out[3] = d2 + 48;
    out[4] = d1 + 48;
    out[5] = d0 + 48;
    return out[0..6];
}

fn main() {
    test("1. @cap/bloom-filter — Insert & zero false negatives (strings + bytes)", () => {
        let bits: [512]u8 = undefined;
        let bf = BloomFilter.new(&bits, 7);

        bf.insert(&bits, "hello");
        bf.insert(&bits, "world");
        bf.insert(&bits, "cap-lang");
        bf.insert(&bits, "");

        expect_eq(bf.contains(&bits, "hello"), true);
        expect_eq(bf.contains(&bits, "world"), true);
        expect_eq(bf.contains(&bits, "cap-lang"), true);
        expect_eq(bf.contains(&bits, ""), true);

        let key_buf: [6]u8 = undefined;
        let i: usize = 0;
        while (i < 150) {
            const key = formatKey(97, i, &key_buf); // 'a-0000'..'a-0149'
            bf.insertBytes(&bits, key);
            i += 1;
        }
        expect_eq(bf.inserted, 154);

        let fn_count: usize = 0;
        let j: usize = 0;
        while (j < 150) {
            const qkey = formatKey(97, j, &key_buf);
            if (!bf.containsBytes(&bits, qkey)) {
                fn_count += 1;
            }
            j += 1;
        }
        expect_eq(fn_count, 0);
    });

    test("2. @cap/bloom-filter — Low false positive rate on unseen items", () => {
        let bits: [1024]u8 = undefined; // 8192 bits
        const k = optimalNumHashes(bits.len * 8, 200);
        let bf = BloomFilter.new(&bits, k);

        let key_buf: [6]u8 = undefined;
        let i: usize = 0;
        while (i < 200) {
            const key = formatKey(105, i, &key_buf); // 'i-0000'..'i-0199'
            bf.insertBytes(&bits, key);
            i += 1;
        }

        let fp_count: usize = 0;
        let j: usize = 0;
        while (j < 500) {
            const qkey = formatKey(117, j, &key_buf); // 'u-0000'..'u-0499'
            if (bf.containsBytes(&bits, qkey)) {
                fp_count += 1;
            }
            j += 1;
        }
        // Theoretical FP for m=8192, n=200, k=28 is ~1e-9; allow generous ceiling < 5 (1%)
        expect_eq(fp_count < 5, true);
    });

    test("3. @cap/bloom-filter — clear() resets bits & inserted counter", () => {
        let bits: [128]u8 = undefined;
        let bf = BloomFilter.new(&bits, 5);
        bf.insert(&bits, "alpha");
        bf.insert(&bits, "beta");
        expect_eq(bf.contains(&bits, "alpha"), true);
        expect_eq(popcount(&bits) > 0, true);
        expect_eq(bf.inserted, 2);

        bf.clear(&bits);
        expect_eq(bf.inserted, 0);
        expect_eq(popcount(&bits), 0);
        expect_eq(fillRatio(&bits), 0.0);
        expect_eq(bf.contains(&bits, "alpha"), false);
        expect_eq(bf.contains(&bits, "beta"), false);
    });

    test("4. @cap/bloom-filter — merge() bitwise OR union & mismatch rejection", () => {
        let bits_a: [256]u8 = undefined;
        let bits_b: [256]u8 = undefined;
        let bits_small: [64]u8 = undefined;

        let bf_a = BloomFilter.new(&bits_a, 6);
        let bf_b = BloomFilter.new(&bits_b, 6);
        let _bf_s = BloomFilter.new(&bits_small, 6);

        bf_a.insert(&bits_a, "service-auth");
        bf_a.insert(&bits_a, "service-billing");
        bf_b.insert(&bits_b, "service-search");
        bf_b.insert(&bits_b, "service-metrics");

        expect_eq(bf_a.contains(&bits_a, "service-search"), false);
        expect_eq(bf_a.merge(&bits_a, &bits_small), false);
        expect_eq(bf_a.merge(&bits_a, &bits_b), true);

        expect_eq(bf_a.contains(&bits_a, "service-auth"), true);
        expect_eq(bf_a.contains(&bits_a, "service-billing"), true);
        expect_eq(bf_a.contains(&bits_a, "service-search"), true);
        expect_eq(bf_a.contains(&bits_a, "service-metrics"), true);
    });

    test("5. @cap/bloom-filter — popcount() & fillRatio() accuracy", () => {
        let buf: [4]u8 = [0x00, 0xFF, 0xAA, 0x01]; // 0 + 8 + 4 + 1 = 13 bits out of 32
        expect_eq(popcount(&buf), 13);
        const ratio: f64 = fillRatio(&buf);
        const expected: f64 = 13.0 / 32.0;
        const diff: f64 = if (ratio > expected) ratio - expected else expected - ratio;
        expect_eq(diff < 0.000001, true);

        let zeros: [4]u8 = [0, 0, 0, 0];
        expect_eq(popcount(&zeros), 0);
        expect_eq(fillRatio(&zeros), 0.0);

        let full: [2]u8 = [0xFF, 0xFF];
        expect_eq(popcount(&full), 16);
        expect_eq(fillRatio(&full), 1.0);
    });

    test("6. @cap/bloom-filter — optimalNumHashes() & empty buffer boundaries", () => {
        // m/n = 10 -> 10 * 0.6931 -> 7
        expect_eq(optimalNumHashes(10000, 1000), 7);
        // m/n = 14.4 -> ~10
        expect_eq(optimalNumHashes(14400, 1000), 10);
        // edge cases -> clamp to 1
        expect_eq(optimalNumHashes(0, 100), 1);
        expect_eq(optimalNumHashes(100, 0), 1);
        expect_eq(optimalNumHashes(10, 100), 1);

        // Empty slice boundary safety
        let dummy: [1]u8 = [0xFF];
        const empty: []u8 = dummy[0..0];
        let bf_empty = BloomFilter.new(empty, 0);
        expect_eq(bf_empty.bit_count, 0);
        expect_eq(bf_empty.num_hashes, 1);
        bf_empty.insert(empty, "anything");
        expect_eq(bf_empty.contains(empty, "anything"), false);
        bf_empty.clear(empty);
        expect_eq(popcount(empty), 0);
        expect_eq(fillRatio(empty), 0.0);
    });
}
