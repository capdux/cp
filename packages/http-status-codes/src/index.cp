// @cap/http-status — Zero-allocation RFC 9110 / WebDAV / Cloudflare HTTP status code engine
// Clean-room MIT implementation inspired by npm `statuses`, `http-status-codes`,
// and Rust `http::StatusCode`.

import { toLowerByte } from "cap:ascii";

// ---------------------------------------------------------------------------
// 1xx Informational
// ---------------------------------------------------------------------------
pub const CONTINUE: u16 = 100;
pub const SWITCHING_PROTOCOLS: u16 = 101;
pub const PROCESSING: u16 = 102;
pub const EARLY_HINTS: u16 = 103;

// ---------------------------------------------------------------------------
// 2xx Success
// ---------------------------------------------------------------------------
pub const OK: u16 = 200;
pub const CREATED: u16 = 201;
pub const ACCEPTED: u16 = 202;
pub const NON_AUTHORITATIVE_INFORMATION: u16 = 203;
pub const NO_CONTENT: u16 = 204;
pub const RESET_CONTENT: u16 = 205;
pub const PARTIAL_CONTENT: u16 = 206;
pub const MULTI_STATUS: u16 = 207;
pub const ALREADY_REPORTED: u16 = 208;
pub const IM_USED: u16 = 226;

// ---------------------------------------------------------------------------
// 3xx Redirection
// ---------------------------------------------------------------------------
pub const MULTIPLE_CHOICES: u16 = 300;
pub const MOVED_PERMANENTLY: u16 = 301;
pub const FOUND: u16 = 302;
pub const SEE_OTHER: u16 = 303;
pub const NOT_MODIFIED: u16 = 304;
pub const USE_PROXY: u16 = 305;
pub const TEMPORARY_REDIRECT: u16 = 307;
pub const PERMANENT_REDIRECT: u16 = 308;

// ---------------------------------------------------------------------------
// 4xx Client Error
// ---------------------------------------------------------------------------
pub const BAD_REQUEST: u16 = 400;
pub const UNAUTHORIZED: u16 = 401;
pub const PAYMENT_REQUIRED: u16 = 402;
pub const FORBIDDEN: u16 = 403;
pub const NOT_FOUND: u16 = 404;
pub const METHOD_NOT_ALLOWED: u16 = 405;
pub const NOT_ACCEPTABLE: u16 = 406;
pub const PROXY_AUTHENTICATION_REQUIRED: u16 = 407;
pub const REQUEST_TIMEOUT: u16 = 408;
pub const CONFLICT: u16 = 409;
pub const GONE: u16 = 410;
pub const LENGTH_REQUIRED: u16 = 411;
pub const PRECONDITION_FAILED: u16 = 412;
pub const PAYLOAD_TOO_LARGE: u16 = 413;
pub const CONTENT_TOO_LARGE: u16 = 413;
pub const URI_TOO_LONG: u16 = 414;
pub const UNSUPPORTED_MEDIA_TYPE: u16 = 415;
pub const RANGE_NOT_SATISFIABLE: u16 = 416;
pub const EXPECTATION_FAILED: u16 = 417;
pub const IM_A_TEAPOT: u16 = 418;
pub const MISDIRECTED_REQUEST: u16 = 421;
pub const UNPROCESSABLE_ENTITY: u16 = 422;
pub const UNPROCESSABLE_CONTENT: u16 = 422;
pub const LOCKED: u16 = 423;
pub const FAILED_DEPENDENCY: u16 = 424;
pub const TOO_EARLY: u16 = 425;
pub const UPGRADE_REQUIRED: u16 = 426;
pub const PRECONDITION_REQUIRED: u16 = 428;
pub const TOO_MANY_REQUESTS: u16 = 429;
pub const REQUEST_HEADER_FIELDS_TOO_LARGE: u16 = 431;
pub const UNAVAILABLE_FOR_LEGAL_REASONS: u16 = 451;

