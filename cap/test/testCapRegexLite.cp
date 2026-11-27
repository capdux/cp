// cap/test/testCapRegexLite.cp
// Comprehensive 6-test suite for @cap/regex-lite (linear-time Thompson NFA, Rust regex-lite v0.1 parity)

import { test, expect_eq } from "cap:test";
import {
    RegexMatch,
    RegexLite
} from "../../packages/regex-lite/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/regex-lite — Literals, dot '.', ^/$ anchors & @cap/memchr prefix acceleration", () => {
        let re = RegexLite.compile("https://example\\.com/[a-z]+");
        expect_eq(re.isOk(), true);

        let text = "visit https://example.com/docs today!";
        let m = re.find(text);
        expect_eq(m.matched, true);
        expect_eq(m.start, 6 as usize);
        expect_eq(m.end, 30 as usize);
        expect_eq(eqBytes(text[m.start..m.end], "https://example.com/docs"), true);

        let anchored = RegexLite.compile("^hello.world$");
        expect_eq(anchored.isOk(), true);
        expect_eq(anchored.isMatch("hello_world"), true);
        expect_eq(anchored.isMatch("say hello_world"), false);
        expect_eq(anchored.isMatch("hello\nworld"), false);
    });

    test("2. @cap/regex-lite — Quantifiers */+/?/{2,4}, lazy +? & alternation cat|dog", () => {
        let reAlt = RegexLite.compile("(cat|dog)s?");
        expect_eq(reAlt.isOk(), true);
        let m1 = reAlt.find("my dogs ran");
        expect_eq(m1.matched, true);
        expect_eq(eqBytes("my dogs ran"[m1.start..m1.end], "dogs"), true);

        let reCount = RegexLite.compile("a{2,4}");
        expect_eq(reCount.isOk(), true);
        expect_eq(reCount.isMatch("a"), false);
        let m2 = reCount.find("baaaaab");
        expect_eq(m2.matched, true);
        expect_eq(m2.len(), 4 as usize);

        // Greedy vs lazy quantifier
        let greedy = RegexLite.compile("a.+b");
        let lazy = RegexLite.compile("a.+?b");
        let s = "a1b2b";
        let mg = greedy.find(s);
        let ml = lazy.find(s);
        expect_eq(mg.len(), 5 as usize); // "a1b2b"
        expect_eq(ml.len(), 3 as usize); // "a1b"
    });

    test("3. @cap/regex-lite — Character classes [a-z0-9_]+, [^0-9] & \\d/\\w/\\s shorthands", () => {
        let reIdent = RegexLite.compile("[a-z0-9_]+");
        expect_eq(reIdent.isOk(), true);
        let m1 = reIdent.find("$$$var_42!!!");
        expect_eq(m1.matched, true);
        expect_eq(eqBytes("$$$var_42!!!"[m1.start..m1.end], "var_42"), true);

        let reNonDigit = RegexLite.compile("[^0-9]+");
        let m2 = reNonDigit.find("1234abc567");
        expect_eq(m2.matched, true);
        expect_eq(eqBytes("1234abc567"[m2.start..m2.end], "abc"), true);

        let reShort = RegexLite.compile("\\w+\\s+\\d+");
        let m3 = reShort.find("item   2026!");
        expect_eq(m3.matched, true);
        expect_eq(eqBytes("item   2026!"[m3.start..m3.end], "item   2026"), true);
    });

    test("4. @cap/regex-lite — findAll extracting multiple emails/numbers & splitCount", () => {
        let reEmail = RegexLite.compile("[a-z0-9._-]+@[a-z0-9-]+\\.[a-z]+");
        expect_eq(reEmail.isOk(), true);
        let text = "contact a@b.io or dev_1@cap-lang.org now";
        let matches: [8]RegexMatch = [RegexMatch { matched: false, start: 0, end: 0 }; 8];
        let n = reEmail.findAll(text, matches[0..8]);
        expect_eq(n, 2 as usize);
        expect_eq(eqBytes(text[matches[0].start..matches[0].end], "a@b.io"), true);
        expect_eq(eqBytes(text[matches[1].start..matches[1].end], "dev_1@cap-lang.org"), true);

        let reComma = RegexLite.compile(",\\s*");
        expect_eq(reComma.splitCount("one, two,  three,four"), 4 as usize);
    });

    test("5. @cap/regex-lite — replaceAll batch substitution", () => {
        let reNum = RegexLite.compile("\\d+");
        expect_eq(reNum.isOk(), true);
        let out: [64]u8 = [0; 64];
        let n = reNum.replaceAll("v1.20.300-rc4", "#", out[0..64]);
        expect_eq(eqBytes(out[0..n], "v#.#.#-rc#"), true);

        let reWs = RegexLite.compile("\\s+");
        let n2 = reWs.replaceAll("foo   bar\t\nbaz", " ", out[0..64]);
        expect_eq(eqBytes(out[0..n2], "foo bar baz"), true);
    });

    test("6. @cap/regex-lite — Linear-time ReDoS immunity (a?^25 a^25) & invalid syntax rejection", () => {
        // Classic pathological pattern for backtracking engines (2^25 steps), linear O(N*M) in Thompson NFA
        let redos = RegexLite.compile("a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?aaaaaaaaaaaaaaaaaaaaaaaaa");
        expect_eq(redos.isOk(), true);
        let m = redos.find("aaaaaaaaaaaaaaaaaaaaaaaaa");
        expect_eq(m.matched, true);
        expect_eq(m.start, 0 as usize);
        expect_eq(m.end, 25 as usize);
        expect_eq(redos.isMatch("aaaaaaaaaaaaaaaaaaaaaaaa"), false); // 24 'a's -> no match

        // Invalid syntax rejected cleanly via @cap/regex-syntax
        let bad1 = RegexLite.compile("(unclosed[a-z");
        expect_eq(bad1.isOk(), false);
        expect_eq(bad1.isMatch("uncloseda"), false);

        let bad2 = RegexLite.compile("a{5,2}");
        expect_eq(bad2.isOk(), false);
    });
}
