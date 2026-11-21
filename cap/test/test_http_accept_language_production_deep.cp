import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LanguageRange,
    AcceptLanguageIter,
    negotiateLanguage,
} from "cap:http_accept_language";

test("1. LanguageRange.matchesTag enforces RFC 4647 case-insensitive prefix boundaries and wildcard", () => {
    const en = LanguageRange{ tag: "en", q_permille: 800 };
    expect_eq(en.isWildcard(), false);
    expect_eq(en.matchesTag("en"), true);
    expect_eq(en.matchesTag("EN"), true);
    expect_eq(en.matchesTag("en-US"), true);
    expect_eq(en.matchesTag("en-GB-oed"), true);
    expect_eq(en.matchesTag("eng"), false);
    expect_eq(en.matchesTag("fr"), false);
    expect_eq(en.matchesTag(""), false);

    const star = LanguageRange{ tag: "*", q_permille: 500 };
    expect_eq(star.isWildcard(), true);
    expect_eq(star.matchesTag("zh-Hans-CN"), true);
    expect_eq(star.matchesTag("ja"), true);
    expect_eq(star.matchesTag(""), false);
});

test("2. AcceptLanguageIter parses valid tags, qvalues (0..=1000), and skips malformed items", () => {
    const hdr = "zh-CN, en-US;q=0.8, en; q = 0.75 , fr;q=0.125, de;q=0, *;q=1.000";
    let it = AcceptLanguageIter.init(hdr);

    const r1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r1.tag, "zh-CN"), true);
    expect_eq(r1.q_permille, u16(1000));

    const r2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r2.tag, "en-US"), true);
    expect_eq(r2.q_permille, u16(800));

    const r3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r3.tag, "en"), true);
    expect_eq(r3.q_permille, u16(750));

    const r4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r4.tag, "fr"), true);
    expect_eq(r4.q_permille, u16(125));

    const r5 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r5.tag, "de"), true);
    expect_eq(r5.q_permille, u16(0));

    const r6 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r6.tag, "*"), true);
    expect_eq(r6.q_permille, u16(1000));

    expect_eq(it.next() == null, true);
});

test("3. AcceptLanguageIter rejects invalid BCP 47 syntax, out-of-range qvalues, and bad params", () => {
    const hdr = "123-en, en-, -us, toolongsubtag, en;q=1.5, en;q=1.001, en;q=0.1234, en;q=-0.1, en;foo=bar, en;q=0.5;q=0.6, ja-JP;q=0.9";
    let it = AcceptLanguageIter.init(hdr);

    const only = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, only.tag, "ja-JP"), true);
    expect_eq(only.q_permille, u16(900));
    expect_eq(it.next() == null, true);
});

test("4. negotiateLanguage selects highest weighted and most specific language candidate", () => {
    const supported: [4][]const u8 = ["en-US", "en-GB", "zh-CN", "ja-JP"];

    // Higher q wins
    const best1 = negotiateLanguage("en;q=0.7, zh-CN;q=0.9, ja;q=0.5", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best1, "zh-CN"), true);

    // Same q: more specific tag wins ("en-GB" over "en")
    const best2 = negotiateLanguage("en;q=0.8, en-GB;q=0.8", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best2, "en-GB"), true);

    // Wildcard fallback when no exact/prefix match exists
    const best3 = negotiateLanguage("fr-FR, de-DE;q=0.8, *;q=0.1", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best3, "en-US"), true);
});

test("5. negotiateLanguage honors q=0 explicit rejection and returns null when nothing matches", () => {
    const supported: [3][]const u8 = ["en-US", "en-GB", "fr-FR"];

    // Specific en-US;q=0 overrides broader en;q=0.8, so en-GB is chosen
    const best = negotiateLanguage("en-US;q=0, en;q=0.8", &supported) ?? unreachable;
    expect_eq(std.mem.eql(u8, best, "en-GB"), true);

    // Wildcard q=0 rejects unmatched languages; explicit rejection of all returns null
    expect_eq(negotiateLanguage("en-US;q=0, en-GB;q=0, *;q=0", &supported) == null, true);

    // Unmatched languages and empty header return null
    expect_eq(negotiateLanguage("ko-KR, de-DE;q=0.9", &supported) == null, true);
    expect_eq(negotiateLanguage("", &supported) == null, true);

    const empty_sup: [0][]const u8 = [];
    expect_eq(negotiateLanguage("en-US, *", &empty_sup) == null, true);
});
