// cap/test/testCapUtf8Valid.cp
// Comprehensive unit tests for @cap/utf8-valid

import { test, expect_eq } from "cap:test";
import {
    Utf8Error,
    Utf8CheckResult,
    validateUtf8,
    isValidUtf8,
    isAscii,
    toLossyUtf8,
    utf16Len,
} from "../../packages/@cap/utf8-valid/src/index.cp";

fn main() {
    test("1. @cap/utf8-valid — Valid 1/2/3/4-byte UTF-8 & ASCII fast-path", () => {
        expect_eq(isAscii("Hello, CP! 123"), true);
        expect_eq(isAscii("Hello 世界"), false);

        const s = "Aé中🚀"; // 1 + 2 + 3 + 4 = 10 bytes, 4 codepoints
        const r = validateUtf8(s);
        expect_eq(r.valid, true);
        expect_eq(r.valid_up_to, 10 as usize);
        expect_eq(r.error_len, 0 as u8);
        expect_eq(r.codepoint_count, 4 as usize);
        expect_eq(isValidUtf8(""), true);
    });

    test("2. @cap/utf8-valid — Overlong encodings rejected (0xC0/0xC1, 0xE0 0x80, 0xF0 0x80)", () => {
        // 2-byte overlong NUL: C0 80
        let ov2: [3]u8 = ['a', 0xC0, 0x80];
        const r2 = validateUtf8(ov2[0..3]);
        expect_eq(r2.valid, false);
        expect_eq(r2.valid_up_to, 1 as usize);
        expect_eq(r2.error_len, 1 as u8);

        // 3-byte overlong: E0 80 AF
        let ov3: [3]u8 = [0xE0, 0x80, 0xAF];
        const r3 = validateUtf8(ov3[0..3]);
        expect_eq(r3.valid, false);
        expect_eq(r3.valid_up_to, 0 as usize);
        expect_eq(r3.error_len, 1 as u8);

        // 4-byte overlong: F0 80 80 AF
        let ov4: [4]u8 = [0xF0, 0x80, 0x80, 0xAF];
        const r4 = validateUtf8(ov4[0..4]);
        expect_eq(r4.valid, false);
        expect_eq(r4.valid_up_to, 0 as usize);
        expect_eq(r4.error_len, 1 as u8);
    });

    test("3. @cap/utf8-valid — UTF-16 surrogates (U+D800..U+DFFF) & > U+10FFFF rejected", () => {
        // U+D800 encoded as ED A0 80
        let surr: [4]u8 = ['X', 0xED, 0xA0, 0x80];
        const rs = validateUtf8(surr[0..4]);
        expect_eq(rs.valid, false);
        expect_eq(rs.valid_up_to, 1 as usize);
        expect_eq(rs.error_len, 1 as u8);

        // U+110000 encoded as F4 90 80 80 (> U+10FFFF)
        let oob: [4]u8 = [0xF4, 0x90, 0x80, 0x80];
        const ro = validateUtf8(oob[0..4]);
        expect_eq(ro.valid, false);
        expect_eq(ro.valid_up_to, 0 as usize);
        expect_eq(ro.error_len, 1 as u8);

        // Invalid lead byte 0xF5
        let f5: [1]u8 = [0xF5];
        expect_eq(isValidUtf8(f5[0..1]), false);
    });

    test("4. @cap/utf8-valid — Truncated tail sequence reports error_len == 0", () => {
        // Truncated 3-byte CJK character "中" (E4 B8 AD -> missing AD)
        let trunc: [4]u8 = ['H', 'i', 0xE4, 0xB8];
        const r = validateUtf8(trunc[0..4]);
        expect_eq(r.valid, false);
        expect_eq(r.valid_up_to, 2 as usize);
        expect_eq(r.error_len, 0 as u8);
        expect_eq(r.codepoint_count, 2 as usize);
    });

    test("5. @cap/utf8-valid — toLossyUtf8 replaces invalid subparts with U+FFFD (EF BF BD)", () => {
        let raw: [6]u8 = ['O', 'K', 0xFF, 0xE4, 0xB8, '!'];
        let out: [32]u8 = undefined;
        // 0xFF -> U+FFFD, 0xE4 0xB8 (maximal subpart of 2 bytes before '!') -> single U+FFFD
        const clean = toLossyUtf8(raw[0..6], out[0..32]);
        expect_eq(isValidUtf8(clean), true);
        expect_eq(clean, "OK\xef\xbf\xbd\xef\xbf\xbd!");
    });

    test("6. @cap/utf8-valid — utf16Len surrogate-pair accounting (LSP/JS interop)", () => {
        expect_eq(utf16Len("hello"), 5 as usize);
        expect_eq(utf16Len("你好"), 2 as usize);        // 2 BMP chars => 2 UTF-16 units
        expect_eq(utf16Len("🚀"), 2 as usize);          // 1 supplementary char => 2 UTF-16 units
        expect_eq(utf16Len("Aé中🚀"), 5 as usize);      // 1 + 1 + 1 + 2 = 5 UTF-16 units
    });
}
