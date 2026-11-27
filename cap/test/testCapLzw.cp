// cap/test/testCapLzw.cp
// Comprehensive unit tests for @cap/lzw (GIF89a LSB-first LZW / Rust weezl parity)

import { test, expect_eq } from "cap:test";
import { encode, decode } from "../../packages/weezl/src/index.cp";

fn main() {
    test("1. @cap/lzw — empty input & single byte round-trip", () => {
        let comp: [32]u8 = undefined;
        let out: [32]u8 = undefined;
        let empty: [1]u8 = [0];

        const c0 = encode(empty[0..0], comp[0..32]);
        expect_eq(c0 > 0, true);
        expect_eq(decode(comp[0..(c0 as usize)], out[0..32]), 0 as i64);

        const c1 = encode("Z", comp[0..32]);
        expect_eq(c1 > 0, true);
        expect_eq(decode(comp[0..(c1 as usize)], out[0..32]), 1 as i64);
        expect_eq(out[0], 'Z' as u8);
    });

    test("2. @cap/lzw — classic TOBEORNOTTOBEORTOBEORNOT round-trip", () => {
        const msg = "TOBEORNOTTOBEORTOBEORNOT";
        let comp: [128]u8 = undefined;
        let out: [128]u8 = undefined;

        const c_len = encode(msg, comp[0..128]);
        expect_eq(c_len > 0 && c_len < (msg.len as i64), true);

        const d_len = decode(comp[0..(c_len as usize)], out[0..128]);
        expect_eq(d_len, msg.len as i64);
        expect_eq(out[0..(d_len as usize)], msg);
    });

    test("3. @cap/lzw — KwKwK self-referential edge case (ABABAB... & AAAAAA...)", () => {
        let comp: [256]u8 = undefined;
        let out: [256]u8 = undefined;

        const ab = "ABABABABABABABABABABABABABABABAB";
        const c1 = encode(ab, comp[0..256]);
        expect_eq(c1 > 0 && c1 < 20, true);
        expect_eq(decode(comp[0..(c1 as usize)], out[0..256]), ab.len as i64);
        expect_eq(out[0..ab.len], ab);

        let aaa: [200]u8 = undefined;
        let i: usize = 0;
        while (i < 200) {
            aaa[i] = 'A' as u8;
            i += 1;
        }
        const c2 = encode(aaa[0..200], comp[0..256]);
        expect_eq(c2 > 0 && c2 < 35, true);
        expect_eq(decode(comp[0..(c2 as usize)], out[0..256]), 200 as i64);
        expect_eq(out[0], 'A' as u8);
        expect_eq(out[199], 'A' as u8);
    });

    test("4. @cap/lzw — variable code width transition 9 -> 10 bits (> 512 dictionary entries)", () => {
        // Generate 900 bytes with varied pairs to cross next_code == 512 (9 -> 10 bits)
        let raw: [900]u8 = undefined;
        let i: usize = 0;
        while (i < 900) {
            raw[i] = ('a' as u8) + (((i * 7 + (i / 13)) % 26) as u8);
            i += 1;
        }

        let comp: [1500]u8 = undefined;
        let out: [900]u8 = undefined;
        const c_len = encode(raw[0..900], comp[0..1500]);
        expect_eq(c_len > 0, true);

        const d_len = decode(comp[0..(c_len as usize)], out[0..900]);
        expect_eq(d_len, 900 as i64);
        i = 0;
        while (i < 900) {
            expect_eq(out[i], raw[i]);
            i += 1;
        }
    });

    test("5. @cap/lzw — all 256 byte values round-trip", () => {
        let raw: [256]u8 = undefined;
        let i: usize = 0;
        while (i < 256) {
            raw[i] = i as u8;
            i += 1;
        }

        let comp: [512]u8 = undefined;
        let out: [256]u8 = undefined;
        const c_len = encode(raw[0..256], comp[0..512]);
        expect_eq(c_len > 0, true);

        const d_len = decode(comp[0..(c_len as usize)], out[0..256]);
        expect_eq(d_len, 256 as i64);
        expect_eq(out[0], 0 as u8);
        expect_eq(out[128], 128 as u8);
        expect_eq(out[255], 255 as u8);
    });

    test("6. @cap/lzw — rejects truncated streams, invalid codes & tiny buffers", () => {
        let out: [64]u8 = undefined;
        let empty: [1]u8 = [0];
        expect_eq(decode(empty[0..0], out[0..64]), -1 as i64);

        // Truncated stream (missing EOI_CODE)
        let comp: [64]u8 = undefined;
        const c_len = encode("HELLO LZW WORLD", comp[0..64]);
        expect_eq(c_len > 4, true);
        expect_eq(decode(comp[0..2], out[0..64]), -1 as i64);

        // Destination buffer too small for decode
        let tiny: [2]u8 = undefined;
        expect_eq(decode(comp[0..(c_len as usize)], tiny[0..2]), -1 as i64);

        // Destination buffer too small for encode
        expect_eq(encode("HELLO LZW WORLD", tiny[0..2]), -1 as i64);

        // Invalid first code >= 256 (0xFF, 0x01 -> 9-bit code 511 without prior entry)
        let bad_code: [3]u8 = [0xFF, 0x01, 0x00];
        expect_eq(decode(bad_code[0..3], out[0..64]), -1 as i64);
    });
}
