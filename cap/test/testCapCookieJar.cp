import { test, expect_eq } from "cap:test";
import {
    CookieJar, parseSetCookie, serializeSetCookie, getCookieHeaderValue,
    domainMatches, pathMatches, SAMESITE_STRICT, SAMESITE_LAX
} from "../../packages/tough-cookie/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/cookie-jar — parseSetCookie with all RFC 6265 attributes", () => {
        let c = parseSetCookie("sid=abc123; Domain=.Example.COM; Path=/api; Max-Age=3600; Secure; HttpOnly; SameSite=Strict");
        expect_eq(c.name, "sid");
        expect_eq(c.value, "abc123");
        expect_eq(c.domain, "Example.COM");
        expect_eq(c.path, "/api");
        expect_eq(c.has_max_age, true);
        expect_eq(c.max_age, 3600);
        expect_eq(c.secure, true);
        expect_eq(c.http_only, true);
        expect_eq(c.same_site, SAMESITE_STRICT);
    });

    test("2. @cap/cookie-jar — serializeSetCookie round-trip", () => {
        let c = parseSetCookie("theme=dark; Path=/; Max-Age=86400; SameSite=Lax");
        let out: [128]u8 = undefined;
        let n = serializeSetCookie(c, out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "theme=dark; Path=/; Max-Age=86400; SameSite=Lax"), true);
        expect_eq(c.same_site, SAMESITE_LAX);
    });

    test("3. @cap/cookie-jar — getCookieHeaderValue with quoted values", () => {
        let hdr = "a=1; token=\"hello-world\"; lang=zh";
        expect_eq(getCookieHeaderValue(hdr, "a") ?? "", "1");
        expect_eq(getCookieHeaderValue(hdr, "token") ?? "", "hello-world");
        expect_eq(getCookieHeaderValue(hdr, "lang") ?? "", "zh");
        expect_eq(getCookieHeaderValue(hdr, "missing") == null, true);
    });

    test("4. @cap/cookie-jar — RFC 6265 domainMatches & pathMatches", () => {
        expect_eq(domainMatches("api.example.com", "EXAMPLE.COM"), true);
        expect_eq(domainMatches("example.com", "example.com"), true);
        expect_eq(domainMatches("badexample.com", "example.com"), false);
        expect_eq(pathMatches("/api/v1/users", "/api"), true);
        expect_eq(pathMatches("/apiv2", "/api"), false);
        expect_eq(pathMatches("/any", "/"), true);
    });

    test("5. @cap/cookie-jar — CookieJar filtering by host, path & HTTPS Secure flag", () => {
        let jar = CookieJar.new();
        jar.setFromHeader("pub=1; Domain=example.com; Path=/");
        jar.setFromHeader("sec=2; Domain=example.com; Path=/api; Secure");
        jar.setFromHeader("other=3; Domain=other.org; Path=/");
        expect_eq(jar.size(), 3);

        let out: [128]u8 = undefined;
        let n_https = jar.buildCookieHeader("sub.example.com", "/api/items", true, out[0..128]);
        expect_eq(eqBuf(out[0..n_https], n_https, "pub=1; sec=2"), true);
        let n_http = jar.buildCookieHeader("sub.example.com", "/api/items", false, out[0..128]);
        expect_eq(eqBuf(out[0..n_http], n_http, "pub=1"), true);
    });

    test("6. @cap/cookie-jar — CookieJar overwrite & Max-Age=0 deletion", () => {
        let jar = CookieJar.new();
        jar.setFromHeader("k=old; Path=/");
        jar.setFromHeader("k=new; Path=/");
        expect_eq(jar.size(), 1);
        let out: [64]u8 = undefined;
        let n1 = jar.buildCookieHeader("localhost", "/", false, out[0..64]);
        expect_eq(eqBuf(out[0..n1], n1, "k=new"), true);

        jar.setFromHeader("k=; Path=/; Max-Age=0");
        expect_eq(jar.size(), 0);
        let n2 = jar.buildCookieHeader("localhost", "/", false, out[0..64]);
        expect_eq(n2, 0);
    });
}
