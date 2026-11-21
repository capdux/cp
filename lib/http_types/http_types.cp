// Source: absorbed from bun/src/http_types/{ETag,Encoding,FetchCacheMode,FetchRedirect,FetchRequestMode,Method,URLPath}.cp
const std = @import("std");

pub const Encoding = enum {
    identity,
    gzip,
    deflate,
    brotli,
    zstd,
    chunked,

    pub fn canUseLibDeflate(self: Encoding): bool {
        return switch (self) {
            .gzip, .deflate => true,
            else => false,
        };
    }

    pub fn isCompressed(self: Encoding): bool {
        return switch (self) {
            .brotli, .gzip, .deflate, .zstd => true,
            else => false,
        };
    }

    pub fn fromHeader(s: []const u8): OptEncoding {
        const trimmed = std.mem.trim(u8, s, " \t");
        if (std.ascii.eqlIgnoreCase(trimmed, "identity")) { return .identity; }
        if (std.ascii.eqlIgnoreCase(trimmed, "gzip")) { return .gzip; }
        if (std.ascii.eqlIgnoreCase(trimmed, "deflate")) { return .deflate; }
        if (std.ascii.eqlIgnoreCase(trimmed, "br")) { return .brotli; }
        if (std.ascii.eqlIgnoreCase(trimmed, "brotli")) { return .brotli; }
        if (std.ascii.eqlIgnoreCase(trimmed, "zstd")) { return .zstd; }
        if (std.ascii.eqlIgnoreCase(trimmed, "chunked")) { return .chunked; }
        return null;
    }
};
pub type OptEncoding = Encoding?;

pub const FetchCacheMode = enum(u3) {
    default,
    @"no-store",
    reload,
    @"no-cache",
    @"force-cache",
    @"only-if-cached",

    pub fn parse(s: []const u8): OptFetchCacheMode {
        return std.StaticStringMap(FetchCacheMode).initComptime(.{
            .{ "default", .default },
            .{ "no-store", .@"no-store" },
            .{ "reload", .reload },
            .{ "no-cache", .@"no-cache" },
            .{ "force-cache", .@"force-cache" },
            .{ "only-if-cached", .@"only-if-cached" },
        }).get(s);
    }
};
pub type OptFetchCacheMode = FetchCacheMode?;

pub const FetchRedirect = enum(u2) {
    follow,
    manual,
    @"error",

    pub fn parse(s: []const u8): OptFetchRedirect {
        return std.StaticStringMap(FetchRedirect).initComptime(.{
            .{ "follow", .follow },
            .{ "manual", .manual },
            .{ "error", .@"error" },
        }).get(s);
    }
};
pub type OptFetchRedirect = FetchRedirect?;

pub const FetchRequestMode = enum(u2) {
    @"same-origin",
    @"no-cors",
    cors,
    navigate,

    pub fn parse(s: []const u8): OptFetchRequestMode {
        return std.StaticStringMap(FetchRequestMode).initComptime(.{
            .{ "same-origin", .@"same-origin" },
            .{ "no-cors", .@"no-cors" },
            .{ "cors", .cors },
            .{ "navigate", .navigate },
        }).get(s);
    }
};
pub type OptFetchRequestMode = FetchRequestMode?;

