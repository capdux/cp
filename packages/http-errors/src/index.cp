// @cap/http-errors — Zero-allocation HTTP error constructors & RFC 9457 problem+json serializer
// Clean-room MIT implementation inspired by npm `http-errors` and RFC 9457.
// Reuses `@cap/toidentifier` with zero inline duplication.

import { toIdentifier } from "../../toidentifier/src/index.cp";

pub struct HttpError {
    pub status: u16,
    pub code: string,
    pub title: string,
    pub detail: string,
    pub instance: string,
    pub expose: bool,

    pub fn withInstance(instance: string) -> HttpError {
        return HttpError{
            status: _.status,
            code: _.code,
            title: _.title,
            detail: _.detail,
            instance: instance,
            expose: _.expose,
        };
    }

    pub fn withExpose(expose: bool) -> HttpError {
        return HttpError{
            status: _.status,
            code: _.code,
            title: _.title,
            detail: _.detail,
            instance: _.instance,
            expose: expose,
        };
    }

    pub fn withCode(code: string) -> HttpError {
        return HttpError{
            status: _.status,
            code: code,
            title: _.title,
            detail: _.detail,
            instance: _.instance,
            expose: _.expose,
        };
    }
}

// ---------------------------------------------------------------------------
// Standard 4xx/5xx Metadata Lookup & Identifier Conversion (@cap/toidentifier)
// ---------------------------------------------------------------------------

fn statusTitle(status: u16) -> string {
    switch (status) {
        400 => return "Bad Request",
        401 => return "Unauthorized",
        402 => return "Payment Required",
        403 => return "Forbidden",
        404 => return "Not Found",
        405 => return "Method Not Allowed",
        406 => return "Not Acceptable",
        408 => return "Request Timeout",
        409 => return "Conflict",
        410 => return "Gone",
        411 => return "Length Required",
        412 => return "Precondition Failed",
        413 => return "Payload Too Large",
        414 => return "URI Too Long",
        415 => return "Unsupported Media Type",
        418 => return "I'm a Teapot",
        422 => return "Unprocessable Entity",
        423 => return "Locked",
        429 => return "Too Many Requests",
        451 => return "Unavailable For Legal Reasons",
        500 => return "Internal Server Error",
        501 => return "Not Implemented",
        502 => return "Bad Gateway",
        503 => return "Service Unavailable",
        504 => return "Gateway Timeout",
        505 => return "HTTP Version Not Supported",
        else => return if (status >= 400 && status < 500) "Client Error" else "Internal Server Error",
    }
}

/// Formats the PascalCase error class name for `status` (e.g. 400 -> "BadRequest", 418 -> "ImATeapot")
/// by delegating to `@cap/toidentifier`.
pub fn errorClassName(status: u16, out: []u8): usize {
    return toIdentifier(statusTitle(status), out);
}

fn statusCodeName(status: u16) -> string {
    switch (status) {
        400 => return "BAD_REQUEST",
        401 => return "UNAUTHORIZED",
        402 => return "PAYMENT_REQUIRED",
        403 => return "FORBIDDEN",
        404 => return "NOT_FOUND",
        405 => return "METHOD_NOT_ALLOWED",
        406 => return "NOT_ACCEPTABLE",
        408 => return "REQUEST_TIMEOUT",
        409 => return "CONFLICT",
        410 => return "GONE",
        411 => return "LENGTH_REQUIRED",
        412 => return "PRECONDITION_FAILED",
        413 => return "PAYLOAD_TOO_LARGE",
        414 => return "URI_TOO_LONG",
        415 => return "UNSUPPORTED_MEDIA_TYPE",
        418 => return "IM_A_TEAPOT",
        422 => return "UNPROCESSABLE_ENTITY",
        423 => return "LOCKED",
        429 => return "TOO_MANY_REQUESTS",
        451 => return "UNAVAILABLE_FOR_LEGAL_REASONS",
        500 => return "INTERNAL_SERVER_ERROR",
        501 => return "NOT_IMPLEMENTED",
        502 => return "BAD_GATEWAY",
        503 => return "SERVICE_UNAVAILABLE",
        504 => return "GATEWAY_TIMEOUT",
        505 => return "HTTP_VERSION_NOT_SUPPORTED",
        else => return if (status >= 400 && status < 500) "CLIENT_ERROR" else "SERVER_ERROR",
    }
}

// ---------------------------------------------------------------------------
// Public Constructors
// ---------------------------------------------------------------------------

/// Creates an `HttpError` for `status` (400..599). Non-error status codes fall back to 500.
/// 4xx errors default to `expose = true`; 5xx errors default to `expose = false`.
pub fn createHttpError(status: u16, detail: string) -> HttpError {
    const eff_status: u16 = if (status >= 400 && status <= 599) status else 500;
    return HttpError{
        status: eff_status,
        code: statusCodeName(eff_status),
        title: statusTitle(eff_status),
        detail: detail,
        instance: "",
        expose: eff_status < 500,
    };
}

