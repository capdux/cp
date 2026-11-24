// Source: cp/lib/http_router/types.cp (Radix Trie HTTP Router Core Types)

pub const MAX_PARAMS: usize = 16;

pub const RouterError = error {
    OutOfMemory,
    InvalidPattern,
    InvalidCatchAll,
    RouteConflict,
    TooManyParams,
};

pub enum RouterMethod {
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

    pub fn fromString(s: []const u8): RouterMethod? {
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

    pub static fn init(handler_id: usize): RouteMatch {
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
            RouterMethod.GET,
            RouterMethod.POST,
            RouterMethod.PUT,
            RouterMethod.DELETE,
            RouterMethod.PATCH,
            RouterMethod.HEAD,
            RouterMethod.OPTIONS
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

type StrSlice = []const u8;

const MAX_SEGMENTS: usize = 64;

enum NodeKind {
    kind_static,
    kind_param,
    kind_catch_all,
}

struct SegSpan {
    start: usize,
    end: usize,
}

struct TrieNode {
    segment: []const u8,
    kind: NodeKind,
    method_mask: u8,
    handlers: [7]usize,
    static_children: std.ArrayList(*TrieNode),
    param_child: (*TrieNode)?,
    catch_all_child: (*TrieNode)?,

    static fn create(alloc: std.mem.Allocator, segment: []const u8, kind: NodeKind): RouterError!*TrieNode {
        let node = alloc.create(TrieNode) catch return RouterError.OutOfMemory;
        node.* = TrieNode{
            .segment = segment,
            .kind = kind,
            .method_mask = 0,
            .handlers = [0, 0, 0, 0, 0, 0, 0],
            .static_children = std.ArrayList(*TrieNode).empty,
            .param_child = null,
            .catch_all_child = null,
        };
        return node;
    }

    fn destroy(*_, alloc: std.mem.Allocator) {
        for (child of _.static_children.items) {
            child.destroy(alloc);
        }
        _.static_children.deinit(alloc);
        if (p := _.param_child) {
            p.destroy(alloc);
        }
        if (c := _.catch_all_child) {
            c.destroy(alloc);
        }
        alloc.destroy(_);
    }
}

fn splitSegments(path: []const u8, out_spans: *[MAX_SEGMENTS]SegSpan): usize {
    let count: usize = 0;
    let i: usize = 0;
    const len = path.len;
    while (i < len) {
        if (path[i] == '?') break;
        while (i < len and path[i] == '/') {
            i += 1;
        }
        if (i >= len or path[i] == '?') break;
        const start = i;
        while (i < len and path[i] != '/' and path[i] != '?') {
            i += 1;
        }
        if (count < MAX_SEGMENTS) {
            out_spans[count] = SegSpan{ .start = start, .end = i };
            count += 1;
        }
    }
    return count;
}

pub struct RouteGroup {
    router: *HttpRouter,
    prefix: []const u8,

    pub fn addRoute(method: RouterMethod, pattern: []const u8, handler_id: usize): RouterError!void {
        const joined = try _.router.joinPrefix(_.prefix, pattern);
        try _.router.addRoute(method, joined, handler_id);
    }

    pub fn get(pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.GET, pattern, handler_id);
    }

    pub fn post(pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.POST, pattern, handler_id);
    }

    pub fn put(pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.PUT, pattern, handler_id);
    }

    pub fn delete(pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.DELETE, pattern, handler_id);
    }

    pub fn patch(pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.PATCH, pattern, handler_id);
    }

    pub fn head(pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.HEAD, pattern, handler_id);
    }

    pub fn options(pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.OPTIONS, pattern, handler_id);
    }

    pub fn group(sub_prefix: []const u8): RouterError!RouteGroup {
        const joined = try _.router.joinPrefix(_.prefix, sub_prefix);
        return RouteGroup{
            .router = _.router,
            .prefix = joined,
        };
    }
}

pub struct HttpRouter {
    alloc: std.mem.Allocator,
    root: *TrieNode,
    owned_strings: std.ArrayList(StrSlice),

    pub fn init(alloc: std.mem.Allocator): RouterError!HttpRouter {
        const root = try TrieNode.create(alloc, "", .kind_static);
        return HttpRouter{
            .alloc = alloc,
            .root = root,
            .owned_strings = std.ArrayList(StrSlice).empty,
        };
    }

    pub fn deinit(*_) {
        _.root.destroy(_.alloc);
        for (s of _.owned_strings.items) {
            _.alloc.free(s);
        }
        _.owned_strings.deinit(_.alloc);
    }

