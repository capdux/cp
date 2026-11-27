// cap/test/testCapHexColor.cp
// Comprehensive unit tests for @cap/hex-color

import { test, expect_eq } from "cap:test";
import {
    Rgba,
    parseHexColor,
    formatHex6,
    formatHex8,
    packRgba32,
    unpackRgba32,
    invertRgb,
    isDark,
    blendOver,
} from "../../packages/hex-rgb/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/hex-color — Parse 3/4/6/8-digit hex & shorthand #f0a expansion", () => {
        const c3 = parseHexColor("#f0a");
        expect_eq(c3.valid, true);
        expect_eq(c3.r, 0xFF as u8);
        expect_eq(c3.g, 0x00 as u8);
        expect_eq(c3.b, 0xAA as u8);
        expect_eq(c3.a, 0xFF as u8);

        const c4 = parseHexColor("369c");
        expect_eq(c4.valid, true);
        expect_eq(c4.r, 0x33 as u8);
        expect_eq(c4.g, 0x66 as u8);
        expect_eq(c4.b, 0x99 as u8);
        expect_eq(c4.a, 0xCC as u8);

        const c6 = parseHexColor("#1a2B3c");
        expect_eq(c6.valid, true);
        expect_eq(c6.r, 0x1A as u8);
        expect_eq(c6.g, 0x2B as u8);
        expect_eq(c6.b, 0x3C as u8);
        expect_eq(c6.a, 0xFF as u8);

        const c8 = parseHexColor("#11223380");
        expect_eq(c8.valid, true);
        expect_eq(c8.a, 0x80 as u8);
    });

    test("2. @cap/hex-color — formatHex6 & formatHex8 lower/uppercase round-trip", () => {
        const c = parseHexColor("#ff00aa80");
        let b6: [7]u8 = undefined;
        expect_eq(formatHex6(c, false, b6[0..7]), 7 as usize);
        expect_eq(sliceEq(b6[0..7], "#ff00aa"), true);
        expect_eq(formatHex6(c, true, b6[0..7]), 7 as usize);
        expect_eq(sliceEq(b6[0..7], "#FF00AA"), true);

        let b8: [9]u8 = undefined;
        expect_eq(formatHex8(c, false, b8[0..9]), 9 as usize);
        expect_eq(sliceEq(b8[0..9], "#ff00aa80"), true);
        expect_eq(formatHex8(c, true, b8[0..9]), 9 as usize);
        expect_eq(sliceEq(b8[0..9], "#FF00AA80"), true);
    });

    test("3. @cap/hex-color — packRgba32 & unpackRgba32 big-endian round-trip", () => {
        const c = parseHexColor("#12345678");
        const packedVal = packRgba32(c);
        expect_eq(packedVal, 0x12345678 as u32);
        const u = unpackRgba32(packedVal);
        expect_eq(u.r, 0x12 as u8);
        expect_eq(u.g, 0x34 as u8);
        expect_eq(u.b, 0x56 as u8);
        expect_eq(u.a, 0x78 as u8);
    });

    test("4. @cap/hex-color — invertRgb & BT.601 isDark luminance check", () => {
        const black = parseHexColor("#000000");
        const white = parseHexColor("#ffffff");
        expect_eq(isDark(black), true);
        expect_eq(isDark(white), false);
        expect_eq(isDark(parseHexColor("#102030")), true);
        expect_eq(isDark(parseHexColor("#f0f080")), false);

        const inv = invertRgb(parseHexColor("#10203080"));
        expect_eq(inv.r, 239 as u8);
        expect_eq(inv.g, 223 as u8);
        expect_eq(inv.b, 207 as u8);
        expect_eq(inv.a, 0x80 as u8);
    });

    test("5. @cap/hex-color — Porter-Duff source-over blendOver compositing", () => {
        const halfRed = parseHexColor("#ff000080");
        const white = parseHexColor("#ffffff");
        const res = blendOver(halfRed, white);
        expect_eq(res.r, 255 as u8);
        expect_eq(res.g, 127 as u8);
        expect_eq(res.b, 127 as u8);
        expect_eq(res.a, 255 as u8);

        // Fully transparent over transparent stays transparent black.
        const clear = parseHexColor("#00000000");
        const clearRes = blendOver(clear, clear);
        expect_eq(clearRes.a, 0 as u8);
    });

    test("6. @cap/hex-color — Reject invalid lengths/chars & small output buffers", () => {
        expect_eq(parseHexColor("").valid, false);
        expect_eq(parseHexColor("#").valid, false);
        expect_eq(parseHexColor("#12").valid, false);
        expect_eq(parseHexColor("#12345").valid, false);
        expect_eq(parseHexColor("#1234567").valid, false);
        expect_eq(parseHexColor("#123456789").valid, false);
        expect_eq(parseHexColor("#zzzzzz").valid, false);

        const c = parseHexColor("#112233");
        let tiny6: [6]u8 = undefined;
        expect_eq(formatHex6(c, false, tiny6[0..6]), 0 as usize);
        let tiny8: [8]u8 = undefined;
        expect_eq(formatHex8(c, false, tiny8[0..8]), 0 as usize);
    });
}
