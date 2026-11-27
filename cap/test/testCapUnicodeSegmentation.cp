// cap/test/testCapUnicodeSegmentation.cp
// Unit tests for @cap/unicode-segmentation (UAX #29 Extended Grapheme Clusters & Word Boundaries)

import { test, expect_eq } from "cap:test";
import {
    GraphemeSpan,
    GraphemeIter,
    countGraphemes,
    nthGrapheme,
    sliceGraphemes,
    WordIter,
    countUnicodeWords,
} from "../../packages/unicode-segmentation/src/index.cp";

fn sliceEq(a: string, b: string): bool {
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
    test("1. @cap/unicode-segmentation — ASCII + CRLF single grapheme cluster", () => {
        const s = "a\r\nb\nc";
        expect_eq(countGraphemes(s), 5 as usize);

        let it = GraphemeIter.new(s);
        const g0 = it.next().?;
        expect_eq(sliceEq(s[g0.start..g0.end], "a"), true);

        const g1 = it.next().?;
        expect_eq(sliceEq(s[g1.start..g1.end], "\r\n"), true);
        expect_eq(g1.byteLen(), 2 as usize);

        const g2 = it.next().?;
        expect_eq(sliceEq(s[g2.start..g2.end], "b"), true);

        const g3 = it.next().?;
        expect_eq(sliceEq(s[g3.start..g3.end], "\n"), true);

        const g4 = it.next().?;
        expect_eq(sliceEq(s[g4.start..g4.end], "c"), true);

        expect_eq(it.next() == null, true);

        it.reset();
        const again = it.next().?;
        expect_eq(again.start, 0 as usize);
        expect_eq(again.end, 1 as usize);
    });

    test("2. @cap/unicode-segmentation — combining diacritics (e + U+0301)", () => {
        // "cafe" + U+0301 (CC 81) + "!"
        const s = "cafe\xCC\x81!";
        expect_eq(s.len, 7 as usize);
        expect_eq(countGraphemes(s), 5 as usize);

        const g3 = nthGrapheme(s, 3).?;
        expect_eq(g3.start, 3 as usize);
        expect_eq(g3.end, 6 as usize);
        expect_eq(sliceEq(s[g3.start..g3.end], "e\xCC\x81"), true);

        expect_eq(nthGrapheme(s, 5) == null, true);
    });

    test("3. @cap/unicode-segmentation — Emoji skin tone & ZWJ family sequence", () => {
        // 👍🏽 (U+1F44D U+1F3FD) + 👨‍👩‍👧‍👦 (U+1F468 ZWJ U+1F469 ZWJ U+1F467 ZWJ U+1F466) + ❤️ (U+2764 U+FE0F)
        const s = "\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6\xE2\x9D\xA4\xEF\xB8\x8F";
        expect_eq(countGraphemes(s), 3 as usize);

        let it = GraphemeIter.new(s);
        const g0 = it.next().?;
        expect_eq(g0.byteLen(), 8 as usize);  // 👍🏽 = 4 + 4 bytes

        const g1 = it.next().?;
        expect_eq(g1.byteLen(), 25 as usize); // 👨‍👩‍👧‍👦 = 4*4 + 3*3 = 25 bytes

        const g2 = it.next().?;
        expect_eq(g2.byteLen(), 6 as usize);  // ❤️ = 3 + 3 bytes

        expect_eq(it.next() == null, true);
    });

    test("4. @cap/unicode-segmentation — Regional Indicator flag pairs", () => {
        // 🇺🇸 (1F1FA 1F1F8) + 🇨🇳 (1F1E8 1F1F3) + 🇯🇵 (1F1EF 1F1F5)
        const flags = "\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8\xF0\x9F\x87\xA8\xF0\x9F\x87\xB3\xF0\x9F\x87\xAF\xF0\x9F\x87\xB5";
        expect_eq(countGraphemes(flags), 3 as usize);

        let it = GraphemeIter.new(flags);
        const f0 = it.next().?;
        const f1 = it.next().?;
        const f2 = it.next().?;
        expect_eq(f0.byteLen(), 8 as usize);
        expect_eq(f1.byteLen(), 8 as usize);
        expect_eq(f2.byteLen(), 8 as usize);
        expect_eq(it.next() == null, true);
    });

    test("5. @cap/unicode-segmentation — Hangul Jamo L+V+T composition", () => {
        // U+1100 (E1 84 80) + U+1161 (E1 85 A1) + U+11A8 (E1 86 A8) => single Hangul syllable cluster, followed by "!"
        const hangul = "\xE1\x84\x80\xE1\x85\xA1\xE1\x86\xA8!";
        expect_eq(countGraphemes(hangul), 2 as usize);

        const g0 = nthGrapheme(hangul, 0).?;
        expect_eq(g0.start, 0 as usize);
        expect_eq(g0.end, 9 as usize);

        const g1 = nthGrapheme(hangul, 1).?;
        expect_eq(sliceEq(hangul[g1.start..g1.end], "!"), true);
    });

    test("6. @cap/unicode-segmentation — UAX #29 word segmentation & sliceGraphemes", () => {
        const text = "Don't stop at 3.14, 你好!";
        expect_eq(countUnicodeWords(text), 6 as usize);

        let wit = WordIter.new(text);
        const w0 = wit.nextUnicodeWord().?;
        const w1 = wit.nextUnicodeWord().?;
        const w2 = wit.nextUnicodeWord().?;
        const w3 = wit.nextUnicodeWord().?;
        const w4 = wit.nextUnicodeWord().?;
        const w5 = wit.nextUnicodeWord().?;
        expect_eq(wit.nextUnicodeWord() == null, true);

        expect_eq(sliceEq(text[w0.start..w0.end], "Don't"), true);
        expect_eq(sliceEq(text[w1.start..w1.end], "stop"), true);
        expect_eq(sliceEq(text[w2.start..w2.end], "at"), true);
        expect_eq(sliceEq(text[w3.start..w3.end], "3.14"), true);
        expect_eq(sliceEq(text[w4.start..w4.end], "你"), true);
        expect_eq(sliceEq(text[w5.start..w5.end], "好"), true);

        // sliceGraphemes on mixed ASCII + combining mark + CJK
        const mixed = "A\r\ne\xCC\x81你好Z";
        const sub = sliceGraphemes(mixed, 2, 3); // é + 你 + 好
        expect_eq(sliceEq(mixed[sub.start..sub.end], "e\xCC\x81你好"), true);
    });
}
