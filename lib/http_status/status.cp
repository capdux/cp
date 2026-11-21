// Source: Absorbed from /data/dev/bun/src/runtime/server/HTTPStatusText.cp

/// HTTP 状态码大类（RFC 9110 §15）
pub enum StatusClass {
    Informational,
    Success,
    Redirection,
    ClientError,
    ServerError,
    Unknown,
}

/// 返回标准 HTTP 状态行片段（如 `"200 OK"`、`"404 Not Found"`），未知状态码返回 `null`。
pub fn statusLine(code: u16): string? {
    return switch (code) {
        100 => "100 Continue",
        101 => "101 Switching Protocols",
        102 => "102 Processing",
        103 => "103 Early Hints",
        200 => "200 OK",
        201 => "201 Created",
        202 => "202 Accepted",
        203 => "203 Non-Authoritative Information",
        204 => "204 No Content",
        205 => "205 Reset Content",
        206 => "206 Partial Content",
        207 => "207 Multi-Status",
        208 => "208 Already Reported",
        226 => "226 IM Used",
        300 => "300 Multiple Choices",
        301 => "301 Moved Permanently",
        302 => "302 Found",
        303 => "303 See Other",
        304 => "304 Not Modified",
        305 => "305 Use Proxy",
        306 => "306 Switch Proxy",
        307 => "307 Temporary Redirect",
        308 => "308 Permanent Redirect",
        400 => "400 Bad Request",
        401 => "401 Unauthorized",
        402 => "402 Payment Required",
        403 => "403 Forbidden",
        404 => "404 Not Found",
        405 => "405 Method Not Allowed",
        406 => "406 Not Acceptable",
        407 => "407 Proxy Authentication Required",
        408 => "408 Request Timeout",
        409 => "409 Conflict",
        410 => "410 Gone",
        411 => "411 Length Required",
        412 => "412 Precondition Failed",
        413 => "413 Payload Too Large",
        414 => "414 URI Too Long",
        415 => "415 Unsupported Media Type",
        416 => "416 Range Not Satisfiable",
        417 => "417 Expectation Failed",
        418 => "418 I'm a Teapot",
        421 => "421 Misdirected Request",
        422 => "422 Unprocessable Entity",
        423 => "423 Locked",
        424 => "424 Failed Dependency",
        425 => "425 Too Early",
        426 => "426 Upgrade Required",
        428 => "428 Precondition Required",
        429 => "429 Too Many Requests",
        431 => "431 Request Header Fields Too Large",
        451 => "451 Unavailable For Legal Reasons",
        500 => "500 Internal Server Error",
        501 => "501 Not Implemented",
        502 => "502 Bad Gateway",
        503 => "503 Service Unavailable",
        504 => "504 Gateway Timeout",
        505 => "505 HTTP Version Not Supported",
        506 => "506 Variant Also Negotiates",
        507 => "507 Insufficient Storage",
        508 => "508 Loop Detected",
        510 => "510 Not Extended",
        511 => "511 Network Authentication Required",
        else => null,
    };
}

/// 返回标准 HTTP 原因短语（如 `"OK"`、`"Not Found"`），未知状态码返回 `null`。
pub fn statusText(code: u16): string? {
    if (line := statusLine(code)) {
        return line[4..];
    }
    return null;
}

/// 返回标准 HTTP 原因短语，未知状态码返回空字符串 `""`。
pub fn statusTextOrEmpty(code: u16): string {
    if (text := statusText(code)) {
        return text;
    }
    return "";
}

/// 将 HTTP 状态码归类到 RFC 9110 五大类或 `Unknown`。
pub fn classify(code: u16): StatusClass {
    if (code >= 100 && code <= 199) {
        return StatusClass.Informational;
    }
    if (code >= 200 && code <= 299) {
        return StatusClass.Success;
    }
    if (code >= 300 && code <= 399) {
        return StatusClass.Redirection;
    }
    if (code >= 400 && code <= 499) {
        return StatusClass.ClientError;
    }
    if (code >= 500 && code <= 599) {
        return StatusClass.ServerError;
    }
    return StatusClass.Unknown;
}

/// 是否为 1xx 信息响应（100..199）。
pub fn isInformational(code: u16): bool {
    return code >= 100 && code <= 199;
}

/// 是否为 2xx 成功响应（200..299）。
pub fn isSuccess(code: u16): bool {
    return code >= 200 && code <= 299;
}

/// 是否为 3xx 重定向响应（300..399）。
pub fn isRedirect(code: u16): bool {
    return code >= 300 && code <= 399;
}

/// 是否为 4xx 客户端错误响应（400..499）。
pub fn isClientError(code: u16): bool {
    return code >= 400 && code <= 499;
}

/// 是否为 5xx 服务端错误响应（500..599）。
pub fn isServerError(code: u16): bool {
    return code >= 500 && code <= 599;
}

/// 是否为错误响应（4xx 或 5xx，即 400..599）。
pub fn isError(code: u16): bool {
    return code >= 400 && code <= 599;
}

/// 判断该状态码对应的 HTTP 响应是否允许携带消息体（RFC 9110 §6.4.1）。
/// 1xx、204 (No Content)、304 (Not Modified) 以及越界状态码不允许携带消息体。
pub fn canHaveBody(code: u16): bool {
    if (code < 100 || code > 599) {
        return false;
    }
    if (isInformational(code) || code == 204 || code == 304) {
        return false;
    }
    return true;
}

/// 判断该状态码在无显式 Cache-Control 时是否默认可启发式缓存（RFC 9110 §15.1）。
pub fn isCacheableByDefault(code: u16): bool {
    return switch (code) {
        200, 203, 204, 206, 300, 301, 308, 404, 405, 410, 414, 501 => true,
        else => false,
    };
}

/// 判断该状态码是否属于典型的瞬态可重试错误（408/425/429/500/502/503/504）。
pub fn isRetryable(code: u16): bool {
    return switch (code) {
        408, 425, 429, 500, 502, 503, 504 => true,
        else => false,
    };
}
