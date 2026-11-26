// cap/test/testCapCookieJar.cp
// Comprehensive unit tests for @cap/cookie-jar

import { test, expect_eq } from "cap:test";
import {
    CookieEntry,
    domainMatches,
    pathMatches,
    defaultPath,
    parseSetCookie,
    shouldSend,
    buildCookieHeader,
} from "../../packages/@cap/cookie-jar/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn emptyCookie(): CookieEntry {
    return CookieEntry{
        name: "",
        value: "",
        domain: "",
        path: "",
        max_age: 0 as i64,
        has_max_age: false,
        secure: false,
        http_only: false,
        same_site: 0 as u8,
    };
}

fn main() {
    test("1. @cap/cookie-jar — domainMatches blocks spoofed sibling domains", () => {
        expect_eq(domainMatches("example.com", "example.com"), true);
        expect_eq(domainMatches("API.EXAMPLE.COM", ".example.com"), true);
        expect_eq(domainMatches("sub.api.example.com", "example.com"), true);
        // Must reject spoofed suffix without dot boundary.
        expect_eq(domainMatches("evil-example.com", "example.com"), false);
        expect_eq(domainMatches("example.com", "sub.example.com"), false);
        expect_eq(domainMatches("127.0.0.1", "0.0.1"), false);
    });

    test("2. @cap/cookie-jar — pathMatches & defaultPath per RFC 6265", () => {
        expect_eq(pathMatches("/api", "/api"), true);
        expect_eq(pathMatches("/api/v1/users", "/api"), true);
        expect_eq(pathMatches("/api/v1/users", "/api/"), true);
        expect_eq(pathMatches("/apiv2", "/api"), false);
        expect_eq(pathMatches("/other", "/"), true);

        expect_eq(sliceEq(defaultPath("/a/b/c"), "/a/b"), true);
        expect_eq(sliceEq(defaultPath("/a/b/"), "/a/b"), true);
        expect_eq(sliceEq(defaultPath("/a"), "/"), true);
        expect_eq(sliceEq(defaultPath(""), "/"), true);
    });

    test("3. @cap/cookie-jar — parseSetCookie full attribute extraction", () => {
        let c = emptyCookie();
        const hdr = "session=tok_99; Domain=.example.com; Path=/app; Max-Age=3600; Secure; HttpOnly; SameSite=Strict";
        expect_eq(parseSetCookie(hdr, "web.example.com", "/app/home", &c), true);
        expect_eq(sliceEq(c.name, "session"), true);
        expect_eq(sliceEq(c.value, "tok_99"), true);
        expect_eq(sliceEq(c.domain, "example.com"), true);
        expect_eq(sliceEq(c.path, "/app"), true);
        expect_eq(c.has_max_age, true);
        expect_eq(c.max_age, 3600 as i64);
        expect_eq(c.secure, true);
        expect_eq(c.http_only, true);
        expect_eq(c.same_site, 2 as u8);

        // Default domain & defaultPath when omitted.
        let c2 = emptyCookie();
        expect_eq(parseSetCookie("theme=dark; SameSite=Lax", "docs.example.com", "/v1/guide/intro", &c2), true);
        expect_eq(sliceEq(c2.domain, "docs.example.com"), true);
        expect_eq(sliceEq(c2.path, "/v1/guide"), true);
        expect_eq(c2.same_site, 1 as u8);
    });

    test("4. @cap/cookie-jar — shouldSend enforces HTTPS & rejects expired Max-Age", () => {
        let sec = emptyCookie();
        * = parseSetCookie("auth=1; Secure; Max-Age=60; Path=/", "example.com", "/", &sec);
        expect_eq(shouldSend(sec, "example.com", "/dash", true), true);
        expect_eq(shouldSend(sec, "example.com", "/dash", false), false);

        let expired = emptyCookie();
        * = parseSetCookie("old=0; Max-Age=0; Path=/", "example.com", "/", &expired);
        expect_eq(shouldSend(expired, "example.com", "/", true), false);

        let negAge = emptyCookie();
        * = parseSetCookie("old2=0; Max-Age=-10; Path=/", "example.com", "/", &negAge);
        expect_eq(shouldSend(negAge, "example.com", "/", true), false);
    });

    test("5. @cap/cookie-jar — buildCookieHeader filters & joins multiple cookies", () => {
        let jar: [4]CookieEntry = undefined;
        jar[0] = emptyCookie();
        jar[1] = emptyCookie();
        jar[2] = emptyCookie();
        jar[3] = emptyCookie();

        * = parseSetCookie("a=1; Domain=example.com; Path=/", "example.com", "/", &jar[0]);
        * = parseSetCookie("b=2; Domain=.example.com; Path=/api; Secure", "api.example.com", "/api", &jar[1]);
        * = parseSetCookie("other=3; Domain=other.org; Path=/", "other.org", "/", &jar[2]);
        * = parseSetCookie("dead=4; Domain=example.com; Path=/; Max-Age=0", "example.com", "/", &jar[3]);

        let hdrBuf: [128]u8 = undefined;
        const nHttps = buildCookieHeader(jar[0..4], "api.example.com", "/api/v1", true, hdrBuf[0..128]);
        expect_eq(sliceEq(hdrBuf[0..nHttps], "a=1; b=2"), true);

        // Over HTTP, Secure cookie 'b' is excluded.
        const nHttp = buildCookieHeader(jar[0..4], "api.example.com", "/api/v1", false, hdrBuf[0..128]);
        expect_eq(sliceEq(hdrBuf[0..nHttp], "a=1"), true);
    });

    test("6. @cap/cookie-jar — Malformed Set-Cookie & small output buffer protection", () => {
        let c = emptyCookie();
        expect_eq(parseSetCookie("", "example.com", "/", &c), false);
        expect_eq(parseSetCookie("noequalsign", "example.com", "/", &c), false);
        expect_eq(parseSetCookie("=emptyname", "example.com", "/", &c), false);

        let jar: [2]CookieEntry = undefined;
        jar[0] = emptyCookie();
        jar[1] = emptyCookie();
        * = parseSetCookie("k1=v1; Path=/", "example.com", "/", &jar[0]);
        * = parseSetCookie("k2=longvalue; Path=/", "example.com", "/", &jar[1]);

        // Buffer only fits "k1=v1" (5 bytes), must not write partial "; k2=...".
        let tiny: [7]u8 = undefined;
        const n = buildCookieHeader(jar[0..2], "example.com", "/", true, tiny[0..7]);
        expect_eq(n, 5 as usize);
        expect_eq(sliceEq(tiny[0..n], "k1=v1"), true);
    });
}