    fn dupString(*_, s: []const u8): RouterError![]const u8 {
        const copy = _.alloc.dupe(u8, s) catch return RouterError.OutOfMemory;
        _.owned_strings.append(_.alloc, copy) catch {
            _.alloc.free(copy);
            return RouterError.OutOfMemory;
        };
        return copy;
    }

    fn joinPrefix(*_, prefix: []const u8, sub: []const u8): RouterError![]const u8 {
        let p_spans: [MAX_SEGMENTS]SegSpan = undefined;
        let s_spans: [MAX_SEGMENTS]SegSpan = undefined;
        const p_count = splitSegments(prefix, &p_spans);
        const s_count = splitSegments(sub, &s_spans);

        let total_len: usize = 0;
        let i: usize = 0;
        while (i < p_count) {
            total_len += 1 + (p_spans[i].end - p_spans[i].start);
            i += 1;
        }
        i = 0;
        while (i < s_count) {
            total_len += 1 + (s_spans[i].end - s_spans[i].start);
            i += 1;
        }
        if (total_len == 0) {
            return try _.dupString("/");
        }

        let buf = _.alloc.alloc(u8, total_len) catch return RouterError.OutOfMemory;
        let pos: usize = 0;
        i = 0;
        while (i < p_count) {
            buf[pos] = '/';
            pos += 1;
            const seg = prefix[p_spans[i].start..p_spans[i].end];
            let j: usize = 0;
            while (j < seg.len) {
                buf[pos + j] = seg[j];
                j += 1;
            }
            pos += seg.len;
            i += 1;
        }
        i = 0;
        while (i < s_count) {
            buf[pos] = '/';
            pos += 1;
            const seg = sub[s_spans[i].start..s_spans[i].end];
            let j: usize = 0;
            while (j < seg.len) {
                buf[pos + j] = seg[j];
                j += 1;
            }
            pos += seg.len;
            i += 1;
        }

        _.owned_strings.append(_.alloc, buf) catch {
            _.alloc.free(buf);
            return RouterError.OutOfMemory;
        };
        return buf;
    }

    pub fn addRoute(*_, method: RouterMethod, pattern: []const u8, handler_id: usize): RouterError!void {
        let spans: [MAX_SEGMENTS]SegSpan = undefined;
        const count = splitSegments(pattern, &spans);

        let param_count: usize = 0;
        let i: usize = 0;
        while (i < count) {
            const seg = pattern[spans[i].start..spans[i].end];
            if (seg[0] == '*') {
                if (i + 1 != count) return RouterError.InvalidCatchAll;
                if (seg.len <= 1) return RouterError.InvalidPattern;
                param_count += 1;
            } else if (seg[0] == ':') {
                if (seg.len <= 1) return RouterError.InvalidPattern;
                param_count += 1;
            }
            i += 1;
        }
        if (param_count > MAX_PARAMS) {
            return RouterError.TooManyParams;
        }

        let curr = _.root;
        i = 0;
        while (i < count) {
            const seg = pattern[spans[i].start..spans[i].end];
            if (seg[0] == '*') {
                const name = seg[1..];
                if (curr.catch_all_child != null) {
                    const existing = curr.catch_all_child.?;
                    if (!std.mem.eql(u8, existing.segment, name)) {
                        return RouterError.RouteConflict;
                    }
                    curr = existing;
                } else {
                    const owned = try _.dupString(name);
                    const node = try TrieNode.create(_.alloc, owned, .kind_catch_all);
                    curr.catch_all_child = node;
                    curr = node;
                }
            } else if (seg[0] == ':') {
                const name = seg[1..];
                if (curr.param_child != null) {
                    const existing = curr.param_child.?;
                    if (!std.mem.eql(u8, existing.segment, name)) {
                        return RouterError.RouteConflict;
                    }
                    curr = existing;
                } else {
                    const owned = try _.dupString(name);
                    const node = try TrieNode.create(_.alloc, owned, .kind_param);
                    curr.param_child = node;
                    curr = node;
                }
            } else {
                let found: (*TrieNode)? = null;
                for (child of curr.static_children.items) {
                    if (std.mem.eql(u8, child.segment, seg)) {
                        found = child;
                        break;
                    }
                }
                if (found != null) {
                    curr = found.?;
                } else {
                    const owned = try _.dupString(seg);
                    const node = try TrieNode.create(_.alloc, owned, .kind_static);
                    curr.static_children.append(_.alloc, node) catch {
                        node.destroy(_.alloc);
                        return RouterError.OutOfMemory;
                    };
                    curr = node;
                }
            }
            i += 1;
        }

        const mbit = method.bit();
        if ((curr.method_mask & mbit) != 0) {
            return RouterError.RouteConflict;
        }
        curr.method_mask |= mbit;
        curr.handlers[method.index()] = handler_id;
    }

