// cap/test/testCapBcp47.cp
// Comprehensive unit tests for @cap/bcp47

import { test, expect_eq } from "cap:test";
import {
    Bcp47Tag,
    parseBcp47,
    canonicalizeBcp47,
    matchLocale,
} from "../../packages/bcp-47/src/index.cp";

fn main() {
    test("1. @cap/bcp47 — Standard language, script & region tags (ISO-3166 & UN M.49)", () => {
        const zh = parseBcp47("zh-Hans-CN");
        expect_eq(zh.valid, true);
        expect_eq(zh.language, "zh");
        expect_eq(zh.script, "Hans");
        expect_eq(zh.region, "CN");

        const es = parseBcp47("es-419");
        expect_eq(es.valid, true);
        expect_eq(es.language, "es");
        expect_eq(es.script, "");
        expect_eq(es.region, "419");

        const yue = parseBcp47("yue-Hant-HK");
        expect_eq(yue.valid, true);
        expect_eq(yue.language, "yue");
        expect_eq(yue.script, "Hant");
        expect_eq(yue.region, "HK");
    });

    test("2. @cap/bcp47 — Variant, extension & private-use subtags", () => {
        const de = parseBcp47("de-DE-1996");
        expect_eq(de.valid, true);
        expect_eq(de.language, "de");
        expect_eq(de.region, "DE");
        expect_eq(de.variant, "1996");

        const ext = parseBcp47("en-US-u-ca-gregory");
        expect_eq(ext.valid, true);
        expect_eq(ext.language, "en");
        expect_eq(ext.region, "US");
        expect_eq(ext.extension, "u-ca-gregory");

        const priv_tag = parseBcp47("x-custom-tag");
        expect_eq(priv_tag.valid, true);
        expect_eq(priv_tag.language, "");
        expect_eq(priv_tag.private_use, "x-custom-tag");
    });

    test("3. @cap/bcp47 — Underscore compatibility & RFC 5646 canonicalization", () => {
        let out: [64]u8 = undefined;

        const t1 = parseBcp47("ZH_hans_cn");
        expect_eq(t1.valid, true);
        expect_eq(canonicalizeBcp47(t1, out[0..64]), "zh-Hans-CN");

        const t2 = parseBcp47("sr_LATN_rs");
        expect_eq(t2.valid, true);
        expect_eq(canonicalizeBcp47(t2, out[0..64]), "sr-Latn-RS");

        const t3 = parseBcp47("EN_us_U_CA_Gregory");
        expect_eq(t3.valid, true);
        expect_eq(canonicalizeBcp47(t3, out[0..64]), "en-US-u-ca-gregory");
    });

    test("4. @cap/bcp47 — matchLocale fallback hierarchy (exact > script > region > language)", () => {
        const pool1: [4]string = ["en-US", "zh", "zh-Hans", "zh-Hans-CN"];
        expect_eq(matchLocale(parseBcp47("zh-Hans-CN"), pool1[0..4]) ?? "", "zh-Hans-CN");

        // No exact zh-Hans-SG -> falls back to Language+Script (zh-Hans) over bare zh
        const pool2: [3]string = ["en-US", "zh", "zh-Hans"];
        expect_eq(matchLocale(parseBcp47("zh-Hans-SG"), pool2[0..3]) ?? "", "zh-Hans");

        // Language+Region beats language-only
        const pool3: [3]string = ["en", "en-GB", "fr-FR"];
        expect_eq(matchLocale(parseBcp47("en-GB"), pool3[0..3]) ?? "", "en-GB");
        expect_eq(matchLocale(parseBcp47("en-AU"), pool3[0..3]) ?? "", "en");
    });

    test("5. @cap/bcp47 — matchLocale no-match returns null", () => {
        const pool: [2]string = ["en-US", "fr-FR"];
        expect_eq(matchLocale(parseBcp47("ja-JP"), pool[0..2]) == null, true);
        expect_eq(matchLocale(parseBcp47("invalid!"), pool[0..2]) == null, true);
    });

    test("6. @cap/bcp47 — Invalid BCP-47 tags rejected", () => {
        expect_eq(parseBcp47("").valid, false);
        expect_eq(parseBcp47("e").valid, false);
        expect_eq(parseBcp47("english").valid, false);
        expect_eq(parseBcp47("en--US").valid, false);
        expect_eq(parseBcp47("en-US-").valid, false);
        expect_eq(parseBcp47("en-US-Hans").valid, false); // wrong subtag order
        expect_eq(parseBcp47("en-u").valid, false);       // dangling extension singleton
        expect_eq(parseBcp47("x").valid, false);          // dangling private-use
    });
}
