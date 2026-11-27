// cap/test/testCapQsLite.cp
// Comprehensive unit tests for @cap/qs (flat query string, encoding, builder & options)

import { test, expect_eq } from "cap:test";
import {
    QsParseOptions,
    QsParsed,
    QsBuilder,
    percentDecode,
    percentEncode,
} from "../../packages/qs/src/index.cp";

fn tStrEq(a: []const u8, b: string): bool {
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
    test("1. @cap/qs — parseQuery basic flat key-value & query prefix", () => {
        let parsed = QsParsed.parse("?a=1&b=hello+world&c=3", QsParseOptions{}) catch unreachable;
        expect_eq(parsed.len(), 3 as usize);
        expect_eq(tStrEq(parsed.get("a").?, "1"), true);
        expect_eq(tStrEq(parsed.get("b").?, "hello world"), true);
        expect_eq(tStrEq(parsed.get("c").?, "3"), true);
        expect_eq(parsed.get("missing") == null, true);
    });

    test("2. @cap/qs — percentEncode & percentDecode round-trip", () => {
        let enc: [64]u8 = undefined;
        let encoded = percentEncode("hello world/a+b=c", false, enc[0..64]) catch unreachable;
        expect_eq(tStrEq(encoded, "hello%20world%2Fa%2Bb%3Dc"), true);

        let dec_buf: [64]u8 = undefined;
        let decoded = percentDecode(encoded, false, dec_buf[0..64]) catch unreachable;
        expect_eq(tStrEq(decoded, "hello world/a+b=c"), true);
    });

    test("3. @cap/qs — QsBuilder serialization", () => {
        let out: [128]u8 = undefined;
        let b = QsBuilder.init(out[0..128]);
        b.add("lang", "cp") catch unreachable;
        b.add("q", "a b") catch unreachable;
        expect_eq(tStrEq(b.written(), "lang=cp&q=a%20b"), true);
    });

    test("4. @cap/qs — plus_as_space=false preserves literal '+'", () => {
        let parsed = QsParsed.parse("k=1+2&empty=", QsParseOptions{ plus_as_space: false }) catch unreachable;
        expect_eq(parsed.len(), 2 as usize);
        expect_eq(tStrEq(parsed.get("k").?, "1+2"), true);
        expect_eq(tStrEq(parsed.get("empty").?, ""), true);
    });
}