// ---------------------------------------------------------------------------
// 5xx Server Error
// ---------------------------------------------------------------------------
pub const INTERNAL_SERVER_ERROR: u16 = 500;
pub const NOT_IMPLEMENTED: u16 = 501;
pub const BAD_GATEWAY: u16 = 502;
pub const SERVICE_UNAVAILABLE: u16 = 503;
pub const GATEWAY_TIMEOUT: u16 = 504;
pub const HTTP_VERSION_NOT_SUPPORTED: u16 = 505;
pub const VARIANT_ALSO_NEGOTIATES: u16 = 506;
pub const INSUFFICIENT_STORAGE: u16 = 507;
pub const LOOP_DETECTED: u16 = 508;
pub const BANDWIDTH_LIMIT_EXCEEDED: u16 = 509;
pub const NOT_EXTENDED: u16 = 510;
pub const NETWORK_AUTHENTICATION_REQUIRED: u16 = 511;

// ---------------------------------------------------------------------------
// 52x Cloudflare Extensions
// ---------------------------------------------------------------------------
pub const WEB_SERVER_UNKNOWN_ERROR: u16 = 520;
pub const WEB_SERVER_IS_DOWN: u16 = 521;
pub const CONNECTION_TIMED_OUT: u16 = 522;
pub const ORIGIN_IS_UNREACHABLE: u16 = 523;
pub const A_TIMEOUT_OCCURRED: u16 = 524;
pub const SSL_HANDSHAKE_FAILED: u16 = 525;
pub const INVALID_SSL_CERTIFICATE: u16 = 526;

// ---------------------------------------------------------------------------
// Status Class Enum
// ---------------------------------------------------------------------------

pub enum StatusClass: u8 {
    Invalid = 0,
    Informational = 1,
    Success = 2,
    Redirection = 3,
    ClientError = 4,
    ServerError = 5,
}

// ---------------------------------------------------------------------------
// Reason Phrase Lookup (Code -> Phrase)
// ---------------------------------------------------------------------------

/// Returns the standard IANA / RFC 9110 / WebDAV / Cloudflare reason phrase for `code`,
/// or `""` if `code` is unknown.
pub fn reasonPhrase(code: u16) -> string {
    switch (code) {
        // 1xx
        100 => return "Continue",
        101 => return "Switching Protocols",
        102 => return "Processing",
        103 => return "Early Hints",

        // 2xx
        200 => return "OK",
        201 => return "Created",
        202 => return "Accepted",
        203 => return "Non-Authoritative Information",
        204 => return "No Content",
        205 => return "Reset Content",
        206 => return "Partial Content",
        207 => return "Multi-Status",
        208 => return "Already Reported",
        226 => return "IM Used",

        // 3xx
        300 => return "Multiple Choices",
        301 => return "Moved Permanently",
        302 => return "Found",
        303 => return "See Other",
        304 => return "Not Modified",
        305 => return "Use Proxy",
        307 => return "Temporary Redirect",
        308 => return "Permanent Redirect",

        // 4xx
        400 => return "Bad Request",
        401 => return "Unauthorized",
        402 => return "Payment Required",
        403 => return "Forbidden",
        404 => return "Not Found",
        405 => return "Method Not Allowed",
        406 => return "Not Acceptable",
        407 => return "Proxy Authentication Required",
        408 => return "Request Timeout",
        409 => return "Conflict",
        410 => return "Gone",
        411 => return "Length Required",
        412 => return "Precondition Failed",
        413 => return "Payload Too Large",
        414 => return "URI Too Long",
        415 => return "Unsupported Media Type",
        416 => return "Range Not Satisfiable",
        417 => return "Expectation Failed",
        418 => return "I'm a Teapot",
        421 => return "Misdirected Request",
        422 => return "Unprocessable Entity",
        423 => return "Locked",
        424 => return "Failed Dependency",
        425 => return "Too Early",
        426 => return "Upgrade Required",
        428 => return "Precondition Required",
        429 => return "Too Many Requests",
        431 => return "Request Header Fields Too Large",
        451 => return "Unavailable For Legal Reasons",

        // 5xx
        500 => return "Internal Server Error",
        501 => return "Not Implemented",
        502 => return "Bad Gateway",
        503 => return "Service Unavailable",
        504 => return "Gateway Timeout",
        505 => return "HTTP Version Not Supported",
        506 => return "Variant Also Negotiates",
        507 => return "Insufficient Storage",
        508 => return "Loop Detected",
        509 => return "Bandwidth Limit Exceeded",
        510 => return "Not Extended",
        511 => return "Network Authentication Required",

        // 52x Cloudflare extensions
        520 => return "Web Server Returned an Unknown Error",
        521 => return "Web Server Is Down",
        522 => return "Connection Timed Out",
        523 => return "Origin Is Unreachable",
        524 => return "A Timeout Occurred",
        525 => return "SSL Handshake Failed",
        526 => return "Invalid SSL Certificate",

        else => return "",
    }
}

