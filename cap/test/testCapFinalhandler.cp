import { test, expect_eq } from "cap:test";
import {
    normalizeErrorStatus, statusReason, formatErrorHtml,
    formatCannotMessage, buildFinalHttpResponse
} from "../../packages/finalhandler/src/index.cp";

fn containsStr(haystack: []const u8, len: usize, needle: string): bool {
    if (needle.len == 0) { return true; }
    if (needle.len > len) { return false; }
    let i: usize = 0;
    while (i + needle.len <= len) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) { ok = false; break; }
            j = j + 1;
        }
        if (ok) { return true; }
        i = i + 1;
    }
    return false;
}

fn main() {
    test("1. @cap/finalhandler — normalizeErrorStatus coerces non-4xx/5xx to 500", () => {
        expect_eq(normalizeErrorStatus(404), 404);
        expect_eq(normalizeErrorStatus(503), 503);
        expect_eq(normalizeErrorStatus(200), 500);
        expect_eq(normalizeErrorStatus(0), 500);
        expect_eq(normalizeErrorStatus(600), 500);
    });

    test("2. @cap/finalhandler — statusReason delegates to @cap/http-types", () => {
        expect_eq(statusReason(404), "Not Found");
        expect_eq(statusReason(500), "Internal Server Error");
        expect_eq(statusReason(418), "I'm a teapot");
    });

    test("3. @cap/finalhandler — formatErrorHtml escapes XSS payloads via @cap/escape-html", () => {
        let out: [256]u8 = undefined;
        let n = formatErrorHtml("Cannot GET /<script>alert(1)</script>", out[0..256]);
        expect_eq(containsStr(out[0..n], n, "&lt;script&gt;alert(1)&lt;/script&gt;"), true);
        expect_eq(containsStr(out[0..n], n, "<script>"), false);
    });

    test("4. @cap/finalhandler — formatCannotMessage builds Express 404 text", () => {
        let out: [64]u8 = undefined;
        let n = formatCannotMessage("POST", "/api/missing", out[0..64]);
        expect_eq(containsStr(out[0..n], n, "Cannot POST /api/missing"), true);
        expect_eq(n, 24);
    });

    test("5. @cap/finalhandler — buildFinalHttpResponse emits CSP, nosniff & HTML body", () => {
        let out: [1024]u8 = undefined;
        let res = buildFinalHttpResponse(404, "Cannot GET /nope", false, out[0..1024]);
        expect_eq(res.status, 404);
        expect_eq(res.body_len > 0, true);
        expect_eq(containsStr(out[0..res.total_len], res.total_len, "HTTP/1.1 404 Not Found\r\n"), true);
        expect_eq(containsStr(out[0..res.total_len], res.total_len, "Content-Security-Policy: default-src 'none'\r\n"), true);
        expect_eq(containsStr(out[0..res.total_len], res.total_len, "X-Content-Type-Options: nosniff\r\n"), true);
        expect_eq(containsStr(out[0..res.total_len], res.total_len, "<pre>Cannot GET /nope</pre>"), true);
    });

    test("6. @cap/finalhandler — HEAD request omits body while preserving Content-Length", () => {
        let out_get: [1024]u8 = undefined;
        let out_head: [1024]u8 = undefined;
        let r_get = buildFinalHttpResponse(500, "", false, out_get[0..1024]);
        let r_head = buildFinalHttpResponse(500, "", true, out_head[0..1024]);
        expect_eq(r_head.status, 500);
        expect_eq(r_head.body_len, r_get.body_len);
        expect_eq(r_head.total_len + r_head.body_len, r_get.total_len);
    });
}
