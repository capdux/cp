import { test, expect_eq } from "cap:test";
import {
    parseSetCookie,
    splitCookiesCount,
    splitCookiesGet,
} from "../../packages/@cap/set-cookie-parser/src/index.cp";

fn main() {
    test("1. @cap/set-cookie-parser — parses full cookie with all RFC 6265 attributes", () => {
        const c = parseSetCookie("session_id=abc123; Path=/app; Domain=.example.com; Max-Age=3600; Expires=Wed, 21 Oct 2026 07:28:00 GMT; SameSite=Strict; Secure; HttpOnly; Partitioned");
        expect_eq(c.name, "session_id");
        expect_eq(c.value, "abc123");
        expect_eq(c.path, "/app");
        expect_eq(c.domain, ".example.com");
        expect_eq(c.has_max_age, true);
        expect_eq(c.max_age, 3600 as i64);
        expect_eq(c.expires, "Wed, 21 Oct 2026 07:28:00 GMT");
        expect_eq(c.same_site, "Strict");
        expect_eq(c.secure, true);
        expect_eq(c.http_only, true);
        expect_eq(c.partitioned, true);
    });

    test("2. @cap/set-cookie-parser — unquotes quoted cookie values and preserves '=' in value", () => {
        const c1 = parseSetCookie("token=\"a=b=c\"; Path=/");
        expect_eq(c1.name, "token");
        expect_eq(c1.value, "a=b=c");
        expect_eq(c1.path, "/");
        expect_eq(c1.secure, false);
    });

    test("3. @cap/set-cookie-parser — case-insensitive attribute matching and negative Max-Age", () => {
        const c = parseSetCookie("k=v; max-age=-1; samesite=Lax; httponly; SECURE");
        expect_eq(c.has_max_age, true);
        expect_eq(c.max_age, -1 as i64);
        expect_eq(c.same_site, "Lax");
        expect_eq(c.http_only, true);
        expect_eq(c.secure, true);
    });

    test("4. @cap/set-cookie-parser — splits comma-joined Set-Cookie headers ignoring Expires comma", () => {
        const combined = "a=1; Expires=Wed, 21 Oct 2026 07:28:00 GMT; Path=/, b=2; HttpOnly, c=3; Secure";
        expect_eq(splitCookiesCount(combined), 3 as usize);
        expect_eq(splitCookiesGet(combined, 0) ?? "", "a=1; Expires=Wed, 21 Oct 2026 07:28:00 GMT; Path=/");
        expect_eq(splitCookiesGet(combined, 1) ?? "", "b=2; HttpOnly");
        expect_eq(splitCookiesGet(combined, 2) ?? "", "c=3; Secure");
        expect_eq(splitCookiesGet(combined, 3) == null, true);
    });

    test("5. @cap/set-cookie-parser — handles empty and minimal cookies", () => {
        const empty = parseSetCookie("   ");
        expect_eq(empty.name, "");
        expect_eq(empty.value, "");
        expect_eq(splitCookiesCount(""), 0 as usize);
        const simple = parseSetCookie("foo=bar");
        expect_eq(simple.name, "foo");
        expect_eq(simple.value, "bar");
        expect_eq(simple.has_max_age, false);
    });

    test("6. @cap/set-cookie-parser — ignores invalid Max-Age gracefully", () => {
        const c = parseSetCookie("x=1; Max-Age=abc; Path=/v1");
        expect_eq(c.has_max_age, false);
        expect_eq(c.max_age, 0 as i64);
        expect_eq(c.path, "/v1");
    });
}