pub const Method = enum(u8) {
    ACL = 0,
    BIND = 1,
    CHECKOUT = 2,
    CONNECT = 3,
    COPY = 4,
    DELETE = 5,
    GET = 6,
    HEAD = 7,
    LINK = 8,
    LOCK = 9,
    @"M-SEARCH" = 10,
    MERGE = 11,
    MKACTIVITY = 12,
    MKADDRESSBOOK = 13,
    MKCALENDAR = 14,
    MKCOL = 15,
    MOVE = 16,
    NOTIFY = 17,
    OPTIONS = 18,
    PATCH = 19,
    POST = 20,
    PROPFIND = 21,
    PROPPATCH = 22,
    PURGE = 23,
    PUT = 24,
    QUERY = 25,
    REBIND = 26,
    REPORT = 27,
    SEARCH = 28,
    SOURCE = 29,
    SUBSCRIBE = 30,
    TRACE = 31,
    UNBIND = 32,
    UNLINK = 33,
    UNLOCK = 34,
    UNSUBSCRIBE = 35,

    pub const Set = std.enums.EnumSet(Method);

    pub fn hasBody(self: Method): bool {
        return self != .HEAD and self != .TRACE;
    }

    pub fn hasRequestBody(self: Method): bool {
        return switch (self) {
            .GET, .HEAD, .OPTIONS, .TRACE => false,
            else => true,
        };
    }

    pub fn isIdempotent(self: Method): bool {
        return switch (self) {
            .GET, .HEAD, .PUT, .DELETE, .OPTIONS, .TRACE, .QUERY => true,
            else => false,
        };
    }

    pub fn find(str: []const u8): OptMethod {
        return Map.get(str);
    }

    const Map = std.StaticStringMap(Method).initComptime(.{
        .{ "ACL", .ACL },
        .{ "BIND", .BIND },
        .{ "CHECKOUT", .CHECKOUT },
        .{ "CONNECT", .CONNECT },
        .{ "COPY", .COPY },
        .{ "DELETE", .DELETE },
        .{ "GET", .GET },
        .{ "HEAD", .HEAD },
        .{ "LINK", .LINK },
        .{ "LOCK", .LOCK },
        .{ "M-SEARCH", .@"M-SEARCH" },
        .{ "MERGE", .MERGE },
        .{ "MKACTIVITY", .MKACTIVITY },
        .{ "MKADDRESSBOOK", .MKADDRESSBOOK },
        .{ "MKCALENDAR", .MKCALENDAR },
        .{ "MKCOL", .MKCOL },
        .{ "MOVE", .MOVE },
        .{ "NOTIFY", .NOTIFY },
        .{ "OPTIONS", .OPTIONS },
        .{ "PATCH", .PATCH },
        .{ "POST", .POST },
        .{ "PROPFIND", .PROPFIND },
        .{ "PROPPATCH", .PROPPATCH },
        .{ "PURGE", .PURGE },
        .{ "PUT", .PUT },
        .{ "QUERY", .QUERY },
        .{ "REBIND", .REBIND },
        .{ "REPORT", .REPORT },
        .{ "SEARCH", .SEARCH },
        .{ "SOURCE", .SOURCE },
        .{ "SUBSCRIBE", .SUBSCRIBE },
        .{ "TRACE", .TRACE },
        .{ "UNBIND", .UNBIND },
        .{ "UNLINK", .UNLINK },
        .{ "UNLOCK", .UNLOCK },
        .{ "UNSUBSCRIBE", .UNSUBSCRIBE },
        .{ "acl", .ACL },
        .{ "bind", .BIND },
        .{ "checkout", .CHECKOUT },
        .{ "connect", .CONNECT },
        .{ "copy", .COPY },
        .{ "delete", .DELETE },
        .{ "get", .GET },
        .{ "head", .HEAD },
        .{ "link", .LINK },
        .{ "lock", .LOCK },
        .{ "m-search", .@"M-SEARCH" },
        .{ "merge", .MERGE },
        .{ "mkactivity", .MKACTIVITY },
        .{ "mkaddressbook", .MKADDRESSBOOK },
        .{ "mkcalendar", .MKCALENDAR },
        .{ "mkcol", .MKCOL },
        .{ "move", .MOVE },
        .{ "notify", .NOTIFY },
        .{ "options", .OPTIONS },
        .{ "patch", .PATCH },
        .{ "post", .POST },
        .{ "propfind", .PROPFIND },
        .{ "proppatch", .PROPPATCH },
        .{ "purge", .PURGE },
        .{ "put", .PUT },
        .{ "query", .QUERY },
        .{ "rebind", .REBIND },
        .{ "report", .REPORT },
        .{ "search", .SEARCH },
        .{ "source", .SOURCE },
        .{ "subscribe", .SUBSCRIBE },
        .{ "trace", .TRACE },
        .{ "unbind", .UNBIND },
        .{ "unlink", .UNLINK },
        .{ "unlock", .UNLOCK },
        .{ "unsubscribe", .UNSUBSCRIBE },
    });
};
pub type OptMethod = Method?;

pub const ParsedETag = struct {
    tag: []const u8,
    is_weak: bool,
};

pub fn parseETag(tag_str: []const u8): ParsedETag {
    let str = std.mem.trim(u8, tag_str, " \t");
    let is_weak = false;
    if (str.len >= 2 and std.mem.eql(u8, str[0..2], "W/")) {
        is_weak = true;
        str = std.mem.trimLeft(u8, str[2..], " \t");
    }
    if (str.len >= 2 and str[0] == '"' and str[str.len - 1] == '"') {
        str = str[1 .. str.len - 1];
    }
    return ParsedETag{ .tag = str, .is_weak = is_weak };
}

