// cap/test/testCapCobs.cp
// Comprehensive unit tests for @cap/cobs

import { test, expect_eq } from "cap:test";
import {
    max_encoded_len,
    encode,
    decode,
    encode_with_sentinel,
    decode_with_sentinel,
    encode_framed,
    decode_framed,
    CobsDecoder,
} from "../../packages/cobs/src/index.cp";

fn main() {
    test("1. @cap/cobs — Empty & all-zero inputs + max_encoded_len", () => {
        expect_eq(max_encoded_len(0), 1 as usize);
        expect_eq(max_encoded_len(254), 256 as usize);

        let empty_src: [1]u8 = [0];
        let enc: [16]u8 = undefined;
        let dec_buf: [16]u8 = undefined;

        const n0 = encode(empty_src[0..0], enc[0..16]);
        expect_eq(n0, 1 as usize);
        expect_eq(enc[0], 0x01 as u8);
        expect_eq(decode(enc[0..n0], dec_buf[0..16]), 0 as i64);

        // [0x00] -> [0x01, 0x01]
        let z1: [1]u8 = [0x00];
        const nz1 = encode(z1[0..1], enc[0..16]);
        expect_eq(nz1, 2 as usize);
        expect_eq(enc[0], 0x01 as u8);
        expect_eq(enc[1], 0x01 as u8);
        expect_eq(decode(enc[0..nz1], dec_buf[0..16]), 1 as i64);
        expect_eq(dec_buf[0], 0x00 as u8);

        // [0x00, 0x00] -> [0x01, 0x01, 0x01]
        let z2: [2]u8 = [0x00, 0x00];
        const nz2 = encode(z2[0..2], enc[0..16]);
        expect_eq(nz2, 3 as usize);
        expect_eq(decode(enc[0..nz2], dec_buf[0..16]), 2 as i64);
        expect_eq(dec_buf[0], 0x00 as u8);
        expect_eq(dec_buf[1], 0x00 as u8);
    });

    test("2. @cap/cobs — Cheshire & Baker official COBS test vectors", () => {
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        // [0x11, 0x22, 0x00, 0x33] -> [0x03, 0x11, 0x22, 0x02, 0x33]
        let v1: [4]u8 = [0x11, 0x22, 0x00, 0x33];
        const n1 = encode(v1[0..4], enc[0..32]);
        expect_eq(n1, 5 as usize);
        expect_eq(enc[0], 0x03 as u8);
        expect_eq(enc[1], 0x11 as u8);
        expect_eq(enc[2], 0x22 as u8);
        expect_eq(enc[3], 0x02 as u8);
        expect_eq(enc[4], 0x33 as u8);
        expect_eq(decode(enc[0..n1], dec_buf[0..32]), 4 as i64);
        expect_eq(dec_buf[2], 0x00 as u8);
        expect_eq(dec_buf[3], 0x33 as u8);

        // [0x11, 0x00, 0x00, 0x00] -> [0x02, 0x11, 0x01, 0x01, 0x01]
        let v2: [4]u8 = [0x11, 0x00, 0x00, 0x00];
        const n2 = encode(v2[0..4], enc[0..32]);
        expect_eq(n2, 5 as usize);
        expect_eq(enc[0], 0x02 as u8);
        expect_eq(enc[1], 0x11 as u8);
        expect_eq(enc[2], 0x01 as u8);
        expect_eq(enc[3], 0x01 as u8);
        expect_eq(enc[4], 0x01 as u8);
        expect_eq(decode(enc[0..n2], dec_buf[0..32]), 4 as i64);
        expect_eq(dec_buf[0], 0x11 as u8);
        expect_eq(dec_buf[3], 0x00 as u8);
    });

    test("3. @cap/cobs — 254 & 255-byte non-zero block boundaries", () => {
        let src254: [254]u8 = undefined;
        let i: usize = 0;
        while (i < 254) {
            src254[i] = (i + 1) as u8;
            i += 1;
        }
        let enc: [300]u8 = undefined;
        let dec_buf: [300]u8 = undefined;

        const n254 = encode(src254[0..254], enc[0..300]);
        expect_eq(n254, 255 as usize);
        expect_eq(enc[0], 0xFF as u8);
        expect_eq(decode(enc[0..n254], dec_buf[0..300]), 254 as i64);
        expect_eq(dec_buf[0], 1 as u8);
        expect_eq(dec_buf[253], 254 as u8);

        // 255 non-zero bytes: [1..254, 0xAB] -> [0xFF, 1..254, 0x02, 0xAB] (257 bytes)
        let src255: [255]u8 = undefined;
        i = 0;
        while (i < 254) {
            src255[i] = (i + 1) as u8;
            i += 1;
        }
        src255[254] = 0xAB;
        const n255 = encode(src255[0..255], enc[0..300]);
        expect_eq(n255, 257 as usize);
        expect_eq(enc[0], 0xFF as u8);
        expect_eq(enc[255], 0x02 as u8);
        expect_eq(enc[256], 0xAB as u8);
        expect_eq(decode(enc[0..n255], dec_buf[0..300]), 255 as i64);
        expect_eq(dec_buf[254], 0xAB as u8);
    });

    test("4. @cap/cobs — Custom sentinel COBS/XOR (0x55 & 0x7E)", () => {
        let src: [5]u8 = [0x55, 0x00, 0x55, 0xAA, 0xFF];
        let enc: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        const n = encode_with_sentinel(src[0..5], enc[0..32], 0x55);
        expect_eq(n > (0 as usize), true);
        // Encoded output must contain zero occurrences of sentinel 0x55
        let i: usize = 0;
        while (i < n) {
            expect_eq(enc[i] != (0x55 as u8), true);
            i += 1;
        }
        const d = decode_with_sentinel(enc[0..n], dec_buf[0..32], 0x55);
        expect_eq(d, 5 as i64);
        expect_eq(dec_buf[0], 0x55 as u8);
        expect_eq(dec_buf[1], 0x00 as u8);
        expect_eq(dec_buf[4], 0xFF as u8);
    });

    test("5. @cap/cobs — encode_framed & decode_framed delimiter handling", () => {
        let src: [4]u8 = [0xDE, 0x00, 0xAD, 0xBE];
        let framed: [32]u8 = undefined;
        let dec_buf: [32]u8 = undefined;

        const nf = encode_framed(src[0..4], framed[0..32]);
        expect_eq(nf, 6 as usize);
        expect_eq(framed[nf - 1], 0x00 as u8);

        // Append trailing garbage after delimiter; decode_framed must stop at first 0x00
        framed[nf] = 0x99;
        const d = decode_framed(framed[0..(nf + 1)], dec_buf[0..32]);
        expect_eq(d, 4 as i64);
        expect_eq(dec_buf[0], 0xDE as u8);
        expect_eq(dec_buf[1], 0x00 as u8);
        expect_eq(dec_buf[3], 0xBE as u8);
    });

    test("6. @cap/cobs — Streaming CobsDecoder multi-frame & malformed frame rejection", () => {
        let f1_src: [3]u8 = [0x10, 0x00, 0x20];
        let f2_src: [2]u8 = [0xAA, 0xBB];
        let wire: [32]u8 = undefined;
        const n1 = encode_framed(f1_src[0..3], wire[0..16]);
        const n2 = encode_framed(f2_src[0..2], wire[n1..32]);
        const total = n1 + n2;

        let decoder = CobsDecoder.init();
        let frames_ok: usize = 0;
        let i: usize = 0;
        while (i < total) {
            const r = decoder.push(wire[i]);
            expect_eq(r >= (0 as i64), true);
            if (r > (0 as i64)) {
                const fr = decoder.frame();
                if (frames_ok == 0) {
                    expect_eq(fr.len, 3 as usize);
                    expect_eq(fr[0], 0x10 as u8);
                    expect_eq(fr[1], 0x00 as u8);
                    expect_eq(fr[2], 0x20 as u8);
                } else {
                    expect_eq(fr.len, 2 as usize);
                    expect_eq(fr[0], 0xAA as u8);
                    expect_eq(fr[1], 0xBB as u8);
                }
                frames_ok += 1;
            }
            i += 1;
        }
        expect_eq(frames_ok, 2 as usize);

        // Malformed truncated frame: code=0x05 expects 4 data bytes, gets 1 byte then 0x00
        expect_eq(decoder.push(0x05), 0 as i64);
        expect_eq(decoder.push(0x11), 0 as i64);
        expect_eq(decoder.push(0x00), -1 as i64);

        // Direct decode rejects embedded 0x00 and truncated block
        let bad_zero: [3]u8 = [0x03, 0x00, 0x11];
        let bad_trunc: [2]u8 = [0x05, 0x11];
        let out_buf: [16]u8 = undefined;
        expect_eq(decode(bad_zero[0..3], out_buf[0..16]), -1 as i64);
        expect_eq(decode(bad_trunc[0..2], out_buf[0..16]), -1 as i64);
    });
}
