// cap/test/testCapUtf16Codec.cp
// Comprehensive unit tests for @cap/utf16-codec (UTF-8 <-> UTF-16 / UTF-16LE)

import { test, expect_eq } from "cap:test";
import {
    BOM_NONE,
    BOM_UTF16_LE,
    BOM_UTF16_BE,
    BOM_UTF8,
    detectBom,
    utf8ToUtf16,
    utf16ToUtf8,
    utf8ToUtf16LeBytes,
    utf16LeBytesToUtf8,
    countUtf16Units,
} from "../../packages/utf16/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/utf16-codec — ASCII & CJK Chinese UTF-8 <-> UTF-16 round-trip", () => {
        const src = "Hi中文!";
        let u16_buf: [16]u16 = undefined;
        const n: i64 = utf8ToUtf16(src, u16_buf[0..16]);
        expect_eq(n, 5 as i64);
        expect_eq(u16_buf[0], 'H' as u16);
        expect_eq(u16_buf[2], 0x4E2D as u16); // 中
        expect_eq(u16_buf[3], 0x6587 as u16); // 文

        let back: [32]u8 = undefined;
        const bn: i64 = utf16ToUtf8(u16_buf[0..5], back[0..32]);
        expect_eq(bn, src.len as i64);
        expect_eq(sliceEq(back[0..(bn as usize)], src), true);
    });

    test("2. @cap/utf16-codec — 4-byte Emoji U+1F680 surrogate pair 0xD83D 0xDE80", () => {
        // UTF-8 for U+1F680 (🚀): F0 9F 9A 80
        const rocket: [4]u8 = [0xF0, 0x9F, 0x9A, 0x80];
        let u16_buf: [4]u16 = undefined;
        const n: i64 = utf8ToUtf16(rocket[0..4], u16_buf[0..4]);
        expect_eq(n, 2 as i64);
        expect_eq(u16_buf[0], 0xD83D as u16);
        expect_eq(u16_buf[1], 0xDE80 as u16);

        let back: [8]u8 = undefined;
        const bn: i64 = utf16ToUtf8(u16_buf[0..2], back[0..8]);
        expect_eq(bn, 4 as i64);
        expect_eq(sliceEq(back[0..4], rocket[0..4]), true);
    });

    test("3. @cap/utf16-codec — UTF-16LE byte stream round-trip", () => {
        const src = "A中";
        let le: [16]u8 = undefined;
        const n: i64 = utf8ToUtf16LeBytes(src, le[0..16]);
        // 'A' = 0x0041 -> [0x41, 0x00]; '中' = 0x4E2D -> [0x2D, 0x4E]
        expect_eq(n, 4 as i64);
        expect_eq(le[0], 0x41 as u8);
        expect_eq(le[1], 0x00 as u8);
        expect_eq(le[2], 0x2D as u8);
        expect_eq(le[3], 0x4E as u8);

        let back: [16]u8 = undefined;
        const bn: i64 = utf16LeBytesToUtf8(le[0..4], back[0..16]);
        expect_eq(sliceEq(back[0..(bn as usize)], src), true);
    });

    test("4. @cap/utf16-codec — detectBom for UTF-8, UTF-16LE, UTF-16BE & none", () => {
        const b_utf8: [4]u8 = [0xEF, 0xBB, 0xBF, 'a' as u8];
        const b_le: [4]u8 = [0xFF, 0xFE, 0x41, 0x00];
        const b_be: [4]u8 = [0xFE, 0xFF, 0x00, 0x41];
        const b_none: [2]u8 = ['O' as u8, 'K' as u8];

        expect_eq(detectBom(b_utf8[0..4]), BOM_UTF8);
        expect_eq(detectBom(b_le[0..4]), BOM_UTF16_LE);
        expect_eq(detectBom(b_be[0..4]), BOM_UTF16_BE);
        expect_eq(detectBom(b_none[0..2]), BOM_NONE);
    });

    test("5. @cap/utf16-codec — countUtf16Units (ASCII=1, CJK=1, Emoji=2)", () => {
        expect_eq(countUtf16Units("A"), 1 as i64);
        expect_eq(countUtf16Units("中"), 1 as i64);
        const rocket: [4]u8 = [0xF0, 0x9F, 0x9A, 0x80];
        expect_eq(countUtf16Units(rocket[0..4]), 2 as i64);
        expect_eq(countUtf16Units("Hi中文"), 4 as i64);
    });

    test("6. @cap/utf16-codec — rejects lone surrogates, odd byte len & tiny buffers with -1", () => {
        let out: [16]u8 = undefined;
        // Lone high surrogate
        const lone_hi: [1]u16 = [0xD83D];
        expect_eq(utf16ToUtf8(lone_hi[0..1], out[0..16]), -1 as i64);
        // Lone low surrogate
        const lone_lo: [1]u16 = [0xDE80];
        expect_eq(utf16ToUtf8(lone_lo[0..1], out[0..16]), -1 as i64);
        // Odd byte length in LE stream
        const odd_le: [3]u8 = [0x41, 0x00, 0x42];
        expect_eq(utf16LeBytesToUtf8(odd_le[0..3], out[0..16]), -1 as i64);
        // Tiny buffers
        let tiny16: [1]u16 = undefined;
        expect_eq(utf8ToUtf16("AB", tiny16[0..1]), -1 as i64);
        let tiny8: [1]u8 = undefined;
        expect_eq(utf8ToUtf16LeBytes("A", tiny8[0..1]), -1 as i64);
    });
}