pub fn badRequest(detail: string) -> HttpError { return createHttpError(400, detail); }
pub fn unauthorized(detail: string) -> HttpError { return createHttpError(401, detail); }
pub fn forbidden(detail: string) -> HttpError { return createHttpError(403, detail); }
pub fn notFound(detail: string) -> HttpError { return createHttpError(404, detail); }
pub fn methodNotAllowed(detail: string) -> HttpError { return createHttpError(405, detail); }
pub fn conflict(detail: string) -> HttpError { return createHttpError(409, detail); }
pub fn gone(detail: string) -> HttpError { return createHttpError(410, detail); }
pub fn payloadTooLarge(detail: string) -> HttpError { return createHttpError(413, detail); }
pub fn unsupportedMediaType(detail: string) -> HttpError { return createHttpError(415, detail); }
pub fn unprocessableEntity(detail: string) -> HttpError { return createHttpError(422, detail); }
pub fn tooManyRequests(detail: string) -> HttpError { return createHttpError(429, detail); }
pub fn internalServerError(detail: string) -> HttpError { return createHttpError(500, detail); }
pub fn notImplemented(detail: string) -> HttpError { return createHttpError(501, detail); }
pub fn badGateway(detail: string) -> HttpError { return createHttpError(502, detail); }
pub fn serviceUnavailable(detail: string) -> HttpError { return createHttpError(503, detail); }
pub fn gatewayTimeout(detail: string) -> HttpError { return createHttpError(504, detail); }

// ---------------------------------------------------------------------------
// Semantic Predicates
// ---------------------------------------------------------------------------

pub fn isClientError(err: HttpError) -> bool {
    return err.status >= 400 && err.status <= 499;
}

pub fn isServerError(err: HttpError) -> bool {
    return err.status >= 500 && err.status <= 599;
}

/// Returns `true` for transient errors commonly safe to retry (408, 429, 502, 503, 504).
pub fn isRetryable(err: HttpError) -> bool {
    switch (err.status) {
        408, 429, 502, 503, 504 => return true,
        else => return false,
    }
}

// ---------------------------------------------------------------------------
// RFC 9457 `application/problem+json` Serializer (Zero Heap Allocation)
// ---------------------------------------------------------------------------

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn hexDigitLower(v: u8) -> u8 {
    if (v < 10) return '0' + v;
    return 'a' + (v - 10);
}

fn appendJsonEscaped(out: []u8, start_pos: usize, s: string) -> usize {
    let pos = start_pos;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            pos = appendStr(out, pos, "\\\"");
        } else if (c == '\\') {
            pos = appendStr(out, pos, "\\\\");
        } else if (c == '\n') {
            pos = appendStr(out, pos, "\\n");
        } else if (c == '\r') {
            pos = appendStr(out, pos, "\\r");
        } else if (c == '\t') {
            pos = appendStr(out, pos, "\\t");
        } else if (c < 0x20) {
            pos = appendStr(out, pos, "\\u00");
            pos = appendByte(out, pos, hexDigitLower((c >> 4) & 0x0F));
            pos = appendByte(out, pos, hexDigitLower(c & 0x0F));
        } else {
            pos = appendByte(out, pos, c);
        }
        i += 1;
    }
    return pos;
}

fn appendU16(out: []u8, pos: usize, val: u16) -> usize {
    if (val == 0) {
        return appendByte(out, pos, '0');
    }
    let digits: [8]u8 = undefined;
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        digits[len] = ('0' as u16 + (v % 10)) as u8;
        v /= 10;
        len += 1;
    }
    let p = pos;
    let k = len;
    while (k > 0) {
        k -= 1;
        p = appendByte(out, p, digits[k]);
    }
    return p;
}

/// Serializes `err` as an RFC 9457 `application/problem+json` payload into `out`.
/// When `err.expose == false` and `include_hidden_detail == false`, sensitive `err.detail`
/// is omitted to prevent leaking internal stack/database details.
pub fn toProblemJson(err: HttpError, include_hidden_detail: bool, out: []u8) -> string {
    let pos: usize = 0;
    pos = appendStr(out, pos, "{\"type\":\"about:blank\",\"title\":\"");
    pos = appendJsonEscaped(out, pos, err.title);
    pos = appendStr(out, pos, "\",\"status\":");
    pos = appendU16(out, pos, err.status);
    pos = appendStr(out, pos, ",\"code\":\"");
    pos = appendJsonEscaped(out, pos, err.code);
    pos = appendByte(out, pos, '"');

    if ((err.expose || include_hidden_detail) && err.detail.len > 0) {
        pos = appendStr(out, pos, ",\"detail\":\"");
        pos = appendJsonEscaped(out, pos, err.detail);
        pos = appendByte(out, pos, '"');
    }

    if (err.instance.len > 0) {
        pos = appendStr(out, pos, ",\"instance\":\"");
        pos = appendJsonEscaped(out, pos, err.instance);
        pos = appendByte(out, pos, '"');
    }

    pos = appendByte(out, pos, '}');
    return out[0..pos];
}