// ---------------------------------------------------------------------------
// Reverse Lookup (Phrase -> Code)
// ---------------------------------------------------------------------------

fn isSep(c: u8) -> bool {
    return c == ' ' || c == '_' || c == '-' || c == '\t';
}

/// Case-insensitive comparison treating spaces, underscores, and hyphens as equivalent
/// single separators and ignoring apostrophes (e.g., `"im_a_teapot"` matches `"I'm a Teapot"`).
fn eqNormalizedPhrase(input: string, target: string) -> bool {
    let i: usize = 0;
    let j: usize = 0;

    // Trim leading separators in input
    while (i < input.len && isSep(input[i])) {
        i += 1;
    }

    while (i < input.len && j < target.len) {
        if (input[i] == '\'') {
            i += 1;
            continue;
        }
        if (target[j] == '\'') {
            j += 1;
            continue;
        }
        const i_sep = isSep(input[i]);
        const j_sep = isSep(target[j]);
        if (i_sep && j_sep) {
            while (i < input.len && isSep(input[i])) {
                i += 1;
            }
            while (j < target.len && isSep(target[j])) {
                j += 1;
            }
            continue;
        }
        if (i_sep || j_sep) {
            return false;
        }
        if (toLowerByte(input[i]) != toLowerByte(target[j])) {
            return false;
        }
        i += 1;
        j += 1;
    }

    // Skip trailing apostrophes/separators
    while (i < input.len && (isSep(input[i]) || input[i] == '\'')) {
        i += 1;
    }
    while (j < target.len && target[j] == '\'') {
        j += 1;
    }

    return i == input.len && j == target.len;
}

