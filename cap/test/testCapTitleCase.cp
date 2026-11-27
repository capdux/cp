import { test, expect_eq } from "cap:test";
import {
    isSmallWord,
    toTitleCase,
    toSentenceCase,
    capitalizeFirst,
} from "../../packages/title-case/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/title-case — Standard title small words 'the lord of the rings'", () => {
        expect_eq(isSmallWord("the"), true);
        expect_eq(isSmallWord("OF"), true);
        expect_eq(isSmallWord("via"), true);
        expect_eq(isSmallWord("lord"), false);

        let buf: [128]u8 = undefined;
        const n = toTitleCase("the lord of the rings", buf[0..128]);
        expect_eq(testEq(buf[0..n], "The Lord of the Rings"), true);
    });

    test("2. @cap/title-case — First & last small words always capitalized", () => {
        let buf: [128]u8 = undefined;
        const n = toTitleCase("what it is to be in", buf[0..128]);
        expect_eq(testEq(buf[0..n], "What It Is to Be In"), true);
    });

    test("3. @cap/title-case — Post-colon capitalization, acronyms & hyphenated words", () => {
        let buf: [128]u8 = undefined;
        const n1 = toTitleCase("a guide to the REST API: an overview", buf[0..128]);
        expect_eq(testEq(buf[0..n1], "A Guide to the REST API: An Overview"), true);

        const n2 = toTitleCase("zero-alloc HTTP server in CP", buf[0..128]);
        expect_eq(testEq(buf[0..n2], "Zero-Alloc HTTP Server in CP"), true);
    });

    test("4. @cap/title-case — toSentenceCase multi-sentence handling", () => {
        let buf: [128]u8 = undefined;
        const n = toSentenceCase("hELLO WORLD! how ARE you? fine.", buf[0..128]);
        expect_eq(testEq(buf[0..n], "Hello world! How are you? Fine."), true);
    });

    test("5. @cap/title-case — capitalizeFirst preserves remaining casing", () => {
        let buf: [64]u8 = undefined;
        const n1 = capitalizeFirst("helloWorld", buf[0..64]);
        expect_eq(testEq(buf[0..n1], "HelloWorld"), true);

        const n2 = capitalizeFirst("  cpLang", buf[0..64]);
        expect_eq(testEq(buf[0..n2], "  CpLang"), true);
    });

    test("6. @cap/title-case — Empty string & undersized buffer protection", () => {
        let buf: [16]u8 = undefined;
        expect_eq(toTitleCase("", buf[0..16]), 0 as usize);
        expect_eq(toSentenceCase("", buf[0..16]), 0 as usize);
        expect_eq(capitalizeFirst("", buf[0..16]), 0 as usize);

        let tiny: [4]u8 = undefined;
        expect_eq(toTitleCase("hello world", tiny[0..4]), 0 as usize);
        expect_eq(toSentenceCase("hello world", tiny[0..4]), 0 as usize);
        expect_eq(capitalizeFirst("hello world", tiny[0..4]), 0 as usize);
    });
}
