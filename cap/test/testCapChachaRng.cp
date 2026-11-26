// cap/test/testCapChachaRng.cp
// Comprehensive unit tests for @cap/chacha-rng

import { test, expect_eq } from "cap:test";
import {
    chachaBlock,
    ChaCha20Rng,
} from "../../packages/@cap/chacha-rng/src/index.cp";

fn main() {
    test("1. @cap/chacha-rng — RFC 8439 Section 2.3.2 official 20-round test vector", () => {
        // Key bytes 00..1f little-endian words:
        let key: [8]u32 = undefined;
        key[0] = 0x03020100 as u32;
        key[1] = 0x07060504 as u32;
        key[2] = 0x0B0A0908 as u32;
        key[3] = 0x0F0E0D0C as u32;
        key[4] = 0x13121110 as u32;
        key[5] = 0x17161514 as u32;
        key[6] = 0x1B1A1918 as u32;
        key[7] = 0x1F1E1D1C as u32;

        let nonce: [3]u32 = undefined;
        nonce[0] = 0x09000000 as u32;
        nonce[1] = 0x4A000000 as u32;
        nonce[2] = 0x00000000 as u32;

        let block: [64]u8 = undefined;
        chachaBlock(key, 1 as u32, nonce, 20 as u32, block[0..64]);

        // First 4 bytes: 10 f1 e7 e4
        expect_eq(block[0], 0x10 as u8);
        expect_eq(block[1], 0xF1 as u8);
        expect_eq(block[2], 0xE7 as u8);
        expect_eq(block[3], 0xE4 as u8);
        // Next 4 bytes: d1 3b 59 15
        expect_eq(block[4], 0xD1 as u8);
        expect_eq(block[5], 0x3B as u8);
        expect_eq(block[6], 0x59 as u8);
        expect_eq(block[7], 0x15 as u8);
        // Last 4 bytes (word 15 = 0x4e3c50a2 little-endian): a2 50 3c 4e
        expect_eq(block[60], 0xA2 as u8);
        expect_eq(block[61], 0x50 as u8);
        expect_eq(block[62], 0x3C as u8);
        expect_eq(block[63], 0x4E as u8);
    });

    test("2. @cap/chacha-rng — ChaCha8 vs ChaCha12 vs ChaCha20 produce distinct blocks", () => {
        let key: [8]u32 = undefined;
        let i: usize = 0;
        while (i < 8) {
            key[i] = (i + 1) as u32;
            i += 1;
        }
        let nonce: [3]u32 = undefined;
        nonce[0] = 0 as u32;
        nonce[1] = 0 as u32;
        nonce[2] = 0 as u32;

        let b8: [64]u8 = undefined;
        let b12: [64]u8 = undefined;
        let b20: [64]u8 = undefined;
        chachaBlock(key, 0 as u32, nonce, 8 as u32, b8[0..64]);
        chachaBlock(key, 0 as u32, nonce, 12 as u32, b12[0..64]);
        chachaBlock(key, 0 as u32, nonce, 20 as u32, b20[0..64]);

        expect_eq(b8[0] != b20[0] || b8[1] != b20[1], true);
        expect_eq(b12[0] != b20[0] || b12[1] != b20[1], true);
    });

    test("3. @cap/chacha-rng — Deterministic seeding (fromU64 & fromSeed)", () => {
        let r1 = ChaCha20Rng.fromU64(987654321 as u64);
        let r2 = ChaCha20Rng.fromU64(987654321 as u64);
        let r3 = ChaCha20Rng.fromU64(987654322 as u64);

        const a1 = r1.nextU64();
        const a2 = r2.nextU64();
        const a3 = r3.nextU64();
        expect_eq(a1, a2);
        expect_eq(a1 != a3, true);
        expect_eq(r1.nextU32(), r2.nextU32());
    });

    test("4. @cap/chacha-rng — fillBytes across 64-byte block boundaries matches streaming", () => {
        let rBulk = ChaCha20Rng.fromU64(42 as u64);
        let bulk: [150]u8 = undefined;
        rBulk.fillBytes(bulk[0..150]);

        let rChunked = ChaCha20Rng.fromU64(42 as u64);
        let chunked: [150]u8 = undefined;
        rChunked.fillBytes(chunked[0..17]);
        rChunked.fillBytes(chunked[17..80]);
        rChunked.fillBytes(chunked[80..150]);

        let allEq = true;
        let i: usize = 0;
        while (i < 150) {
            if (bulk[i] != chunked[i]) {
                allEq = false;
                break;
            }
            i += 1;
        }
        expect_eq(allEq, true);
    });

    test("5. @cap/chacha-rng — setWordPos random-access block seeking", () => {
        let rSeq = ChaCha20Rng.fromU64(777 as u64);
        let skip: [192]u8 = undefined;
        rSeq.fillBytes(skip[0..192]); // consumes blocks 0, 1, 2
        const expectedAt3 = rSeq.nextU64();

        let rSeek = ChaCha20Rng.fromU64(777 as u64);
        rSeek.setWordPos(3 as u32);
        expect_eq(rSeek.nextU64(), expectedAt3);
    });

    test("6. @cap/chacha-rng — nextF64 stays strictly within [0.0, 1.0) with variance", () => {
        let rng = ChaCha20Rng.fromU64(2026 as u64);
        let inRange = true;
        let sum: f64 = 0.0;
        let i: usize = 0;
        while (i < 32) {
            const f: f64 = rng.nextF64();
            if (f < (0.0 as f64) || f >= (1.0 as f64)) {
                inRange = false;
            }
            sum = sum + f;
            i += 1;
        }
        expect_eq(inRange, true);
        expect_eq(sum > (4.0 as f64) && sum < (28.0 as f64), true);
    });
}
