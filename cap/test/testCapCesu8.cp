import { test, expect_eq } from "cap:test";
import {
    toCesu8,
    fromCesu8,
    isValidCesu8AsciiOrBmp,
} from "../../packages/cesu8/src/index.cp";

fn main() {
    test("1. @cap/cesu8 — encodes and decodes U+10400 (Deseret) 4-byte UTF-8 <-> 6-byte CESU-8", () => {
        // U+10400: UTF-8 = F0 90 90 80; CESU-8 = ED A0 81 ED B0 80
        const utf8: [4]u8 = [0xF0, 0x90, 0x90, 0x80];
        let cesu: [16]u8 = [0; 16];
        const clen = toCesu8(utf8[0..4], false, cesu[0..16]);
        expect_eq(clen, 6 as i64);
        expect_eq(cesu[0], 0xED as u8);
        expect_eq(cesu[1], 0xA0 as u8);
        expect_eq(cesu[2], 0x81 as u8);
        expect_eq(cesu[3], 0xED as u8);
        expect_eq(cesu[4], 0xB0 as u8);
        expect_eq(cesu[5], 0x80 as u8);

        let back: [16]u8 = [0; 16];
        const ulen = fromCesu8(cesu[0..6], false, back[0..16]);
        expect_eq(ulen, 4 as i64);
        expect_eq(back[0], 0xF0 as u8);
        expect_eq(back[1], 0x90 as u8);
        expect_eq(back[2], 0x90 as u8);
        expect_eq(back[3], 0x80 as u8);
    });

    test("2. @cap/cesu8 — round-trips emoji U+1F600 with surrounding ASCII", () => {
        // "A😀B": 41 F0 9F 98 80 42
        const utf8: [6]u8 = [0x41, 0xF0, 0x9F, 0x98, 0x80, 0x42];
        let cesu: [16]u8 = [0; 16];
        const clen = toCesu8(utf8[0..6], false, cesu[0..16]);
        expect_eq(clen, 8 as i64);
        let back: [16]u8 = [0; 16];
        const ulen = fromCesu8(cesu[0..8], false, back[0..16]);
        expect_eq(ulen, 6 as i64);
        expect_eq(back[0], 0x41 as u8);
        expect_eq(back[5], 0x42 as u8);
    });

    test("3. @cap/cesu8 — Java Modified UTF-8 encodes NUL as 0xC0 0x80 and decodes back", () => {
        const raw: [3]u8 = [0x61, 0x00, 0x62];
        let mutf: [16]u8 = [0; 16];
        const mlen = toCesu8(raw[0..3], true, mutf[0..16]);
        expect_eq(mlen, 4 as i64);
        expect_eq(mutf[1], 0xC0 as u8);
        expect_eq(mutf[2], 0x80 as u8);

        let back: [16]u8 = [0; 16];
        const blen = fromCesu8(mutf[0..4], true, back[0..16]);
        expect_eq(blen, 3 as i64);
        expect_eq(back[1], 0x00 as u8);
    });

    test("4. @cap/cesu8 — passes BMP UTF-8 (2-byte & 3-byte) through unchanged", () => {
        // "é你" = C3 A9 E4 BD A0
        const bmp: [5]u8 = [0xC3, 0xA9, 0xE4, 0xBD, 0xA0];
        expect_eq(isValidCesu8AsciiOrBmp(bmp[0..5], false), true);
        let out: [16]u8 = [0; 16];
        expect_eq(toCesu8(bmp[0..5], false, out[0..16]), 5 as i64);
    });

    test("5. @cap/cesu8 — rejects unpaired surrogate in fromCesu8", () => {
        const bad: [3]u8 = [0xED, 0xA0, 0x81];
        let out: [16]u8 = [0; 16];
        expect_eq(fromCesu8(bad[0..3], false, out[0..16]), -1 as i64);
    });

    test("6. @cap/cesu8 — isValidCesu8AsciiOrBmp detects 4-byte UTF-8 and NUL", () => {
        const emoji: [4]u8 = [0xF0, 0x9F, 0x98, 0x80];
        expect_eq(isValidCesu8AsciiOrBmp(emoji[0..4], false), false);
        const nul: [1]u8 = [0x00];
        expect_eq(isValidCesu8AsciiOrBmp(nul[0..1], false), true);
        expect_eq(isValidCesu8AsciiOrBmp(nul[0..1], true), false);
    });
}
