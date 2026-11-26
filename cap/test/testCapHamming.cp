import { test, expect_eq } from "cap:test";
import {
    popcountU64,
    hammingU64,
    weightBytes,
    distanceBytes,
    simhash64,
    hamming74Encode,
    hamming74Decode,
} from "../../packages/@cap/hamming/src/index.cp";

fn main() {
    test("1. @cap/hamming — popcountU64 & hammingU64 boundary values", () => {
        const all_ones: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        const alt_a: u64 = ((0xAAAAAAAA as u64) << 32) | (0xAAAAAAAA as u64);
        const alt_5: u64 = ((0x55555555 as u64) << 32) | (0x55555555 as u64);

        expect_eq(popcountU64(0), 0 as u32);
        expect_eq(popcountU64(1), 1 as u32);
        expect_eq(popcountU64((1 as u64) << 63), 1 as u32);
        expect_eq(popcountU64(all_ones), 64 as u32);
        expect_eq(popcountU64(alt_a), 32 as u32);

        expect_eq(hammingU64(0, 0), 0 as u32);
        expect_eq(hammingU64(all_ones, all_ones), 0 as u32);
        expect_eq(hammingU64(0, all_ones), 64 as u32);
        expect_eq(hammingU64(alt_a, alt_5), 64 as u32);
        expect_eq(hammingU64(0xF0F0, 0x0FF0), 8 as u32);
    });

    test("2. @cap/hamming — weightBytes, distanceBytes & mismatched length -1", () => {
        const a: [4]u8 = [0xFF, 0x00, 0xAA, 0x0F]; // 8 + 0 + 4 + 4 = 16 bits
        const b: [4]u8 = [0xF0, 0x0F, 0x55, 0x0F]; // xor: 0x0F(4) + 0x0F(4) + 0xFF(8) + 0x00(0) = 16
        const short_buf: [2]u8 = [0xFF, 0x00];

        expect_eq(weightBytes(a[0..4]), 16 as u64);
        expect_eq(distanceBytes(a[0..4], a[0..4]), 0 as i64);
        expect_eq(distanceBytes(a[0..4], b[0..4]), 16 as i64);
        expect_eq(distanceBytes(a[0..4], short_buf[0..2]), -1 as i64);
    });

    test("3. @cap/hamming — simhash64 near-duplicate distance << unrelated text", () => {
        const doc1 = "the quick brown fox jumps over the lazy dog in the sunny park";
        const doc2 = "the quick brown fox leaps over the lazy dog in the sunny park";
        const doc3 = "quantum mechanics describes subatomic wave function collapse and entanglement";

        const h1 = simhash64(doc1);
        const h2 = simhash64(doc2);
        const h3 = simhash64(doc3);

        const d_near = hammingU64(h1, h2);
        const d_far = hammingU64(h1, h3);

        expect_eq(hammingU64(h1, h1), 0 as u32);
        expect_eq(d_near < 15, true);
        expect_eq(d_near < d_far, true);
    });

    test("4. @cap/hamming — hamming74Encode & hamming74Decode all 16 nibbles error-free", () => {
        let nib: u8 = 0;
        while (nib < 16) {
            const cw = hamming74Encode(nib);
            expect_eq(cw < 128, true);
            const res = hamming74Decode(cw);
            expect_eq(res.data, nib);
            expect_eq(res.corrected, false);
            expect_eq(res.error_bit, 0 as u8);
            nib += 1;
        }
    });

    test("5. @cap/hamming — hamming74Decode 100% single-bit error correction across all 7 bits", () => {
        let nib: u8 = 0;
        while (nib < 16) {
            const cw = hamming74Encode(nib);
            let bit: u8 = 0;
            while (bit < 7) {
                const mask: u8 = (1 as u8) << (bit as u3);
                const corrupted = cw ^ mask;
                const res = hamming74Decode(corrupted);
                expect_eq(res.data, nib);
                expect_eq(res.corrected, true);
                expect_eq(res.error_bit, bit + 1);
                bit += 1;
            }
            nib += 1;
        }
    });

    test("6. @cap/hamming — All-zero & all-one byte streams + empty input", () => {
        let zeros: [32]u8 = undefined;
        let ones: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            zeros[i] = 0x00;
            ones[i] = 0xFF;
            i += 1;
        }

        expect_eq(weightBytes(zeros[0..0]), 0 as u64);
        expect_eq(distanceBytes(zeros[0..0], ones[0..0]), 0 as i64);
        expect_eq(weightBytes(zeros[0..32]), 0 as u64);
        expect_eq(weightBytes(ones[0..32]), 256 as u64);
        expect_eq(distanceBytes(zeros[0..32], ones[0..32]), 256 as i64);
        expect_eq(simhash64(zeros[0..0]), 0 as u64);
    });
}
