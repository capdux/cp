// Source: Absorbed from /data/dev/bun/src/router/{Route,route_types}.cp | Synced: 2026-11-21T14:00:43Z

/// 路由类型枚举（数值越小优先级越高）
pub enum RouteKind: u8 {
    static_route = 0,
    dynamic = 1,
    catch_all = 2,
    optional_catch_all = 3,

    fn asStr(): string {
        return switch (_) {
            .static_route => "static",
            .dynamic => "dynamic",
            .catch_all => "catch_all",
            .optional_catch_all => "optional_catch_all",
        };
    }
}

/// 路径段类型枚举
pub enum SegmentKind: u8 {
    literal = 0,
    param = 1,
    catch_all = 2,
    optional_catch_all = 3,
}

/// 单个路由段定义
pub struct RouteSegment {
    kind: SegmentKind = .literal,
    text: []const u8 = "",
}

/// 零拷贝路由参数键值对
pub struct RouteParam {
    name: []const u8 = "",
    value: []const u8 = "",
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn hasInvalidParamChar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '[' || c == ']' || c == '/' || c == ' ' || c == '\t') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn hasBracket(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '[' || s[i] == ']') { return true; }
        i += 1;
    }
    return false;
}

fn sortWeight(c: u8): u8 {
    if (c == '[') { return 252; }
    if (c == ']') { return 253; }
    if (c == '/') { return 254; }
    return c;
}

fn cmpSortString(a: []const u8, b: []const u8): i8 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        const wa = sortWeight(a[i]);
        const wb = sortWeight(b[i]);
        if (wa < wb) { return -1; }
        if (wa > wb) { return 1; }
        i += 1;
    }
    if (a.len < b.len) { return -1; }
    if (a.len > b.len) { return 1; }
    return 0;
}

fn stripQueryAndTrailingSlash(pathname: []const u8): []const u8 {
    let end = pathname.len;
    let i: usize = 0;
    while (i < pathname.len) {
        if (pathname[i] == '?' || pathname[i] == '#') {
            end = i;
            break;
        }
        i += 1;
    }
    while (end > 1 && pathname[end - 1] == '/') {
        end -= 1;
    }
    return pathname[0..end];
}

/// 路由匹配结果（含命中的路由索引与最多 8 个零拷贝参数）
pub struct RouteMatch {
    route_idx: usize = 0,
    params: [8]RouteParam = [RouteParam{}; 8],
    param_len: usize = 0,

    fn getParam(param_name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.param_len) {
            if (eqlBytes(_.params[i].name, param_name)) {
                return _.params[i].value;
            }
            i += 1;
        }
        return null;
    }
}

/// 预解析的文件系统路由模式（零堆分配，支持最多 16 段与 8 参数）
pub struct RoutePattern {
    raw: []const u8 = "",
    kind: RouteKind = .static_route,
    param_count: usize = 0,
    segments: [16]RouteSegment = [RouteSegment{}; 16],
    seg_len: usize = 0,

    /// 解析路由模式（如 `/users/[id]`、`/docs/[...slug]`、`/shop/[[...cats]]`）
    /// 校验 catch-all 必须位于末段、参数名非空、括号配对合法
    static fn parse(pattern: []const u8): RoutePattern? {
        if (pattern.len == 0 || pattern[0] != '/') {
            return null;
        }

        // 归一化尾斜杠（保留根路径 "/"）
        let end = pattern.len;
        while (end > 1 && pattern[end - 1] == '/') {
            end -= 1;
        }
        const norm = pattern[0..end];

        let result = RoutePattern{
            raw: norm,
            kind: .static_route,
        };

        if (norm.len == 1) {
            // 根路径 "/"
            return result;
        }

        let i: usize = 1;
        let seen_catch_all = false;
        while (i < norm.len) {
            if (seen_catch_all) {
                // catch-all 之后不允许出现任何后续路径段
                return null;
            }
            if (result.seg_len >= 16) {
                return null;
            }

            const seg_start = i;
            while (i < norm.len && norm[i] != '/') {
                i += 1;
            }
            const seg = norm[seg_start..i];
            if (seg.len == 0) {
                // 拒绝空段如 "//"
                return null;
            }

            if (seg.len >= 2 && seg[0] == '[' && seg[seg.len - 1] == ']') {
                if (seg.len >= 4 && seg[1] == '[' && seg[seg.len - 2] == ']') {
                    // 可选 catch-all: [[...name]]
                    const inner = seg[2..(seg.len - 2)];
                    if (inner.len <= 3 || inner[0] != '.' || inner[1] != '.' || inner[2] != '.') {
                        return null;
                    }
                    const pname = inner[3..inner.len];
                    if (pname.len == 0 || hasInvalidParamChar(pname) || result.param_count >= 8) {
                        return null;
                    }
                    result.segments[result.seg_len] = RouteSegment{ kind: .optional_catch_all, text: pname };
                    result.seg_len += 1;
                    result.param_count += 1;
                    result.kind = .optional_catch_all;
                    seen_catch_all = true;
                } else {
                    const inner = seg[1..(seg.len - 1)];
                    if (inner.len >= 3 && inner[0] == '.' && inner[1] == '.' && inner[2] == '.') {
                        // 必选 catch-all: [...name]
                        const pname = inner[3..inner.len];
                        if (pname.len == 0 || hasInvalidParamChar(pname) || result.param_count >= 8) {
                            return null;
                        }
                        result.segments[result.seg_len] = RouteSegment{ kind: .catch_all, text: pname };
                        result.seg_len += 1;
                        result.param_count += 1;
                        result.kind = .catch_all;
                        seen_catch_all = true;
                    } else {
                        // 动态段: [name]
                        if (inner.len == 0 || hasInvalidParamChar(inner) || result.param_count >= 8) {
                            return null;
                        }
                        result.segments[result.seg_len] = RouteSegment{ kind: .param, text: inner };
                        result.seg_len += 1;
                        result.param_count += 1;
                        if (result.kind == .static_route) {
                            result.kind = .dynamic;
                        }
                    }
                }
            } else {
                // 静态字面量段：不允许未闭合或残留方括号
                if (hasBracket(seg)) {
                    return null;
                }
                result.segments[result.seg_len] = RouteSegment{ kind: .literal, text: seg };
                result.seg_len += 1;
            }

            if (i < norm.len && norm[i] == '/') {
                i += 1;
            }
        }

        return result;
    }