    pub fn get(*_, pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.GET, pattern, handler_id);
    }

    pub fn post(*_, pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.POST, pattern, handler_id);
    }

    pub fn put(*_, pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.PUT, pattern, handler_id);
    }

    pub fn delete(*_, pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.DELETE, pattern, handler_id);
    }

    pub fn patch(*_, pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.PATCH, pattern, handler_id);
    }

    pub fn head(*_, pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.HEAD, pattern, handler_id);
    }

    pub fn options(*_, pattern: []const u8, handler_id: usize): RouterError!void {
        try _.addRoute(.OPTIONS, pattern, handler_id);
    }

    pub fn group(*_, prefix: []const u8): RouterError!RouteGroup {
        const norm = try _.joinPrefix("", prefix);
        return RouteGroup{
            .router = _,
            .prefix = norm,
        };
    }

    pub fn match(method: RouterMethod, path: []const u8): RouteResult {
        let spans: [MAX_SEGMENTS]SegSpan = undefined;
        const count = splitSegments(path, &spans);
        let rm = RouteMatch.init(0);
        let allow_mask: u8 = 0;

        if (matchNode(_.root, method, path, &spans, count, 0, &rm, &allow_mask)) {
            return RouteResult{ .matched = rm };
        }
        if (allow_mask != 0) {
            return RouteResult{ .method_not_allowed = AllowInfo.fromMask(allow_mask) };
        }
        return RouteResult{ .not_found = {} };
    }
}

fn matchNode(
    node: *const TrieNode,
    method: RouterMethod,
    path: []const u8,
    spans: *const [MAX_SEGMENTS]SegSpan,
    count: usize,
    idx: usize,
    rm: *RouteMatch,
    allow_mask: *u8,
): bool {
    if (idx == count) {
        if (node.method_mask != 0) {
            if ((node.method_mask & method.bit()) != 0) {
                rm.handler_id = node.handlers[method.index()];
                return true;
            }
            allow_mask.* |= node.method_mask;
        }
        if (ca := node.catch_all_child) {
            if (ca.method_mask != 0) {
                const saved_count = rm.param_count;
                if (rm.param_count < MAX_PARAMS) {
                    rm.params[rm.param_count] = PathParam{ .name = ca.segment, .value = "" };
                    rm.param_count += 1;
                }
                if ((ca.method_mask & method.bit()) != 0) {
                    rm.handler_id = ca.handlers[method.index()];
                    return true;
                }
                allow_mask.* |= ca.method_mask;
                rm.param_count = saved_count;
            }
        }
        return false;
    }

    const seg = path[spans[idx].start..spans[idx].end];

    // 1. Static priority
    for (child of node.static_children.items) {
        if (std.mem.eql(u8, child.segment, seg)) {
            if (matchNode(child, method, path, spans, count, idx + 1, rm, allow_mask)) {
                return true;
            }
            break;
        }
    }

    // 2. Param priority
    if (p := node.param_child) {
        if (rm.param_count < MAX_PARAMS) {
            const saved_count = rm.param_count;
            rm.params[rm.param_count] = PathParam{ .name = p.segment, .value = seg };
            rm.param_count += 1;
            if (matchNode(p, method, path, spans, count, idx + 1, rm, allow_mask)) {
                return true;
            }
            rm.param_count = saved_count;
        }
    }

    // 3. Catch-All priority
    if (ca := node.catch_all_child) {
        if (ca.method_mask != 0) {
            const saved_count = rm.param_count;
            if (rm.param_count < MAX_PARAMS) {
                const start_pos = spans[idx].start;
                const end_pos = spans[count - 1].end;
                rm.params[rm.param_count] = PathParam{
                    .name = ca.segment,
                    .value = path[start_pos..end_pos],
                };
                rm.param_count += 1;
            }
            if ((ca.method_mask & method.bit()) != 0) {
                rm.handler_id = ca.handlers[method.index()];
                return true;
            }
            allow_mask.* |= ca.method_mask;
            rm.param_count = saved_count;
        }
    }

    return false;
}
