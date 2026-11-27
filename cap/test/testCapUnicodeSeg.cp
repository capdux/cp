// cap/test/testCapUnicodeSeg.cp
// Comprehensive unit tests for @cap/unicode-seg

import { test, expect_eq } from "cap:test";
import {
    DecodedRune,
    TextSpan,
    decodeUtf8At,
    nextGrapheme,
    graphemeCount,
    truncateGraphemes,
    nextWord,
    wordCount,
} from "../../packages/unicode-segmentation/src/index.cp";

fn main() {
    test("1. @cap/unicode-seg — ASCII & CRLF grapheme boundaries", () => {
        expect_eq(graphemeCount(""), 0 as usize);
        expect_eq(graphemeCount("abc"), 3 as usize);
        // "a\r\nb" => ["a", "\r\n", "b"] = 3 graphemes
        expect_eq(graphemeCount("a\r\nb"), 3 as usize);

        const g1 = nextGrapheme("a\r\nb", 1).?;
        expect_eq(g1.slice, "\r\n");
        expect_eq(g1.end, 3 as usize);
    });

    test("2. @cap/unicode-seg — Combining diacritical marks (e + U+0301)", () => {
        // "cafe\u0301" = "cafe" + combining acute accent (0xCC 0x81) => 4 graphemes, 6 bytes
        const cafe = "cafe\xcc\x81";
        expect_eq(cafe.len, 6 as usize);
        expect_eq(graphemeCount(cafe), 4 as usize);

        const g3 = nextGrapheme(cafe, 3).?;
        expect_eq(g3.slice, "e\xcc\x81");
        expect_eq(truncateGraphemes(cafe, 4), cafe);
        expect_eq(truncateGraphemes(cafe, 3), "caf");
    });

    test("3. @cap/unicode-seg — Emoji ZWJ family & Fitzpatrick skin-tone modifiers", () => {
        // 👨‍👩‍👧 = U+1F468 + ZWJ + U+1F469 + ZWJ + U+1F467 (18 bytes, 1 grapheme)
        const family = "👨‍👩‍👧";
        expect_eq(graphemeCount(family), 1 as usize);

        // 👍🏽 = U+1F44D + U+1F3FD (8 bytes, 1 grapheme)
        const thumbs = "👍🏽";
        expect_eq(graphemeCount(thumbs), 1 as usize);

        const combo = "A👨‍👩‍👧👍🏽B";
        expect_eq(graphemeCount(combo), 4 as usize);
    });

    test("4. @cap/unicode-seg — Regional Indicator flag pairs (🇨🇳🇺🇸)", () => {
        const flags = "🇨🇳🇺🇸";
        expect_eq(graphemeCount(flags), 2 as usize);

        const f1 = nextGrapheme(flags, 0).?;
        expect_eq(f1.slice, "🇨🇳");
        const f2 = nextGrapheme(flags, f1.end).?;
        expect_eq(f2.slice, "🇺🇸");
        expect_eq(nextGrapheme(flags, f2.end) == null, true);
    });

    test("5. @cap/unicode-seg — Safe truncateGraphemes never splits Emoji or diacritics", () => {
        const s = "Hi👨‍👩‍👧🇨🇳!";
        expect_eq(truncateGraphemes(s, 0), "");
        expect_eq(truncateGraphemes(s, 2), "Hi");
        expect_eq(truncateGraphemes(s, 3), "Hi👨‍👩‍👧");
        expect_eq(truncateGraphemes(s, 4), "Hi👨‍👩‍👧🇨🇳");
        expect_eq(truncateGraphemes(s, 99), s);
    });

    test("6. @cap/unicode-seg — Mixed EN/CJK word segmentation & malformed UTF-8 resilience", () => {
        const text = "Hello, 世界! CP's v3.14 works.";
        // Words: "Hello", "世", "界", "CP's", "v3.14", "works" => 6
        expect_eq(wordCount(text), 6 as usize);

        const w0 = nextWord(text, 0).?;
        expect_eq(w0.slice, "Hello");
        const w1 = nextWord(text, w0.end).?;
        expect_eq(w1.slice, "世");
        const w2 = nextWord(text, w1.end).?;
        expect_eq(w2.slice, "界");
        const w3 = nextWord(text, w2.end).?;
        expect_eq(w3.slice, "CP's");
        const w4 = nextWord(text, w3.end).?;
        expect_eq(w4.slice, "v3.14");

        // Malformed UTF-8 byte safely decodes to U+FFFD with byte_len=1
        let bad: [2]u8 = [0xFF, 'A'];
        const r = decodeUtf8At(bad[0..2], 0);
        expect_eq(r.cp, 0xFFFD as u32);
        expect_eq(r.byte_len, 1 as usize);
    });
}
