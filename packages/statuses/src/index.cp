// @cap/statuses — Zero-heap RFC 9110 / IANA HTTP status codes & predicates
// MIT License (clean-room CP implementation)

import { eqIgnoreCase } from "cap:ascii";

const STATUS_COUNT: usize = 62;

const STATUS_CODES: [62]u16 = [
    100, 101, 102, 103,
    200, 201, 202, 203, 204, 205, 206, 207, 208, 226,
    300, 301, 302, 303, 304, 305, 307, 308,
    400, 401, 402, 403, 404, 405, 406, 407, 408, 409,
    410, 411, 412, 413, 414, 415, 416, 417, 418, 421,
    422, 423, 424, 425, 426, 428, 429, 431, 451,
    500, 501, 502, 503, 504, 505, 506, 507, 508, 510, 511,
];

const STATUS_MSGS: [62]string = [
    "Continue",
    "Switching Protocols",
    "Processing",
    "Early Hints",
    "OK",
    "Created",
    "Accepted",
    "Non-Authoritative Information",
    "No Content",
    "Reset Content",
    "Partial Content",
    "Multi-Status",
    "Already Reported",
    "IM Used",
    "Multiple Choices",
    "Moved Permanently",
    "Found",
    "See Other",
    "Not Modified",
    "Use Proxy",
    "Temporary Redirect",
    "Permanent Redirect",
    "Bad Request",
    "Unauthorized",
    "Payment Required",
    "Forbidden",
    "Not Found",
    "Method Not Allowed",
    "Not Acceptable",
    "Proxy Authentication Required",
    "Request Timeout",
    "Conflict",
    "Gone",
    "Length Required",
    "Precondition Failed",
    "Payload Too Large",
    "URI Too Long",
    "Unsupported Media Type",
    "Range Not Satisfiable",
    "Expectation Failed",
    "I'm a Teapot",
    "Misdirected Request",
    "Unprocessable Entity",
    "Locked",
    "Failed Dependency",
    "Too Early",
    "Upgrade Required",
    "Precondition Required",
    "Too Many Requests",
    "Request Header Fields Too Large",
    "Unavailable For Legal Reasons",
    "Internal Server Error",
    "Not Implemented",
    "Bad Gateway",
    "Service Unavailable",
    "Gateway Timeout",
    "HTTP Version Not Supported",
    "Variant Also Negotiates",
    "Insufficient Storage",
    "Loop Detected",
    "Not Extended",
    "Network Authentication Required",
];

pub fn message(code: u16) -> string {
    let lo: usize = 0;
    let hi: usize = STATUS_COUNT;
    while (lo < hi) {
        const mid: usize = lo + (hi - lo) / 2;
        const c: u16 = STATUS_CODES[mid];
        if (c == code) {
            return STATUS_MSGS[mid];
        }
        if (c < code) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return "";
}

pub fn codeFromMessage(msg: string) -> u16 {
    if (msg.len == 0) {
        return 0;
    }
    let i: usize = 0;
    while (i < STATUS_COUNT) {
        if (eqIgnoreCase(STATUS_MSGS[i], msg)) {
            return STATUS_CODES[i];
        }
        i += 1;
    }
    return 0;
}

pub fn isValid(code: u16) -> bool {
    return message(code).len > 0;
}

pub fn isInformational(code: u16) -> bool {
    return code >= 100 && code <= 199;
}

pub fn isSuccess(code: u16) -> bool {
    return code >= 200 && code <= 299;
}

pub fn isRedirect(code: u16) -> bool {
    return code == 300 || code == 301 || code == 302 || code == 303
        || code == 305 || code == 307 || code == 308;
}

pub fn isEmpty(code: u16) -> bool {
    return code == 204 || code == 205 || code == 304;
}

pub fn isRetry(code: u16) -> bool {
    return code == 408 || code == 425 || code == 429
        || code == 500 || code == 502 || code == 503 || code == 504;
}

pub fn isClientError(code: u16) -> bool {
    return code >= 400 && code <= 499;
}

pub fn isServerError(code: u16) -> bool {
    return code >= 500 && code <= 599;
}
