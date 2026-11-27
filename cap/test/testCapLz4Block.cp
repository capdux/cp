// cap/test/testCapLz4Block.cp
// Comprehensive unit tests for @cap/lz4-block (LZ4 Raw Block / Rust lz4_flex parity)

import { test, expect_eq } from "cap:test";
import {
    maxCompressedLen,
    compressBlock,
    decompressBlock,
    compressWithSize,
    decompressWithSize,
} from "../../packages/lz4_flex/src/index.cp";

fn main() {
    test("1. @cap/lz4-block — empty input & short inputs (< 12 bytes)", () => {
        expect_eq(maxCompressedLen(0 as usize), 16 as usize);
        expect_eq(maxCompressedLen(255 as usize), 272 as usize);

        let comp: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;
        let empty: [1]u8 = [0];

        expect_eq(compressBlock(empty[0..0], comp[0..64]), 0 as i64);
        expect_eq(decompressBlock(empty[0..0], dec_buf[0..64]), 0 as i64);

        const short_msg = "Hello CP!"; // 9 bytes (< MFLIMIT 12)
        const c_len = compressBlock(short_msg, comp[0..64]);
        expect_eq(c_len, 10 as i64); // 1 token + 9 literals
        const d_len = decompressBlock(comp[0..(c_len as usize)], dec_buf[0..64]);
        expect_eq(d_len, 9 as i64);
        expect_eq(dec_buf[0..9], short_msg);
    });

    test("2. @cap/lz4-block — high-repetition data achieves high compression ratio", () => {
        let raw: [512]u8 = undefined;
        let i: usize = 0;
        while (i < 512) {
            raw[i] = ('A' as u8) + ((i % 4) as u8);
            i += 1;
        }

        let comp: [600]u8 = undefined;
        let dec_buf: [512]u8 = undefined;
        const c_len = compressBlock(raw[0..512], comp[0..600]);
        expect_eq(c_len > 0 && c_len < 32, true);

        const d_len = decompressBlock(comp[0..(c_len as usize)], dec_buf[0..512]);
        expect_eq(d_len, 512 as i64);
        expect_eq(dec_buf[0], 'A' as u8);
        expect_eq(dec_buf[1], 'B' as u8);
        expect_eq(dec_buf[511], 'D' as u8);
    });

    test("3. @cap/lz4-block — overlapping RLE match copy (offset == 1 and offset == 3)", () => {
        let comp: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;

        // offset == 1: 40 identical 'X' bytes
        const rep1 = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
        const c1 = compressBlock(rep1, comp[0..128]);
        expect_eq(c1 > 0 && c1 < 16, true);
        expect_eq(decompressBlock(comp[0..(c1 as usize)], dec_buf[0..128]), 40 as i64);
        expect_eq(dec_buf[0..40], rep1);

        // offset == 3: repeating 3-byte pattern "abc" * 20 = 60 bytes
        const rep3 = "abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc";
        const c3 = compressBlock(rep3, comp[0..128]);
        expect_eq(c3 > 0 && c3 < 20, true);
        expect_eq(decompressBlock(comp[0..(c3 as usize)], dec_buf[0..128]), 60 as i64);
        expect_eq(dec_buf[0..60], rep3);
    });

    test("4. @cap/lz4-block — extended literal & match lengths (>= 15 and >= 270)", () => {
        // 300 distinct-ish literal bytes followed by 300 repeated 'Z' bytes -> exercises 255-chained ext len
        let raw: [600]u8 = undefined;
        let i: usize = 0;
        while (i < 300) {
            raw[i] = ((i * 37 + 13) & 0xFF) as u8;
            i += 1;
        }
        while (i < 600) {
            raw[i] = 'Z' as u8;
            i += 1;
        }

        let comp: [700]u8 = undefined;
        let dec_buf: [600]u8 = undefined;
        const c_len = compressBlock(raw[0..600], comp[0..700]);
        expect_eq(c_len > 0 && c_len < 340, true);

        const d_len = decompressBlock(comp[0..(c_len as usize)], dec_buf[0..600]);
        expect_eq(d_len, 600 as i64);
        expect_eq(dec_buf[0], raw[0]);
        expect_eq(dec_buf[299], raw[299]);
        expect_eq(dec_buf[300], 'Z' as u8);
        expect_eq(dec_buf[599], 'Z' as u8);
    });

    test("5. @cap/lz4-block — compressWithSize & decompressWithSize round-trip", () => {
        const payload = "CapDux LZ4 Block Compression — CapDux LZ4 Block Compression — 2026!";
        let comp: [256]u8 = undefined;
        let dec_buf: [256]u8 = undefined;

        const c_len = compressWithSize(payload, comp[0..256]);
        expect_eq(c_len > 4 && c_len < (payload.len as i64), true);

        const d_len = decompressWithSize(comp[0..(c_len as usize)], dec_buf[0..256]);
        expect_eq(d_len, payload.len as i64);
        expect_eq(dec_buf[0..(d_len as usize)], payload);

        // Tampered size header must fail with -1
        comp[0] = comp[0] ^ (1 as u8);
        expect_eq(decompressWithSize(comp[0..(c_len as usize)], dec_buf[0..256]), -1 as i64);
    });

    test("6. @cap/lz4-block — rejects offset=0, out-of-range offset, truncated stream & tiny buffers", () => {
        let dec_buf: [64]u8 = undefined;

        // offset == 0 is invalid in LZ4: token=0x10 (1 lit, 4 match), lit='A', offset=0x0000
        let zero_off: [4]u8 = [0x10, 'A', 0x00, 0x00];
        expect_eq(decompressBlock(zero_off[0..4], dec_buf[0..64]), -1 as i64);

        // offset > out_pos: 1 lit 'A', offset=5
        let oob_off: [4]u8 = [0x10, 'A', 0x05, 0x00];
        expect_eq(decompressBlock(oob_off[0..4], dec_buf[0..64]), -1 as i64);

        // Truncated literal length extension (token=0xF0, no following length byte)
        let trunc_lit: [1]u8 = [0xF0];
        expect_eq(decompressBlock(trunc_lit[0..1], dec_buf[0..64]), -1 as i64);

        // Truncated offset (only 1 byte after literals)
        let trunc_off: [3]u8 = [0x10, 'A', 0x01];
        expect_eq(decompressBlock(trunc_off[0..3], dec_buf[0..64]), -1 as i64);

        // Destination buffer too small
        let tiny: [2]u8 = undefined;
        expect_eq(compressBlock("hello world", tiny[0..2]), -1 as i64);
    });
}
