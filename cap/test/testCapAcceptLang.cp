// cap/test/testCapAcceptLang.cp
// Comprehensive unit tests for @cap/accept-lang (RFC 9110 / BCP-47)

import { test, expect_eq } from "cap:test";
import {
    LangRange,
    parseQuality,
    parseAcceptLanguage,
    matchesTag,
    pickLanguage,
} from "../../packages/accept-language-parser/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/accept-lang — parseAcceptLanguage sorts descending by quality stably", () => {
        let out: [8]LangRange = undefined;
        const n: usize = parseAcceptLanguage("fr;q=0.5, en-US, de;q=0.85, ja;q=0.85", out[0..8]);
        expect_eq(n, 4 as usize);

        expect_eq(sliceEq(out[0].tag, "en-US"), true);
        expect_eq(sliceEq(out[0].primary, "en"), true);
        expect_eq(sliceEq(out[0].region, "US"), true);
        expect_eq(out[0].quality, 1000 as u16);

        expect_eq(sliceEq(out[1].tag, "de"), true);
        expect_eq(out[1].quality, 850 as u16);

        expect_eq(sliceEq(out[2].tag, "ja"), true);
        expect_eq(out[2].quality, 850 as u16);

        expect_eq(sliceEq(out[3].tag, "fr"), true);
        expect_eq(out[3].quality, 500 as u16);
    });

    test("2. @cap/accept-lang — exact & primary prefix negotiation for zh-CN,zh;q=0.9,en;q=0.8", () => {
        const s1: [3]string = ["en-US", "zh-TW", "zh-CN"];
        expect_eq(sliceEq(pickLanguage("zh-CN,zh;q=0.9,en;q=0.8", s1[0..3]), "zh-CN"), true);

        // No exact zh-CN -> falls back to zh;q=0.9 matching zh-TW over en;q=0.8
        const s2: [2]string = ["en-US", "zh-TW"];
        expect_eq(sliceEq(pickLanguage("zh-CN,zh;q=0.9,en;q=0.8", s2[0..2]), "zh-TW"), true);

        // matchesTag boundaries
        expect_eq(matchesTag("zh", "zh-CN"), true);
        expect_eq(matchesTag("zh-CN", "zh-CN"), true);
        expect_eq(matchesTag("zh-CN", "zh"), false);
        expect_eq(matchesTag("en", "enigma"), false);
    });

    test("3. @cap/accept-lang — q=0 explicitly excludes language", () => {
        const supported: [2]string = ["en", "fr"];
        expect_eq(sliceEq(pickLanguage("en;q=0, *;q=0.5", supported[0..2]), "fr"), true);
        expect_eq(pickLanguage("en;q=0, fr;q=0", supported[0..2]).len, 0 as usize);
    });

    test("4. @cap/accept-lang — wildcard '*' fallback", () => {
        const supported: [2]string = ["ja-JP", "ko-KR"];
        expect_eq(sliceEq(pickLanguage("de-DE, *;q=0.1", supported[0..2]), "ja-JP"), true);
        expect_eq(matchesTag("*", "ja-JP"), true);
        expect_eq(matchesTag("*", ""), false);
    });

    test("5. @cap/accept-lang — case-insensitive & whitespace tolerance", () => {
        const supported: [2]string = ["en-GB", "fr-FR"];
        expect_eq(sliceEq(pickLanguage("  EN-gb ; Q = 0.7 ,  fr ; q=0.9 ", supported[0..2]), "fr-FR"), true);
        expect_eq(sliceEq(pickLanguage("  en-gb  ", supported[0..2]), "en-GB"), true);
    });

    test("6. @cap/accept-lang — empty header & invalid quality values", () => {
        expect_eq(parseQuality("1"), 1000 as i32);
        expect_eq(parseQuality("1.000"), 1000 as i32);
        expect_eq(parseQuality("0"), 0 as i32);
        expect_eq(parseQuality("0.85"), 850 as i32);
        expect_eq(parseQuality("0.005"), 5 as i32);
        expect_eq(parseQuality("1.5"), -1 as i32);
        expect_eq(parseQuality("2"), -1 as i32);
        expect_eq(parseQuality("0.1234"), -1 as i32);
        expect_eq(parseQuality("abc"), -1 as i32);

        const supported: [1]string = ["en"];
        expect_eq(pickLanguage("", supported[0..1]).len, 0 as usize);
        expect_eq(pickLanguage("en;q=1.5", supported[0..1]).len, 0 as usize);
    });
}
