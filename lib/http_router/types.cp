// Source: cp/lib/http_router/types.cp (Radix Trie HTTP Router Core Types)

pub const MAX_PARAMS: usize = 16;

pub const RouterError = error {
    OutOfMemory,
    InvalidPattern,
    InvalidCatchAll,
    RouteConflict,
    TooManyParams,
};

pub enum HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,

    pub fn index(): usize {
        return switch (_) {
            .GET => 0,
            .POST => 1,
            .PUT => 2,
            .DELETE => 3,
            .PATCH => 4,
            .HEAD => 5,
            .OPTIONS => 6,
        };
    }

    pub fn bit(): u8 {
        return @as(u8, 1) << @as(u3, @intCast(_.index()));
    }

    pub fn toString(): []const u8 {
        return switch (_) {
            .GET => "GET",
            .POST => "POST",
            .PUT => "PUT",
            .DELETE => "DELETE",
            .PATCH => "PATCH",
            .HEAD => "HEAD",
            .OPTIONS => "OPTIONS",
        };
    }

    pub fn fromString(s: []const u8): HttpMethod? {
        if (std.mem.eql(u8, s, "GET")) return .GET;
        if (std.mem.eql(u8, s, "POST")) return .POST;
        if (std.mem.eql(u8, s, "PUT")) return .PUT;
        if (std.mem.eql(u8, s, "DELETE")) return .DELETE;
        if (std.mem.eql(u8, s, "PATCH")) return .PATCH;
        if (std.mem.eql(u8, s, "HEAD")) return .HEAD;
        if (std.mem.eql(u8, s, "OPTIONS")) return .OPTIONS;
        return null;
    }
}

pub struct PathParam {
    name: []const u8,
    value: []const u8,
}

pub struct RouteMatch {
    handler_id: usize,
    params: [MAX_PARAMS]PathParam,
    param_count: usize,

    pub fn init(handler_id: usize): RouteMatch {
        let rm: RouteMatch = undefined;
        rm.handler_id = handler_id;
        rm.param_count = 0;
        return rm;
    }

    pub fn getParam(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.param_count) {
            if (std.mem.eql(u8, _.params[i].name, name)) {
                return _.params[i].value;
            }
            i += 1;
        }
        return null;
    }

    pub fn paramSlice(): []const PathParam {
        return _.params[0.._.param_count];
    }
}

pub struct AllowInfo {
    buf: [64]u8,
    len: usize,

    pub fn fromMask(mask: u8): AllowInfo {
        let info: AllowInfo = undefined;
        info.len = 0;
        const methods = [
            HttpMethod.GET,
            HttpMethod.POST,
            HttpMethod.PUT,
            HttpMethod.DELETE,
            HttpMethod.PATCH,
            HttpMethod.HEAD,
            HttpMethod.OPTIONS
        ];
        let first = true;
        for (m of methods) {
            if ((mask & m.bit()) != 0) {
                if (!first) {
                    if (info.len + 2 <= info.buf.len) {
                        info.buf[info.len] = ',';
                        info.buf[info.len + 1] = ' ';
                        info.len += 2;
                    }
                }
                const s = m.toString();
                if (info.len + s.len <= info.buf.len) {
                    let j: usize = 0;
                    while (j < s.len) {
                        info.buf[info.len + j] = s[j];
                        j += 1;
                    }
                    info.len += s.len;
                }
                first = false;
            }
        }
        return info;
    }

    pub fn allowHeader(): []const u8 {
        return _.buf[0.._.len];
    }
}

pub union RouteResult {
    matched: RouteMatch,
    method_not_allowed: AllowInfo,
    not_found: void,

    pub fn matchInfo(): RouteMatch? {
        if (_ == .matched) return _.matched;
        return null;
    }

    pub fn allowInfo(): AllowInfo? {
        if (_ == .method_not_allowed) return _.method_not_allowed;
        return null;
    }

    pub fn isNotFound(): bool {
        return _ == .not_found;
    }
}
