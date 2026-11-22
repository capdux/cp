// Source: Absorbed from /data/dev/bun/src/http_types/Method.cp | Synced: 2026-11-22T10:18:27Z
// lib/http/method.cp
// HTTP Method enumeration and helper functions

const std = @import("std");

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

    pub fn isIdempotent(): bool {
        return switch (_) {
            .GET, .HEAD, .PUT, .DELETE, .OPTIONS, .TRACE, .QUERY => true,
            else => false,
        };
    }

    pub fn hasRequestBody(): bool {
        return switch (_) {
            .GET, .HEAD, .OPTIONS, .TRACE => false,
            else => true,
        };
    }

    pub fn toString(): string {
        return @tagName(_);
    }

    pub static fn fromString(str: []const u8): Method? {
        if (std.ascii.eqlIgnoreCase(str, "GET")) return Method.GET;
        if (std.ascii.eqlIgnoreCase(str, "POST")) return Method.POST;
        if (std.ascii.eqlIgnoreCase(str, "PUT")) return Method.PUT;
        if (std.ascii.eqlIgnoreCase(str, "DELETE")) return Method.DELETE;
        if (std.ascii.eqlIgnoreCase(str, "HEAD")) return Method.HEAD;
        if (std.ascii.eqlIgnoreCase(str, "OPTIONS")) return Method.OPTIONS;
        if (std.ascii.eqlIgnoreCase(str, "PATCH")) return Method.PATCH;
        if (std.ascii.eqlIgnoreCase(str, "CONNECT")) return Method.CONNECT;
        if (std.ascii.eqlIgnoreCase(str, "TRACE")) return Method.TRACE;
        if (std.ascii.eqlIgnoreCase(str, "QUERY")) return Method.QUERY;
        if (std.ascii.eqlIgnoreCase(str, "PURGE")) return Method.PURGE;
        if (std.ascii.eqlIgnoreCase(str, "PROPFIND")) return Method.PROPFIND;
        if (std.ascii.eqlIgnoreCase(str, "PROPPATCH")) return Method.PROPPATCH;
        if (std.ascii.eqlIgnoreCase(str, "MKCOL")) return Method.MKCOL;
        if (std.ascii.eqlIgnoreCase(str, "COPY")) return Method.COPY;
        if (std.ascii.eqlIgnoreCase(str, "MOVE")) return Method.MOVE;
        if (std.ascii.eqlIgnoreCase(str, "LOCK")) return Method.LOCK;
        if (std.ascii.eqlIgnoreCase(str, "UNLOCK")) return Method.UNLOCK;
        return null;
    }
}
