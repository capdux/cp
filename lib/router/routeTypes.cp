// Source: Absorbed from /data/dev/bun/src/router/route_types.cp | Synced: 2026-11-23T22:06:23Z
// Pure-CP zero-allocation Next.js / Bun FileSystemRouter Bracket Pattern Validator & Matcher:
// - 32-bit packed `RouteTinyPtr` (u16 offset + u16 len) aligned with Bun `TinyPtr`
// - Strict tokenizer & validator for static segments, `[param]`, `[...catchall]`, and `[[...opt]]`
//   with exact Bun diagnostic error classification (`CatchAllMustBeAtTheEnd`, `InvalidCatchAllRoute`,
//   `InvalidOptionalCatchAllRoute`, `MissingParamName`, `PatternMissingClosingBracket`, `NonAsciiRoute`)
// - Zero-allocation path matcher capturing up to 16 route parameters into `RouteCapturedParams`.

/// 单条路由捕获参数最大槽位数
pub const ROUTE_PATTERN_MAX_PARAMS: usize = 16;

/// 路由模式校验与匹配错误集（精确对齐 Bun `_PatternParseError`）
pub err RoutePatternError {
    NonAsciiRoute,
    EmptyRoute,
    CatchAllMustBeAtTheEnd,
    InvalidCatchAllRoute,
    InvalidOptionalCatchAllRoute,
    InvalidRoutePattern,
    MissingParamName,
    PatternMissingClosingBracket,
    TooManyParams,
}

/// 路由段模式类别（对齐 Bun `_PatternTag`）
pub enum RoutePatternTag: u8 {
    Static = 0,
    Dynamic = 1,
    CatchAll = 2,
    OptionalCatchAll = 3,

    fn rank(): u8 {
        return switch (_) {
            .Static => 0,
            .Dynamic => 1,
            .CatchAll => 2,
            .OptionalCatchAll => 3,
        };
    }
}

/// 32-bit 紧凑切片指针（对齐 Bun `@packed(u32) TinyPtr`）
pub struct RouteTinyPtr {
    offset: u16 = 0,
    len: u16 = 0,

    fn sliceFrom(backing: []const u8): []const u8 {
        const off = @as(usize, _.offset);
        const l = @as(usize, _.len);
        if (l == 0 || off > backing.len || l > backing.len - off) {
            return "";
        }
        return backing[off .. off + l];
    }

    fn packU32(): u32 {
        return @as(u32, _.offset) | (@as(u32, _.len) << 16);
    }

    static fn unpackU32(raw: u32): RouteTinyPtr {
        return RouteTinyPtr{
            offset: @as(u16, @truncate(raw)),
            len: @as(u16, @truncate(raw >> 16)),
        };
    }

    fn eql(other: RouteTinyPtr): bool {
        return _.offset == other.offset && _.len == other.len;
    }
}

/// 单个已解析路由段 Token
pub struct RoutePatternToken {
    tag: RoutePatternTag = .Static,
    span: RouteTinyPtr = RouteTinyPtr{},
    next_offset: u16 = 0,

