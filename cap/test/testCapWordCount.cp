// cap/test/testCapWordCount.cp
// Comprehensive unit tests for @cap/word-count

import { test, expect_eq } from "cap:test";
import {
    isCjkCodepoint,
    analyze,
    countWords,
    estimateReadingSeconds,
} from "../../packages/@cap/word-count/src/index.cp";

fn main() {
    test("1. @cap/word-count — English with punctuation & contractions", () => {
        const s = "Hello, world! It's CP 2026.";
        const c = analyze(s);
        expect_eq(c.bytes, s.len);
        expect_eq(c.chars, s.len);
        expect_eq(c.latin_words, 5 as usize);
        expect_eq(c.cjk_chars, 0 as usize);
        expect_eq(c.words, 5 as usize);
        expect_eq(c.lines, 1 as usize);
    });

    test("2. @cap/word-count — Chinese + Latin mixed string", () => {
        const s = "你好世界，欢迎使用CP语言";
        const c = analyze(s);
        expect_eq(c.cjk_chars, 10 as usize);
        expect_eq(c.latin_words, 1 as usize);
        expect_eq(c.words, 11 as usize);
        expect_eq(c.chars, 13 as usize);
        expect_eq(countWords(s), 11 as usize);
    });

    test("3. @cap/word-count — Chinese + Japanese + Korean + English multilingual text", () => {
        // 中文(2) + こんにちは(5) + 안녕(2) + CP(1) = 9 CJK + 1 Latin = 10 words
        const s = "中文 こんにちは 안녕 CP";
        const c = analyze(s);
        expect_eq(c.cjk_chars, 9 as usize);
        expect_eq(c.latin_words, 1 as usize);
        expect_eq(c.words, 10 as usize);

        expect_eq(isCjkCodepoint(0x4E2D as u32), true);  // 中
        expect_eq(isCjkCodepoint(0x3053 as u32), true);  // こ
        expect_eq(isCjkCodepoint(0xC548 as u32), true);  // 안
        expect_eq(isCjkCodepoint('A' as u32), false);
    });

    test("4. @cap/word-count — Multi-line LF & CRLF line counting", () => {
        const lf = "line one\nline two\nline three";
        const c1 = analyze(lf);
        expect_eq(c1.lines, 3 as usize);
        expect_eq(c1.words, 6 as usize);

        const crlf = "first\r\nsecond\r\nthird\r\n";
        const c2 = analyze(crlf);
        expect_eq(c2.lines, 3 as usize);
        expect_eq(c2.words, 3 as usize);
    });

    test("5. @cap/word-count — estimateReadingSeconds ceiling accuracy", () => {
        // 10 words at 200 WPM => ceil(10 * 60 / 200) = 3 seconds
        const tenWords = "one two three four five six seven eight nine ten";
        expect_eq(estimateReadingSeconds(tenWords, 200 as u32), 3 as u32);
        // 10 words at 60 WPM => 10 seconds
        expect_eq(estimateReadingSeconds(tenWords, 60 as u32), 10 as u32);
        // 1 word at 200 WPM => ceil(60 / 200) = 1 second
        expect_eq(estimateReadingSeconds("hello", 200 as u32), 1 as u32);
        // WPM=0 falls back safely to 200 WPM
        expect_eq(estimateReadingSeconds(tenWords, 0 as u32), 3 as u32);
    });

    test("6. @cap/word-count — Empty & whitespace-only boundary cases", () => {
        const empty = analyze("");
        expect_eq(empty.bytes, 0 as usize);
        expect_eq(empty.chars, 0 as usize);
        expect_eq(empty.words, 0 as usize);
        expect_eq(empty.lines, 0 as usize);
        expect_eq(estimateReadingSeconds("", 200 as u32), 0 as u32);

        const spaces = analyze("   \t\n  ");
        expect_eq(spaces.words, 0 as usize);
        expect_eq(spaces.lines, 2 as usize);
        expect_eq(estimateReadingSeconds("   \t\n  ", 200 as u32), 0 as u32);
    });
}
