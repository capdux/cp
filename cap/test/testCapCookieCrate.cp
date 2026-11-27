import { test, expect_eq } from "cap:test";
import { Cookie } from "../../packages/tough-cookie/src/index.cp";
import {
    CookieBuilder,
    EncodedCookieMeta,
    parseEncodedCookie,
    formatEncodedCookie,
    makeRemovalCookie,
    COOKIE_SAMESITE_LAX,
    COOKIE_SAMESITE_STRICT,
    COOKIE_SAMESITE_NONE,
    COOKIE_SAMESITE_UNSET,
} from "../../packages/cookie/src/index.cp";

fn testEqBytes(a: []const u8, b: string): bool {
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
    test("1. @cap/cookie-crate — CookieBuilder fluent chaining sets all attributes", () => {
        let b = CookieBuilder.new("sid", "abc123");
        const c: Cookie = b
            .domain("example.com")
            .path("/api")
            .maxAge(3600)
            .secure(true)
            .httpOnly(true)
            .sameSite(COOKIE_SAMESITE_STRICT)
            .build();

        expect_eq(testEqBytes(c.name, "sid"), true);
        expect_eq(testEqBytes(c.value, "abc123"), true);
        expect_eq(testEqBytes(c.domain, "example.com"), true);
        expect_eq(testEqBytes(c.path, "/api"), true);
        expect_eq(c.max_age, 3600 as i64);
        expect_eq(c.has_max_age, true);
        expect_eq(c.secure, true);
        expect_eq(c.http_only, true);
        expect_eq(c.same_site, COOKIE_SAMESITE_STRICT);
    });

    test("2. @cap/cookie-crate — formatEncodedCookie percent-encodes value and serializes attributes", () => {
        let b = CookieBuilder.new("prefs", "theme=dark&lang=zh CN");
        const c: Cookie = b
            .path("/")
            .maxAge(86400)
            .secure(true)
            .sameSite(COOKIE_SAMESITE_LAX)
            .build();

        let out: [256]u8 = undefined;
        const n: usize = formatEncodedCookie(c, out[0..256]);
        expect_eq(
            testEqBytes(
                out[0..n],
                "prefs=theme%3Ddark%26lang%3Dzh%20CN; Path=/; Max-Age=86400; Secure; SameSite=Lax"
            ),
            true
        );
    });

    test("3. @cap/cookie-crate — parseEncodedCookie decodes percent-encoded name & value with attributes", () => {
        const hdr: string = "user%20id=hello%20world%2F2026%3D1; Domain=app.example.org; Path=/v1; Max-Age=120; Secure; HttpOnly; SameSite=None";
        let name_buf: [64]u8 = undefined;
        let val_buf: [128]u8 = undefined;

        const meta: EncodedCookieMeta = parseEncodedCookie(hdr, name_buf[0..64], val_buf[0..128]);
        expect_eq(meta.ok, true);
        expect_eq(testEqBytes(name_buf[0..meta.name_len], "user id"), true);
        expect_eq(testEqBytes(val_buf[0..meta.value_len], "hello world/2026=1"), true);
        expect_eq(testEqBytes(meta.domain, "app.example.org"), true);
        expect_eq(testEqBytes(meta.path, "/v1"), true);
        expect_eq(meta.has_max_age, true);
        expect_eq(meta.max_age, 120 as i64);
        expect_eq(meta.secure, true);
        expect_eq(meta.http_only, true);
        expect_eq(meta.same_site, COOKIE_SAMESITE_NONE);
    });

    test("4. @cap/cookie-crate — Round-trip formatEncodedCookie -> parseEncodedCookie", () => {
        let b = CookieBuilder.new("token", "a+b/c;d=e f%g");
        const c: Cookie = b.domain("cap.dev").path("/auth").maxAge(600).httpOnly(true).build();

        let wire: [256]u8 = undefined;
        const wn: usize = formatEncodedCookie(c, wire[0..256]);

        let nb: [64]u8 = undefined;
        let vb: [128]u8 = undefined;
        const meta: EncodedCookieMeta = parseEncodedCookie(wire[0..wn], nb[0..64], vb[0..128]);
        expect_eq(meta.ok, true);
        expect_eq(testEqBytes(nb[0..meta.name_len], "token"), true);
        expect_eq(testEqBytes(vb[0..meta.value_len], "a+b/c;d=e f%g"), true);
        expect_eq(testEqBytes(meta.domain, "cap.dev"), true);
        expect_eq(testEqBytes(meta.path, "/auth"), true);
        expect_eq(meta.max_age, 600 as i64);
        expect_eq(meta.http_only, true);
        expect_eq(meta.secure, false);
        expect_eq(meta.same_site, COOKIE_SAMESITE_UNSET);
    });

    test("5. @cap/cookie-crate — makeRemovalCookie generates standard Max-Age=0 deletion header", () => {
        let out1: [128]u8 = undefined;
        const n1: usize = makeRemovalCookie("session_id", "/admin", out1[0..128]);
        expect_eq(testEqBytes(out1[0..n1], "session_id=; Path=/admin; Max-Age=0"), true);

        let out2: [128]u8 = undefined;
        const n2: usize = makeRemovalCookie("legacy", "", out2[0..128]);
        expect_eq(testEqBytes(out2[0..n2], "legacy=; Path=/; Max-Age=0"), true);
    });

    test("6. @cap/cookie-crate — Invalid / empty Set-Cookie header returns ok=false safely", () => {
        let nb: [32]u8 = undefined;
        let vb: [32]u8 = undefined;
        const m1 = parseEncodedCookie("", nb[0..32], vb[0..32]);
        expect_eq(m1.ok, false);
        expect_eq(m1.name_len, 0 as usize);
        expect_eq(m1.value_len, 0 as usize);

        const m2 = parseEncodedCookie("=orphan_value; Path=/", nb[0..32], vb[0..32]);
        expect_eq(m2.ok, false);
    });
}