    /// 从 `pattern[start_offset..]` 解析下一个 `/` 分隔段
    static fn nextFrom(pattern: []const u8, start_offset: u16): RoutePatternToken! {
        let pos = @as(usize, start_offset);
        while (pos < pattern.len && pattern[pos] == '/') {
            pos += 1;
        }
        if (pos >= pattern.len) {
            return RoutePatternToken{
                tag: .Static,
                span: RouteTinyPtr{ offset: @as(u16, @truncate(pos)), len: 0 },
                next_offset: @as(u16, @truncate(pattern.len)),
            };
        }

        let seg_end = pos;
        while (seg_end < pattern.len && pattern[seg_end] != '/') {
            seg_end += 1;
        }
        const seg = pattern[pos..seg_end];
        const next_off = @as(u16, @truncate(if (seg_end < pattern.len) seg_end + 1 else seg_end));
        const has_more = hasNonSlashAfter(pattern, seg_end);

        if (seg[0] == '[') {
            if (seg.len >= 2 && seg[1] == '[') {
                // Optional catch-all: `[[...name]]`
                if (seg.len < 4 || seg[seg.len - 1] != ']' || seg[seg.len - 2] != ']') {
                    return err.InvalidOptionalCatchAllRoute;
                }
                const inner = seg[2 .. seg.len - 2];
                if (inner.len < 3 || inner[0] != '.' || inner[1] != '.' || inner[2] != '.') {
                    return err.InvalidOptionalCatchAllRoute;
                }
                const pname = inner[3..];
                if (pname.len == 0) {
                    return err.MissingParamName;
                }
                if (!isValidParamIdent(pname)) {
                    return err.InvalidOptionalCatchAllRoute;
                }
                if (has_more) {
                    return err.CatchAllMustBeAtTheEnd;
                }
                return RoutePatternToken{
                    tag: .OptionalCatchAll,
                    span: RouteTinyPtr{
                        offset: @as(u16, @truncate(pos + 5)),
                        len: @as(u16, @truncate(pname.len)),
                    },
                    next_offset: next_off,
                };
            }

            // Dynamic `[name]` or Catch-all `[...name]`
            if (seg[seg.len - 1] != ']') {
                return err.PatternMissingClosingBracket;
            }
            const inner = seg[1 .. seg.len - 1];
            if (inner.len == 0) {
                return err.MissingParamName;
            }
            if (inner[0] == '.') {
                if (inner.len < 3 || inner[1] != '.' || inner[2] != '.') {
                    return err.InvalidCatchAllRoute;
                }
                const pname = inner[3..];
                if (pname.len == 0) {
                    return err.MissingParamName;
                }
                if (!isValidParamIdent(pname)) {
                    return err.InvalidCatchAllRoute;
                }
                if (has_more) {
                    return err.CatchAllMustBeAtTheEnd;
                }
                return RoutePatternToken{
                    tag: .CatchAll,
                    span: RouteTinyPtr{
                        offset: @as(u16, @truncate(pos + 4)),
                        len: @as(u16, @truncate(pname.len)),
                    },
                    next_offset: next_off,
                };
            }
            if (!isValidParamIdent(inner)) {
                return err.InvalidRoutePattern;
            }
            return RoutePatternToken{
                tag: .Dynamic,
                span: RouteTinyPtr{
                    offset: @as(u16, @truncate(pos + 1)),
                    len: @as(u16, @truncate(inner.len)),
                },
                next_offset: next_off,
            };
        }

        // 静态段中严禁残留未配对的 `[` 或 `]`
        let k: usize = 0;
        while (k < seg.len) {
            if (seg[k] == '[' || seg[k] == ']') {
                return err.InvalidRoutePattern;
            }
            k += 1;
        }

        return RoutePatternToken{
            tag: .Static,
            span: RouteTinyPtr{
                offset: @as(u16, @truncate(pos)),
                len: @as(u16, @truncate(seg.len)),
            },
            next_offset: next_off,
        };
    }
}

/// 路由模式校验结果（对齐 Bun `_PatternValidationResult`）
pub struct RoutePatternValidation {
    param_count: u16 = 0,
    kind: RoutePatternTag = .Static,
}

/// 单个捕获到的动态路由参数
pub struct RouteCapturedParam {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// 零堆分配路由参数捕获表
pub struct RouteCapturedParams {
    entries: [ROUTE_PATTERN_MAX_PARAMS]RouteCapturedParam = [RouteCapturedParam{}; ROUTE_PATTERN_MAX_PARAMS],
    count: usize = 0,

    static fn init(): RouteCapturedParams {
        return RouteCapturedParams{};
    }

    fn clear(*_) {
        _.count = 0;
    }

    fn push(*_, name: []const u8, value: []const u8): usize! {
        if (_.count >= ROUTE_PATTERN_MAX_PARAMS) {
            return err.TooManyParams;
        }
        _.entries[_.count] = RouteCapturedParam{ name: name, value: value };
        _.count += 1;
        return _.count;
    }

    fn get(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) {
            if (bytesEql(_.entries[i].name, name)) {
                return _.entries[i].value;
            }
            i += 1;
        }
        return null;
    }

    fn slice(): []const RouteCapturedParam {
        return _.entries[0.._.count];
    }
}

/// Next.js / Bun 风格文件系统路由模式校验与零分配匹配引擎
pub struct RoutePatternEngine {
    /// 校验路由模式语法合法性（ASCII 明文检查 + 全段解析 + 参数计数与最高动态级别）
    static fn validate(pattern: []const u8): RoutePatternValidation! {
        if (pattern.len == 0) {
            return err.EmptyRoute;
        }
        if (pattern.len > 65535) {
            return err.InvalidRoutePattern;
        }
        if (!isPlainAscii(pattern)) {
            return err.NonAsciiRoute;
        }

        let res = RoutePatternValidation{};
        let off: u16 = 0;
        while (@as(usize, off) < pattern.len) {
            // 跳过尾随斜杠
            while (@as(usize, off) < pattern.len && pattern[@as(usize, off)] == '/') {
                off += 1;
            }
            if (@as(usize, off) >= pattern.len) break;

            const tok = RoutePatternToken.nextFrom(pattern, off)?;
            if (tok.tag != .Static) {
                res.param_count += 1;
            }
            if (tok.tag.rank() > res.kind.rank()) {
                res.kind = tok.tag;
            }
            off = tok.next_offset;
        }
        return res;
    }

