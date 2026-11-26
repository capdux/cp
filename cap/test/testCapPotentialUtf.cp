// cap/test/testCapPotentialUtf.cp
// Comprehensive unit tests for @cap/potential-utf

import { test, expect_eq } from "cap:test";
import {
    PotentialUtf8,
    PotentialUtf16,
    PotentialCodePoint,
} from "../../packages/@cap/potential-utf/src/index.cp";

fn main() {
    test("1. @cap/potential-utf — PotentialUtf8 valid ASCII & multi-byte UTF-8", () => {
        const empty = PotentialUtf8.fromStr("");
        expect_eq(empty.len(), 0 as usize);
        expect_eq(empty.isEmpty(), true);
        expect_eq(empty.isValidUtf8(), true);
        expect_eq(empty.toValidPrefix().len, 0 as usize);

        // "Hié中😀" = 1 + 1 + 2 + 3 + 4 = 11 bytes
        const raw: [11]u8 = @as([11]u8, [
            0x48, 0x69,
            0xC3, 0xA9,
            0xE4, 0xB8, 0xAD,
            0xF0, 0x9F, 0x98, 0x80,
        ]);
        const u8w = PotentialUtf8.fromBytes(raw[0..11]);
        expect_eq(u8w.len(), 11 as usize);
        expect_eq(u8w.isEmpty(), false);
        expect_eq(u8w.isValidUtf8(), true);
        expect_eq(u8w.asBytes().len, 11 as usize);
        expect_eq(u8w.toValidPrefix().len, 11 as usize);

        let out: [16]u8 = undefined;
        const n = u8w.decodeLossy(out[0..16]);
        expect_eq(n, 11 as usize);
        expect_eq(out[0], 0x48 as u8);
        expect_eq(out[10], 0x80 as u8);
    });

    test("2. @cap/potential-utf — PotentialUtf8 invalid sequences & toValidPrefix", () => {
        // "AB" + overlong 0xC0 0xAF + "Z"
        const badOverlong: [5]u8 = @as([5]u8, [0x41, 0x42, 0xC0, 0xAF, 0x5A]);
        const p1 = PotentialUtf8.fromBytes(badOverlong[0..5]);
        expect_eq(p1.isValidUtf8(), false);
        const pref1 = p1.toValidPrefix();
        expect_eq(pref1.len, 2 as usize);
        expect_eq(pref1[0], 0x41 as u8);
        expect_eq(pref1[1], 0x42 as u8);

        // UTF-8 encoded surrogate U+D800: ED A0 80 (must be rejected)
        const badSurrogate: [3]u8 = @as([3]u8, [0xED, 0xA0, 0x80]);
        const p2 = PotentialUtf8.fromBytes(badSurrogate[0..3]);
        expect_eq(p2.isValidUtf8(), false);
        expect_eq(p2.toValidPrefix().len, 0 as usize);

        // Out-of-range > U+10FFFF: F4 90 80 80
        const badRange: [4]u8 = @as([4]u8, [0xF4, 0x90, 0x80, 0x80]);
        const p3 = PotentialUtf8.fromBytes(badRange[0..4]);
        expect_eq(p3.isValidUtf8(), false);
    });

    test("3. @cap/potential-utf — PotentialUtf8 decodeLossy with U+FFFD & maximal subpart", () => {
        // "A" + truncated 3-byte (E4 B8) + "B" + lone invalid 0xFF + "C"
        const raw: [6]u8 = @as([6]u8, [0x41, 0xE4, 0xB8, 0x42, 0xFF, 0x43]);
        const p = PotentialUtf8.fromBytes(raw[0..6]);
        let out: [32]u8 = undefined;
        const n = p.decodeLossy(out[0..32]);
        // Expected: 'A' (1) + U+FFFD (3) + 'B' (1) + U+FFFD (3) + 'C' (1) = 9 bytes
        expect_eq(n, 9 as usize);
        expect_eq(out[0], 0x41 as u8);
        expect_eq(out[1], 0xEF as u8);
        expect_eq(out[2], 0xBF as u8);
        expect_eq(out[3], 0xBD as u8);
        expect_eq(out[4], 0x42 as u8);
        expect_eq(out[5], 0xEF as u8);
        expect_eq(out[6], 0xBF as u8);
        expect_eq(out[7], 0xBD as u8);
        expect_eq(out[8], 0x43 as u8);
    });

    test("4. @cap/potential-utf — PotentialUtf16 valid BMP & surrogate pairs", () => {
        // 'A' (0x0041), 'é' (0x00E9), '中' (0x4E2D), '😀' (0xD83D, 0xDE00)
        const units: [5]u16 = @as([5]u16, [0x0041, 0x00E9, 0x4E2D, 0xD83D, 0xDE00]);
        const u16w = PotentialUtf16.fromSlice(units[0..5]);
        expect_eq(u16w.len(), 5 as usize);
        expect_eq(u16w.isEmpty(), false);
        expect_eq(u16w.isValidUtf16(), true);

        let out: [32]u8 = undefined;
        const n = u16w.toUtf8Lossy(out[0..32]);
        // 1 + 2 + 3 + 4 = 10 bytes
        expect_eq(n, 10 as usize);
        expect_eq(out[0], 0x41 as u8);
        expect_eq(out[1], 0xC3 as u8);
        expect_eq(out[2], 0xA9 as u8);
        expect_eq(out[3], 0xE4 as u8);
        expect_eq(out[4], 0xB8 as u8);
        expect_eq(out[5], 0xAD as u8);
        expect_eq(out[6], 0xF0 as u8);
        expect_eq(out[7], 0x9F as u8);
        expect_eq(out[8], 0x98 as u8);
        expect_eq(out[9], 0x80 as u8);
    });

    test("5. @cap/potential-utf — PotentialUtf16 unpaired high/low surrogates to U+FFFD", () => {
        // lone high 0xD800, 'X', lone low 0xDC00, trailing lone high 0xDBFF
        const units: [4]u16 = @as([4]u16, [0xD800, 0x0058, 0xDC00, 0xDBFF]);
        const u16w = PotentialUtf16.fromSlice(units[0..4]);
        expect_eq(u16w.isValidUtf16(), false);

        let out: [32]u8 = undefined;
        const n = u16w.toUtf8Lossy(out[0..32]);
        // U+FFFD (3) + 'X' (1) + U+FFFD (3) + U+FFFD (3) = 10 bytes
        expect_eq(n, 10 as usize);
        expect_eq(out[0], 0xEF as u8);
        expect_eq(out[1], 0xBF as u8);
        expect_eq(out[2], 0xBD as u8);
        expect_eq(out[3], 0x58 as u8);
        expect_eq(out[4], 0xEF as u8);
        expect_eq(out[7], 0xEF as u8);
        expect_eq(out[9], 0xBD as u8);
    });

    test("6. @cap/potential-utf — PotentialCodePoint scalar validation & replacement", () => {
        const ascii = PotentialCodePoint.fromU32(0x41 as u32);
        expect_eq(ascii.toU32(), 0x41 as u32);
        expect_eq(ascii.isValidScalar(), true);
        expect_eq(ascii.toCharOrReplacement(), 0x41 as u32);

        const maxScalar = PotentialCodePoint.fromU32(0x10FFFF as u32);
        expect_eq(maxScalar.isValidScalar(), true);
        expect_eq(maxScalar.toCharOrReplacement(), 0x10FFFF as u32);

        const surLow = PotentialCodePoint.fromU32(0xD800 as u32);
        expect_eq(surLow.isValidScalar(), false);
        expect_eq(surLow.toCharOrReplacement(), 0xFFFD as u32);

        const surHigh = PotentialCodePoint.fromU32(0xDFFF as u32);
        expect_eq(surHigh.isValidScalar(), false);
        expect_eq(surHigh.toCharOrReplacement(), 0xFFFD as u32);

        const overMax = PotentialCodePoint.fromU32(0x110000 as u32);
        expect_eq(overMax.isValidScalar(), false);
        expect_eq(overMax.toCharOrReplacement(), 0xFFFD as u32);

        let buf: [4]u8 = undefined;
        const n = overMax.encodeUtf8Lossy(buf[0..4]);
        expect_eq(n, 3 as usize);
        expect_eq(buf[0], 0xEF as u8);
        expect_eq(buf[1], 0xBF as u8);
        expect_eq(buf[2], 0xBD as u8);
    });
}