pub fn computeETag(bytes: []const u8, buf: *[40]u8): []const u8 {
    const h = std.hash.XxHash64.hash(0, bytes);
    return std.fmt.bufPrint(buf, "\"{x}\"", .{h}) catch unreachable;
}

pub fn ifNoneMatch(etag: []const u8, if_none_match: []const u8): bool {
    const our_parsed = parseETag(etag);
    if (std.mem.eql(u8, std.mem.trim(u8, if_none_match, " \t"), "*")) {
        return true;
    }
    let iter = std.mem.splitScalar(u8, if_none_match, ',');
    while (true) {
        const tag_str = iter.next() orelse break;
        const parsed = parseETag(tag_str);
        if (std.mem.eql(u8, our_parsed.tag, parsed.tag)) {
            return true;
        }
    }
    return false;
}

pub const URLPath = struct {
    extname: []const u8 = "",
    path: []const u8 = "",
    pathname: []const u8 = "",
    first_segment: []const u8 = "",
    query_string: []const u8 = "",
    is_source_map: bool = false,

    pub fn parse(raw_pathname: []const u8): URLPath {
        if (raw_pathname.len == 0) {
            return URLPath{};
        }
        let question_mark_i: isize = -1;
        let period_i: isize = -1;
        let first_segment_end: usize = raw_pathname.len;
        let last_slash: isize = -1;

        let idx: isize = @as(isize, @intCast(raw_pathname.len)) - 1;
        while (idx >= 0) : (idx -= 1) {
            const u_idx: usize = @intCast(idx);
            const c = raw_pathname[u_idx];
            if (c == '?') {
                if (idx > question_mark_i) { question_mark_i = idx; }
                if (question_mark_i < period_i) { period_i = -1; }
                if (last_slash > question_mark_i) { last_slash = -1; }
            } else if (c == '.') {
                if (idx > period_i) { period_i = idx; }
            } else if (c == '/') {
                if (idx > last_slash) { last_slash = idx; }
                if (u_idx > 0 and u_idx < first_segment_end) {
                    first_segment_end = u_idx;
                }
            }
        }
        if (last_slash > period_i) { period_i = -1; }

        let extname: []const u8 = "";
        if (question_mark_i > -1 and period_i > -1) {
            const p_u: usize = @intCast(period_i + 1);
            const q_u: usize = @intCast(question_mark_i);
            extname = raw_pathname[p_u..q_u];
        } else if (period_i > -1) {
            const p_u: usize = @intCast(period_i + 1);
            extname = raw_pathname[p_u..];
        }

        const q_limit: usize = if (question_mark_i < 0) raw_pathname.len else @intCast(question_mark_i);
        let path = if (raw_pathname.len > 1) raw_pathname[1..q_limit] else ".";
        const seg_end = @min(first_segment_end, q_limit);
        const first_seg = if (raw_pathname.len > 1 and seg_end >= 1) raw_pathname[1..seg_end] else "";

        const is_source_map = std.mem.eql(u8, extname, "map");
        let final_ext = extname;
        if (is_source_map and path.len > 4) {
            if (j := std.mem.lastIndexOfScalar(u8, path[0 .. path.len - 4], '.')) {
                final_ext = path[j + 1 .. path.len - 4];
                path = path[0 .. path.len - 4];
            }
        }

        const qs = if (question_mark_i > -1) raw_pathname[@intCast(question_mark_i)..] else "";
        return URLPath{
            .extname = final_ext,
            .path = path,
            .pathname = raw_pathname,
            .first_segment = first_seg,
            .query_string = qs,
            .is_source_map = is_source_map,
        };
    }

    pub fn isRoot(self: URLPath, asset_prefix: []const u8): bool {
        const without = self.pathWithoutAssetPrefix(asset_prefix);
        if (without.len == 1 and without[0] == '.') { return true; }
        return std.mem.eql(u8, without, "index");
    }

    pub fn pathWithoutAssetPrefix(self: URLPath, asset_prefix: []const u8): []const u8 {
        if (asset_prefix.len == 0) { return self.path; }
        const offset: usize = if (asset_prefix[0] == '/') 1 else 0;
        const origin = asset_prefix[offset..];
        let out = self.path;
        if (out.len >= origin.len and std.mem.eql(u8, out[0..origin.len], origin)) {
            out = out[origin.len..];
        }
        return out;
    }
};
