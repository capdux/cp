// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z
// lib/http/status.cp
// RFC 9110 Complete HTTP Status Code Registry & Reason Phrase Lookup

pub struct HttpStatusEntry {
    pub code: u16 = 0;
    pub phrase: []const u8 = "";
}

pub const HTTP_STATUS_TABLE: [64]HttpStatusEntry = [
    // 1xx Informational
    HttpStatusEntry{ code: 100, phrase: "Continue" },
    HttpStatusEntry{ code: 101, phrase: "Switching Protocols" },
    HttpStatusEntry{ code: 102, phrase: "Processing" },
    HttpStatusEntry{ code: 103, phrase: "Early Hints" },

    // 2xx Successful
    HttpStatusEntry{ code: 200, phrase: "OK" },
    HttpStatusEntry{ code: 201, phrase: "Created" },
    HttpStatusEntry{ code: 202, phrase: "Accepted" },
    HttpStatusEntry{ code: 203, phrase: "Non-Authoritative Information" },
    HttpStatusEntry{ code: 204, phrase: "No Content" },
    HttpStatusEntry{ code: 205, phrase: "Reset Content" },
    HttpStatusEntry{ code: 206, phrase: "Partial Content" },
    HttpStatusEntry{ code: 207, phrase: "Multi-Status" },
    HttpStatusEntry{ code: 208, phrase: "Already Reported" },
    HttpStatusEntry{ code: 226, phrase: "IM Used" },

    // 3xx Redirection
    HttpStatusEntry{ code: 300, phrase: "Multiple Choices" },
    HttpStatusEntry{ code: 301, phrase: "Moved Permanently" },
    HttpStatusEntry{ code: 302, phrase: "Found" },
    HttpStatusEntry{ code: 303, phrase: "See Other" },
    HttpStatusEntry{ code: 304, phrase: "Not Modified" },
    HttpStatusEntry{ code: 305, phrase: "Use Proxy" },
    HttpStatusEntry{ code: 307, phrase: "Temporary Redirect" },
    HttpStatusEntry{ code: 308, phrase: "Permanent Redirect" },

    // 4xx Client Error
    HttpStatusEntry{ code: 400, phrase: "Bad Request" },
    HttpStatusEntry{ code: 401, phrase: "Unauthorized" },
    HttpStatusEntry{ code: 402, phrase: "Payment Required" },
    HttpStatusEntry{ code: 403, phrase: "Forbidden" },
    HttpStatusEntry{ code: 404, phrase: "Not Found" },
    HttpStatusEntry{ code: 405, phrase: "Method Not Allowed" },
    HttpStatusEntry{ code: 406, phrase: "Not Acceptable" },
    HttpStatusEntry{ code: 407, phrase: "Proxy Authentication Required" },
    HttpStatusEntry{ code: 408, phrase: "Request Timeout" },
    HttpStatusEntry{ code: 409, phrase: "Conflict" },
    HttpStatusEntry{ code: 410, phrase: "Gone" },
    HttpStatusEntry{ code: 411, phrase: "Length Required" },
    HttpStatusEntry{ code: 412, phrase: "Precondition Failed" },
    HttpStatusEntry{ code: 413, phrase: "Content Too Large" },
    HttpStatusEntry{ code: 414, phrase: "URI Too Long" },
    HttpStatusEntry{ code: 415, phrase: "Unsupported Media Type" },
    HttpStatusEntry{ code: 416, phrase: "Range Not Satisfiable" },
    HttpStatusEntry{ code: 417, phrase: "Expectation Failed" },
    HttpStatusEntry{ code: 418, phrase: "I'm a teapot" },
    HttpStatusEntry{ code: 421, phrase: "Misdirected Request" },
    HttpStatusEntry{ code: 422, phrase: "Unprocessable Content" },
    HttpStatusEntry{ code: 423, phrase: "Locked" },
    HttpStatusEntry{ code: 424, phrase: "Failed Dependency" },
    HttpStatusEntry{ code: 425, phrase: "Too Early" },
    HttpStatusEntry{ code: 426, phrase: "Upgrade Required" },
    HttpStatusEntry{ code: 428, phrase: "Precondition Required" },
    HttpStatusEntry{ code: 429, phrase: "Too Many Requests" },
    HttpStatusEntry{ code: 431, phrase: "Request Header Fields Too Large" },
    HttpStatusEntry{ code: 451, phrase: "Unavailable For Legal Reasons" },

    // 5xx Server Error
    HttpStatusEntry{ code: 500, phrase: "Internal Server Error" },
    HttpStatusEntry{ code: 501, phrase: "Not Implemented" },
    HttpStatusEntry{ code: 502, phrase: "Bad Gateway" },
    HttpStatusEntry{ code: 503, phrase: "Service Unavailable" },
    HttpStatusEntry{ code: 504, phrase: "Gateway Timeout" },
    HttpStatusEntry{ code: 505, phrase: "HTTP Version Not Supported" },
    HttpStatusEntry{ code: 506, phrase: "Variant Also Negotiates" },
    HttpStatusEntry{ code: 507, phrase: "Insufficient Storage" },
    HttpStatusEntry{ code: 508, phrase: "Loop Detected" },
    HttpStatusEntry{ code: 510, phrase: "Not Extended" },
    HttpStatusEntry{ code: 511, phrase: "Network Authentication Required" },
    HttpStatusEntry{ code: 599, phrase: "Network Connect Timeout Error" },
    HttpStatusEntry{ code: 444, phrase: "Connection Closed Without Response" },
];

pub struct HTTPStatus {
    pub static fn reasonPhraseOpt(code: u16): ([]const u8)? {
        let i: usize = 0;
        while (i < HTTP_STATUS_TABLE.len) : (i += 1) {
            if (HTTP_STATUS_TABLE[i].code == code) {
                return HTTP_STATUS_TABLE[i].phrase;
            }
        }
        return null;
    }

    pub static fn reasonPhrase(code: u16): []const u8 {
        if (p := HTTPStatus.reasonPhraseOpt(code)) {
            return p;
        }
        return "Unknown";
    }

    pub static fn isInformational(code: u16): bool {
        return code >= 100 && code < 200;
    }

    pub static fn isSuccess(code: u16): bool {
        return code >= 200 && code < 300;
    }

    pub static fn isRedirection(code: u16): bool {
        return code >= 300 && code < 400;
    }

    pub static fn isClientError(code: u16): bool {
        return code >= 400 && code < 500;
    }

    pub static fn isServerError(code: u16): bool {
        return code >= 500 && code < 600;
    }

    pub static fn isError(code: u16): bool {
        return code >= 400 && code < 600;
    }

    /// RFC 9110 §15: 1xx, 204, and 304 responses must not contain a message body.
    pub static fn allowsBody(code: u16): bool {
        if (code >= 100 && code < 200) return false;
        if (code == 204 || code == 304) return false;
        return true;
    }

    /// RFC 9110 §15.1: Status codes heuristically cacheable by default.
    pub static fn isCacheableByDefault(code: u16): bool {
        return switch (code) {
            200, 203, 204, 206, 300, 301, 404, 405, 410, 414, 501 => true,
            else => false,
        };
    }
}
