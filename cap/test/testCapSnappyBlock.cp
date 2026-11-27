// cap/test/testCapSnappyBlock.cp
// Comprehensive unit tests for @cap/snappy-block (Google Snappy Raw Block / Rust snap parity)

import { test, expect_eq } from "cap:test";
import {
    maxCompressedLen,
    uncompressedLen,
    compress,
    decompress,
} from "../../packages/snap/src/index.cp";

fn main() {
    test("1. @cap/snappy-block — empty input & short input round-trip", () => {
        expect_eq(maxCompressedLen(0 as usize), 32 as usize);
        expect_eq(maxCompressedLen(60 as usize), 102 as usize);

        let comp: [64]u8 = undefined;
        let out: [64]u8 = undefined;
        let empty: [1]u8 = [0];

        const c0 = compress(empty[0..0], comp[0..64]);
        expect_eq(c0, 1 as i64); // single varint byte 0x00
        expect_eq(uncompressedLen(comp[0..1]), 0 as i64);
        expect_eq(decompress(comp[0..1], out[0..64]), 0 as i64);

        const short_msg = "Hello!";
        const c1 = compress(short_msg, comp[0..64]);
        expect_eq(uncompressedLen(comp[0..(c1 as usize)]), 6 as i64);
        expect_eq(decompress(comp[0..(c1 as usize)], out[0..64]), 6 as i64);
        expect_eq(out[0..6], short_msg);
    });

    test("2. @cap/snappy-block — high-repetition text compression ratio & uncompressedLen", () => {
        let raw: [512]u8 = undefined;
        let i: usize = 0;
        while (i < 512) {
            raw[i] = ('a' as u8) + ((i % 4) as u8);
            i += 1;
        }

        let comp: [640]u8 = undefined;
        let out: [512]u8 = undefined;
        const c_len = compress(raw[0..512], comp[0..640]);
        expect_eq(c_len > 0 && c_len < 48, true);
        expect_eq(uncompressedLen(comp[0..(c_len as usize)]), 512 as i64);

        const d_len = decompress(comp[0..(c_len as usize)], out[0..512]);
        expect_eq(d_len, 512 as i64);
        expect_eq(out[0], 'a' as u8);
        expect_eq(out[511], 'd' as u8);
    });

    test("3. @cap/snappy-block — long literal > 60 bytes (extended 1-byte & 2-byte length)", () => {
        let raw: [300]u8 = undefined;
        let st: u64 = 123456789;
        let i: usize = 0;
        while (i < 300) {
            st = (st * (1103515245 as u64) + (12345 as u64)) & (0x7FFFFFFF as u64);
            raw[i] = ((st >> 16) & (0xFF as u64)) as u8;
            i += 1;
        }

        let comp: [400]u8 = undefined;
        let out: [300]u8 = undefined;
        const c_len = compress(raw[0..300], comp[0..400]);
        expect_eq(c_len, 305 as i64);
        expect_eq(uncompressedLen(comp[0..(c_len as usize)]), 300 as i64);

        const d_len = decompress(comp[0..(c_len as usize)], out[0..300]);
        expect_eq(d_len, 300 as i64);
        expect_eq(out[0], raw[0]);
        expect_eq(out[150], raw[150]);
        expect_eq(out[299], raw[299]);
    });

    test("4. @cap/snappy-block — overlapping RLE Copy-1 & Copy-2 matches", () => {
        let comp: [256]u8 = undefined;
        let out: [256]u8 = undefined;

        // 1-byte repeating run (triggers overlapping Copy with offset=1)
        const r1 = "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"; // 50 'Z'
        const c1 = compress(r1, comp[0..256]);
        expect_eq(c1 > 0 && c1 < 12, true);
        expect_eq(decompress(comp[0..(c1 as usize)], out[0..256]), 50 as i64);
        expect_eq(out[0..50], r1);

        // 3-byte repeating pattern "xyz" * 30 = 90 bytes (triggers Copy > 64 split)
        let r3: [90]u8 = undefined;
        let i: usize = 0;
        while (i < 90) {
            r3[i] = ('x' as u8) + ((i % 3) as u8);
            i += 1;
        }
        const c3 = compress(r3[0..90], comp[0..256]);
        expect_eq(c3 > 0 && c3 < 20, true);
        expect_eq(decompress(comp[0..(c3 as usize)], out[0..256]), 90 as i64);
        expect_eq(out[0], 'x' as u8);
        expect_eq(out[89], 'z' as u8);
    });

    test("5. @cap/snappy-block — Copy-4 tag decoding compliance", () => {
        // Hand-crafted valid Snappy stream using Copy-4 (tag = ((4-1)<<2)|3 = 0x0F):
        // varint len = 8, Literal 4 bytes "ABCD" (tag=0x0C), Copy-4 len=4 offset=4 (0x04,0x00,0x00,0x00)
        let stream: [11]u8 = [0x08, 0x0C, 'A', 'B', 'C', 'D', 0x0F, 0x04, 0x00, 0x00, 0x00];
        let out: [16]u8 = undefined;
        expect_eq(uncompressedLen(stream[0..11]), 8 as i64);
        expect_eq(decompress(stream[0..11], out[0..16]), 8 as i64);
        expect_eq(out[0..8], "ABCDABCD");
    });

    test("6. @cap/snappy-block — rejects offset=0, truncated streams, length tampering & tiny dst", () => {
        let out: [64]u8 = undefined;
        let empty: [1]u8 = [0];
        expect_eq(uncompressedLen(empty[0..0]), -1 as i64);

        // Copy-1 with offset == 0: header=5, lit 1 'A' (0x00,'A'), copy-1 len=4 off=0 (0x01, 0x00)
        let zero_off: [5]u8 = [0x05, 0x00, 'A', 0x01, 0x00];
        expect_eq(decompress(zero_off[0..5], out[0..64]), -1 as i64);

        // Header claims 20 bytes, stream only delivers 4 bytes
        let wrong_len: [6]u8 = [0x14, 0x0C, 'A', 'B', 'C', 'D'];
        expect_eq(decompress(wrong_len[0..6], out[0..64]), -1 as i64);

        // Truncated literal payload
        let trunc_lit: [3]u8 = [0x04, 0x0C, 'A'];
        expect_eq(decompress(trunc_lit[0..3], out[0..64]), -1 as i64);

        // Tiny output buffer fails compress
        let tiny: [2]u8 = undefined;
        expect_eq(compress("hello snappy", tiny[0..2]), -1 as i64);
    });
}