    /// 匹配请求路径并提取零拷贝路由参数
    fn match(pathname: []const u8): RouteMatch? {
        if (pathname.len == 0 || pathname[0] != '/') {
            return null;
        }
        const clean = stripQueryAndTrailingSlash(pathname);

        let res = RouteMatch{};
        let p_pos: usize = if (clean.len > 1) 1 else clean.len;
        let seg_idx: usize = 0;

        while (seg_idx < _.seg_len) {
            const seg = _.segments[seg_idx];

            if (seg.kind == .optional_catch_all) {
                const rest = if (p_pos < clean.len) clean[p_pos..clean.len] else "";
                res.params[res.param_len] = RouteParam{ name: seg.text, value: rest };
                res.param_len += 1;
                return res;
            }

            if (seg.kind == .catch_all) {
                if (p_pos >= clean.len) {
                    return null;
                }
                const rest = clean[p_pos..clean.len];
                if (rest.len == 0) {
                    return null;
                }
                res.params[res.param_len] = RouteParam{ name: seg.text, value: rest };
                res.param_len += 1;
                return res;
            }

            // literal 或 param 需要消耗一个非空路径段
            if (p_pos >= clean.len) {
                return null;
            }
            const seg_start = p_pos;
            while (p_pos < clean.len && clean[p_pos] != '/') {
                p_pos += 1;
            }
            const val = clean[seg_start..p_pos];
            if (val.len == 0) {
                return null;
            }

            if (seg.kind == .literal) {
                if (!eqlBytes(val, seg.text)) {
                    return null;
                }
            } else {
                res.params[res.param_len] = RouteParam{ name: seg.text, value: val };
                res.param_len += 1;
            }

            if (p_pos < clean.len && clean[p_pos] == '/') {
                p_pos += 1;
            }
            seg_idx += 1;
        }

        if (p_pos < clean.len) {
            return null;
        }
        return res;
    }

    /// 比较路由匹配优先级：返回 -1 表示当前路由优先级更高，1 表示更低，0 表示相等
    /// 规则：静态 > 动态 > catch-all > optional-catch-all，同级别按段具体度与字母序排序
    fn comparePriority(other: RoutePattern): i8 {
        const k1 = @intFromEnum(_.kind);
        const k2 = @intFromEnum(other.kind);
        if (k1 < k2) { return -1; }
        if (k1 > k2) { return 1; }

        if (_.kind == .catch_all || _.kind == .optional_catch_all) {
            // 前缀段越多（更具体）优先级越高
            if (_.seg_len > other.seg_len) { return -1; }
            if (_.seg_len < other.seg_len) { return 1; }
            if (_.param_count > other.param_count) { return -1; }
            if (_.param_count < other.param_count) { return 1; }
        } else if (_.kind == .dynamic) {
            // 逐段比较：静态字面量段优先于动态参数段
            const min_segs = if (_.seg_len < other.seg_len) _.seg_len else other.seg_len;
            let i: usize = 0;
            while (i < min_segs) {
                const s1 = @intFromEnum(_.segments[i].kind);
                const s2 = @intFromEnum(other.segments[i].kind);
                if (s1 < s2) { return -1; }
                if (s1 > s2) { return 1; }
                i += 1;
            }
        }

        return cmpSortString(_.raw, other.raw);
    }
}

/// 固定容量（64 路由）零堆分配文件系统路由器
pub struct FsRouter {
    routes: [64]RoutePattern = [RoutePattern{}; 64],
    len: usize = 0,

    static fn init(): FsRouter {
        return FsRouter{};
    }

    /// 注册路由模式，返回分配的稳定路由索引；模式非法或超出 64 条容量返回 null
    fn addRoute(*_, pattern: []const u8): usize? {
        if (_.len >= 64) {
            return null;
        }
        if (parsed := RoutePattern.parse(pattern)) {
            const idx = _.len;
            _.routes[idx] = parsed;
            _.len += 1;
            return idx;
        }
        return null;
    }

    /// 匹配请求路径，始终返回最高优先级的命中结果
    fn matchPath(pathname: []const u8): RouteMatch? {
        let best_match: RouteMatch? = null;
        let best_idx: usize = 0;

        let i: usize = 0;
        while (i < _.len) {
            if (m := _.routes[i].match(pathname)) {
                if (best_match == null || _.routes[i].comparePriority(_.routes[best_idx]) < 0) {
                    let chosen = m;
                    chosen.route_idx = i;
                    best_match = chosen;
                    best_idx = i;
                }
            }
            i += 1;
        }

        return best_match;
    }
}
