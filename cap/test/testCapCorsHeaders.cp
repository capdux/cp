import { test, expect_eq } from "cap:test";
import {
    CorsOptions, isOriginAllowed, isMethodAllowed, evaluateCors, formatCorsHeaders
} from "../../packages/@cap/cors-headers/src/index.cp";

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
    test("1. @cap/cors-headers — Permissive default emits '*' without Vary", () => {
        let opts = CorsOptions.permissive();
        let d = evaluateCors(opts, "GET", "https://app.example.com", "", "");
        expect_eq(d.allowed, true);
        expect_eq(d.is_preflight, false);
        expect_eq(d.allow_origin, "*");
        expect_eq(d.vary_origin, false);
        let out: [256]u8 = undefined;
        let n = formatCorsHeaders(d, out[0..256]);
        expect_eq(eqBuf(out[0..n], n, "Access-Control-Allow-Origin: *\r\n"), true);
    });

    test("2. @cap/cors-headers — Credentials forces Origin reflection + Vary: Origin", () => {
        let opts = CorsOptions.permissive();
        opts.allow_credentials = true;
        opts.exposed_headers = "X-Request-Id";
        let d = evaluateCors(opts, "POST", "https://web.capdux.org", "", "");
        expect_eq(d.allowed, true);
        expect_eq(d.allow_origin, "https://web.capdux.org");
        expect_eq(d.vary_origin, true);
        expect_eq(d.allow_credentials, true);
        let out: [512]u8 = undefined;
        let n = formatCorsHeaders(d, out[0..512]);
        expect_eq(eqBuf(out[0..n], n, "Access-Control-Allow-Origin: https://web.capdux.org\r\nVary: Origin\r\nAccess-Control-Allow-Credentials: true\r\nAccess-Control-Expose-Headers: X-Request-Id\r\n"), true);
    });

    test("3. @cap/cors-headers — Wildcard origin whitelist via @cap/wildmatch", () => {
        let opts = CorsOptions.restrictive();
        opts.addOrigin("https://*.example.com");
        opts.addOrigin("http://localhost:*");
        expect_eq(isOriginAllowed(opts, "https://api.example.com"), true);
        expect_eq(isOriginAllowed(opts, "http://localhost:3000"), true);
        expect_eq(isOriginAllowed(opts, "https://evil.org"), false);
        expect_eq(isOriginAllowed(opts, ""), false);
    });

    test("4. @cap/cors-headers — Preflight OPTIONS with Max-Age & reflected headers", () => {
        let opts = CorsOptions.restrictive();
        opts.addOrigin("https://app.example.com");
        opts.allowed_methods = "GET, POST, PUT";
        opts.max_age = 600;
        opts.has_max_age = true;
        let d = evaluateCors(opts, "OPTIONS", "https://app.example.com", "put", "Authorization, Content-Type");
        expect_eq(d.allowed, true);
        expect_eq(d.is_preflight, true);
        expect_eq(d.allow_headers, "Authorization, Content-Type");
        let out: [512]u8 = undefined;
        let n = formatCorsHeaders(d, out[0..512]);
        expect_eq(eqBuf(out[0..n], n, "Access-Control-Allow-Origin: https://app.example.com\r\nVary: Origin\r\nAccess-Control-Allow-Methods: GET, POST, PUT\r\nAccess-Control-Allow-Headers: Authorization, Content-Type\r\nAccess-Control-Max-Age: 600\r\n"), true);
    });

    test("5. @cap/cors-headers — Disallowed preflight method is rejected", () => {
        let opts = CorsOptions.restrictive();
        opts.addOrigin("https://app.example.com");
        let d = evaluateCors(opts, "OPTIONS", "https://app.example.com", "DELETE", "");
        expect_eq(d.allowed, false);
        let out: [64]u8 = undefined;
        expect_eq(formatCorsHeaders(d, out[0..64]), 0);
    });

    test("6. @cap/cors-headers — isMethodAllowed case-insensitive helper", () => {
        expect_eq(isMethodAllowed("GET, POST, PATCH", "patch"), true);
        expect_eq(isMethodAllowed("GET, POST, PATCH", "DELETE"), false);
        expect_eq(isMethodAllowed("GET, POST", ""), false);
    });
}
