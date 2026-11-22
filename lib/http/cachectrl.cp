// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T12:49:17Z (RFC 9111 Cache-Control Directive Parser & Formatter)

const U64_MAX: u64 = 0xFFFF_FFFF_FFFF_FFFF;

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// 严格解析 RFC 9111 delta-seconds（非负十进制整数，防 u64 溢出，拒绝负数与非数字）
fn parseDeltaSeconds(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let val: u64 = 0;
    for (c of s) {
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = u64(c - '0');
        if (val > (U64_MAX - digit) / 10) {
            return null;
        }
        val = val * 10 + digit;
    }
    return val;
}

/// 单条 Cache-Control 指令（零拷贝切片）
export struct CacheDirective {
    name: []const u8;
    value: ([]const u8)?;
    was_quoted: bool;
}

/// RFC 9111 Cache-Control 逗号分隔指令流式迭代器（支持引号内逗号与反斜杠转义）
export struct CacheControlIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): CacheControlIter {
        return CacheControlIter{
            src: header,
            pos: 0,
        };
    }

    /// 读取下一条非空指令；到达末尾返回 null
    fn next(*_): CacheDirective? {
        while (_.pos < _.src.len) {
            // 跳过前导空白与空逗号
            while (_.pos < _.src.len && (isSpace(_.src[_.pos]) || _.src[_.pos] == ',')) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len) {
                return null;
            }

            const seg_start = _.pos;
            let in_quote = false;
            let escaped = false;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_quote = false;
                    }
                } else {
                    if (c == '"') {
                        in_quote = true;
                    } else if (c == ',') {
                        break;
                    }
                }
                _.pos += 1;
            }
            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0) {
                continue;
            }

            if (eq_idx := findByte(seg, '=')) {
                const name = trimAscii(seg[0..eq_idx]);
                if (name.len == 0) {
                    continue;
                }
                let val = trimAscii(seg[eq_idx + 1..]);
                let quoted = false;
                if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                    val = val[1..val.len - 1];
                    quoted = true;
                }
                return CacheDirective{
                    name: name,
                    value: val,
                    was_quoted: quoted,
                };
            }

            return CacheDirective{
                name: seg,
                value: null,
                was_quoted: false,
            };
        }
        return null;
    }
}

struct FmtWriter {
    out: []u8;
    pos: usize;
    has_item: bool;

    fn writeSep(*_): bool {
        if (!_.has_item) {
            _.has_item = true;
            return true;
        }
        if (_.pos + 2 > _.out.len) {
            return false;
        }
        _.out[_.pos] = ',';
        _.out[_.pos + 1] = ' ';
        _.pos += 2;
        return true;
    }

    fn writeSlice(*_, s: []const u8): bool {
        if (_.pos + s.len > _.out.len) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    fn writeFlag(*_, name: []const u8): bool {
        if (!_.writeSep()) return false;
        return _.writeSlice(name);
    }

    fn writeU64(*_, val_in: u64): bool {
        let tmp: [20]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let v = val_in;
        let len: usize = 0;
        if (v == 0) {
            tmp[0] = '0';
            len = 1;
        } else {
            while (v > 0) {
                tmp[len] = u8('0') + u8(v % 10);
                v /= 10;
                len += 1;
            }
        }
        if (_.pos + len > _.out.len) {
            return false;
        }
        let i: usize = len;
        while (i > 0) {
            i -= 1;
            _.out[_.pos] = tmp[i];
            _.pos += 1;
        }
        return true;
    }

    fn writeKeyVal(*_, name: []const u8, val: u64): bool {
        if (!_.writeSep()) return false;
        if (!_.writeSlice(name)) return false;
        if (!_.writeSlice("=")) return false;
        return _.writeU64(val);
    }
}

/// RFC 9111 Cache-Control 解析结果与规范化格式化器（零堆分配）
export struct CacheControl {
    max_age: u64? = null;
    s_maxage: u64? = null;
    stale_while_revalidate: u64? = null;
    stale_if_error: u64? = null;
    min_fresh: u64? = null;
    max_stale: u64? = null;

    is_public: bool = false;
    is_private: bool = false;
    no_cache: bool = false;
    no_store: bool = false;
    no_transform: bool = false;
    must_revalidate: bool = false;
    proxy_revalidate: bool = false;
    immutable: bool = false;
    only_if_cached: bool = false;

