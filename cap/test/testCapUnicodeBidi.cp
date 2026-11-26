import { test, expect_eq } from "cap:test";
import {
    BIDI_L,
    BIDI_R,
    BIDI_AL,
    BIDI_EN,
    BIDI_AN,
    BIDI_WS,
    BIDI_ON,
    BIDI_B,
    DIR_LTR,
    DIR_RTL,
    BidiRun,
    bidiClass,
    baseDirection,
    hasRtl,
    isLtrOnly,
    computeVisualRuns,
    mirrorAsciiBracket,
} from "../../packages/@cap/unicode-bidi/src/index.cp";

fn main() {
    test("1. @cap/unicode-bidi — bidiClass Latin, Hebrew, Arabic, digits & whitespace", () => {
        expect_eq(bidiClass(0x41), BIDI_L);     // 'A'
        expect_eq(bidiClass(0x4E2D), BIDI_L);   // '中'
        expect_eq(bidiClass(0x05D0), BIDI_R);   // Hebrew Alef 'א'
        expect_eq(bidiClass(0x0627), BIDI_AL);  // Arabic Alef 'ا'
        expect_eq(bidiClass(0x35), BIDI_EN);    // '5'
        expect_eq(bidiClass(0x0665), BIDI_AN);  // Arabic-Indic '٥'
        expect_eq(bidiClass(0x20), BIDI_WS);    // ' '
        expect_eq(bidiClass(0x28), BIDI_ON);    // '('
        expect_eq(bidiClass(0x0A), BIDI_B);     // '\n'
    });

    test("2. @cap/unicode-bidi — baseDirection UAX #9 P2/P3 rules", () => {
        expect_eq(baseDirection("Hello world"), DIR_LTR);
        expect_eq(baseDirection("中文测试"), DIR_LTR);
        expect_eq(baseDirection("123456"), DIR_LTR);
        expect_eq(baseDirection("   (123) "), DIR_LTR);
        // Hebrew "שלום"
        expect_eq(baseDirection("שלום world"), DIR_RTL);
        // Arabic "مرحبا"
        expect_eq(baseDirection("123 مرحبا"), DIR_RTL);
        // English first, then Arabic -> LTR
        expect_eq(baseDirection("Hi مرحبا"), DIR_LTR);
    });

    test("3. @cap/unicode-bidi — hasRtl & isLtrOnly detection", () => {
        expect_eq(hasRtl("Hello 123 中文"), false);
        expect_eq(isLtrOnly("Hello 123 中文"), true);
        expect_eq(hasRtl("Hello שלום"), true);
        expect_eq(isLtrOnly("Hello שלום"), false);
        expect_eq(hasRtl("العربية"), true);
        expect_eq(isLtrOnly("العربية"), false);
    });

    test("4. @cap/unicode-bidi — computeVisualRuns splits mixed LTR/RTL runs", () => {
        let runs: [8]BidiRun = [BidiRun { start: 0, len: 0, level: 0 }; 8];
        // "Hello שלום!" -> run 0: "Hello " (level 0, even=LTR), run 1: "שלום!" (level 1, odd=RTL)
        let mixed: string = "Hello שלום!";
        let n: usize = computeVisualRuns(mixed, runs[0..8]);
        expect_eq(n, 2 as usize);
        expect_eq(runs[0].start, 0 as usize);
        expect_eq(runs[0].len, 6 as usize);
        expect_eq((runs[0].level & 1), 0 as u8);
        expect_eq(runs[1].start, 6 as usize);
        expect_eq((runs[1].level & 1), 1 as u8);
        expect_eq(runs[0].len + runs[1].len, mixed.len);

        // RTL base with embedded LTR word: "שלום abc"
        let rtl_mixed: string = "שלום abc";
        let n2: usize = computeVisualRuns(rtl_mixed, runs[0..8]);
        expect_eq(n2, 2 as usize);
        expect_eq((runs[0].level & 1), 1 as u8); // RTL run (level 1)
        expect_eq((runs[1].level & 1), 0 as u8); // Embedded LTR run (level 2)
    });

    test("5. @cap/unicode-bidi — mirrorAsciiBracket UAX #9 L4 pairs", () => {
        expect_eq(mirrorAsciiBracket(40), 41 as u8);   // '(' -> ')'
        expect_eq(mirrorAsciiBracket(41), 40 as u8);   // ')' -> '('
        expect_eq(mirrorAsciiBracket(91), 93 as u8);   // '[' -> ']'
        expect_eq(mirrorAsciiBracket(93), 91 as u8);   // ']' -> '['
        expect_eq(mirrorAsciiBracket(123), 125 as u8); // '{' -> '}'
        expect_eq(mirrorAsciiBracket(125), 123 as u8); // '}' -> '{'
        expect_eq(mirrorAsciiBracket(60), 62 as u8);   // '<' -> '>'
        expect_eq(mirrorAsciiBracket(62), 60 as u8);   // '>' -> '<'
        expect_eq(mirrorAsciiBracket(65), 65 as u8);   // 'A' unchanged
    });

    test("6. @cap/unicode-bidi — empty string & small out_runs buffer safety", () => {
        let runs: [1]BidiRun = [BidiRun { start: 0, len: 0, level: 0 }; 1];
        expect_eq(baseDirection(""), DIR_LTR);
        expect_eq(hasRtl(""), false);
        expect_eq(isLtrOnly(""), true);
        expect_eq(computeVisualRuns("", runs[0..1]), 0 as usize);

        // Buffer of length 1 safely truncates multi-run output without overflow
        let n: usize = computeVisualRuns("Hello שלום world", runs[0..1]);
        expect_eq(n, 1 as usize);
        expect_eq(runs[0].level, 0 as u8);
    });
}
