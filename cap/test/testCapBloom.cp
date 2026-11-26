// cap/test/testCapBloom.cp
// Comprehensive unit tests for @cap/bloom

import { test, expect_eq } from "cap:test";
import {
    BloomFilter1024,
    BloomFilter4096,
} from "../../packages/@cap/bloom/src/index.cp";

fn main() {
    test("1. @cap/bloom — Basic string insert, zero false negatives & clear", () => {
        let bf = BloomFilter1024.init(5 as u8);
        expect_eq(bf.bitCount(), 1024 as usize);
        expect_eq(bf.setBits(), 0 as usize);
        expect_eq(bf.itemCount(), 0 as usize);
        expect_eq(bf.mightContain("hello"), false);

        bf.insert("hello");
        bf.insert("world");
        bf.insert("cp-lang");
        expect_eq(bf.itemCount(), 3 as usize);
        expect_eq(bf.setBits() > (0 as usize), true);
        expect_eq(bf.mightContain("hello"), true);
        expect_eq(bf.mightContain("world"), true);
        expect_eq(bf.mightContain("cp-lang"), true);

        bf.clear();
        expect_eq(bf.setBits(), 0 as usize);
        expect_eq(bf.itemCount(), 0 as usize);
        expect_eq(bf.mightContain("hello"), false);
    });

    test("2. @cap/bloom — Zero false negatives across 120 inserted u64 keys on BloomFilter4096", () => {
        let bf = BloomFilter4096.init(6 as u8);
        expect_eq(bf.bitCount(), 4096 as usize);

        let i: u64 = 1;
        while (i <= 120) {
            bf.insertU64(i * (7919 as u64));
            i += 1;
        }
        expect_eq(bf.itemCount(), 120 as usize);

        let j: u64 = 1;
        while (j <= 120) {
            expect_eq(bf.mightContainU64(j * (7919 as u64)), true);
            j += 1;
        }
    });

    test("3. @cap/bloom — Low false-positive rate on 200 unseen probes", () => {
        let bf = BloomFilter4096.init(6 as u8);
        let i: u64 = 0;
        while (i < 100) {
            bf.insertU64(i + (1000 as u64));
            i += 1;
        }
        let fps: usize = 0;
        let probe: u64 = 0;
        while (probe < 200) {
            if (bf.mightContainU64(probe + (900000 as u64))) {
                fps += 1;
            }
            probe += 1;
        }
        // With m=4096, n=100, k=6 theoretical FPR < 0.01% (expected ~0 false positives)
        expect_eq(fps < (5 as usize), true);
    });

    test("4. @cap/bloom — unionWith distributed filter merge", () => {
        let nodeA = BloomFilter1024.init(5 as u8);
        let nodeB = BloomFilter1024.init(5 as u8);

        nodeA.insert("user:alice");
        nodeA.insert("user:bob");
        nodeB.insert("user:carol");
        nodeB.insert("user:dave");

        nodeA.unionWith(nodeB);
        expect_eq(nodeA.itemCount(), 4 as usize);
        expect_eq(nodeA.mightContain("user:alice"), true);
        expect_eq(nodeA.mightContain("user:bob"), true);
        expect_eq(nodeA.mightContain("user:carol"), true);
        expect_eq(nodeA.mightContain("user:dave"), true);
    });

    test("5. @cap/bloom — estimatedFpr theoretical curve & clamping", () => {
        let bf = BloomFilter4096.init(0 as u8); // clamps to 1
        expect_eq(bf.num_hashes, 1 as u8);
        expect_eq(bf.estimatedFpr() == 0.0, true);

        let bf2 = BloomFilter4096.init(99 as u8); // clamps to 16
        expect_eq(bf2.num_hashes, 16 as u8);

        let bf3 = BloomFilter1024.init(5 as u8);
        let i: u64 = 0;
        while (i < 100) {
            bf3.insertU64(i);
            i += 1;
        }
        const fpr = bf3.estimatedFpr();
        // For m=1024, n=100, k=5: (1 - e^(-500/1024))^5 ≈ 0.0086
        expect_eq(fpr > 0.004 && fpr < 0.02, true);
    });

    test("6. @cap/bloom — BloomFilter4096 unionWith & clear", () => {
        let a = BloomFilter4096.init(7 as u8);
        let b = BloomFilter4096.init(7 as u8);
        a.insert("alpha");
        b.insert("beta");
        a.unionWith(b);
        expect_eq(a.mightContain("alpha"), true);
        expect_eq(a.mightContain("beta"), true);
        a.clear();
        expect_eq(a.setBits(), 0 as usize);
        expect_eq(a.mightContain("alpha"), false);
    });
}