    /// 解析 HTTP Cache-Control 头部（大小写无关，严格校验 delta-seconds）
    fn parse(header: []const u8): CacheControl {
        let cc = CacheControl{};
        let it = CacheControlIter.init(header);

        while (true) {
            if (dir := it.next()) {
                if (asciiEqlIgnoreCase(dir.name, "max-age")) {
                    if (raw := dir.value) {
                        if (secs := parseDeltaSeconds(raw)) {
                            cc.max_age = secs;
                        }
                    }
                } else if (asciiEqlIgnoreCase(dir.name, "s-maxage")) {
                    if (raw := dir.value) {
                        if (secs := parseDeltaSeconds(raw)) {
                            cc.s_maxage = secs;
                        }
                    }
                } else if (asciiEqlIgnoreCase(dir.name, "stale-while-revalidate")) {
                    if (raw := dir.value) {
                        if (secs := parseDeltaSeconds(raw)) {
                            cc.stale_while_revalidate = secs;
                        }
                    }
                } else if (asciiEqlIgnoreCase(dir.name, "stale-if-error")) {
                    if (raw := dir.value) {
                        if (secs := parseDeltaSeconds(raw)) {
                            cc.stale_if_error = secs;
                        }
                    }
                } else if (asciiEqlIgnoreCase(dir.name, "min-fresh")) {
                    if (raw := dir.value) {
                        if (secs := parseDeltaSeconds(raw)) {
                            cc.min_fresh = secs;
                        }
                    }
                } else if (asciiEqlIgnoreCase(dir.name, "max-stale")) {
                    if (raw := dir.value) {
                        if (secs := parseDeltaSeconds(raw)) {
                            cc.max_stale = secs;
                        }
                    } else {
                        cc.max_stale = U64_MAX;
                    }
                } else if (asciiEqlIgnoreCase(dir.name, "public")) {
                    cc.is_public = true;
                } else if (asciiEqlIgnoreCase(dir.name, "private")) {
                    cc.is_private = true;
                } else if (asciiEqlIgnoreCase(dir.name, "no-cache")) {
                    cc.no_cache = true;
                } else if (asciiEqlIgnoreCase(dir.name, "no-store")) {
                    cc.no_store = true;
                } else if (asciiEqlIgnoreCase(dir.name, "no-transform")) {
                    cc.no_transform = true;
                } else if (asciiEqlIgnoreCase(dir.name, "must-revalidate")) {
                    cc.must_revalidate = true;
                } else if (asciiEqlIgnoreCase(dir.name, "proxy-revalidate")) {
                    cc.proxy_revalidate = true;
                } else if (asciiEqlIgnoreCase(dir.name, "immutable")) {
                    cc.immutable = true;
                } else if (asciiEqlIgnoreCase(dir.name, "only-if-cached")) {
                    cc.only_if_cached = true;
                }
            } else {
                break;
            }
        }
        return cc;
    }

    /// 响应是否允许被缓存存储（未设置 no-store）
    fn isStorable(): bool {
        return !_.no_store;
    }

    /// 响应是否允许被共享缓存（CDN / 代理）存储
    fn isStorableInSharedCache(): bool {
        return !_.no_store && !_.is_private;
    }

    /// 计算有效最大缓存秒数（共享缓存下 s-maxage 优先于 max-age；no-store 或共享缓存遇 private 返回 null）
    fn effectiveMaxAge(is_shared_cache: bool): u64? {
        if (_.no_store) {
            return null;
        }
        if (is_shared_cache) {
            if (_.is_private) {
                return null;
            }
            if (sm := _.s_maxage) {
                return sm;
            }
        }
        return _.max_age;
    }

    /// 将当前指令集规范化格式化写入 out，返回写入字节数；缓冲区不足返回 null
    fn format(out: []u8): usize? {
        let w = FmtWriter{ out: out, pos: 0, has_item: false };

        if (_.is_public) {
            if (!w.writeFlag("public")) return null;
        }
        if (_.is_private) {
            if (!w.writeFlag("private")) return null;
        }
        if (_.no_cache) {
            if (!w.writeFlag("no-cache")) return null;
        }
        if (_.no_store) {
            if (!w.writeFlag("no-store")) return null;
        }
        if (_.no_transform) {
            if (!w.writeFlag("no-transform")) return null;
        }
        if (_.must_revalidate) {
            if (!w.writeFlag("must-revalidate")) return null;
        }
        if (_.proxy_revalidate) {
            if (!w.writeFlag("proxy-revalidate")) return null;
        }
        if (_.immutable) {
            if (!w.writeFlag("immutable")) return null;
        }
        if (_.only_if_cached) {
            if (!w.writeFlag("only-if-cached")) return null;
        }
        if (v := _.max_age) {
            if (!w.writeKeyVal("max-age", v)) return null;
        }
        if (v := _.s_maxage) {
            if (!w.writeKeyVal("s-maxage", v)) return null;
        }
        if (v := _.stale_while_revalidate) {
            if (!w.writeKeyVal("stale-while-revalidate", v)) return null;
        }
        if (v := _.stale_if_error) {
            if (!w.writeKeyVal("stale-if-error", v)) return null;
        }
        if (v := _.min_fresh) {
            if (!w.writeKeyVal("min-fresh", v)) return null;
        }
        if (v := _.max_stale) {
            if (v == U64_MAX) {
                if (!w.writeFlag("max-stale")) return null;
            } else {
                if (!w.writeKeyVal("max-stale", v)) return null;
            }
        }
        return w.pos;
    }
}
