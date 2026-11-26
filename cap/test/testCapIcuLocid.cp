// cap/test/testCapIcuLocid.cp
// Comprehensive unit tests for @cap/icu-locid

import { test, expect_eq } from "cap:test";
import {
    LOCALE_OK,
    LOCALE_ERR_EMPTY,
    LOCALE_ERR_EMPTY_SUBTAG,
    LOCALE_ERR_INVALID_LANGUAGE,
    LOCALE_ERR_INVALID_SUBTAG,
    LOCALE_ERR_DUPLICATE_VARIANT,
    LanguageIdentifier,
} from "../../packages/@cap/icu-locid/src/index.cp";

fn locTestEqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/icu-locid — Case canonicalization ZH_hans_cn -> zh-Hans-CN & en_us -> en-US", () => {
        const r1 = LanguageIdentifier.parse("ZH_hans_cn");
        expect_eq(r1.ok, true);
        expect_eq(r1.err, LOCALE_OK);
        expect_eq(r1.locale.has_script, true);
        expect_eq(r1.locale.has_region, true);
        expect_eq(r1.locale.variant_count, 0 as usize);

        let buf: [64]u8 = [0; 64];
        const n1: usize = r1.locale.toStr(buf[0..64]);
        expect_eq(n1, 10 as usize);
        expect_eq(locTestEqBytes(buf[0..n1], "zh-Hans-CN"), true);

        const r2 = LanguageIdentifier.parse("en_us");
        expect_eq(r2.ok, true);
        expect_eq(r2.locale.has_script, false);
        expect_eq(r2.locale.has_region, true);
        const n2: usize = r2.locale.toStr(buf[0..64]);
        expect_eq(locTestEqBytes(buf[0..n2], "en-US"), true);

        // Canonical equality across separator & case variations
        const r3 = LanguageIdentifier.parse("zh-HANS-cn");
        expect_eq(r3.ok, true);
        expect_eq(r1.locale.eq(r3.locale), true);
        expect_eq(r1.locale.matches(r2.locale), false);
    });

    test("2. @cap/icu-locid — UN M.49 numeric region es-419 & 3-letter language fil-PH", () => {
        const es = LanguageIdentifier.parse("ES-419");
        expect_eq(es.ok, true);
        expect_eq(es.locale.has_script, false);
        expect_eq(es.locale.has_region, true);
        let buf: [64]u8 = [0; 64];
        const n_es: usize = es.locale.toStr(buf[0..64]);
        expect_eq(locTestEqBytes(buf[0..n_es], "es-419"), true);

        const fil = LanguageIdentifier.parse("FIL_ph");
        expect_eq(fil.ok, true);
        expect_eq(fil.locale.has_region, true);
        const n_fil: usize = fil.locale.toStr(buf[0..64]);
        expect_eq(locTestEqBytes(buf[0..n_fil], "fil-PH"), true);
    });

    test("3. @cap/icu-locid — Multiple variant subtags parsing, sorting & formatting", () => {
        // Variants "POSIX" and "1996" must be lowercased and sorted lexicographically -> "1996-posix"
        const r = LanguageIdentifier.parse("de-DE-POSIX-1996");
        expect_eq(r.ok, true);
        expect_eq(r.locale.has_script, false);
        expect_eq(r.locale.has_region, true);
        expect_eq(r.locale.variant_count, 2 as usize);

        let buf: [64]u8 = [0; 64];
        const n: usize = r.locale.toStr(buf[0..64]);
        expect_eq(locTestEqBytes(buf[0..n], "de-DE-1996-posix"), true);

        // Duplicate variant rejected
        const dup = LanguageIdentifier.parse("de-DE-posix-POSIX");
        expect_eq(dup.ok, false);
        expect_eq(dup.err, LOCALE_ERR_DUPLICATE_VARIANT);
    });

    test("4. @cap/icu-locid — Undetermined language und & isUnd() + matches()", () => {
        const u = LanguageIdentifier.parse("UND");
        expect_eq(u.ok, true);
        expect_eq(u.locale.isUnd(), true);

        let buf: [32]u8 = [0; 32];
        const n: usize = u.locale.toStr(buf[0..32]);
        expect_eq(locTestEqBytes(buf[0..n], "und"), true);

        const zh = LanguageIdentifier.parse("zh-Hans-CN").locale;
        expect_eq(zh.isUnd(), false);
        expect_eq(u.locale.matches(zh), true);

        const u_latn = LanguageIdentifier.parse("und-Latn").locale;
        expect_eq(u_latn.isUnd(), true);
        expect_eq(u_latn.matches(zh), false);
    });

    test("5. @cap/icu-locid — maximizeLikelySubtags() for common world languages", () => {
        let buf: [64]u8 = [0; 64];

        const zh_max: LanguageIdentifier = LanguageIdentifier.parse("zh").locale.maximizeLikelySubtags();
        expect_eq(locTestEqBytes(buf[0..zh_max.toStr(buf[0..64])], "zh-Hans-CN"), true);

        const zh_tw: LanguageIdentifier = LanguageIdentifier.parse("zh-TW").locale.maximizeLikelySubtags();
        expect_eq(locTestEqBytes(buf[0..zh_tw.toStr(buf[0..64])], "zh-Hant-TW"), true);

        const en_max: LanguageIdentifier = LanguageIdentifier.parse("en").locale.maximizeLikelySubtags();
        expect_eq(locTestEqBytes(buf[0..en_max.toStr(buf[0..64])], "en-Latn-US"), true);

        const ja_max: LanguageIdentifier = LanguageIdentifier.parse("ja").locale.maximizeLikelySubtags();
        expect_eq(locTestEqBytes(buf[0..ja_max.toStr(buf[0..64])], "ja-Jpan-JP"), true);

        const ko_max: LanguageIdentifier = LanguageIdentifier.parse("ko").locale.maximizeLikelySubtags();
        expect_eq(locTestEqBytes(buf[0..ko_max.toStr(buf[0..64])], "ko-Kore-KR"), true);

        const ar_max: LanguageIdentifier = LanguageIdentifier.parse("ar").locale.maximizeLikelySubtags();
        expect_eq(locTestEqBytes(buf[0..ar_max.toStr(buf[0..64])], "ar-Arab-EG"), true);

        const ru_max: LanguageIdentifier = LanguageIdentifier.parse("ru").locale.maximizeLikelySubtags();
        expect_eq(locTestEqBytes(buf[0..ru_max.toStr(buf[0..64])], "ru-Cyrl-RU"), true);
    });

    test("6. @cap/icu-locid — Strict rejection of malformed inputs", () => {
        // Empty input
        const e1 = LanguageIdentifier.parse("");
        expect_eq(e1.ok, false);
        expect_eq(e1.err, LOCALE_ERR_EMPTY);

        // Numeric language
        const e2 = LanguageIdentifier.parse("123");
        expect_eq(e2.ok, false);
        expect_eq(e2.err, LOCALE_ERR_INVALID_LANGUAGE);

        // Single-letter language
        const e3 = LanguageIdentifier.parse("a");
        expect_eq(e3.ok, false);
        expect_eq(e3.err, LOCALE_ERR_INVALID_LANGUAGE);

        // Double separator (empty subtag)
        const e4 = LanguageIdentifier.parse("en--US");
        expect_eq(e4.ok, false);
        expect_eq(e4.err, LOCALE_ERR_EMPTY_SUBTAG);

        // Trailing separator
        const e5 = LanguageIdentifier.parse("en-US-");
        expect_eq(e5.ok, false);
        expect_eq(e5.err, LOCALE_ERR_EMPTY_SUBTAG);

        // Region placed after 4-digit variant subtag ("zh-1234-CN")
        const e6 = LanguageIdentifier.parse("zh-1234-CN");
        expect_eq(e6.ok, false);
        expect_eq(e6.err, LOCALE_ERR_INVALID_SUBTAG);
    });
}
