// cap/test/testCapFoldhash.cp
// Comprehensive unit tests for @cap/foldhash

import { test, expect_eq } from "cap:test";
import {
    foldedMultiply,
    FoldHasher,
    hashBytesFast,
    hashBytesQuality,
    hashU64,
} from "../../packages/@cap/foldhash/src/index.cp";

fn popcount64(x_in: u64) -> usize {
    let x = x_in;
    let c: usize = 0;
    while (x != (0 as u64)) {
        x = x & (x - (1 as u64));
        c += 1;
    }
    return c;
}

fn main() {
    test("1. @cap/foldhash — foldedMultiply exact 128-bit lo ^ hi verification", () => {
        expect_eq(foldedMultiply(0 as u64, 12345 as u64), 0 as u64);
        expect_eq(foldedMultiply(7 as u64, 11 as u64), 77 as u64);
        // 2^32 * 2^32 = 2^64 => lo = 0, hi = 1 => lo ^ hi = 1
        expect_eq(foldedMultiply(0x100000000 as u64, 0x100000000 as u64), 1 as u64);
        // (2^64 - 1)^2 => hi = 2^64 - 2, lo = 1 => lo ^ hi = 2^64 - 1
        const maxU64: u64 = ((0xFFFFFFFF as u64) * (0x100000000 as u64)) | (0xFFFFFFFF as u64);
        expect_eq(foldedMultiply(maxU64, maxU64), maxU64);
    });

    test("2. @cap/foldhash — Empty & single-byte determinism and collision resistance", () => {
        const seed: u64 = 12345;
        const hEmpty1 = hashBytesFast("", seed);
        const hEmpty2 = hashBytesFast("", seed);
        expect_eq(hEmpty1, hEmpty2);

        const hA = hashBytesFast("a", seed);
        const hB = hashBytesFast("b", seed);
        expect_eq(hEmpty1 != hA, true);
        expect_eq(hA != hB, true);

        // Zero-byte vs empty string must differ due to length mixing
        let zero1: [1]u8 = undefined;
        zero1[0] = 0 as u8;
        let zero2: [2]u8 = undefined;
        zero2[0] = 0 as u8;
        zero2[1] = 0 as u8;
        expect_eq(hashBytesFast(zero1[0..1], seed) != hEmpty1, true);
        expect_eq(hashBytesFast(zero1[0..1], seed) != hashBytesFast(zero2[0..2], seed), true);
    });

    test("3. @cap/foldhash — Different seeds produce strongly isolated hashes", () => {
        const h1 = hashBytesFast("cp-lang-hash-key", 1 as u64);
        const h2 = hashBytesFast("cp-lang-hash-key", 2 as u64);
        const h3 = hashBytesQuality("cp-lang-hash-key", 1 as u64);
        const h4 = hashBytesQuality("cp-lang-hash-key", 2 as u64);
        expect_eq(h1 != h2, true);
        expect_eq(h3 != h4, true);
        expect_eq(h1 != h3, true);
        expect_eq(hashU64(42 as u64, 1 as u64) != hashU64(42 as u64, 2 as u64), true);
    });

    test("4. @cap/foldhash — Streaming chunked writeBytes matches monolithic hashing", () => {
        const msg = "The quick brown fox jumps over the lazy dog 0123456789";
        const seed: u64 = 987654;

        const wholeFast = hashBytesFast(msg, seed);
        let hf = FoldHasher.initFast(seed);
        hf.writeBytes(msg[0..3]);
        hf.writeBytes(msg[3..11]);
        hf.writeBytes(msg[11..27]);
        hf.writeBytes(msg[27..msg.len]);
        expect_eq(hf.finish(), wholeFast);

        const wholeQual = hashBytesQuality(msg, seed);
        let hq = FoldHasher.initQuality(seed);
        hq.writeBytes(msg[0..1]);
        hq.writeBytes(msg[1..17]);
        hq.writeBytes(msg[17..msg.len]);
        expect_eq(hq.finish(), wholeQual);
    });

    test("5. @cap/foldhash — Fast vs Quality avalanche effect on 1-bit input flip", () => {
        const seed: u64 = 55555;
        const aFast = hashBytesFast("avalanche-test-0", seed);
        const bFast = hashBytesFast("avalanche-test-1", seed);
        const diffFast = popcount64(aFast ^ bFast);
        expect_eq(diffFast >= (16 as usize), true);

        const aQual = hashBytesQuality("avalanche-test-0", seed);
        const bQual = hashBytesQuality("avalanche-test-1", seed);
        const diffQual = popcount64(aQual ^ bQual);
        expect_eq(diffQual >= (16 as usize), true);
    });

    test("6. @cap/foldhash — Length boundary sweep (0..33 bytes) & writeU32/writeU64", () => {
        const seed: u64 = 314159;
        let buf: [33]u8 = undefined;
        let k: usize = 0;
        while (k < 33) {
            buf[k] = ('A' as u8) + ((k % (26 as usize)) as u8);
            k += 1;
        }
        let prev = hashBytesFast(buf[0..0], seed);
        let n: usize = 1;
        while (n <= 33) {
            const cur = hashBytesFast(buf[0..n], seed);
            expect_eq(cur != prev, true);
            prev = cur;
            n += 1;
        }

        let h = FoldHasher.initFast(seed);
        h.writeU32(0x12345678 as u32);
        h.writeU64(0x1122334455667788 as u64);
        const res = h.finish();
        expect_eq(res != (0 as u64), true);
    });
}
