import { test, expect_eq } from "cap:test";
import {
    DOTFILES_IGNORE,
    DOTFILES_ALLOW,
    DOTFILES_DENY,
    SendOptions,
    SendPlan,
    planSendFile,
    formatRedirectUrl
} from "../../packages/send/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn tStrEq(a: string, b: string): bool {
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

fn main() {
    test("1. @cap/send — rejects path traversal and NULL bytes with 403", () => {
        let opts = SendOptions.defaults("/var/www");
        let etag_buf: [64]u8 = undefined;

        let p1: SendPlan = planSendFile("/assets/../../etc/passwd", 100 as u64, 1000 as u64, "", "", opts, etag_buf[0..64]);
        expect_eq(p1.status, 403 as u16);

        let p2: SendPlan = planSendFile("/index.html\x00.txt", 100 as u64, 1000 as u64, "", "", opts, etag_buf[0..64]);
        expect_eq(p2.status, 403 as u16);
    });

    test("2. @cap/send — enforces dotfiles ignore (404), deny (403), and allow (200)", () => {
        let etag_buf: [64]u8 = undefined;

        let ignore_opts = SendOptions.defaults("/var/www");
        let p_ign = planSendFile("/.env", 64 as u64, 5000 as u64, "", "", ignore_opts, etag_buf[0..64]);
        expect_eq(p_ign.status, 404 as u16);

        let deny_opts = SendOptions {
            root: "/var/www",
            dotfiles: DOTFILES_DENY,
            etag: true,
            accept_ranges: true,
            max_age_secs: 0 as u64,
            immutable: false
        };
        let p_deny = planSendFile("/sub/.git/config", 64 as u64, 5000 as u64, "", "", deny_opts, etag_buf[0..64]);
        expect_eq(p_deny.status, 403 as u16);

        let allow_opts = SendOptions {
            root: "/var/www",
            dotfiles: DOTFILES_ALLOW,
            etag: true,
            accept_ranges: true,
            max_age_secs: 0 as u64,
            immutable: false
        };
        let p_allow = planSendFile("/.well-known/security.txt", 32 as u64, 5000 as u64, "", "", allow_opts, etag_buf[0..64]);
        expect_eq(p_allow.status, 200 as u16);
        expect_eq(tStrEq(p_allow.content_type, "text/plain"), true);
    });

    test("3. @cap/send — derives MIME type and returns 200 full response", () => {
        let opts = SendOptions.defaults("/public");
        let etag_buf: [64]u8 = undefined;

        let p1 = planSendFile("/app/bundle.js", 2048 as u64, 1700000000 as u64, "", "", opts, etag_buf[0..64]);
        expect_eq(p1.status, 200 as u16);
        expect_eq(p1.is_partial, false);
        expect_eq(p1.content_length, 2048 as u64);
        expect_eq(p1.range_start, 0 as u64);
        expect_eq(p1.range_end, 2047 as u64);
        expect_eq(tStrEq(p1.content_type, "application/javascript") || tStrEq(p1.content_type, "text/javascript"), true);

        let p2 = planSendFile("/data/blob.unknownext", 16 as u64, 100 as u64, "", "", opts, etag_buf[0..64]);
        expect_eq(p2.status, 200 as u16);
        expect_eq(tStrEq(p2.content_type, "application/octet-stream"), true);
    });

    test("4. @cap/send — generates ETag and negotiates 304 Not Modified via If-None-Match", () => {
        let opts = SendOptions.defaults("/public");
        let etag_buf: [64]u8 = undefined;

        let first = planSendFile("/index.html", 512 as u64, 99999 as u64, "", "", opts, etag_buf[0..64]);
        expect_eq(first.status, 200 as u16);

        // Compute generated ETag length by scanning up to closing quote
        let etag_len: usize = 0;
        let quotes: usize = 0;
        while (etag_len < 64 && quotes < 2) {
            if (etag_buf[etag_len] == ('"' as u8)) {
                quotes = quotes + 1;
            }
            etag_len = etag_len + 1;
        }
        expect_eq(etag_len > 2, true);

        let etag_buf2: [64]u8 = undefined;
        let second = planSendFile("/index.html", 512 as u64, 99999 as u64, "", etag_buf[0..etag_len], opts, etag_buf2[0..64]);
        expect_eq(second.status, 304 as u16);
        expect_eq(second.content_length, 0 as u64);
    });

    test("5. @cap/send — parses Range into 206 Partial Content and rejects invalid range with 416", () => {
        let opts = SendOptions.defaults("/media");
        let etag_buf: [64]u8 = undefined;

        let p_ok = planSendFile("/video.mp4", 1000 as u64, 12345 as u64, "bytes=100-199", "", opts, etag_buf[0..64]);
        expect_eq(p_ok.status, 206 as u16);
        expect_eq(p_ok.is_partial, true);
        expect_eq(p_ok.range_start, 100 as u64);
        expect_eq(p_ok.range_end, 199 as u64);
        expect_eq(p_ok.content_length, 100 as u64);

        let p_oob = planSendFile("/video.mp4", 1000 as u64, 12345 as u64, "bytes=5000-6000", "", opts, etag_buf[0..64]);
        expect_eq(p_oob.status, 416 as u16);
        expect_eq(p_oob.is_partial, false);
    });

    test("6. @cap/send — formatRedirectUrl encodes spaces, appends trailing slash, and prevents open redirects", () => {
        let out: [128]u8 = undefined;

        let n1 = formatRedirectUrl("/my docs/guide", out[0..128]);
        expect_eq(eqBytes(out[0..128], n1, "/my%20docs/guide/"), true);

        let n2 = formatRedirectUrl("///evil.example/path", out[0..128]);
        expect_eq(eqBytes(out[0..128], n2, "/evil.example/path/"), true);

        let n3 = formatRedirectUrl("/already/", out[0..128]);
        expect_eq(eqBytes(out[0..128], n3, "/already/"), true);
    });
}
