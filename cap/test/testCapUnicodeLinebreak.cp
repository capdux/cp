// cap/test/testCapUnicodeLinebreak.cp
// Comprehensive unit tests for @cap/unicode-linebreak (UAX #14)

import { test, expect_eq } from "cap:test";
import {
    BreakKind,
    LineBreak,
    findLineBreaks,
    countLineBreaks,
} from "../../packages/@cap/unicode-linebreak/src/index.cp";

fn main() {
    test("1. @cap/unicode-linebreak — Mandatory breaks on LF and CRLF without splitting CRLF", () => {
        let out: [16]LineBreak = undefined;
        const s = "a\r\nb\nc";
        const n = findLineBreaks(s, out[0..16]);
        expect_eq(n, 3 as usize);
        expect_eq(out[0].offset, 3 as usize);
        expect_eq(out[0].kind == BreakKind.Mandatory, true);
        expect_eq(out[1].offset, 5 as usize);
        expect_eq(out[1].kind == BreakKind.Mandatory, true);
        expect_eq(out[2].offset, 6 as usize);
        expect_eq(out[2].kind == BreakKind.Mandatory, true);
    });

    test("2. @cap/unicode-linebreak — Space and hyphen allowed break opportunities", () => {
        let out: [16]LineBreak = undefined;
        const s = "hello well-known";
        const n = findLineBreaks(s, out[0..16]);
        expect_eq(n, 3 as usize);
        // After space at offset 6
        expect_eq(out[0].offset, 6 as usize);
        expect_eq(out[0].kind == BreakKind.Allowed, true);
        // After hyphen at offset 11
        expect_eq(out[1].offset, 11 as usize);
        expect_eq(out[1].kind == BreakKind.Allowed, true);
        // End of text mandatory at offset 16
        expect_eq(out[2].offset, 16 as usize);
        expect_eq(out[2].kind == BreakKind.Mandatory, true);
    });

    test("3. @cap/unicode-linebreak — CJK ideographs allow breaks between chars, not before closing punct", () => {
        let out: [16]LineBreak = undefined;
        // "中文。测试" -> 5 chars * 3 bytes = 15 bytes
        const s = "中文。测试";
        const n = findLineBreaks(s, out[0..16]);
        expect_eq(n, 4 as usize);
        // After '中' (3)
        expect_eq(out[0].offset, 3 as usize);
        expect_eq(out[0].kind == BreakKind.Allowed, true);
        // No break at 6 (before '。'); break after '。' (9)
        expect_eq(out[1].offset, 9 as usize);
        expect_eq(out[1].kind == BreakKind.Allowed, true);
        // After '测' (12)
        expect_eq(out[2].offset, 12 as usize);
        expect_eq(out[2].kind == BreakKind.Allowed, true);
        // End of text (15) Mandatory
        expect_eq(out[3].offset, 15 as usize);
        expect_eq(out[3].kind == BreakKind.Mandatory, true);
    });

    test("4. @cap/unicode-linebreak — Prohibit breaks around U+00A0 non-breaking space", () => {
        let out: [16]LineBreak = undefined;
        // "100\u{00A0}km" in UTF-8: '1','0','0', 0xC2, 0xA0, 'k','m'
        const s = "100\u{00A0}km";
        const n = findLineBreaks(s, out[0..16]);
        expect_eq(n, 1 as usize);
        expect_eq(out[0].offset, s.len);
        expect_eq(out[0].kind == BreakKind.Mandatory, true);
    });

    test("5. @cap/unicode-linebreak — Prohibit breaks after opening '(' / '[' and before ')' / ',' / '.'", () => {
        let out: [16]LineBreak = undefined;
        const s = "(a), [b].";
        const n = findLineBreaks(s, out[0..16]);
        expect_eq(n, 2 as usize);
        // Only break is after space at offset 5, and end at offset 9
        expect_eq(out[0].offset, 5 as usize);
        expect_eq(out[0].kind == BreakKind.Allowed, true);
        expect_eq(out[1].offset, 9 as usize);
        expect_eq(out[1].kind == BreakKind.Mandatory, true);
    });

    test("6. @cap/unicode-linebreak — countLineBreaks, empty string, and small buffer clamping", () => {
        expect_eq(countLineBreaks(""), 0 as usize);
        expect_eq(countLineBreaks("hello world"), 2 as usize);
        expect_eq(countLineBreaks("中文测试"), 4 as usize);

        let small: [2]LineBreak = undefined;
        const written = findLineBreaks("中文测试", small[0..2]);
        expect_eq(written, 2 as usize);
        expect_eq(small[0].offset, 3 as usize);
        expect_eq(small[1].offset, 6 as usize);
    });
}