    /// 零堆分配匹配请求路径与 Next.js 路由模式，并将捕获参数写入 `out_params`
    static fn match(
        pattern: []const u8,
        raw_path: []const u8,
        allow_optional_catch_all: bool,
        out_params: *RouteCapturedParams,
    ): bool! {
        out_params.clear();
        * = RoutePatternEngine.validate(pattern)?;

        let path = trimLeadingSlashes(raw_path);
        // 去除请求路径尾部多余斜杠（保留空串表示根 `/`）
        while (path.len > 0 && path[path.len - 1] == '/') {
            path = path[0 .. path.len - 1];
        }

        let off: u16 = 0;
        while (@as(usize, off) < pattern.len) {
            while (@as(usize, off) < pattern.len && pattern[@as(usize, off)] == '/') {
                off += 1;
            }
            if (@as(usize, off) >= pattern.len) break;

            const tok = RoutePatternToken.nextFrom(pattern, off)?;
            off = tok.next_offset;
            const seg_name = tok.span.sliceFrom(pattern);

            switch (tok.tag) {
                .Static => {
                    if (path.len == 0) {
                        out_params.clear();
                        return false;
                    }
                    const slash = indexOfSlash(path);
                    const pseg = if (slash < path.len) path[0..slash] else path;
                    if (!bytesEql(seg_name, pseg)) {
                        out_params.clear();
                        return false;
                    }
                    path = if (slash < path.len) path[slash + 1 ..] else "";
                },
                .Dynamic => {
                    if (path.len == 0) {
                        out_params.clear();
                        return false;
                    }
                    const slash = indexOfSlash(path);
                    const pseg = if (slash < path.len) path[0..slash] else path;
                    * = out_params.push(seg_name, pseg)?;
                    path = if (slash < path.len) path[slash + 1 ..] else "";
                },
                .CatchAll => {
                    if (path.len == 0) {
                        out_params.clear();
                        return false;
                    }
                    * = out_params.push(seg_name, path)?;
                    return true;
                },
                .OptionalCatchAll => {
                    if (!allow_optional_catch_all) {
                        out_params.clear();
                        return false;
                    }
                    if (path.len > 0) {
                        * = out_params.push(seg_name, path)?;
                    }
                    return true;
                },
            }
        }

        if (path.len != 0) {
            out_params.clear();
            return false;
        }
        return true;
    }

    /// 返回与 Bun `route_types.cp` 一致的人类可读错误提示
    static fn diagnosticMessage(e: RoutePatternError): []const u8 {
        return switch (e) {
            .NonAsciiRoute => "Route name must be plaintext",
            .EmptyRoute => "Route pattern cannot be empty",
            .CatchAllMustBeAtTheEnd => "Catch-all route must be at the end of the path",
            .InvalidCatchAllRoute => "Invalid catch-all route, e.g. should be [...param]",
            .InvalidOptionalCatchAllRoute => "Invalid optional catch-all route, e.g. should be [[...param]]",
            .InvalidRoutePattern => "Invalid route pattern",
            .MissingParamName => "Route parameter name cannot be empty",
            .PatternMissingClosingBracket => "Route parameter is missing closing ']'",
            .TooManyParams => "Route exceeds maximum supported parameter count",
        };
    }
}

// ----------------------------------------------------------------------------
// 内部零分配辅助函数
// ----------------------------------------------------------------------------

fn isPlainAscii(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] >= 0x80 || s[i] == 0) return false;
        i += 1;
    }
    return true;
}

fn hasNonSlashAfter(s: []const u8, start: usize): bool {
    let i = start;
    while (i < s.len) {
        if (s[i] != '/') return true;
        i += 1;
    }
    return false;
}

fn isValidParamIdent(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '[' || c == ']' || c == '/' || c == '.' || c <= 0x20) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimLeadingSlashes(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len && s[i] == '/') {
        i += 1;
    }
    return s[i..];
}

fn indexOfSlash(s: []const u8): usize {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') return i;
        i += 1;
    }
    return s.len;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}
