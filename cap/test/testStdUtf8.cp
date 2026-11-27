import { test, expect_eq } from "cap:test";
import {
    DecodedChar,
    isValidUtf8,
    decodeCodepoint,
    encodeCodepoint,
    encodeCodepointLossy,
    utf8SeqLen,
    countCodepoints,
    isCharBoundary,
    floorCharBoundary,
} from "cap:utf8";

fn main() {
    test("cap:utf8 - utf8SeqLen lead byte classification", () => {
        expect_eq(utf8SeqLen(0x00), 1);
        expect_eq(utf8SeqLen(0x41), 1);
        expect_eq(utf8SeqLen(0x7F), 1);

        expect_eq(utf8SeqLen(0x80), 0);
        expect_eq(utf8SeqLen(0xBF), 0);
        expect_eq(utf8SeqLen(0xC0), 0);
        expect_eq(utf8SeqLen(0xC1), 0);

        expect_eq(utf8SeqLen(0xC2), 2);
        expect_eq(utf8SeqLen(0xDF), 2);

        expect_eq(utf8SeqLen(0xE0), 3);
        expect_eq(utf8SeqLen(0xEF), 3);

        expect_eq(utf8SeqLen(0xF0), 4);
        expect_eq(utf8SeqLen(0xF4), 4);

        expect_eq(utf8SeqLen(0xF5), 0);
        expect_eq(utf8SeqLen(0xFF), 0);
    });

    test("cap:utf8 - isValidUtf8 accepts valid ASCII and multibyte sequences", () => {
        expect_eq(isValidUtf8(""), true);
        expect_eq(isValidUtf8("Hello, CP standard library 2026!"), true);
        expect_eq(isValidUtf8("café"), true);
        expect_eq(isValidUtf8("你好，世界"), true);
        expect_eq(isValidUtf8("Aé中🦀"), true);

        // Boundary valid sequences: U+0080, U+07FF, U+0800, U+D7FF, U+E000, U+FFFF, U+10000, U+10FFFF
        const boundary: [25]u8 = [
            0xC2, 0x80,
            0xDF, 0xBF,
            0xE0, 0xA0, 0x80,
            0xED, 0x9F, 0xBF,
            0xEE, 0x80, 0x80,
            0xEF, 0xBF, 0xBF,
            0xF0, 0x90, 0x80, 0x80,
            0xF4, 0x8F, 0xBF, 0xBF,
            0x7F,
        ];
        expect_eq(isValidUtf8(boundary[0..]), true);
    });

    test("cap:utf8 - isValidUtf8 strictly rejects overlong, surrogate, >U+10FFFF, and truncated", () => {
        // Overlong 2-byte NUL (C0 80)
        const overlong2: [2]u8 = [0xC0, 0x80];
        expect_eq(isValidUtf8(overlong2[0..]), false);

        // Overlong 3-byte (E0 9F BF)
        const overlong3: [3]u8 = [0xE0, 0x9F, 0xBF];
        expect_eq(isValidUtf8(overlong3[0..]), false);

        // Overlong 4-byte (F0 8F BF BF)
        const overlong4: [4]u8 = [0xF0, 0x8F, 0xBF, 0xBF];
        expect_eq(isValidUtf8(overlong4[0..]), false);

        // UTF-16 surrogates U+D800 (ED A0 80) and U+DFFF (ED BF BF)
        const surr_lo: [3]u8 = [0xED, 0xA0, 0x80];
        const surr_hi: [3]u8 = [0xED, 0xBF, 0xBF];
        expect_eq(isValidUtf8(surr_lo[0..]), false);
        expect_eq(isValidUtf8(surr_hi[0..]), false);

        // > U+10FFFF: U+110000 (F4 90 80 80)
        const too_large: [4]u8 = [0xF4, 0x90, 0x80, 0x80];
        expect_eq(isValidUtf8(too_large[0..]), false);

        // Truncated 2, 3, 4-byte tails
        const trunc2: [1]u8 = [0xC2];
        const trunc3: [2]u8 = [0xE4, 0xB8];
        const trunc4: [3]u8 = [0xF0, 0x9F, 0xA6];
        expect_eq(isValidUtf8(trunc2[0..]), false);
        expect_eq(isValidUtf8(trunc3[0..]), false);
        expect_eq(isValidUtf8(trunc4[0..]), false);

        // Stray continuation byte
        const stray: [2]u8 = ['a', 0x80];
        expect_eq(isValidUtf8(stray[0..]), false);
    });

    test("cap:utf8 - decodeCodepoint across 1/2/3/4-byte scalars and invalid input", () => {
        const sample = "Aé中🦀";
        const d0: DecodedChar = decodeCodepoint(sample, 0);
        expect_eq(d0.valid, true);
        expect_eq(d0.len, 1);
        expect_eq(d0.codepoint, 0x41);

        const d1: DecodedChar = decodeCodepoint(sample, 1);
        expect_eq(d1.valid, true);
        expect_eq(d1.len, 2);
        expect_eq(d1.codepoint, 0x00E9);

        const d2: DecodedChar = decodeCodepoint(sample, 3);
        expect_eq(d2.valid, true);
        expect_eq(d2.len, 3);
        expect_eq(d2.codepoint, 0x4E2D);

        const d3: DecodedChar = decodeCodepoint(sample, 6);
        expect_eq(d3.valid, true);
        expect_eq(d3.len, 4);
        expect_eq(d3.codepoint, 0x1F980);

        const d_eof: DecodedChar = decodeCodepoint(sample, 10);
        expect_eq(d_eof.valid, false);
        expect_eq(d_eof.len, 0);
        expect_eq(d_eof.codepoint, 0xFFFD);

        const bad: [3]u8 = [0xED, 0xA0, 0x80];
        const d_bad: DecodedChar = decodeCodepoint(bad[0..], 0);
        expect_eq(d_bad.valid, false);
        expect_eq(d_bad.len, 1);
        expect_eq(d_bad.codepoint, 0xFFFD);
    });

    test("cap:utf8 - encodeCodepoint and encodeCodepointLossy round-trip", () => {
        let buf: [4]u8 = undefined;

        const n1 = encodeCodepoint(0x24, buf[0..]);
        expect_eq(n1, 1);
        expect_eq(decodeCodepoint(buf[0..n1], 0).codepoint, 0x24);

        const n2 = encodeCodepoint(0x00E9, buf[0..]);
        expect_eq(n2, 2);
        expect_eq(decodeCodepoint(buf[0..n2], 0).codepoint, 0x00E9);

        const n3 = encodeCodepoint(0x4E2D, buf[0..]);
        expect_eq(n3, 3);
        expect_eq(decodeCodepoint(buf[0..n3], 0).codepoint, 0x4E2D);

        const n4 = encodeCodepoint(0x1F980, buf[0..]);
        expect_eq(n4, 4);
        expect_eq(decodeCodepoint(buf[0..n4], 0).codepoint, 0x1F980);

        // Invalid scalars rejected by encodeCodepoint
        expect_eq(encodeCodepoint(0xD800, buf[0..]), 0);
        expect_eq(encodeCodepoint(0xDFFF, buf[0..]), 0);
        expect_eq(encodeCodepoint(0x110000, buf[0..]), 0);

        // Buffer too small
        let tiny: [2]u8 = undefined;
        expect_eq(encodeCodepoint(0x4E2D, tiny[0..]), 0);

        // Lossy encoding writes U+FFFD (EF BF BD)
        const nl = encodeCodepointLossy(0xD800, buf[0..]);
        expect_eq(nl, 3);
        expect_eq(buf[0], 0xEF);
        expect_eq(buf[1], 0xBF);
        expect_eq(buf[2], 0xBD);
        expect_eq(decodeCodepoint(buf[0..nl], 0).codepoint, 0xFFFD);
    });

    test("cap:utf8 - countCodepoints, isCharBoundary, and floorCharBoundary", () => {
        const s = "Aé中🦀"; // bytes: [0]='A', [1..3]='é', [3..6]='中', [6..10]='🦀'
        expect_eq(countCodepoints(s), 4);
        expect_eq(countCodepoints(""), 0);

        expect_eq(isCharBoundary(s, 0), true);
        expect_eq(isCharBoundary(s, 1), true);
        expect_eq(isCharBoundary(s, 2), false);
        expect_eq(isCharBoundary(s, 3), true);
        expect_eq(isCharBoundary(s, 4), false);
        expect_eq(isCharBoundary(s, 5), false);
        expect_eq(isCharBoundary(s, 6), true);
        expect_eq(isCharBoundary(s, 7), false);
        expect_eq(isCharBoundary(s, 8), false);
        expect_eq(isCharBoundary(s, 9), false);
        expect_eq(isCharBoundary(s, 10), true);
        expect_eq(isCharBoundary(s, 11), false);

        expect_eq(floorCharBoundary(s, 0), 0);
        expect_eq(floorCharBoundary(s, 2), 1);
        expect_eq(floorCharBoundary(s, 3), 3);
        expect_eq(floorCharBoundary(s, 5), 3);
        expect_eq(floorCharBoundary(s, 9), 6);
        expect_eq(floorCharBoundary(s, 10), 10);
        expect_eq(floorCharBoundary(s, 99), 10);
    });
}
