import { test, expect_eq } from "cap:test";
import {
    ParsedUrl,
    parseUrl,
    hasDotDotSegment,
    stripQueryAndHash,
} from "../../packages/@cap/parseurl/src/index.cp";

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
    test("1. @cap/parseurl — Plain relative path '/api/items' fast path", () => {
        const u: string = "/api/items";
        const p: ParsedUrl = parseUrl(u);
        expect_eq(p.valid, true);
        expect_eq(p.has_scheme, false);
        expect_eq(p.has_query, false);
        expect_eq(strEq(u[p.path_start .. p.path_start + p.path_len], "/api/items"), true);
        expect_eq(strEq(u[p.pathname_start .. p.pathname_start + p.pathname_len], "/api/items"), true);
        expect_eq(p.search_len, 0);
        expect_eq(p.query_len, 0);
        expect_eq(strEq(stripQueryAndHash(u), "/api/items"), true);
    });

    test("2. @cap/parseurl — Query string '/search?q=cp&lang=zh' extracts all spans", () => {
        const u: string = "/search?q=cp&lang=zh";
        const p: ParsedUrl = parseUrl(u);
        expect_eq(p.valid, true);
        expect_eq(p.has_query, true);
        expect_eq(strEq(u[p.path_start .. p.path_start + p.path_len], "/search?q=cp&lang=zh"), true);
        expect_eq(strEq(u[p.pathname_start .. p.pathname_start + p.pathname_len], "/search"), true);
        expect_eq(strEq(u[p.search_start .. p.search_start + p.search_len], "?q=cp&lang=zh"), true);
        expect_eq(strEq(u[p.query_start .. p.query_start + p.query_len], "q=cp&lang=zh"), true);
    });

    test("3. @cap/parseurl — Empty query '/a?' sets has_query=true, search='?', query=''", () => {
        const u: string = "/a?";
        const p: ParsedUrl = parseUrl(u);
        expect_eq(p.valid, true);
        expect_eq(p.has_query, true);
        expect_eq(strEq(u[p.pathname_start .. p.pathname_start + p.pathname_len], "/a"), true);
        expect_eq(strEq(u[p.search_start .. p.search_start + p.search_len], "?"), true);
        expect_eq(p.query_len, 0);
    });

    test("4. @cap/parseurl — Hash fragment '/a?b=1#top' is excluded from path/search/query", () => {
        const u: string = "/a?b=1#top?ignored";
        const p: ParsedUrl = parseUrl(u);
        expect_eq(p.valid, true);
        expect_eq(p.has_query, true);
        expect_eq(strEq(u[p.path_start .. p.path_start + p.path_len], "/a?b=1"), true);
        expect_eq(strEq(u[p.pathname_start .. p.pathname_start + p.pathname_len], "/a"), true);
        expect_eq(strEq(u[p.search_start .. p.search_start + p.search_len], "?b=1"), true);
        expect_eq(strEq(u[p.query_start .. p.query_start + p.query_len], "b=1"), true);

        const u_hash_only: string = "/docs/intro#section-2";
        const p2: ParsedUrl = parseUrl(u_hash_only);
        expect_eq(p2.valid, true);
        expect_eq(p2.has_query, false);
        expect_eq(strEq(stripQueryAndHash(u_hash_only), "/docs/intro"), true);
    });

    test("5. @cap/parseurl — Absolute URI strips scheme & authority cleanly", () => {
        const u: string = "https://host:8443/v2/users?id=1#profile";
        const p: ParsedUrl = parseUrl(u);
        expect_eq(p.valid, true);
        expect_eq(p.has_scheme, true);
        expect_eq(p.has_query, true);
        expect_eq(strEq(u[p.path_start .. p.path_start + p.path_len], "/v2/users?id=1"), true);
        expect_eq(strEq(u[p.pathname_start .. p.pathname_start + p.pathname_len], "/v2/users"), true);
        expect_eq(strEq(u[p.query_start .. p.query_start + p.query_len], "id=1"), true);
        expect_eq(strEq(stripQueryAndHash(u), "/v2/users"), true);

        // Origin-only URL with no trailing slash
        const u_origin: string = "http://example.com?k=v";
        const p_orig: ParsedUrl = parseUrl(u_origin);
        expect_eq(p_orig.valid, true);
        expect_eq(p_orig.has_scheme, true);
        expect_eq(p_orig.pathname_len, 0);
        expect_eq(strEq(u_origin[p_orig.query_start .. p_orig.query_start + p_orig.query_len], "k=v"), true);
    });

    test("6. @cap/parseurl — hasDotDotSegment & empty/control-char rejection", () => {
        expect_eq(hasDotDotSegment("/a/../b"), true);
        expect_eq(hasDotDotSegment("../etc/passwd"), true);
        expect_eq(hasDotDotSegment("/a/b/.."), true);
        expect_eq(hasDotDotSegment("/a/b..c/d"), false);
        expect_eq(hasDotDotSegment("/a/.../b"), false);
        expect_eq(hasDotDotSegment("/clean/path"), false);

        expect_eq(parseUrl("").valid, false);
        expect_eq(strEq(stripQueryAndHash(""), ""), true);

        let bad_ctrl: [8]u8 = [47, 97, 13, 10, 47, 98, 0, 0]; // "/a\r\n/b"
        expect_eq(parseUrl(bad_ctrl[0..6]).valid, false);
        expect_eq(strEq(stripQueryAndHash(bad_ctrl[0..6]), ""), true);
    });
}
