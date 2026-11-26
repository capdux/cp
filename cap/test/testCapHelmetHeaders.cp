import { test, expect_eq } from "cap:test";
import {
    HelmetOptions,
    formatHelmetHeaders,
    formatHstsHeader,
    formatCspHeader,
    isSafeHeaderValue,
} from "../../packages/@cap/helmet-headers/src/index.cp";

fn strEq(a: []const u8, b: string): bool {
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

fn contains(hay: []const u8, needle: string): bool {
    if (needle.len == 0 || needle.len > hay.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= hay.len) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j = j + 1;
        }
        if (ok) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn main() {
    test("1. @cap/helmet-headers — defaultStrict() emits all 10 security headers with CRLF", () => {
        let out: [2048]u8 = undefined;
        const opts = HelmetOptions.defaultStrict();
        const n = formatHelmetHeaders(opts, &out);
        const res = out[0..n];

        expect_eq(contains(res, "Content-Security-Policy: default-src 'self';"), true);
        expect_eq(contains(res, "Strict-Transport-Security: max-age=31536000; includeSubDomains\r\n"), true);
        expect_eq(contains(res, "X-Content-Type-Options: nosniff\r\n"), true);
        expect_eq(contains(res, "X-Frame-Options: DENY\r\n"), true);
        expect_eq(contains(res, "Referrer-Policy: no-referrer\r\n"), true);
        expect_eq(contains(res, "Cross-Origin-Opener-Policy: same-origin\r\n"), true);
        expect_eq(contains(res, "Cross-Origin-Resource-Policy: same-origin\r\n"), true);
        expect_eq(contains(res, "X-DNS-Prefetch-Control: off\r\n"), true);
        expect_eq(contains(res, "X-Permitted-Cross-Domain-Policies: none\r\n"), true);
        expect_eq(contains(res, "Origin-Agent-Cluster: ?1\r\n"), true);
    });

    test("2. @cap/helmet-headers — minimal() emits baseline headers only", () => {
        let out: [512]u8 = undefined;
        const opts = HelmetOptions.minimal();
        const n = formatHelmetHeaders(opts, &out);
        const expected = "X-Content-Type-Options: nosniff\r\nX-Frame-Options: SAMEORIGIN\r\nReferrer-Policy: strict-origin-when-cross-origin\r\n";
        expect_eq(strEq(out[0..n], expected), true);
    });

    test("3. @cap/helmet-headers — formatHstsHeader with max_age, includeSubDomains & preload", () => {
        let out: [128]u8 = undefined;

        const n1 = formatHstsHeader(63072000, true, true, &out);
        expect_eq(strEq(out[0..n1], "max-age=63072000; includeSubDomains; preload"), true);

        const n2 = formatHstsHeader(0, false, false, &out);
        expect_eq(strEq(out[0..n2], "max-age=0"), true);

        const n3 = formatHstsHeader(86400, false, true, &out);
        expect_eq(strEq(out[0..n3], "max-age=86400; preload"), true);
    });

    test("4. @cap/helmet-headers — formatCspHeader composes non-empty directives cleanly", () => {
        let out: [512]u8 = undefined;
        const n1 = formatCspHeader("'self'", "'self' https://cdn.example.com", "", "data: https:", "'self' wss:", &out);
        const expected = "default-src 'self'; script-src 'self' https://cdn.example.com; img-src data: https:; connect-src 'self' wss:";
        expect_eq(strEq(out[0..n1], expected), true);

        // Unsafe CRLF directive is silently dropped
        const n2 = formatCspHeader("'self'", "'self'\r\nEvil: 1", "'none'", "", "", &out);
        expect_eq(strEq(out[0..n2], "default-src 'self'; style-src 'none'"), true);
    });

    test("5. @cap/helmet-headers — isSafeHeaderValue blocks CRLF, NUL & control chars", () => {
        expect_eq(isSafeHeaderValue("nosniff"), true);
        expect_eq(isSafeHeaderValue("default-src 'self';\timg-src data:"), true);
        expect_eq(isSafeHeaderValue("DENY\r\nSet-Cookie: a=1"), false);
        expect_eq(isSafeHeaderValue("bad\nline"), false);
        expect_eq(isSafeHeaderValue("bad\rline"), false);
        expect_eq(isSafeHeaderValue("null\x00byte"), false);
        expect_eq(isSafeHeaderValue("del\x7fchar"), false);
    });

    test("6. @cap/helmet-headers — CRLF injection in custom options dropped & small buffer safe", () => {
        let opts = HelmetOptions.minimal();
        opts.frame_options = "DENY\r\nX-Injected: yes";
        opts.hsts_enabled = true;
        opts.hsts_max_age = 3600;
        opts.hsts_include_subdomains = false;
        opts.hsts_preload = false;

        let out: [512]u8 = undefined;
        const n = formatHelmetHeaders(opts, &out);
        const res = out[0..n];
        expect_eq(contains(res, "X-Injected"), false);
        expect_eq(contains(res, "X-Frame-Options"), false);
        expect_eq(contains(res, "Strict-Transport-Security: max-age=3600\r\n"), true);

        // Small buffer truncation never overflows
        let tiny: [12]u8 = undefined;
        const nt = formatHstsHeader(31536000, true, true, &tiny);
        expect_eq(nt, 12);
        expect_eq(strEq(tiny[0..nt], "max-age=3153"), true);
    });
}
