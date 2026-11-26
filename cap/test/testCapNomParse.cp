// cap/test/testCapNomParse.cp
// Unit tests for @cap/nom-parse (Zero-heap nom v7 parser combinator primitives)

import { test, expect_eq } from "cap:test";
import {
    ParseRes,
    IntRes,
    UintRes,
    tag,
    tagNoCase,
    take,
    takeUntil,
    isNot,
    isA,
    digit0,
    digit1,
    hexDigit1,
    alpha0,
    alpha1,
    alphanumeric0,
    alphanumeric1,
    space0,
    space1,
    multispace0,
    multispace1,
    lineEnding,
    notLineEnding,
    delimitedTag,
    precededTag,
    terminatedTag,
    separatedPairTag,
    parseU64,
    parseI64,
    parseHexU64,
} from "../../packages/@cap/nom-parse/src/index.cp";

fn strEq(a: string, b: string): bool {
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
    test("1. @cap/nom-parse — tag, tagNoCase (@cap/unicase), take, takeUntil", () => {
        const r1 = tag("HTTP/1.1 200 OK", "HTTP/");
        expect_eq(r1.ok, true);
        expect_eq(strEq(r1.matched, "HTTP/"), true);
        expect_eq(strEq(r1.rest, "1.1 200 OK"), true);

        const r2 = tagNoCase("Content-Type: text/plain", "content-type:");
        expect_eq(r2.ok, true);
        expect_eq(strEq(r2.matched, "Content-Type:"), true);
        expect_eq(strEq(r2.rest, " text/plain"), true);

        const r3 = take("abcdef", 3);
        expect_eq(r3.ok, true);
        expect_eq(strEq(r3.matched, "abc"), true);
        expect_eq(strEq(r3.rest, "def"), true);
        expect_eq(take("ab", 5).ok, false);

        const r4 = takeUntil("key=value;more", ";");
        expect_eq(r4.ok, true);
        expect_eq(strEq(r4.matched, "key=value"), true);
        expect_eq(strEq(r4.rest, ";more"), true);
        expect_eq(takeUntil("no_semi", ";").ok, false);
    });

    test("2. @cap/nom-parse — isNot & isA byte-set combinators", () => {
        const r1 = isNot("hello,world", ",;");
        expect_eq(r1.ok, true);
        expect_eq(strEq(r1.matched, "hello"), true);
        expect_eq(strEq(r1.rest, ",world"), true);
        expect_eq(isNot(",world", ",").ok, false);

        const r2 = isA("10110abc", "01");
        expect_eq(r2.ok, true);
        expect_eq(strEq(r2.matched, "10110"), true);
        expect_eq(strEq(r2.rest, "abc"), true);
        expect_eq(isA("abc", "01").ok, false);
    });

    test("3. @cap/nom-parse — character class combinators (digit/hex/alpha/alnum/space/line)", () => {
        expect_eq(strEq(digit0("abc").matched, ""), true);
        expect_eq(digit1("abc").ok, false);
        expect_eq(strEq(digit1("1234xyz").matched, "1234"), true);

        const hx = hexDigit1("DeadBeef_tail");
        expect_eq(hx.ok, true);
        expect_eq(strEq(hx.matched, "DeadBeef"), true);
        expect_eq(strEq(hx.rest, "_tail"), true);

        expect_eq(strEq(alpha1("Hello123").matched, "Hello"), true);
        expect_eq(strEq(alpha0("123").matched, ""), true);
        expect_eq(strEq(alphanumeric1("abc99!").matched, "abc99"), true);
        expect_eq(strEq(alphanumeric0("!").matched, ""), true);

        expect_eq(strEq(space1(" \t hi").matched, " \t "), true);
        expect_eq(space1("\nhi").ok, false);
        expect_eq(strEq(space0("hi").matched, ""), true);

        expect_eq(strEq(multispace1(" \r\n\tOK").rest, "OK"), true);
        expect_eq(strEq(multispace0("OK").rest, "OK"), true);

        const ln = notLineEnding("first line\r\nsecond");
        expect_eq(strEq(ln.matched, "first line"), true);
        const le = lineEnding(ln.rest);
        expect_eq(le.ok, true);
        expect_eq(strEq(le.matched, "\r\n"), true);
        expect_eq(strEq(le.rest, "second"), true);
    });

    test("4. @cap/nom-parse — sequence combinators (delimited/preceded/terminated/separatedPair)", () => {
        const d = delimitedTag("[[payload]]tail", "[[", "]]");
        expect_eq(d.ok, true);
        expect_eq(strEq(d.matched, "payload"), true);
        expect_eq(strEq(d.rest, "tail"), true);
        expect_eq(delimitedTag("[[unclosed", "[[", "]]").ok, false);

        const p = precededTag("Bearer tok_123", "Bearer ");
        expect_eq(p.ok, true);
        expect_eq(strEq(p.matched, "tok_123"), true);

        const t = terminatedTag("stmt;next", ";");
        expect_eq(t.ok, true);
        expect_eq(strEq(t.matched, "stmt"), true);
        expect_eq(strEq(t.rest, "next"), true);

        const kv = separatedPairTag("Host: example.com", ": ");
        expect_eq(kv.ok, true);
        expect_eq(strEq(kv.matched, "Host"), true);
        expect_eq(strEq(kv.rest, "example.com"), true);
    });

    test("5. @cap/nom-parse — numeric parsers (parseU64, parseI64, parseHexU64 & overflow)", () => {
        const u = parseU64("18446744073709551615!");
        const expected_u64_max: u64 = ((9223372036854775807 as u64) << (1 as u6)) | (1 as u64);
        expect_eq(u.ok, true);
        expect_eq(u.value, expected_u64_max);
        expect_eq(strEq(u.rest, "!"), true);
        expect_eq(parseU64("18446744073709551616").ok, false); // u64 overflow

        const i_pos = parseI64("+42ms");
        expect_eq(i_pos.ok, true);
        expect_eq(i_pos.value, 42 as i64);
        expect_eq(strEq(i_pos.rest, "ms"), true);

        const i_min = parseI64("-9223372036854775808");
        expect_eq(i_min.ok, true);
        expect_eq(i_min.value, (-9223372036854775807 as i64) - (1 as i64));
        expect_eq(parseI64("-9223372036854775809").ok, false); // i64 underflow

        const h1 = parseHexU64("0xCAFE_end");
        expect_eq(h1.ok, true);
        expect_eq(h1.value, 0xCAFE as u64);
        expect_eq(strEq(h1.rest, "_end"), true);

        const h2 = parseHexU64("ff00z");
        expect_eq(h2.ok, true);
        expect_eq(h2.value, 0xFF00 as u64);
        expect_eq(strEq(h2.rest, "z"), true);
    });

    test("6. @cap/nom-parse — chained HTTP request-line parsing pipeline", () => {
        const req = "get /api/v2/items HTTP/1.1\r\nHost: cap.dev\r\n";
        const m = tagNoCase(req, "GET");
        expect_eq(m.ok, true);
        const s1 = space1(m.rest);
        expect_eq(s1.ok, true);
        const path = isNot(s1.rest, " \r\n");
        expect_eq(path.ok, true);
        expect_eq(strEq(path.matched, "/api/v2/items"), true);
        const s2 = space1(path.rest);
        const ver_prefix = tag(s2.rest, "HTTP/");
        expect_eq(ver_prefix.ok, true);
        const major = parseU64(ver_prefix.rest);
        expect_eq(major.ok, true);
        expect_eq(major.value, 1 as u64);
        const dot = tag(major.rest, ".");
        const minor = parseU64(dot.rest);
        expect_eq(minor.ok, true);
        expect_eq(minor.value, 1 as u64);
        const crlf = lineEnding(minor.rest);
        expect_eq(crlf.ok, true);
        const hdr = separatedPairTag(notLineEnding(crlf.rest).matched, ": ");
        expect_eq(hdr.ok, true);
        expect_eq(strEq(hdr.matched, "Host"), true);
        expect_eq(strEq(hdr.rest, "cap.dev"), true);
    });
}