/// Looks up the numeric HTTP status code from a reason phrase or snake_case/kebab-case name.
/// Case-insensitive and separator-tolerant (e.g. `"Not Found"`, `"not_found"`, `"NOT-FOUND"` -> `404`).
pub fn codeFromPhrase(phrase: string) -> u16? {
    if (phrase.len == 0) {
        return null;
    }

    // 1xx
    if (eqNormalizedPhrase(phrase, "Continue")) return CONTINUE;
    if (eqNormalizedPhrase(phrase, "Switching Protocols")) return SWITCHING_PROTOCOLS;
    if (eqNormalizedPhrase(phrase, "Processing")) return PROCESSING;
    if (eqNormalizedPhrase(phrase, "Early Hints")) return EARLY_HINTS;

    // 2xx
    if (eqNormalizedPhrase(phrase, "OK")) return OK;
    if (eqNormalizedPhrase(phrase, "Created")) return CREATED;
    if (eqNormalizedPhrase(phrase, "Accepted")) return ACCEPTED;
    if (eqNormalizedPhrase(phrase, "Non-Authoritative Information")) return NON_AUTHORITATIVE_INFORMATION;
    if (eqNormalizedPhrase(phrase, "No Content")) return NO_CONTENT;
    if (eqNormalizedPhrase(phrase, "Reset Content")) return RESET_CONTENT;
    if (eqNormalizedPhrase(phrase, "Partial Content")) return PARTIAL_CONTENT;
    if (eqNormalizedPhrase(phrase, "Multi-Status")) return MULTI_STATUS;
    if (eqNormalizedPhrase(phrase, "Already Reported")) return ALREADY_REPORTED;
    if (eqNormalizedPhrase(phrase, "IM Used")) return IM_USED;

    // 3xx
    if (eqNormalizedPhrase(phrase, "Multiple Choices")) return MULTIPLE_CHOICES;
    if (eqNormalizedPhrase(phrase, "Moved Permanently")) return MOVED_PERMANENTLY;
    if (eqNormalizedPhrase(phrase, "Found")) return FOUND;
    if (eqNormalizedPhrase(phrase, "See Other")) return SEE_OTHER;
    if (eqNormalizedPhrase(phrase, "Not Modified")) return NOT_MODIFIED;
    if (eqNormalizedPhrase(phrase, "Use Proxy")) return USE_PROXY;
    if (eqNormalizedPhrase(phrase, "Temporary Redirect")) return TEMPORARY_REDIRECT;
    if (eqNormalizedPhrase(phrase, "Permanent Redirect")) return PERMANENT_REDIRECT;

    // 4xx
    if (eqNormalizedPhrase(phrase, "Bad Request")) return BAD_REQUEST;
    if (eqNormalizedPhrase(phrase, "Unauthorized")) return UNAUTHORIZED;
    if (eqNormalizedPhrase(phrase, "Payment Required")) return PAYMENT_REQUIRED;
    if (eqNormalizedPhrase(phrase, "Forbidden")) return FORBIDDEN;
    if (eqNormalizedPhrase(phrase, "Not Found")) return NOT_FOUND;
    if (eqNormalizedPhrase(phrase, "Method Not Allowed")) return METHOD_NOT_ALLOWED;
    if (eqNormalizedPhrase(phrase, "Not Acceptable")) return NOT_ACCEPTABLE;
    if (eqNormalizedPhrase(phrase, "Proxy Authentication Required")) return PROXY_AUTHENTICATION_REQUIRED;
    if (eqNormalizedPhrase(phrase, "Request Timeout")) return REQUEST_TIMEOUT;
    if (eqNormalizedPhrase(phrase, "Conflict")) return CONFLICT;
    if (eqNormalizedPhrase(phrase, "Gone")) return GONE;
    if (eqNormalizedPhrase(phrase, "Length Required")) return LENGTH_REQUIRED;
    if (eqNormalizedPhrase(phrase, "Precondition Failed")) return PRECONDITION_FAILED;
    if (eqNormalizedPhrase(phrase, "Payload Too Large") || eqNormalizedPhrase(phrase, "Content Too Large")) return PAYLOAD_TOO_LARGE;
    if (eqNormalizedPhrase(phrase, "URI Too Long")) return URI_TOO_LONG;
    if (eqNormalizedPhrase(phrase, "Unsupported Media Type")) return UNSUPPORTED_MEDIA_TYPE;
    if (eqNormalizedPhrase(phrase, "Range Not Satisfiable")) return RANGE_NOT_SATISFIABLE;
    if (eqNormalizedPhrase(phrase, "Expectation Failed")) return EXPECTATION_FAILED;
    if (eqNormalizedPhrase(phrase, "I'm a Teapot")) return IM_A_TEAPOT;
    if (eqNormalizedPhrase(phrase, "Misdirected Request")) return MISDIRECTED_REQUEST;
    if (eqNormalizedPhrase(phrase, "Unprocessable Entity") || eqNormalizedPhrase(phrase, "Unprocessable Content")) return UNPROCESSABLE_ENTITY;
    if (eqNormalizedPhrase(phrase, "Locked")) return LOCKED;
    if (eqNormalizedPhrase(phrase, "Failed Dependency")) return FAILED_DEPENDENCY;
    if (eqNormalizedPhrase(phrase, "Too Early")) return TOO_EARLY;
    if (eqNormalizedPhrase(phrase, "Upgrade Required")) return UPGRADE_REQUIRED;
    if (eqNormalizedPhrase(phrase, "Precondition Required")) return PRECONDITION_REQUIRED;
    if (eqNormalizedPhrase(phrase, "Too Many Requests")) return TOO_MANY_REQUESTS;
    if (eqNormalizedPhrase(phrase, "Request Header Fields Too Large")) return REQUEST_HEADER_FIELDS_TOO_LARGE;
    if (eqNormalizedPhrase(phrase, "Unavailable For Legal Reasons")) return UNAVAILABLE_FOR_LEGAL_REASONS;

    // 5xx
    if (eqNormalizedPhrase(phrase, "Internal Server Error")) return INTERNAL_SERVER_ERROR;
    if (eqNormalizedPhrase(phrase, "Not Implemented")) return NOT_IMPLEMENTED;
    if (eqNormalizedPhrase(phrase, "Bad Gateway")) return BAD_GATEWAY;
    if (eqNormalizedPhrase(phrase, "Service Unavailable")) return SERVICE_UNAVAILABLE;
    if (eqNormalizedPhrase(phrase, "Gateway Timeout")) return GATEWAY_TIMEOUT;
    if (eqNormalizedPhrase(phrase, "HTTP Version Not Supported")) return HTTP_VERSION_NOT_SUPPORTED;
    if (eqNormalizedPhrase(phrase, "Variant Also Negotiates")) return VARIANT_ALSO_NEGOTIATES;
    if (eqNormalizedPhrase(phrase, "Insufficient Storage")) return INSUFFICIENT_STORAGE;
    if (eqNormalizedPhrase(phrase, "Loop Detected")) return LOOP_DETECTED;
    if (eqNormalizedPhrase(phrase, "Bandwidth Limit Exceeded")) return BANDWIDTH_LIMIT_EXCEEDED;
    if (eqNormalizedPhrase(phrase, "Not Extended")) return NOT_EXTENDED;
    if (eqNormalizedPhrase(phrase, "Network Authentication Required")) return NETWORK_AUTHENTICATION_REQUIRED;

    // 52x Cloudflare extensions
    if (eqNormalizedPhrase(phrase, "Web Server Returned an Unknown Error") || eqNormalizedPhrase(phrase, "Web Server Unknown Error")) return WEB_SERVER_UNKNOWN_ERROR;
    if (eqNormalizedPhrase(phrase, "Web Server Is Down")) return WEB_SERVER_IS_DOWN;
    if (eqNormalizedPhrase(phrase, "Connection Timed Out")) return CONNECTION_TIMED_OUT;
    if (eqNormalizedPhrase(phrase, "Origin Is Unreachable")) return ORIGIN_IS_UNREACHABLE;
    if (eqNormalizedPhrase(phrase, "A Timeout Occurred")) return A_TIMEOUT_OCCURRED;
    if (eqNormalizedPhrase(phrase, "SSL Handshake Failed")) return SSL_HANDSHAKE_FAILED;
    if (eqNormalizedPhrase(phrase, "Invalid SSL Certificate")) return INVALID_SSL_CERTIFICATE;

    return null;
}

