// Source: Absorbed from /data/dev/bun/src/http/HeaderValueIterator.cp | Synced: 2026-11-22T11:12:13Z

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) { start += 1; }
    while (end > start && isOws(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn parseDeltaSeconds(raw: []const u8): u32? {
    let s = trimOws(raw);
    if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
        s = s[1..(s.len - 1)];
    }
    if (s.len == 0) { return null; }

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') { return null; }
        acc = acc * 10 + @as(u64, c - '0');
        if (acc > 4294967295) {
            return 4294967295;
        }
        i += 1;
    }
    return @as(u32, @intCast(acc));
}

/// RFC 9110 逗号分隔 HTTP 头部多值迭代器（零拷贝、自动裁剪 OWS、跳过空项、保护引号内逗号与 `\"` 转义）
pub struct HeaderValueIterator {
    input: []const u8,
    pos: usize = 0,

    static fn init(input: []const u8): HeaderValueIterator {
        return HeaderValueIterator{ input: input, pos: 0 };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.input.len) {
            const start = _.pos;
            let in_quote = false;
            while (_.pos < _.input.len) {
                const c = _.input[_.pos];
                if (in_quote) {
                    if (c == '\\' && _.pos + 1 < _.input.len) {
                        _.pos += 2;
                        continue;
                    }
                    if (c == '"') {
                        in_quote = false;
                    }
                    _.pos += 1;
                } else {
                    if (c == '"') {
                        in_quote = true;
                        _.pos += 1;
                    } else if (c == ',') {
                        break;
                    } else {
                        _.pos += 1;
                    }
                }
            }

            const end = _.pos;
            if (_.pos < _.input.len && _.input[_.pos] == ',') {
                _.pos += 1;
            }

            const item = trimOws(_.input[start..end]);
            if (item.len > 0) {
                return item;
            }
        }
        return null;
    }
}

/// RFC 9110 分号分隔 HTTP 头部参数项（如 `filename="a;b.txt"`、`HttpOnly`）
pub struct HeaderParam {
    key: []const u8 = "",
    value: []const u8 = "",
    was_quoted: bool = false,
}

/// RFC 9110 分号分隔 HTTP 头部参数迭代器（支持 `Content-Disposition` / `Set-Cookie` / `Content-Type`）
pub struct HeaderParamIterator {
    input: []const u8,
    pos: usize = 0,

    static fn init(input: []const u8): HeaderParamIterator {
        return HeaderParamIterator{ input: input, pos: 0 };
    }

    fn next(*_): HeaderParam? {
        while (_.pos < _.input.len) {
            const start = _.pos;
            let in_quote = false;
            while (_.pos < _.input.len) {
                const c = _.input[_.pos];
                if (in_quote) {
                    if (c == '\\' && _.pos + 1 < _.input.len) {
                        _.pos += 2;
                        continue;
                    }
                    if (c == '"') {
                        in_quote = false;
                    }
                    _.pos += 1;
                } else {
                    if (c == '"') {
                        in_quote = true;
                        _.pos += 1;
                    } else if (c == ';') {
                        break;
                    } else {
                        _.pos += 1;
                    }
                }
            }

            const end = _.pos;
            if (_.pos < _.input.len && _.input[_.pos] == ';') {
                _.pos += 1;
            }

            const seg = trimOws(_.input[start..end]);
            if (seg.len == 0) {
                continue;
            }

            // 查找首个 '='
            let eq_idx: usize? = null;
            let k: usize = 0;
            while (k < seg.len) {
                if (seg[k] == '=') {
                    eq_idx = k;
                    break;
                }
                k += 1;
            }

            if (eq := eq_idx) {
                const key = trimOws(seg[0..eq]);
                if (key.len == 0) {
                    continue;
                }
                const raw_val = trimOws(seg[(eq + 1)..seg.len]);
                if (raw_val.len >= 2 && raw_val[0] == '"' && raw_val[raw_val.len - 1] == '"') {
                    return HeaderParam{
                        key: key,
                        value: raw_val[1..(raw_val.len - 1)],
                        was_quoted: true,
                    };
                }
                if (raw_val.len >= 1 && raw_val[0] == '"') {
                    return HeaderParam{
                        key: key,
                        value: raw_val[1..raw_val.len],
                        was_quoted: true,
                    };
                }
                return HeaderParam{
                    key: key,
                    value: raw_val,
                    was_quoted: false,
                };
            }

            return HeaderParam{
                key: seg,
                value: "",
                was_quoted: false,
            };
        }
        return null;
    }
}

/// RFC 9111 `Cache-Control` 头部指令解析结果
pub struct CacheControl {
    no_cache: bool = false,
    no_store: bool = false,
    no_transform: bool = false,
    must_revalidate: bool = false,
    is_public: bool = false,
    is_private: bool = false,
    immutable: bool = false,
    max_age: u32? = null,
    s_maxage: u32? = null,
    stale_while_revalidate: u32? = null,

    /// 基于 `HeaderValueIterator` 解析 `Cache-Control` 头部值
    static fn parse(header: []const u8): CacheControl {
        let cc = CacheControl{};
        let iter = HeaderValueIterator.init(header);

        while (directive := iter.next()) {
            let eq_idx: usize? = null;
            let i: usize = 0;
            while (i < directive.len) {
                if (directive[i] == '=') {
                    eq_idx = i;
                    break;
                }
                i += 1;
            }

            let key: []const u8 = directive;
            let val: []const u8 = "";
            if (eq := eq_idx) {
                key = trimOws(directive[0..eq]);
                val = trimOws(directive[(eq + 1)..directive.len]);
            }

            if (eqlIgnoreCaseAscii(key, "no-cache")) {
                cc.no_cache = true;
            } else if (eqlIgnoreCaseAscii(key, "no-store")) {
                cc.no_store = true;
            } else if (eqlIgnoreCaseAscii(key, "no-transform")) {
                cc.no_transform = true;
            } else if (eqlIgnoreCaseAscii(key, "must-revalidate")) {
                cc.must_revalidate = true;
            } else if (eqlIgnoreCaseAscii(key, "public")) {
                cc.is_public = true;
            } else if (eqlIgnoreCaseAscii(key, "private")) {
                cc.is_private = true;
            } else if (eqlIgnoreCaseAscii(key, "immutable")) {
                cc.immutable = true;
            } else if (eqlIgnoreCaseAscii(key, "max-age")) {
                if (sec := parseDeltaSeconds(val)) {
                    cc.max_age = sec;
                }
            } else if (eqlIgnoreCaseAscii(key, "s-maxage")) {
                if (sec := parseDeltaSeconds(val)) {
                    cc.s_maxage = sec;
                }
            } else if (eqlIgnoreCaseAscii(key, "stale-while-revalidate")) {
                if (sec := parseDeltaSeconds(val)) {
                    cc.stale_while_revalidate = sec;
                }
            }
        }

        return cc;
    }
}
