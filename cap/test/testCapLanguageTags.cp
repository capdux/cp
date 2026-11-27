import { test, expect_eq } from "cap:test";
import {
    parseLanguageTag,
    formatCanonicalTag,
    matchesLanguageRange,
} from "../../packages/@cap/language-tags/src/index.cp";

fn main() {
    test("1. @cap/language-tags — parses basic language and region tags", () => {
        const t1 = parseLanguageTag("en");
        expect_eq(t1.valid, true);
        expect_eq(t1.language, "en");
        expect_eq(t1.region, "");

        const t2 = parseLanguageTag("en-US");
        expect_eq(t2.valid, true);
        expect_eq(t2.language, "en");
        expect_eq(t2.region, "US");
    });

    test("2. @cap/language-tags — parses script, UN M.49 numeric region, and variant subtags", () => {
        const t1 = parseLanguageTag("zh-Hans-CN");
        expect_eq(t1.valid, true);
        expect_eq(t1.language, "zh");
        expect_eq(t1.script, "Hans");
        expect_eq(t1.region, "CN");

        const t2 = parseLanguageTag("es-419");
        expect_eq(t2.valid, true);
        expect_eq(t2.language, "es");
        expect_eq(t2.region, "419");

        const t3 = parseLanguageTag("de-DE-1996");
        expect_eq(t3.valid, true);
        expect_eq(t3.language, "de");
        expect_eq(t3.region, "DE");
        expect_eq(t3.variant, "1996");
    });

    test("3. @cap/language-tags — supports underscore separators", () => {
        const t = parseLanguageTag("zh_Hant_TW");
        expect_eq(t.valid, true);
        expect_eq(t.language, "zh");
        expect_eq(t.script, "Hant");
        expect_eq(t.region, "TW");
    });

    test("4. @cap/language-tags — formatCanonicalTag normalizes BCP 47 casing", () => {
        let buf: [64]u8 = [0; 64];
        const t1 = parseLanguageTag("ZH-hans-cn");
        const n1 = formatCanonicalTag(t1, buf[0..64]);
        expect_eq(buf[0..n1] as string, "zh-Hans-CN");

        const t2 = parseLanguageTag("SR_cyrl_rs");
        const n2 = formatCanonicalTag(t2, buf[0..64]);
        expect_eq(buf[0..n2] as string, "sr-Cyrl-RS");

        const t3 = parseLanguageTag("DE-de-1996");
        const n3 = formatCanonicalTag(t3, buf[0..64]);
        expect_eq(buf[0..n3] as string, "de-DE-1996");
    });

    test("5. @cap/language-tags — matchesLanguageRange performs RFC 4647 prefix matching", () => {
        expect_eq(matchesLanguageRange("zh-Hans-CN", "*"), true);
        expect_eq(matchesLanguageRange("zh-Hans-CN", "zh"), true);
        expect_eq(matchesLanguageRange("zh-Hans-CN", "zh-hans"), true);
        expect_eq(matchesLanguageRange("zh_Hans_CN", "zh-Hans-CN"), true);
        expect_eq(matchesLanguageRange("zh-Hans-CN", "zh-Hant"), false);
        expect_eq(matchesLanguageRange("en-US", "e"), false);
    });

    test("6. @cap/language-tags — rejects malformed language tags", () => {
        expect_eq(parseLanguageTag("").valid, false);
        expect_eq(parseLanguageTag("123").valid, false);
        expect_eq(parseLanguageTag("en-").valid, false);
        expect_eq(parseLanguageTag("en--US").valid, false);
        expect_eq(parseLanguageTag("a").valid, false);
        let buf: [16]u8 = [0; 16];
        expect_eq(formatCanonicalTag(parseLanguageTag("123"), buf[0..16]), 0 as usize);
    });
}
