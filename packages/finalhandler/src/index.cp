// @cap/finalhandler — Zero-heap HTTP final response & error handler modeled on npm `finalhandler` v2.
// Clean-room MIT implementation; reuses `@cap/escape-html`, `@cap/http-types`, and `@cap/itoa`.

import { escapeHtml } from "../../escape-html/src/index.cp";
import { StatusCode } from "../../http/src/index.cp";
import { formatU64 } from "../../itoa/src/index.cp";

pub enum FinalEnv {
    Development,
    Production,
}

pub struct ResponseHeader {
    pub name: string,
    pub value: string,
}

pub struct FinalError {
    pub status: u16,
    pub status_code: u16,
    pub expose: bool,
    pub message: string,
    pub stack: string,
}

pub struct FinalHandlerOptions {
    pub env: FinalEnv,
    pub is_head: bool,
    pub method: string,
    pub url: string,

    pub static fn default(): FinalHandlerOptions {
        return FinalHandlerOptions {
            env: .Production,
            is_head: false,
            method: "GET",
            url: "/",
        };
    }
}

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

/// Resolves the HTTP error status code from a `FinalError` (`err.status` -> `err.status_code` -> `500`).
pub fn statusCode(err: FinalError): u16 {
    if (err.status >= 400 && err.status <= 599) { return err.status; }
    if (err.status_code >= 400 && err.status_code <= 599) { return err.status_code; }
    return 500;
}

/// Alias for `statusCode(err)` matching npm `finalhandler`'s `getErrorStatusCode`.
pub fn getErrorStatusCode(err: FinalError): u16 {
    return statusCode(err);
}

/// Returns the canonical HTTP status reason phrase via `@cap/http-types`.
pub fn statusReason(code: u16): string {
    let sc = StatusCode.fromU16(code);
    if (sc != null) {
        return (sc ?? StatusCode { code: 500 }).canonicalReason();
    }
    return "Internal Server Error";
}

/// Re-exported HTML escaping helper backed by `@cap/escape-html`.
pub fn escapeHtmlText(input: string, out: []u8): usize {
    return escapeHtml(input, out);
}

/// Formats an error message respecting `env` (Production redacts 5xx and non-exposed details;
/// Development surfaces `err.stack` or `err.message`).
pub fn formatErrorMessage(err: FinalError, status: u16, env: FinalEnv): string {
    if (env == .Production) {
        if (status >= 500 || !err.expose) {
            return statusReason(status);
        }
        if (err.message.len > 0) { return err.message; }
        return statusReason(status);
    }
    if (err.stack.len > 0) { return err.stack; }
    if (err.message.len > 0) { return err.message; }
    return statusReason(status);
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

/// npm `finalhandler` `createHtmlDocument`: escapes HTML, converts newlines to `<br>` and
/// consecutive spaces to ` &nbsp;`, wrapped in the canonical Express error HTML document.
pub fn createHtmlDocument(message: string, out: []u8): usize {
    let esc_buf: [1024]u8 = undefined;
    const esc_len = escapeHtml(message, esc_buf[0..1024]);

    let p: usize = 0;
    p = fhPush(out, p, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<title>Error</title>\n</head>\n<body>\n<pre>");

    let i: usize = 0;
    while (i < esc_len && p < out.len) {
        const c = esc_buf[i];
        if (c == '\n') {
            p = fhPush(out, p, "<br>");
            i += 1;
        } else if (c == ' ' && i + 1 < esc_len && esc_buf[i + 1] == ' ') {
            p = fhPush(out, p, " &nbsp;");
            i += 2;
        } else {
            out[p] = c;
            p += 1;
            i += 1;
        }
    }

    p = fhPush(out, p, "</pre>\n</body>\n</html>\n");
    return p;
}

/// Populates the default security and content-type headers emitted by `finalhandler`.
pub fn defaultSecurityHeaders(out: []ResponseHeader): usize {
    let n: usize = 0;
    if (n < out.len) {
        out[n] = ResponseHeader { name: "Content-Security-Policy", value: "default-src 'none'" };
        n += 1;
    }
    if (n < out.len) {
        out[n] = ResponseHeader { name: "X-Content-Type-Options", value: "nosniff" };
        n += 1;
    }
    if (n < out.len) {
        out[n] = ResponseHeader { name: "Content-Type", value: "text/html; charset=utf-8" };
        n += 1;
    }
    return n;
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

/// Full `finalhandler` HTTP response builder supporting `FinalError?`, `env` redaction,
/// default 404 `Cannot <METHOD> <URL>`, custom `extra_headers`, and `HEAD` body suppression.
pub fn buildFinalHttpResponseWithOptions(
    err: FinalError?,
    opts: FinalHandlerOptions,
    extra_headers: []const ResponseHeader,
    out: []u8,
): FinalResponse {
    let code: u16 = 404;
    let msg_buf: [256]u8 = undefined;
    let effective_msg: string = "";

    if (err != null) {
        const e = err.?;
        code = statusCode(e);
        effective_msg = formatErrorMessage(e, code, opts.env);
    } else {
        code = 404;
        const mn = formatCannotMessage(opts.method, opts.url, msg_buf[0..256]);
        effective_msg = msg_buf[0..mn] as string;
    }

    const reason = statusReason(code);
    let body_buf: [1024]u8 = undefined;
    const body_len = createHtmlDocument(effective_msg, body_buf[0..1024]);

    let p: usize = 0;
    p = fhPush(out, p, "HTTP/1.1 ");
    p = fhPushU64(out, p, code as u64);
    p = fhPush(out, p, " ");
    p = fhPush(out, p, reason);
    p = fhPush(out, p, "\r\nContent-Security-Policy: default-src 'none'\r\nX-Content-Type-Options: nosniff\r\nContent-Type: text/html; charset=utf-8\r\n");

    let hi: usize = 0;
    while (hi < extra_headers.len) : (hi += 1) {
        p = fhPush(out, p, extra_headers[hi].name);
        p = fhPush(out, p, ": ");
        p = fhPush(out, p, extra_headers[hi].value);
        p = fhPush(out, p, "\r\n");
    }

    p = fhPush(out, p, "Content-Length: ");
    p = fhPushU64(out, p, body_len as u64);
    p = fhPush(out, p, "\r\n\r\n");

    if (!opts.is_head) {
        let i: usize = 0;
        while (i < body_len && p < out.len) : (i += 1) {
            out[p] = body_buf[i];
            p += 1;
        }
    }

    return FinalResponse { status: code, body_len: body_len, total_len: p };
}
