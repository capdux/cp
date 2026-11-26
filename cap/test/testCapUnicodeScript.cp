import { test, expect_eq } from "cap:test";
import { Script, scriptOf, scriptName, isLatin, isHan, isCjkScript, dominantScript } from "../../packages/@cap/unicode-script/src/index.cp";

fn main() {
    test("scriptOf classifies ASCII, Latin, Greek, and Cyrillic", () => {
        expect_eq(scriptOf('A'), Script.Latin);
        expect_eq(scriptOf('z'), Script.Latin);
        expect_eq(scriptOf(0x00E9), Script.Latin); // é
        expect_eq(scriptOf('0'), Script.Common);
        expect_eq(scriptOf(' '), Script.Common);
        expect_eq(scriptOf(0x03B1), Script.Greek); // α
        expect_eq(scriptOf(0x0416), Script.Cyrillic); // Ж
    });

    test("scriptOf classifies CJK scripts (Han, Hiragana, Katakana, Hangul)", () => {
        expect_eq(scriptOf(0x4E2D), Script.Han); // 中
        expect_eq(scriptOf(0x3042), Script.Hiragana); // あ
        expect_eq(scriptOf(0x30A2), Script.Katakana); // ア
        expect_eq(scriptOf(0xD55C), Script.Hangul); // 한
        expect_eq(isHan(0x4E2D), true);
        expect_eq(isHan('A'), false);
        expect_eq(isLatin('A'), true);
        expect_eq(isLatin(0x4E2D), false);
    });

    test("scriptOf classifies Arabic, Hebrew, Devanagari, Thai, Inherited, Unknown", () => {
        expect_eq(scriptOf(0x0627), Script.Arabic); // ا
        expect_eq(scriptOf(0x05D0), Script.Hebrew); // א
        expect_eq(scriptOf(0x0905), Script.Devanagari); // अ
        expect_eq(scriptOf(0x0E01), Script.Thai); // ก
        expect_eq(scriptOf(0x0301), Script.Inherited); // combining acute
        expect_eq(scriptOf(0x110000), Script.Unknown); // out of range
    });

    test("scriptName and isCjkScript return canonical results", () => {
        expect_eq(std.mem.eql(u8, scriptName(Script.Latin), "Latin"), true);
        expect_eq(std.mem.eql(u8, scriptName(Script.Han), "Han"), true);
        expect_eq(std.mem.eql(u8, scriptName(Script.Inherited), "Inherited"), true);
        expect_eq(std.mem.eql(u8, scriptName(Script.Unknown), "Unknown"), true);

        expect_eq(isCjkScript(Script.Han), true);
        expect_eq(isCjkScript(Script.Hiragana), true);
        expect_eq(isCjkScript(Script.Katakana), true);
        expect_eq(isCjkScript(Script.Hangul), true);
        expect_eq(isCjkScript(Script.Latin), false);
        expect_eq(isCjkScript(Script.Common), false);
    });

    test("dominantScript detects dominant UTF-8 script skipping Common/Inherited", () => {
        expect_eq(dominantScript("Hello, World! 12345"), Script.Latin);
        expect_eq(dominantScript("2026年，你好世界！CP"), Script.Han);
        expect_eq(dominantScript("こんにちは、世界"), Script.Hiragana);
        expect_eq(dominantScript("Привет мир 2026!"), Script.Cyrillic);
    });

    test("dominantScript handles empty and punctuation-only strings", () => {
        expect_eq(dominantScript(""), Script.Unknown);
        expect_eq(dominantScript("12345 --- !!!"), Script.Common);
    });
}
