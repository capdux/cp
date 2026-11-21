import std from "std";
import { test, expect_eq } from "cap:test";
import {
    isValidBcp47Tag,
    parseLanguageTagParts,
    ContentLanguageIter,
    canonicalizeLanguageTag,
    hasContentLanguage,
} from "cap:http_content_language";

test("1. isValidBcp47Tag validates primary 2..=3 alpha and 1..=8 alnum subtags", () => {
    expect_eq(isValidBcp47Tag("en"), true);
    expect_eq(isValidBcp47Tag("zh-CN"), true);
    expect_eq(isValidBcp47Tag("zh-Hans-CN"), true);
    expect_eq(isValidBcp47Tag("es-419"), true);
    expect_eq(isValidBcp47Tag("cmn-Hans-SG"), true);
    expect_eq(isValidBcp47Tag("de-CH-1996"), true);
    expect_eq(isValidBcp47Tag("en-x-custom1"), true);

    // Invalid primary length, digits in primary, empty subtags, >8 chars, or illegal chars
    expect_eq(isValidBcp47Tag(""), false);
    expect_eq(isValidBcp47Tag("e"), false);
    expect_eq(isValidBcp47Tag("engl"), false);
    expect_eq(isValidBcp47Tag("12-US"), false);
    expect_eq(isValidBcp47Tag("-en"), false);
    expect_eq(isValidBcp47Tag("en-"), false);
    expect_eq(isValidBcp47Tag("en--US"), false);
    expect_eq(isValidBcp47Tag("en-toolongsubtag"), false);
    expect_eq(isValidBcp47Tag("en_US"), false);
    expect_eq(isValidBcp47Tag("en US"), false);
});

test("2. parseLanguageTagParts extracts primary, script, and alpha/numeric region", () => {
    const p1 = parseLanguageTagParts("zh-Hans-CN") ?? unreachable;
    expect_eq(std.mem.eql(u8, p1.primary, "zh"), true);
    expect_eq(std.mem.eql(u8, p1.script ?? unreachable, "Hans"), true);
    expect_eq(std.mem.eql(u8, p1.region ?? unreachable, "CN"), true);

    const p2 = parseLanguageTagParts("es-419") ?? unreachable;
    expect_eq(std.mem.eql(u8, p2.primary, "es"), true);
    expect_eq(p2.script == null, true);
    expect_eq(std.mem.eql(u8, p2.region ?? unreachable, "419"), true);

    const p3 = parseLanguageTagParts("sr-Cyrl") ?? unreachable;
    expect_eq(std.mem.eql(u8, p3.primary, "sr"), true);
    expect_eq(std.mem.eql(u8, p3.script ?? unreachable, "Cyrl"), true);
    expect_eq(p3.region == null, true);

    // Singleton stops script/region extraction
    const p4 = parseLanguageTagParts("en-x-us") ?? unreachable;
    expect_eq(std.mem.eql(u8, p4.primary, "en"), true);
    expect_eq(p4.script == null, true);
    expect_eq(p4.region == null, true);

    expect_eq(parseLanguageTagParts("bad_tag") == null, true);
});

test("3. ContentLanguageIter iterates comma-separated BCP 47 tags and skips invalid entries", () => {
    const hdr = " , de-DE , \t en-US , , invalid_tag , 123 , zh-Hans-CN , ";
    let it = ContentLanguageIter.init(hdr);

    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "de-DE"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "en-US"), true);
    expect_eq(std.mem.eql(u8, it.next() ?? unreachable, "zh-Hans-CN"), true);
    expect_eq(it.next() == null, true);

    let empty_it = ContentLanguageIter.init(" , , \t ");
    expect_eq(empty_it.next() == null, true);
});

test("4. canonicalizeLanguageTag applies RFC 5646 casing rules and validates buffer capacity", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);

    const c1 = canonicalizeLanguageTag("zh-hans-cn", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, c1, "zh-Hans-CN"), true);

    const c2 = canonicalizeLanguageTag("EN-us", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, c2, "en-US"), true);

    const c3 = canonicalizeLanguageTag("SR-CYRL-rs", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, c3, "sr-Cyrl-RS"), true);

    const c4 = canonicalizeLanguageTag("ES-419", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, c4, "es-419"), true);

    // Private-use subtags after singleton 'x' are lowercased
    const c5 = canonicalizeLanguageTag("EN-X-US-Custom", &buf) ?? unreachable;
    expect_eq(std.mem.eql(u8, c5, "en-x-us-custom"), true);

    // Invalid tag or too-small buffer returns null
    expect_eq(canonicalizeLanguageTag("bad_tag", &buf) == null, true);
    let tiny: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(canonicalizeLanguageTag("en-US", &tiny) == null, true);
});

test("5. hasContentLanguage matches exact tags and primary prefixes case-insensitively", () => {
    const hdr = "zh-Hans-CN, en-US, fr";

    expect_eq(hasContentLanguage(hdr, "zh"), true);
    expect_eq(hasContentLanguage(hdr, "ZH-HANS"), true);
    expect_eq(hasContentLanguage(hdr, "zh-hans-cn"), true);
    expect_eq(hasContentLanguage(hdr, "en"), true);
    expect_eq(hasContentLanguage(hdr, "EN-us"), true);
    expect_eq(hasContentLanguage(hdr, "FR"), true);

    // Prefix boundary and non-matching tags
    expect_eq(hasContentLanguage(hdr, "zh-TW"), false);
    expect_eq(hasContentLanguage(hdr, "eng"), false);
    expect_eq(hasContentLanguage(hdr, "fr-CA"), false);
    expect_eq(hasContentLanguage(hdr, "de"), false);

    // Invalid target or empty header
    expect_eq(hasContentLanguage(hdr, ""), false);
    expect_eq(hasContentLanguage(hdr, "en_US"), false);
    expect_eq(hasContentLanguage("", "en"), false);
});
