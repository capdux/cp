// Source: Custom implementation by Antigravity AI
// lib/http/method.cp
// HTTP Method enumeration and helper functions

pub enum Method: u8 {
    ACL,
    BIND,
    CHECKOUT,
    CONNECT,
    COPY,
    DELETE,
    GET,
    HEAD,
    LINK,
    LOCK,
    MERGE,
    MKACTIVITY,
    MKCALENDAR,
    MKCOL,
    MOVE,
    NOTIFY,
    OPTIONS,
    PATCH,
    POST,
    PROPFIND,
    PROPPATCH,
    PURGE,
    PUT,
    QUERY,
    REBIND,
    REPORT,
    SEARCH,
    SOURCE,
    SUBSCRIBE,
    TRACE,
    UNBIND,
    UNLINK,
    UNLOCK,
    UNSUBSCRIBE,

    fn isIdempotent(): bool {
        return switch (_) {
            .GET, .HEAD, .PUT, .DELETE, .OPTIONS, .TRACE, .QUERY => true,
            else => false,
        };
    }

    fn hasRequestBody(): bool {
        return switch (_) {
            .GET, .HEAD, .OPTIONS, .TRACE => false,
            else => true,
        };
    }

    fn toString(): string {
        return @tagName(_);
    }
}
