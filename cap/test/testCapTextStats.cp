// cap/test/testCapTextStats.cp
// Comprehensive unit tests for @cap/text-stats

import { test, expect_eq } from "cap:test";
import {
    TextStats,
    analyzeText,
    analyzeTextWithWpm,
    formatReadingTime,
} from "../../packages/@cap/text-stats/src/index.cp";

fn main() {
    test("1. @cap/text-stats — Pure English prose (words, sentences, decimals, hyphens)", () => {
        const text = "Hello world! CP is a zero-alloc language. Version 3.14 works great?";
        const st = analyzeText(text);
        expect_eq(st.bytes, text.len);
        expect_eq(st.chars, text.len);
        expect_eq(st.ascii_words, 11 as usize);
        expect_eq(st.cjk_chars, 0 as usize);
        expect_eq(st.total_words, 11 as usize);
        expect_eq(st.sentences, 3 as usize);
        expect_eq(st.lines, 1 as usize);
        expect_eq(st.paragraphs, 1 as usize);
        expect_eq(st.est_llm_tokens > 11 as usize, true);
    });

    test("2. @cap/text-stats — Pure Chinese prose (CJK chars, Chinese punctuation)", () => {
        const text = "春眠不觉晓，处处闻啼鸟。夜来风雨声，花落知多少？";
        const st = analyzeText(text);
        expect_eq(st.ascii_words, 0 as usize);
        expect_eq(st.cjk_chars, 20 as usize);
        expect_eq(st.total_words, 20 as usize);
        expect_eq(st.sentences, 2 as usize);
        expect_eq(st.paragraphs, 1 as usize);
        expect_eq(st.est_llm_tokens, 30 as usize);
    });

    test("3. @cap/text-stats — Mixed EN/CJK technical documentation", () => {
        const doc = "CP编译器支持zero-copy并发。It runs on BEAM and Zig!";
        const st = analyzeText(doc);
        // ASCII words: CP, zero-copy, It, runs, on, BEAM, and, Zig => 8
        // CJK chars: 编, 译, 器, 支, 持, 并, 发 => 7
        expect_eq(st.ascii_words, 8 as usize);
        expect_eq(st.cjk_chars, 7 as usize);
        expect_eq(st.total_words, 15 as usize);
        expect_eq(st.sentences, 2 as usize);
    });

    test("4. @cap/text-stats — Multi-paragraph, blank lines & line counts", () => {
        const doc = "First paragraph line one.\nFirst paragraph line two.\n\n\nSecond paragraph.\n   \nThird paragraph.";
        const st = analyzeText(doc);
        expect_eq(st.lines, 7 as usize);
        expect_eq(st.non_empty_lines, 4 as usize);
        expect_eq(st.paragraphs, 3 as usize);
        expect_eq(st.sentences, 4 as usize);
    });

    test("5. @cap/text-stats — Custom WPM & formatReadingTime (EN & ZH)", () => {
        let out: [64]u8 = undefined;

        // Short text -> < 1 min
        const short_st = analyzeText("Quick note.");
        expect_eq(short_st.est_reading_min, 0 as usize);
        expect_eq(formatReadingTime(short_st, false, out[0..64]), "< 1 min read");
        expect_eq(formatReadingTime(short_st, true, out[0..64]), "不足 1 分钟");

        // 30 English words at 10 WPM -> exactly 180s = 3 min
        const thirty = "one two three four five six seven eight nine ten "
            ++ "one two three four five six seven eight nine ten "
            ++ "one two three four five six seven eight nine ten";
        const long_st = analyzeTextWithWpm(thirty, 10, 350);
        expect_eq(long_st.ascii_words, 30 as usize);
        expect_eq(long_st.est_reading_sec, 180 as usize);
        expect_eq(long_st.est_reading_min, 3 as usize);
        expect_eq(formatReadingTime(long_st, false, out[0..64]), "3 min read");
        expect_eq(formatReadingTime(long_st, true, out[0..64]), "约 3 分钟读完");
    });

    test("6. @cap/text-stats — Empty & whitespace-only boundary cases", () => {
        const empty = analyzeText("");
        expect_eq(empty.bytes, 0 as usize);
        expect_eq(empty.chars, 0 as usize);
        expect_eq(empty.total_words, 0 as usize);
        expect_eq(empty.lines, 0 as usize);
        expect_eq(empty.paragraphs, 0 as usize);
        expect_eq(empty.sentences, 0 as usize);
        expect_eq(empty.est_reading_sec, 0 as usize);
        expect_eq(empty.est_llm_tokens, 0 as usize);

        const ws = analyzeText("   \n\t\n  ");
        expect_eq(ws.total_words, 0 as usize);
        expect_eq(ws.lines, 3 as usize);
        expect_eq(ws.non_empty_lines, 0 as usize);
        expect_eq(ws.paragraphs, 0 as usize);
        expect_eq(ws.sentences, 0 as usize);
        expect_eq(ws.est_reading_sec, 0 as usize);
    });
}
