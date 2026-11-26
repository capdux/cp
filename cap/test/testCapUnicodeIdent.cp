// cap/test/testCapUnicodeIdent.cp
// Comprehensive unit tests for @cap/unicode-ident

import { test, expect_eq } from "cap:test";
import {
    Utf8Codepoint,
    decodeUtf8At,
    isXidStart,
    isXidContinue,
    isIdentStart,
    isValidIdent,
    scanIdentLen,
} from "../../packages/@cap/unicode-ident/src/index.cp";

fn main() {
    test("1. @cap/unicode-ident — ASCII fast path (XID_Start, XID_Continue, IdentStart)", () => {
        expect_eq(isXidStart(0x41 as u32), true);  // 'A'
        expect_eq(isXidStart(0x5A as u32), true);  // 'Z'
        expect_eq(isXidStart(0x61 as u32), true);  // 'a'
        expect_eq(isXidStart(0x7A as u32), true);  // 'z'
        expect_eq(isXidStart(0x5F as u32), false); // '_' is NOT XID_Start
        expect_eq(isXidStart(0x30 as u32), false); // '0'
        expect_eq(isXidStart(0x20 as u32), false); // ' '

        expect_eq(isIdentStart(0x5F as u32), true);  // '_' IS IdentStart
        expect_eq(isIdentStart(0x61 as u32), true);  // 'a'
        expect_eq(isIdentStart(0x30 as u32), false); // '0'

        expect_eq(isXidContinue(0x30 as u32), true);  // '0'
        expect_eq(isXidContinue(0x39 as u32), true);  // '9'
        expect_eq(isXidContinue(0x5F as u32), true);  // '_'
        expect_eq(isXidContinue(0x2D as u32), false); // '-'
        expect_eq(isXidContinue(0x24 as u32), false); // '$'
    });

    test("2. @cap/unicode-ident — Latin-1, Greek & Cyrillic Unicode ranges", () => {
        // Latin-1 Supplement: É (0xC9), ü (0xFC), excluding × (0xD7) and ÷ (0xF7)
        expect_eq(isXidStart(0x00C9 as u32), true);
        expect_eq(isXidStart(0x00FC as u32), true);
        expect_eq(isXidStart(0x00D7 as u32), false);
        expect_eq(isXidStart(0x00F7 as u32), false);

        // Greek: α (0x03B1), Ω (0x03A9)
        expect_eq(isXidStart(0x03B1 as u32), true);
        expect_eq(isXidStart(0x03A9 as u32), true);

        // Cyrillic: Д (0x0414), я (0x044F)
        expect_eq(isXidStart(0x0414 as u32), true);
        expect_eq(isXidStart(0x044F as u32), true);

        // Combining acute accent (0x0301): XID_Continue only, not XID_Start
        expect_eq(isXidStart(0x0301 as u32), false);
        expect_eq(isXidContinue(0x0301 as u32), true);
    });

    test("3. @cap/unicode-ident — CJK, Hiragana, Katakana, Hangul & non-ident symbols", () => {
        expect_eq(isXidStart(0x4E2D as u32), true);  // 中 (CJK)
        expect_eq(isXidStart(0x3042 as u32), true);  // あ (Hiragana)
        expect_eq(isXidStart(0x30A2 as u32), true);  // ア (Katakana)
        expect_eq(isXidStart(0xAC00 as u32), true);  // 가 (Hangul)
        expect_eq(isXidStart(0x20000 as u32), true); // CJK Ext-B

        // Emoji, surrogates, out-of-range must be rejected
        expect_eq(isXidStart(0x1F600 as u32), false);
        expect_eq(isXidContinue(0x1F600 as u32), false);
        expect_eq(isXidStart(0xD800 as u32), false);
        expect_eq(isXidStart(0x110000 as u32), false);

        // Fullwidth digit '０' (0xFF10): XID_Continue only
        expect_eq(isXidStart(0xFF10 as u32), false);
        expect_eq(isXidContinue(0xFF10 as u32), true);
    });

    test("4. @cap/unicode-ident — isValidIdent on ASCII & multi-script UTF-8 strings", () => {
        expect_eq(isValidIdent("foo"), true);
        expect_eq(isValidIdent("_bar_123"), true);
        expect_eq(isValidIdent("_"), true);
        expect_eq(isValidIdent("变量名_1"), true);
        expect_eq(isValidIdent("αβγ_Δ"), true);
        expect_eq(isValidIdent("привет"), true);
        expect_eq(isValidIdent("こんにちは_カタカナ_한글"), true);
        expect_eq(isValidIdent("café"), true);

        expect_eq(isValidIdent(""), false);
        expect_eq(isValidIdent("123abc"), false);
        expect_eq(isValidIdent("a-b"), false);
        expect_eq(isValidIdent("hello world"), false);
        expect_eq(isValidIdent("foo!"), false);
    });

    test("5. @cap/unicode-ident — scanIdentLen prefix scanning & boundaries", () => {
        expect_eq(scanIdentLen(""), 0 as usize);
        expect_eq(scanIdentLen("123foo"), 0 as usize);
        expect_eq(scanIdentLen("foo_12 + bar"), 6 as usize);
        expect_eq(scanIdentLen("_x(1)"), 2 as usize);
        // "αβ_1" = 2 + 2 + 1 + 1 = 6 bytes
        expect_eq(scanIdentLen("αβ_1=9"), 6 as usize);
        // "中文a" = 3 + 3 + 1 = 7 bytes
        expect_eq(scanIdentLen("中文a;rest"), 7 as usize);
        expect_eq(scanIdentLen("  leadingSpace"), 0 as usize);
    });

    test("6. @cap/unicode-ident — Strict UTF-8 validation rejects malformed/overlong/surrogate bytes", () => {
        // Lone continuation byte 0x80
        let bad1: [2]u8 = [ 0x80 as u8, 0x61 as u8 ];
        expect_eq(isValidIdent(bad1[0..]), false);
        expect_eq(scanIdentLen(bad1[0..]), 0 as usize);

        // Overlong 2-byte encoding of 'A' (0xC1 0x81)
        let bad2: [2]u8 = [ 0xC1 as u8, 0x81 as u8 ];
        expect_eq(isValidIdent(bad2[0..]), false);
        const d2 = decodeUtf8At(bad2[0..], 0);
        expect_eq(d2.ok, false);

        // UTF-16 surrogate U+D800 encoded in UTF-8 (0xED 0xA0 0x80)
        let bad3: [3]u8 = [ 0xED as u8, 0xA0 as u8, 0x80 as u8 ];
        expect_eq(isValidIdent(bad3[0..]), false);
        const d3 = decodeUtf8At(bad3[0..], 0);
        expect_eq(d3.ok, false);

        // Truncated 3-byte sequence after valid ASCII prefix "ab"
        let bad4: [4]u8 = [ 0x61 as u8, 0x62 as u8, 0xE4 as u8, 0xB8 as u8 ];
        expect_eq(isValidIdent(bad4[0..]), false);
        expect_eq(scanIdentLen(bad4[0..]), 2 as usize);
    });
}
