// cap/test/testCapCookieSign.cp
// Comprehensive unit tests for @cap/cookie-sign (RFC 6265 + HMAC-SHA256)

import { test, expect_eq } from "cap:test";
import {
    SAME_SITE_STRICT,
    SAME_SITE_LAX,
    CookieSlice,
    CookieOptions,
    getCookie,
    sign,
    unsign,
    formatSetCookie,
} from "../../packages/@cap/cookie-sign/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/cookie-sign — getCookie multi-field, whitespace, quotes & miss", () => {
        const hdr = "a=1;  session=\"abc_123\" ; theme=dark";

        const r1: CookieSlice = getCookie(hdr, "a");
        expect_eq(r1.found, true);
        expect_eq(sliceEq(hdr[r1.start..(r1.start + r1.len)], "1"), true);

        const r2: CookieSlice = getCookie(hdr, "session");
        expect_eq(r2.found, true);
        expect_eq(sliceEq(hdr[r2.start..(r2.start + r2.len)], "abc_123"), true);

        const r3: CookieSlice = getCookie(hdr, "theme");
        expect_eq(r3.found, true);
        expect_eq(sliceEq(hdr[r3.start..(r3.start + r3.len)], "dark"), true);

        const miss: CookieSlice = getCookie(hdr, "missing");
        expect_eq(miss.found, false);
    });

    test("2. @cap/cookie-sign — sign & unsign round-trip", () => {
        let buf: [128]u8 = undefined;
        const n: i64 = sign("user_42", "super-secret-key", buf[0..128]);
        // "user_42" (7) + "." (1) + 43 = 51 bytes
        expect_eq(n, 51 as i64);

        const verified: CookieSlice = unsign(buf[0..(n as usize)], "super-secret-key");
        expect_eq(verified.found, true);
        expect_eq(sliceEq(buf[verified.start..(verified.start + verified.len)], "user_42"), true);
    });

    test("3. @cap/cookie-sign — rejects single-char value or signature modification", () => {
        let buf: [128]u8 = undefined;
        const n: i64 = sign("session_token", "secret-1", buf[0..128]);
        expect_eq(n > 0, true);
        const total: usize = n as usize;

        // Flip 1 character in payload
        buf[0] = 'X' as u8;
        expect_eq(unsign(buf[0..total], "secret-1").found, false);

        // Restore payload, flip 1 character in signature
        buf[0] = 's' as u8;
        const orig_sig_byte: u8 = buf[total - 1];
        if (orig_sig_byte == 'A' as u8) {
            buf[total - 1] = 'B' as u8;
        } else {
            buf[total - 1] = 'A' as u8;
        }
        expect_eq(unsign(buf[0..total], "secret-1").found, false);
    });

    test("4. @cap/cookie-sign — rejects wrong secret & missing dot separator", () => {
        let buf: [128]u8 = undefined;
        const n: i64 = sign("payload", "key-A", buf[0..128]);
        expect_eq(n > 0, true);

        expect_eq(unsign(buf[0..(n as usize)], "key-B").found, false);
        expect_eq(unsign("nosignaturehere", "key-A").found, false);
        expect_eq(unsign("short.sig", "key-A").found, false);
    });

    test("5. @cap/cookie-sign — value containing multiple dots splits at the last dot", () => {
        const val = "v1.user.alice@example.com";
        let buf: [128]u8 = undefined;
        const n: i64 = sign(val, "dot-secret", buf[0..128]);
        expect_eq(n, (val.len + 1 + 43) as i64);

        const res: CookieSlice = unsign(buf[0..(n as usize)], "dot-secret");
        expect_eq(res.found, true);
        expect_eq(sliceEq(buf[res.start..(res.start + res.len)], val), true);
    });

    test("6. @cap/cookie-sign — formatSetCookie full attributes & tiny buffer rejection", () => {
        const opts = CookieOptions {
            max_age: 3600,
            path: "/",
            domain: "capdux.org",
            http_only: true,
            secure: true,
            same_site: SAME_SITE_STRICT,
        };
        let dst: [160]u8 = undefined;
        const n: i64 = formatSetCookie("sid", "tok123", opts, dst[0..160]);
        expect_eq(n > 0, true);
        expect_eq(
            sliceEq(
                dst[0..(n as usize)],
                "sid=tok123; Max-Age=3600; Domain=capdux.org; Path=/; HttpOnly; Secure; SameSite=Strict",
            ),
            true,
        );

        // Lax without max_age/domain
        const opts2 = CookieOptions {
            max_age: -1,
            path: "/app",
            domain: "",
            http_only: false,
            secure: false,
            same_site: SAME_SITE_LAX,
        };
        const n2: i64 = formatSetCookie("pref", "1", opts2, dst[0..160]);
        expect_eq(sliceEq(dst[0..(n2 as usize)], "pref=1; Path=/app; SameSite=Lax"), true);

        // Buffer overflow returns -1
        let tiny: [10]u8 = undefined;
        expect_eq(formatSetCookie("sid", "tok123", opts, tiny[0..10]), -1 as i64);
        expect_eq(sign("user_42", "secret", tiny[0..10]), -1 as i64);
    });
}
