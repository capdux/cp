// cap/test/testCapUnicodeWidth.cp
// Comprehensive unit tests for @cap/unicode-width

import { test, expect_eq } from "cap:test";
import {
    charWidth,
    charWidthCjk,
    strWidth,
    strWidthCjk,
    truncateToWidth,
} from "../../packages/@cap/unicode-width/src/index.cp";

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
    test("1. @cap/unicode-width — Printable ASCII string & char widths", () => {
        expect_eq(charWidth('A' as u32), 1 as usize);
        expect_eq(charWidth(' ' as u32), 1 as usize);
        expect_eq(strWidth("Hello, CP!"), 10 as usize);
    });

    test("2. @cap/unicode-width — CJK Chinese, Japanese & Korean wide characters (width = 2)", () => {
        expect_eq(strWidth("你好"), 4 as usize);
        expect_eq(strWidth("こんにちは"), 10 as usize);
        expect_eq(strWidth("안녕"), 4 as usize);
        expect_eq(strWidth("你好CP"), 6 as usize);
    });

    test("3. @cap/unicode-width — Fullwidth punctuation, Emoji & CJK Ambiguous width", () => {
        // Fullwidth 'Ａ' (U+FF21) and '！' (U+FF01) -> width 2 each
        expect_eq(charWidth(0xFF21 as u32), 2 as usize);
        expect_eq(charWidth(0xFF01 as u32), 2 as usize);
        // Emoji 🚀 (U+1F680) -> width 2
        expect_eq(charWidth(0x1F680 as u32), 2 as usize);
        expect_eq(strWidth("🚀CP"), 4 as usize);

        // East Asian Ambiguous box-drawing '─' (U+2500) & 'α' (U+03B1): 1 in standard, 2 in CJK
        expect_eq(charWidth(0x2500 as u32), 1 as usize);
        expect_eq(charWidthCjk(0x2500 as u32), 2 as usize);
        expect_eq(strWidthCjk("αβ"), 4 as usize);
    });

    test("4. @cap/unicode-width — Combining marks, ZWJ, BOM & control chars (width = 0)", () => {
        expect_eq(charWidth(0x00 as u32), 0 as usize);
        expect_eq(charWidth('\n' as u32), 0 as usize);
        expect_eq(charWidth(0x7F as u32), 0 as usize);
        expect_eq(charWidth(0x0301 as u32), 0 as usize); // Combining acute accent
        expect_eq(charWidth(0x200D as u32), 0 as usize); // ZWJ
        expect_eq(charWidth(0xFE0F as u32), 0 as usize); // Variation Selector-16
        expect_eq(charWidth(0xFEFF as u32), 0 as usize); // BOM

        // 'e' + combining acute accent (U+0301 = 0xCC 0x81) => display width 1
        let eAcute: [3]u8 = undefined;
        eAcute[0] = 'e' as u8;
        eAcute[1] = 0xCC as u8;
        eAcute[2] = 0x81 as u8;
        expect_eq(strWidth(eAcute[0..3]), 1 as usize);
    });

    test("5. @cap/unicode-width — truncateToWidth UTF-8 safe column truncation", () => {
        const mixed = "你好CP世界"; // widths: 2 + 2 + 1 + 1 + 2 + 2 = 10
        // max_cols = 3: "你" (2 cols, 3 bytes) fits; "好" would make 4 > 3 -> cut at 3 bytes!
        const c3 = truncateToWidth(mixed, 3);
        expect_eq(sliceEq(mixed[0..c3], "你"), true);

        // max_cols = 5: "你好C" (2 + 2 + 1 = 5 cols, 7 bytes)
        const c5 = truncateToWidth(mixed, 5);
        expect_eq(sliceEq(mixed[0..c5], "你好C"), true);

        // max_cols = 20: entire string fits
        expect_eq(truncateToWidth(mixed, 20), mixed.len);
        // max_cols = 0: empty prefix
        expect_eq(truncateToWidth(mixed, 0), 0 as usize);
    });

    test("6. @cap/unicode-width — Empty string & malformed UTF-8 byte resilience", () => {
        expect_eq(strWidth(""), 0 as usize);
        expect_eq(strWidthCjk(""), 0 as usize);
        expect_eq(truncateToWidth("", 10), 0 as usize);

        // Stray invalid UTF-8 bytes must not hang or crash.
        let bad: [3]u8 = undefined;
        bad[0] = 0xFF as u8;
        bad[1] = 'a' as u8;
        bad[2] = 0x80 as u8;
        expect_eq(strWidth(bad[0..3]), 3 as usize);
    });
}
