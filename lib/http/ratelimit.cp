// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T12:17:21Z (IETF RateLimit & RateLimit-Policy HTTP Headers)

const U64_MAX: u64 = 0xFFFF_FFFF_FFFF_FFFF;

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

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn parseU64Decimal(s: []const u8): u64? {
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

struct RlWriter {
    out: []u8;
    pos: usize;

    fn writeByte(*_, b: u8): bool {
        if (_.pos >= _.out.len) return false;
        _.out[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeSlice(*_, s: []const u8): bool {
        if (_.pos + s.len > _.out.len) return false;
        let i: usize = 0;
        while (i < s.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    fn writeQuoted(*_, s: []const u8): bool {
        if (!_.writeByte('"')) return false;
        for (c of s) {
            if (c == '"' || c == '\\') {
                if (!_.writeByte('\\')) return false;
            }
            if (!_.writeByte(c)) return false;
        }
        return _.writeByte('"');
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
        if (_.pos + len > _.out.len) return false;
        let i: usize = len;
        while (i > 0) {
            i -= 1;
            _.out[_.pos] = tmp[i];
            _.pos += 1;
        }
        return true;
    }
}

/// IETF `RateLimit` 响应头解析与构造器（零堆分配）
export struct RateLimitHeader {
    limit: u64? = null;
    remaining: u64? = null;
    reset_sec: u64? = null;
    policy_name: ([]const u8)? = null;

    /// 解析 IETF `"default";r=50;t=30` 结构化格式或 `limit=100, remaining=50, reset=30` 字典格式
    fn parse(header: []const u8): RateLimitHeader? {
        const s = trimAscii(header);
        if (s.len == 0) {
            return null;
        }

        let res = RateLimitHeader{};

        // 1. IETF Draft-08+ 结构化格式："policy";r=50;t=30
        if (s[0] == '"') {
            const close_q = findByte(s[1..], '"') ?? return null;
            const pname = s[1..1 + close_q];
            if (pname.len == 0) {
                return null;
            }
            res.policy_name = pname;

            const rest = trimAscii(s[2 + close_q..]);
            let start: usize = 0;
            let i: usize = 0;
            while (i <= rest.len) {
                if (i == rest.len || rest[i] == ';') {
                    const seg = trimAscii(rest[start..i]);
                    if (seg.len > 0) {
                        if (eq_idx := findByte(seg, '=')) {
                            const k = trimAscii(seg[0..eq_idx]);
                            const v_str = trimAscii(seg[eq_idx + 1..]);
                            if (v := parseU64Decimal(v_str)) {
                                if (asciiEqlIgnoreCase(k, "r") || asciiEqlIgnoreCase(k, "remaining")) {
                                    res.remaining = v;
                                } else if (asciiEqlIgnoreCase(k, "t") || asciiEqlIgnoreCase(k, "reset")) {
                                    res.reset_sec = v;
                                } else if (asciiEqlIgnoreCase(k, "q") || asciiEqlIgnoreCase(k, "limit")) {
                                    res.limit = v;
                                }
                            }
                        }
                    }
                    start = i + 1;
                }
                i += 1;
            }

            if (res.limit == null && res.remaining == null && res.reset_sec == null) {
                return null;
            }
            return res;
        }

        // 2. 逗号分隔字典格式：limit=100, remaining=50, reset=30
        let start: usize = 0;
        let i: usize = 0;
        while (i <= s.len) {
            if (i == s.len || s[i] == ',') {
                const seg = trimAscii(s[start..i]);
                if (seg.len > 0) {
                    if (eq_idx := findByte(seg, '=')) {
                        const k = trimAscii(seg[0..eq_idx]);
                        const v_str = trimAscii(seg[eq_idx + 1..]);
                        if (v := parseU64Decimal(v_str)) {
                            if (asciiEqlIgnoreCase(k, "limit") || asciiEqlIgnoreCase(k, "q")) {
                                res.limit = v;
                            } else if (asciiEqlIgnoreCase(k, "remaining") || asciiEqlIgnoreCase(k, "r")) {
                                res.remaining = v;
                            } else if (asciiEqlIgnoreCase(k, "reset") || asciiEqlIgnoreCase(k, "t")) {
                                res.reset_sec = v;
                            }
                        }
                    }
                }
                start = i + 1;
            }
            i += 1;
        }

        if (res.limit == null && res.remaining == null && res.reset_sec == null) {
            return null;
        }
        return res;
    }

    /// 规范化格式化为 `"policy";r=...;t=...` 或 `limit=..., remaining=..., reset=...`
    fn format(out: []u8): ([]const u8)? {
        let w = RlWriter{ out: out, pos: 0 };

        if (pname := _.policy_name) {
            if (_.limit == null && _.remaining == null && _.reset_sec == null) {
                return null;
            }
            if (!w.writeQuoted(pname)) return null;
            if (r := _.remaining) {
                if (!w.writeSlice(";r=")) return null;
                if (!w.writeU64(r)) return null;
            }
            if (t := _.reset_sec) {
                if (!w.writeSlice(";t=")) return null;
                if (!w.writeU64(t)) return null;
            }
            if (q := _.limit) {
                if (!w.writeSlice(";q=")) return null;
                if (!w.writeU64(q)) return null;
            }
            return out[0..w.pos];
        }

        let has_item = false;
        if (l := _.limit) {
            if (!w.writeSlice("limit=")) return null;
            if (!w.writeU64(l)) return null;
            has_item = true;
        }
        if (r := _.remaining) {
            if (has_item) {
                if (!w.writeSlice(", ")) return null;
            }
            if (!w.writeSlice("remaining=")) return null;
            if (!w.writeU64(r)) return null;
            has_item = true;
        }
        if (t := _.reset_sec) {
            if (has_item) {
                if (!w.writeSlice(", ")) return null;
            }
            if (!w.writeSlice("reset=")) return null;
            if (!w.writeU64(t)) return null;
            has_item = true;
        }
        if (!has_item) {
            return null;
        }
        return out[0..w.pos];
    }
}

/// IETF `RateLimit-Policy` 响应头解析与构造器（零堆分配）
export struct RateLimitPolicy {
    quota: u64;
    window_sec: u64;
    policy_name: ([]const u8)? = null;
    comment: ([]const u8)? = null;

    /// 解析 `100;w=60` 或 `"burst";q=100;w=60`（严格要求 window_sec > 0）
    fn parse(header: []const u8): RateLimitPolicy? {
        const s = trimAscii(header);
        if (s.len == 0) {
            return null;
        }

        let has_quota = false;
        let quota: u64 = 0;
        let has_window = false;
        let window_sec: u64 = 0;
        let policy_name: ([]const u8)? = null;
        let comment: ([]const u8)? = null;
        let rest: []const u8 = "";

        if (s[0] == '"') {
            const close_q = findByte(s[1..], '"') ?? return null;
            const pname = s[1..1 + close_q];
            if (pname.len == 0) {
                return null;
            }
            policy_name = pname;
            rest = trimAscii(s[2 + close_q..]);
        } else {
            let semi_pos: usize = s.len;
            if (idx := findByte(s, ';')) {
                semi_pos = idx;
            }
            const q_str = trimAscii(s[0..semi_pos]);
            quota = parseU64Decimal(q_str) ?? return null;
            has_quota = true;
            if (semi_pos < s.len) {
                rest = s[semi_pos + 1..];
            }
        }

        let start: usize = 0;
        let i: usize = 0;
        let in_quote = false;
        while (i <= rest.len) {
            const at_sep = (i == rest.len) || (!in_quote && rest[i] == ';');
            if (i < rest.len && rest[i] == '"') {
                in_quote = !in_quote;
            }
            if (at_sep) {
                const seg = trimAscii(rest[start..i]);
                if (seg.len > 0) {
                    if (eq_idx := findByte(seg, '=')) {
                        const k = trimAscii(seg[0..eq_idx]);
                        let v_str = trimAscii(seg[eq_idx + 1..]);
                        if (asciiEqlIgnoreCase(k, "q")) {
                            quota = parseU64Decimal(v_str) ?? return null;
                            has_quota = true;
                        } else if (asciiEqlIgnoreCase(k, "w")) {
                            window_sec = parseU64Decimal(v_str) ?? return null;
                            has_window = true;
                        } else if (asciiEqlIgnoreCase(k, "comment")) {
                            if (v_str.len >= 2 && v_str[0] == '"' && v_str[v_str.len - 1] == '"') {
                                v_str = v_str[1..v_str.len - 1];
                            }
                            comment = v_str;
                        }
                    }
                }
                start = i + 1;
            }
            i += 1;
        }

        if (!has_quota || !has_window || window_sec == 0) {
            return null;
        }
        return RateLimitPolicy{
            quota: quota,
            window_sec: window_sec,
            policy_name: policy_name,
            comment: comment,
        };
    }

    /// 规范化格式化为 `"burst";q=100;w=60` 或 `100;w=60`（含可选 comment）
    fn format(out: []u8): ([]const u8)? {
        if (_.window_sec == 0) {
            return null;
        }
        let w = RlWriter{ out: out, pos: 0 };

        if (pname := _.policy_name) {
            if (!w.writeQuoted(pname)) return null;
            if (!w.writeSlice(";q=")) return null;
            if (!w.writeU64(_.quota)) return null;
        } else {
            if (!w.writeU64(_.quota)) return null;
        }

        if (!w.writeSlice(";w=")) return null;
        if (!w.writeU64(_.window_sec)) return null;

        if (cm := _.comment) {
            if (!w.writeSlice(";comment=")) return null;
            if (!w.writeQuoted(cm)) return null;
        }
        return out[0..w.pos];
    }
}
