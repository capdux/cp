import { test, expect_eq } from "cap:test";
import {
    DetectedCharset,
    detect,
    detectName,
} from "../../packages/@cap/chardet/src/index.cp";

fn main() {
    test("1. @cap/chardet — detects UTF-8, UTF-16LE/BE, and UTF-32 BOMs", () => {
        const utf8_bom: [6]u8 = [0xEF, 0xBB, 0xBF, 0x48, 0x69, 0x21];
        const m1 = detect(utf8_bom[0..6]);
        expect_eq(m1.charset == .Utf8Bom, true);
        expect_eq(m1.name, "UTF-8");
        expect_eq(m1.confidence, 100 as u8);

        const utf16le_bom: [6]u8 = [0xFF, 0xFE, 0x48, 0x00, 0x69, 0x00];
        expect_eq(detect(utf16le_bom[0..6]).charset == .Utf16Le, true);

        const utf16be_bom: [6]u8 = [0xFE, 0xFF, 0x00, 0x48, 0x00, 0x69];
        expect_eq(detect(utf16be_bom[0..6]).charset == .Utf16Be, true);

        const utf32be_bom: [8]u8 = [0x00, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x41];
        expect_eq(detect(utf32be_bom[0..8]).charset == .Utf32Be, true);
    });

    test("2. @cap/chardet — detects pure ASCII and multi-byte UTF-8", () => {
        const ascii: [5]u8 = [0x68, 0x65, 0x6C, 0x6C, 0x6F];
        expect_eq(detect(ascii[0..5]).charset == .Ascii, true);
        expect_eq(detectName(ascii[0..5]), "ASCII");

        // "你好" in UTF-8: E4 BD A0 E5 A5 BD
        const utf8_cn: [6]u8 = [0xE4, 0xBD, 0xA0, 0xE5, 0xA5, 0xBD];
        const m = detect(utf8_cn[0..6]);
        expect_eq(m.charset == .Utf8, true);
        expect_eq(m.name, "UTF-8");
    });

    test("3. @cap/chardet — detects BOM-less UTF-16LE and UTF-16BE via null cadence", () => {
        const le: [8]u8 = [0x48, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6C, 0x00];
        expect_eq(detect(le[0..8]).charset == .Utf16Le, true);

        const be: [8]u8 = [0x00, 0x48, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6C];
        expect_eq(detect(be[0..8]).charset == .Utf16Be, true);
    });

    test("4. @cap/chardet — detects Shift-JIS half-width Katakana and double-byte pairs", () => {
        // Shift-JIS "テスト" (0x83 0x65 0x83 0x58 0x83 0x67)
        const sjis: [6]u8 = [0x83, 0x65, 0x83, 0x58, 0x83, 0x67];
        expect_eq(detect(sjis[0..6]).charset == .ShiftJis, true);
        expect_eq(detectName(sjis[0..6]), "Shift_JIS");
    });

    test("5. @cap/chardet — detects GBK / GB18030 double-byte sequences", () => {
        // GBK "中文" (0xD6 0xD0 0xCE 0xC4) — invalid Shift-JIS lead 0xD6, valid GBK
        const gbk: [4]u8 = [0xD6, 0xD0, 0xCE, 0xC4];
        expect_eq(detect(gbk[0..4]).charset == .Gbk, true);
        expect_eq(detectName(gbk[0..4]), "GB18030");
    });

    test("6. @cap/chardet — distinguishes Windows-1252 and ISO-8859-1", () => {
        // Windows-1252 smart quote 0x93 + "hi" + 0x94 (invalid UTF-8/SJIS/GBK because single high bytes)
        const w1252: [5]u8 = [0x93, 0x68, 0x69, 0x94, 0x21];
        expect_eq(detect(w1252[0..5]).charset == .Windows1252, true);

        // ISO-8859-1 "caf\xE9" (0xE9 followed by ASCII '!')
        const latin1: [5]u8 = [0x63, 0x61, 0x66, 0xE9, 0x21];
        expect_eq(detect(latin1[0..5]).charset == .Iso8859_1, true);
    });
}
