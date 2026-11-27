// cap/test/testCapBlockBuffer.cp
// Comprehensive unit tests for @cap/block-buffer

import { test, expect_eq } from "cap:test";
import {
    BlockBuffer64,
    BlockBuffer128,
} from "../../packages/block-buffer/src/index.cp";

fn main() {
    test("1. @cap/block-buffer — Multi-feed 64-byte block chunking & total_bytes tracking", () => {
        let bb = BlockBuffer64.new();
        let out: [256]u8 = undefined;

        let part1: [30]u8 = undefined;
        let i: usize = 0;
        while (i < 30) { part1[i] = 'A' as u8; i += 1; }
        expect_eq(bb.feed(part1[0..30], out[0..256]), 0 as usize);
        expect_eq(bb.getPos(), 30 as usize);
        expect_eq(bb.remaining(), 34 as usize);

        // Feed 110 bytes -> 30 + 110 = 140 = 2 full blocks (128) + 12 buffered
        let part2: [110]u8 = undefined;
        let j: usize = 0;
        while (j < 110) { part2[j] = 'B' as u8; j += 1; }
        expect_eq(bb.feed(part2[0..110], out[0..256]), 2 as usize);
        expect_eq(bb.getPos(), 12 as usize);
        expect_eq(bb.total_bytes, 140 as u64);
        expect_eq(out[0], 'A' as u8);
        expect_eq(out[29], 'A' as u8);
        expect_eq(out[30], 'B' as u8);
        expect_eq(out[127], 'B' as u8);
    });

    test("2. @cap/block-buffer — padMdBe64 single-block (pos < 56) & two-block (pos >= 56)", () => {
        // Case A: "abc" (3 bytes -> 24 bits = 0x18) -> 1 block
        let bb1 = BlockBuffer64.new();
        let out1: [128]u8 = undefined;
        * = bb1.feed("abc", out1[0..128]);
        const n1 = bb1.padMdBe64(out1[0..128]);
        expect_eq(n1, 1 as usize);
        expect_eq(out1[0], 'a' as u8);
        expect_eq(out1[2], 'c' as u8);
        expect_eq(out1[3], 0x80 as u8);
        expect_eq(out1[55], 0x00 as u8);
        expect_eq(out1[63], 24 as u8);

        // Case B: 56 bytes (pos == 56 -> 448 bits = 0x01C0) -> 2 blocks
        let bb2 = BlockBuffer64.new();
        let msg56: [56]u8 = undefined;
        let k: usize = 0;
        while (k < 56) { msg56[k] = 'x' as u8; k += 1; }
        let out2: [128]u8 = undefined;
        * = bb2.feed(msg56[0..56], out2[0..128]);
        const n2 = bb2.padMdBe64(out2[0..128]);
        expect_eq(n2, 2 as usize);
        expect_eq(out2[56], 0x80 as u8);
        expect_eq(out2[63], 0x00 as u8);
        expect_eq(out2[126], 0x01 as u8);
        expect_eq(out2[127], 0xC0 as u8);
    });

    test("3. @cap/block-buffer — padMdLe64 little-endian bit length (MD5 style)", () => {
        let bb = BlockBuffer64.new();
        let out: [128]u8 = undefined;
        // 32 bytes = 256 bits = 0x0100 LE -> [0x00, 0x01, 0, 0, 0, 0, 0, 0]
        let msg32: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) { msg32[i] = 0x5A as u8; i += 1; }
        * = bb.feed(msg32[0..32], out[0..128]);
        expect_eq(bb.padMdLe64(out[0..128]), 1 as usize);
        expect_eq(out[32], 0x80 as u8);
        expect_eq(out[56], 0x00 as u8);
        expect_eq(out[57], 0x01 as u8);
        expect_eq(out[63], 0x00 as u8);
    });

    test("4. @cap/block-buffer — PKCS#7, ISO 7816-4 & Zero padding", () => {
        let bb = BlockBuffer64.new();
        let blk: [64]u8 = undefined;

        * = bb.feed("hello", blk[0..64]);
        expect_eq(bb.padPkcs7(blk[0..64]), true);
        expect_eq(blk[0], 'h' as u8);
        expect_eq(blk[5], 59 as u8); // 64 - 5 = 59
        expect_eq(blk[63], 59 as u8);

        * = bb.feed("hi", blk[0..64]);
        expect_eq(bb.padIso7816(blk[0..64]), true);
        expect_eq(blk[2], 0x80 as u8);
        expect_eq(blk[3], 0x00 as u8);
        expect_eq(blk[63], 0x00 as u8);

        * = bb.feed("ok", blk[0..64]);
        expect_eq(bb.padZero(blk[0..64]), true);
        expect_eq(blk[1], 'k' as u8);
        expect_eq(blk[2], 0x00 as u8);
    });

    test("5. @cap/block-buffer — BlockBuffer128 128-byte streaming & SHA-512 padding", () => {
        let bb = BlockBuffer128.new();
        let out: [256]u8 = undefined;

        let chunk: [150]u8 = undefined;
        let i: usize = 0;
        while (i < 150) { chunk[i] = 0x33 as u8; i += 1; }
        expect_eq(bb.feed(chunk[0..150], out[0..256]), 1 as usize);
        expect_eq(bb.getPos(), 22 as usize);
        expect_eq(bb.remaining(), 106 as usize);
        expect_eq(bb.total_bytes, 150 as u64);

        // 150 bytes = 1200 bits = 0x04B0
        expect_eq(bb.padMdBe64(out[0..256]), 1 as usize);
        expect_eq(out[22], 0x80 as u8);
        expect_eq(out[126], 0x04 as u8);
        expect_eq(out[127], 0xB0 as u8);
    });

    test("6. @cap/block-buffer — reset, empty input & insufficient output buffer", () => {
        let bb = BlockBuffer64.new();
        let out: [64]u8 = undefined;
        expect_eq(bb.feed("", out[0..64]), 0 as usize);
        expect_eq(bb.getPos(), 0 as usize);

        * = bb.feed("temp", out[0..64]);
        bb.reset();
        expect_eq(bb.getPos(), 0 as usize);
        expect_eq(bb.total_bytes, 0 as u64);

        // Empty PKCS#7 pad emits a full block of 64s.
        expect_eq(bb.padPkcs7(out[0..64]), true);
        expect_eq(out[0], 64 as u8);
        expect_eq(out[63], 64 as u8);

        let tiny: [16]u8 = undefined;
        expect_eq(bb.padPkcs7(tiny[0..16]), false);
        expect_eq(bb.padMdBe64(tiny[0..16]), 0 as usize);
    });
}
