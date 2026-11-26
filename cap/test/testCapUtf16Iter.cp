import { test, expect_eq } from "cap:test";
import {
    UTF16_REPLACEMENT_CHAR,
    Utf16Chars,
    Utf16Latin1,
    countUtf16Chars,
    utf16ToUtf8,
} from "../../packages/@cap/utf16-iter/src/index.cp";

fn main() {
    test("1. @cap/utf16-iter — forward iteration over ASCII, BMP & surrogate pairs", () => {
        // 'A' (0x0041), '中' (0x4E2D), '😀' (U+1F600 = 0xD83D, 0xDE00), '!' (0x0021)
        const data: [5]u16 = [0x0041, 0x4E2D, 0xD83D, 0xDE00, 0x0021];
        let it: Utf16Chars = Utf16Chars.new(data[0..5]);

        expect_eq(it.isEmpty(), false);
        expect_eq(it.asSlice().len, 5 as usize);

        expect_eq(it.next().?, 0x0041 as u32);
        expect_eq(it.next().?, 0x4E2D as u32);
        expect_eq(it.next().?, 0x1F600 as u32);
        expect_eq(it.asSlice().len, 1 as usize);
        expect_eq(it.next().?, 0x0021 as u32);
        expect_eq(it.isEmpty(), true);
        expect_eq(it.next() == null, true);
    });

    test("2. @cap/utf16-iter — reverse iteration (nextBack) & double-ended meeting", () => {
        // 'a', '𝄞' (U+1D11E = 0xD834, 0xDD1E), 'b', '😀' (0xD83D, 0xDE00)
        const data: [6]u16 = [0x0061, 0xD834, 0xDD1E, 0x0062, 0xD83D, 0xDE00];
        let it: Utf16Chars = Utf16Chars.new(data[0..6]);

        expect_eq(it.nextBack().?, 0x1F600 as u32);
        expect_eq(it.next().?, 0x0061 as u32);
        expect_eq(it.asSlice().len, 3 as usize);
        expect_eq(it.nextBack().?, 0x0062 as u32);
        expect_eq(it.nextBack().?, 0x1D11E as u32);
        expect_eq(it.isEmpty(), true);
        expect_eq(it.nextBack() == null, true);
        expect_eq(it.next() == null, true);
    });

    test("3. @cap/utf16-iter — unpaired high/low surrogates yield U+FFFD both ways", () => {
        // lone high (0xD800), 'X', lone low (0xDC00), reversed pair (0xDC00, 0xD800), trailing high (0xDBFF)
        const data: [6]u16 = [0xD800, 0x0058, 0xDC00, 0xDC00, 0xD800, 0xDBFF];

        let fwd: Utf16Chars = Utf16Chars.new(data[0..6]);
        expect_eq(fwd.next().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(fwd.next().?, 0x0058 as u32);
        expect_eq(fwd.next().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(fwd.next().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(fwd.next().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(fwd.next().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(fwd.next() == null, true);

        let rev: Utf16Chars = Utf16Chars.new(data[0..6]);
        expect_eq(rev.nextBack().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(rev.nextBack().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(rev.nextBack().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(rev.nextBack().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(rev.nextBack().?, 0x0058 as u32);
        expect_eq(rev.nextBack().?, UTF16_REPLACEMENT_CHAR);
        expect_eq(rev.nextBack() == null, true);
    });

    test("4. @cap/utf16-iter — Utf16Latin1 expansion & bidirectional iteration", () => {
        // "Aéÿ" in Latin-1: 0x41, 0xE9, 0xFF
        const latin1: [3]u8 = [0x41, 0xE9, 0xFF];
        let out: [4]u16 = [0; 4];

        const n: usize = Utf16Latin1.encode(latin1[0..3], out[0..4]);
        expect_eq(n, 3 as usize);
        expect_eq(out[0], 0x0041 as u16);
        expect_eq(out[1], 0x00E9 as u16);
        expect_eq(out[2], 0x00FF as u16);

        let it: Utf16Latin1 = Utf16Latin1.new(latin1[0..3]);
        expect_eq(it.isEmpty(), false);
        expect_eq(it.next().?, 0x0041 as u16);
        expect_eq(it.asSlice().len, 2 as usize);

        let rem_out: [2]u16 = [0; 2];
        expect_eq(it.writeTo(rem_out[0..2]), 2 as usize);
        expect_eq(rem_out[0], 0x00E9 as u16);
        expect_eq(rem_out[1], 0x00FF as u16);

        expect_eq(it.nextBack().?, 0x00FF as u16);
        expect_eq(it.next().?, 0x00E9 as u16);
        expect_eq(it.isEmpty(), true);
    });

    test("5. @cap/utf16-iter — countUtf16Chars on empty, BMP & supplementary strings", () => {
        const empty: [1]u16 = [0];
        expect_eq(countUtf16Chars(empty[0..0]), 0 as usize);

        // 'A' + '😀' (2 units) + lone high (1 unit -> U+FFFD) + '中'
        const mixed: [5]u16 = [0x0041, 0xD83D, 0xDE00, 0xD800, 0x4E2D];
        expect_eq(countUtf16Chars(mixed[0..5]), 4 as usize);
    });

    test("6. @cap/utf16-iter — utf16ToUtf8 encodes 1/2/3/4-byte sequences & respects buffer limit", () => {
        // 'A' (1B), 'é' U+00E9 (2B: C3 A9), '中' U+4E2D (3B: E4 B8 AD), '😀' U+1F600 (4B: F0 9F 98 80)
        const src: [5]u16 = [0x0041, 0x00E9, 0x4E2D, 0xD83D, 0xDE00];
        let buf: [16]u8 = [0; 16];

        const len: usize = utf16ToUtf8(src[0..5], buf[0..16]);
        expect_eq(len, 10 as usize);
        expect_eq(buf[0], 0x41 as u8);
        expect_eq(buf[1], 0xC3 as u8);
        expect_eq(buf[2], 0xA9 as u8);
        expect_eq(buf[3], 0xE4 as u8);
        expect_eq(buf[4], 0xB8 as u8);
        expect_eq(buf[5], 0xAD as u8);
        expect_eq(buf[6], 0xF0 as u8);
        expect_eq(buf[7], 0x9F as u8);
        expect_eq(buf[8], 0x98 as u8);
        expect_eq(buf[9], 0x80 as u8);

        // Truncated buffer (capacity 6 fits 'A' + 'é' + '中' = 6 bytes, stops cleanly before 4-byte emoji)
        let small: [6]u8 = [0; 6];
        expect_eq(utf16ToUtf8(src[0..5], small[0..6]), 6 as usize);
    });
}