// ---------------------------------------------------------------------------
// Classification & Semantic Predicates
// ---------------------------------------------------------------------------

/// Returns the RFC 9110 status class for `code`.
pub fn statusClass(code: u16) -> StatusClass {
    if (code >= 100 && code <= 199) return StatusClass.Informational;
    if (code >= 200 && code <= 299) return StatusClass.Success;
    if (code >= 300 && code <= 399) return StatusClass.Redirection;
    if (code >= 400 && code <= 499) return StatusClass.ClientError;
    if (code >= 500 && code <= 599) return StatusClass.ServerError;
    return StatusClass.Invalid;
}

/// Returns true if `code` falls within the valid HTTP status code range (100..=599).
pub fn isValidStatus(code: u16) -> bool {
    return code >= 100 && code <= 599;
}

/// Returns true if `code` is a known standard/WebDAV/Cloudflare HTTP status code.
pub fn isKnownStatus(code: u16) -> bool {
    return reasonPhrase(code).len > 0;
}

pub fn isInformational(code: u16) -> bool {
    return code >= 100 && code <= 199;
}

pub fn isSuccess(code: u16) -> bool {
    return code >= 200 && code <= 299;
}

pub fn isRedirect(code: u16) -> bool {
    return code >= 300 && code <= 399;
}

pub fn isClientError(code: u16) -> bool {
    return code >= 400 && code <= 499;
}

pub fn isServerError(code: u16) -> bool {
    return code >= 500 && code <= 599;
}

/// Returns true if responses with `code` must not contain a message body (1xx, 204, 205, 304).
pub fn isEmptyBody(code: u16) -> bool {
    return (code >= 100 && code <= 199) || code == 204 || code == 205 || code == 304;
}

/// Returns true if the request is commonly safe/advisable to retry on `code`
/// (408, 425, 429, 500, 502, 503, 504, and Cloudflare 522/524 timeout codes).
pub fn isRetryable(code: u16) -> bool {
    switch (code) {
        408, 425, 429, 500, 502, 503, 504, 522, 524 => return true,
        else => return false,
    }
}

/// Returns true if responses with `code` are heuristically cacheable by default per RFC 9110
/// (200, 203, 204, 206, 300, 301, 404, 405, 410, 414, 501).
pub fn isCacheableByDefault(code: u16) -> bool {
    switch (code) {
        200, 203, 204, 206, 300, 301, 404, 405, 410, 414, 501 => return true,
        else => return false,
    }
}
