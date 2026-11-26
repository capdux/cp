// cap/test/testCapIconvLite.cp
// Comprehensive unit tests for @cap/iconv-lite

import { test, expect_eq } from "cap:test";
import {
    Charset,
    parseCharset,
    encodingExists,
    decodeToUtf8,
    encodeFromUtf8,
} from "../../packages/@cap/iconv-lite/src/index.cp";

fn main() {
    test("1. @cap/iconv-lite — parseCharset normalization & encodingExists", () => {
        expect_eq(parseCharset("utf-8") == .Utf8, true);
        expect_eq(parseCharset("UTF_8") == .Utf8, true);
        expect_eq(parseCharset("ISO-8859-1") == .Latin1, true);
        expect_eq(parseCharset("binary") == .Latin1, true);
        expect_eq(parseCharset("windows-1252") == .Win1252, true);
        expect_eq(parseCharset("cp1252") == .Win1252, true);
        expect_eq(parseCharset("ucs2") == .Utf16Le, true);
        expect_eq(parseCharset("UTF-16BE") == .Utf16Be, true);
        expect_eq(parseCharset("ASCII") == .Ascii, true);
        expect_eq(parseCharset("shift-jis") == .Unknown, true);

        expect_eq(encodingExists("win1252"), true);
        expect_eq(encodingExists("nonexistent-codec"), false);
    });

    test("2. @cap/iconv-lite — Windows-1252 0x80..0x9F special mappings (€, “, ”, ™)", () => {
        const raw: [4]u8 = [u8(0x80), u8(0x93), u8(0x94), u8(0x99)];
        let out: [32]u8 = undefined;
        const n = decodeToUtf8(raw[0..4], .Win1252, true, out[0..32]);
        // € (3B) + “ (3B) + ” (3B) + ™ (3B) = 12 bytes
        expect_eq(n, 12 as usize);
        expect_eq(std.mem.eql(u8, out[0..n], "\u{20AC}\u{201C}\u{201D}\u{2122}"), true);

        // Round-trip encode back to Windows-1252
        let enc: [16]u8 = undefined;
        const en = encodeFromUtf8(out[0..n], .Win1252, false, enc[0..16]);
        expect_eq(en, 4 as usize);
        expect_eq(enc[0], u8(0x80));
        expect_eq(enc[1], u8(0x93));
        expect_eq(enc[2], u8(0x94));
        expect_eq(enc[3], u8(0x99));
    });

    test("3. @cap/iconv-lite — UTF-16LE/BE BOM stripping & surrogate pair decoding (😀 U+1F600)", () => {
        // UTF-16LE: BOM (FF FE) + 'A' (41 00) + U+1F600 (3D D8 00 DE)
        const le_data: [8]u8 = [u8(0xFF), u8(0xFE), u8(0x41), u8(0x00), u8(0x3D), u8(0xD8), u8(0x00), u8(0xDE)];
        let out: [32]u8 = undefined;
        const n = decodeToUtf8(le_data[0..8], .Utf16Le, true, out[0..32]);
        expect_eq(n, 5 as usize);
        expect_eq(std.mem.eql(u8, out[0..n], "A\u{1F600}"), true);

        // UTF-16BE: BOM (FE FF) + 'A' (00 41) + U+1F600 (D8 3D DE 00)
        const be_data: [8]u8 = [u8(0xFE), u8(0xFF), u8(0x00), u8(0x41), u8(0xD8), u8(0x3D), u8(0xDE), u8(0x00)];
        const n2 = decodeToUtf8(be_data[0..8], .Utf16Be, true, out[0..32]);
        expect_eq(n2, 5 as usize);
        expect_eq(std.mem.eql(u8, out[0..n2], "A\u{1F600}"), true);
    });

    test("4. @cap/iconv-lite — UTF-8 to UTF-16LE/BE encoding with BOM & surrogate pairs", () => {
        let out: [32]u8 = undefined;
        const n = encodeFromUtf8("A\u{1F600}", .Utf16Le, true, out[0..32]);
        expect_eq(n, 8 as usize);
        expect_eq(out[0], u8(0xFF));
        expect_eq(out[1], u8(0xFE));
        expect_eq(out[2], u8(0x41));
        expect_eq(out[3], u8(0x00));
        expect_eq(out[4], u8(0x3D));
        expect_eq(out[5], u8(0xD8));
        expect_eq(out[6], u8(0x00));
        expect_eq(out[7], u8(0xDE));

        const n_be = encodeFromUtf8("Hi", .Utf16Be, true, out[0..32]);
        expect_eq(n_be, 6 as usize);
        expect_eq(out[0], u8(0xFE));
        expect_eq(out[1], u8(0xFF));
        expect_eq(out[2], u8(0x00));
        expect_eq(out[3], u8('H'));
        expect_eq(out[4], u8(0x00));
        expect_eq(out[5], u8('i'));
    });

    test("5. @cap/iconv-lite — Latin1 & ASCII decode/encode with '?' fallback", () => {
        // café in Latin1: 63 61 66 E9
        const latin: [4]u8 = [u8('c'), u8('a'), u8('f'), u8(0xE9)];
        let u8buf: [16]u8 = undefined;
        const n = decodeToUtf8(latin[0..4], .Latin1, false, u8buf[0..16]);
        expect_eq(std.mem.eql(u8, u8buf[0..n], "caf\u{00E9}"), true);

        // Encoding € (U+20AC) into Latin1 and ASCII falls back to '?'
        let enc: [16]u8 = undefined;
        const ln = encodeFromUtf8("a\u{20AC}\u{00E9}", .Latin1, false, enc[0..16]);
        expect_eq(ln, 3 as usize);
        expect_eq(enc[0], u8('a'));
        expect_eq(enc[1], u8('?'));
        expect_eq(enc[2], u8(0xE9));

        const an = encodeFromUtf8("a\u{00E9}b", .Ascii, false, enc[0..16]);
        expect_eq(an, 3 as usize);
        expect_eq(std.mem.eql(u8, enc[0..an], "a?b"), true);
    });

    test("6. @cap/iconv-lite — UTF-8 BOM stripping and injection", () => {
        const bom_utf8: [6]u8 = [u8(0xEF), u8(0xBB), u8(0xBF), u8('O'), u8('K'), u8('!')];
        let out: [16]u8 = undefined;
        const n_strip = decodeToUtf8(bom_utf8[0..6], .Utf8, true, out[0..16]);
        expect_eq(n_strip, 3 as usize);
        expect_eq(std.mem.eql(u8, out[0..n_strip], "OK!"), true);

        const n_keep = decodeToUtf8(bom_utf8[0..6], .Utf8, false, out[0..16]);
        expect_eq(n_keep, 6 as usize);

        const n_enc = encodeFromUtf8("OK!", .Utf8, true, out[0..16]);
        expect_eq(n_enc, 6 as usize);
        expect_eq(out[0], u8(0xEF));
        expect_eq(out[1], u8(0xBB));
        expect_eq(out[2], u8(0xBF));
    });
}
