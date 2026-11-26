// @cap/finalhandler — Zero-heap HTTP final response & error handler modeled on npm `finalhandler` v2.
// Clean-room MIT implementation; reuses `@cap/escape-html`, `@cap/http-types`, and `@cap/itoa`.

import { escapeHtml } from "../../escape-html/src/index.cp";
import { StatusCode } from "../../http-types/src/index.cp";
import { formatU64 } from "../../itoa/src/index.cp";

pub struct FinalResponse {
    pub status: u16,
    pub body_len: usize,
    pub total_len: usize,
}

fn fhPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn fhPushU64(out: []u8, pos: usize, v: u64): usize {
    let tmp: [24]u8 = undefined;
    let n = formatU64(v, tmp[0..24]);
    let p = pos;
    let i: usize = 0;
    while (i < n && p < out.len) {
        out[p] = tmp[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

/// Normalizes an HTTP error status code per `finalhandler` rules:
/// any code outside `400..599` is coerced to `500` (Internal Server Error).
pub fn normalizeErrorStatus(code: u16): u16 {
    if (code >= 400 && code <= 599) { return code; }
    return 500;
}

/// Returns the canonical HTTP status reason phrase via `@cap/http-types`.
pub fn statusReason(code: u16): string {
    let sc = StatusCode.fromU16(code);
    if (sc != null) {
        return (sc ?? StatusCode { code: 500 }).canonicalReason();
    }
    return "Internal Server Error";
}

/// Formats an XSS-safe HTML error document (`<!DOCTYPE html>...<pre>escaped</pre>...`) into `out`.
pub fn formatErrorHtml(message: string, out: []u8): usize {
    let p: usize = 0;
    p = fhPush(out, p, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<title>Error</title>\n</head>\n<body>\n<pre>");
    if (p < out.len) {
        let esc_len = escapeHtml(message, out[p..out.len]);
        p = p + esc_len;
    }
    p = fhPush(out, p, "</pre>\n</body>\n</html>\n");
    return p;
}

/// Formats the default `Cannot <METHOD> <URL>` 404 message into `out`.
pub fn formatCannotMessage(method: string, url: string, out: []u8): usize {
    let p: usize = 0;
    p = fhPush(out, p, "Cannot ");
    p = fhPush(out, p, method);
    p = fhPush(out, p, " ");
    p = fhPush(out, p, url);
    return p;
}

/// Builds a complete HTTP/1.1 final error response (status line + security headers + HTML body)
/// into `out`. Includes `Content-Security-Policy: default-src 'none'` and `X-Content-Type-Options: nosniff`.
pub fn buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse {
    let code = normalizeErrorStatus(status_code);
    let reason = statusReason(code);

    let body_buf: [512]u8 = undefined;
    let effective_msg = if (message.len > 0) { message } else { reason };
    let body_len = formatErrorHtml(effective_msg, body_buf[0..512]);

    let p: usize = 0;
    p = fhPush(out, p, "HTTP/1.1 ");
    p = fhPushU64(out, p, code as u64);
    p = fhPush(out, p, " ");
    p = fhPush(out, p, reason);
    p = fhPush(out, p, "\r\nContent-Security-Policy: default-src 'none'\r\nX-Content-Type-Options: nosniff\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: ");
    p = fhPushU64(out, p, body_len as u64);
    p = fhPush(out, p, "\r\n\r\n");

    if (!is_head) {
        let i: usize = 0;
        while (i < body_len && p < out.len) {
            out[p] = body_buf[i];
            p = p + 1;
            i = i + 1;
        }
    }
    return FinalResponse { status: code, body_len: body_len, total_len: p };
}
