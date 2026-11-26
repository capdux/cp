// cap/test/testCapQuerystring.cp
// Comprehensive unit tests for @cap/querystring (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    QueryEntry,
    escapeQuery,
    unescapeQuery,
    extractUrlQuery,
    parseQueryString,
    stringifyQueryString,
    pickQueryParam,
} from "../../packages/@cap/querystring/src/index.cp";

fn main() {
    test("1. @cap/querystring — escapeQuery & unescapeQuery via @cap/percent-encoding", () => {
        let buf: [128]u8 = undefined;

        const n1 = escapeQuery("hello world&a=b/c", buf[0..128]);
        expect_eq(buf[0..n1], "hello%20world%26a%3Db%2Fc");

        const n2 = unescapeQuery("hello+world%26a%3Db%2Fc%20%E4%B8%AD%E6%96%87", buf[0..128]);
        expect_eq(buf[0..n2], "hello world&a=b/c 中文");
    });

    test("2. @cap/querystring — parseQueryString skips leading '?', truncates '#hash', handles bare flags", () => {
        let entries: [8]QueryEntry = undefined;
        const n = parseQueryString("?debug&v=1&lang=cp#section-2", 0, 0, entries[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(entries[0].key, "debug");
        expect_eq(entries[0].value, "");
        expect_eq(entries[1].key, "v");
        expect_eq(entries[1].value, "1");
        expect_eq(entries[2].key, "lang");
        expect_eq(entries[2].value, "cp");
    });

    test("3. @cap/querystring — parseQueryString & stringifyQueryString with custom sep and eq", () => {
        let entries: [8]QueryEntry = undefined;
        const n = parseQueryString("k1:v1;k2:v2;;flag", 0x3B as u8, 0x3A as u8, entries[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(entries[0].key, "k1");
        expect_eq(entries[0].value, "v1");
        expect_eq(entries[1].key, "k2");
        expect_eq(entries[1].value, "v2");
        expect_eq(entries[2].key, "flag");
        expect_eq(entries[2].value, "");

        const outEntries: [2]QueryEntry = [
            QueryEntry { key: "user name", value: "a&b" },
            QueryEntry { key: "role", value: "admin/root" },
        ];
        let out: [128]u8 = undefined;
        const w1 = stringifyQueryString(outEntries[0..2], 0, 0, out[0..128]);
        expect_eq(out[0..w1], "user%20name=a%26b&role=admin%2Froot");

        const w2 = stringifyQueryString(outEntries[0..2], 0x3B as u8, 0x3A as u8, out[0..128]);
        expect_eq(out[0..w2], "user%20name:a%26b;role:admin%2Froot");
    });

    test("4. @cap/querystring — extractUrlQuery extracts substring between '?' and '#'", () => {
        expect_eq(extractUrlQuery("https://example.com/path?a=1&b=2#top"), "a=1&b=2");
        expect_eq(extractUrlQuery("https://example.com/path?only=yes"), "only=yes");
        expect_eq(extractUrlQuery("https://example.com/path#hash"), "");
        expect_eq(extractUrlQuery("https://example.com/path"), "");
        expect_eq(extractUrlQuery("?x=9#end"), "x=9");
    });

    test("5. @cap/querystring — pickQueryParam extracts and decodes values from URLs & raw queries", () => {
        let out: [128]u8 = undefined;

        const r1 = pickQueryParam("https://cp-lang.org/search?q=hello+world%21&page=2#results", "q", out[0..128]);
        expect_eq(r1 != null, true);
        if (n := r1) {
            expect_eq(out[0..n], "hello world!");
        }

        const r2 = pickQueryParam("?first%20name=Ada+Lovelace&lang=CP", "first name", out[0..128]);
        expect_eq(r2 != null, true);
        if (n := r2) {
            expect_eq(out[0..n], "Ada Lovelace");
        }

        const r3 = pickQueryParam("https://cp-lang.org/search?q=1", "missing", out[0..128]);
        expect_eq(r3 == null, true);
    });

    test("6. @cap/querystring — empty inputs & small output buffer bounds safety", () => {
        let entries: [2]QueryEntry = undefined;
        expect_eq(parseQueryString("", 0, 0, entries[0..2]), 0 as usize);
        expect_eq(parseQueryString("#only-hash", 0, 0, entries[0..2]), 0 as usize);
        expect_eq(parseQueryString("a=1&b=2&c=3", 0, 0, entries[0..2]), 2 as usize);

        let tiny: [6]u8 = undefined;
        const pair: [1]QueryEntry = [QueryEntry { key: "alpha", value: "beta" }];
        const w = stringifyQueryString(pair[0..1], 0, 0, tiny[0..6]);
        expect_eq(w, 6 as usize);
        expect_eq(tiny[0..w], "alpha=");
    });
}
