// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (W3C Server-Timing Header Parser & Builder)

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

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
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

fn isTokenChar(c: u8): bool {
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return true;
    }
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
        || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
        || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

fn isValidToken(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    for (c of s) {
        if (!isTokenChar(c)) {
            return false;
        }
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

/// 将非负十进制毫秒字符串（如 "53.2"、"0.125"、"10"）解析为微秒整数（防溢出）
fn parseMsToMicros(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let ms_part: u64 = 0;
    let has_int_digit = false;

    while (i < s.len && s[i] != '.') {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        has_int_digit = true;
        const d: u64 = u64(c - '0');
        if (ms_part > (U64_MAX - d) / 10) {
            return null;
        }
        ms_part = ms_part * 10 + d;
        i += 1;
    }

    let frac_us: u64 = 0;
    let has_frac_digit = false;
    if (i < s.len && s[i] == '.') {
        i += 1;
        let scale: u64 = 100;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') {
                return null;
            }
            has_frac_digit = true;
            if (scale > 0) {
                frac_us += u64(c - '0') * scale;
                scale /= 10;
            }
            i += 1;
        }
    }

    if (!has_int_digit && !has_frac_digit) {
        return null;
    }
    if (ms_part > (U64_MAX - frac_us) / 1000) {
        return null;
    }
    return ms_part * 1000 + frac_us;
}

/// W3C Server-Timing 单条指标（零拷贝切片 + 微秒纯整数时长）
export struct ServerTimingMetric {
    name: []const u8;
    dur_us: u64?;
    desc: ([]const u8)?;
}

/// W3C Server-Timing 响应头流式解析迭代器（零堆分配）
/// 支持引号内逗号/分号与反斜杠转义，参数大小写不敏感，重复参数首现优先
export struct ServerTimingIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): ServerTimingIter {
        return ServerTimingIter{
            src: header,
            pos: 0,
        };
    }

    /// 迭代下一个合法的 Server-Timing 指标
    fn next(*_): ServerTimingMetric? {
        while (_.pos < _.src.len) {
            // 1. 按顶层逗号切分单条指标（忽略引号内逗号）
            const item_start = _.pos;
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
            const item_raw = trimAscii(_.src[item_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (item_raw.len == 0) {
                continue;
            }

            // 2. 按顶层分号拆分 metric-name 与参数列表
            let p: usize = 0;
            while (p < item_raw.len && item_raw[p] != ';') {
                p += 1;
            }
            const name = trimAscii(item_raw[0..p]);
            if (!isValidToken(name)) {
                continue;
            }

            let dur_us: u64? = null;
            let desc: ([]const u8)? = null;

            if (p < item_raw.len && item_raw[p] == ';') {
                p += 1;
            }
            while (p < item_raw.len) {
                const seg_start = p;
                let q_in = false;
                let q_esc = false;
                while (p < item_raw.len) {
                    const ch = item_raw[p];
                    if (q_in) {
                        if (q_esc) {
                            q_esc = false;
                        } else if (ch == '\\') {
                            q_esc = true;
                        } else if (ch == '"') {
                            q_in = false;
                        }
                    } else {
                        if (ch == '"') {
                            q_in = true;
                        } else if (ch == ';') {
                            break;
                        }
                    }
                    p += 1;
                }
                const param = trimAscii(item_raw[seg_start..p]);
                if (p < item_raw.len && item_raw[p] == ';') {
                    p += 1;
                }
                if (param.len == 0) {
                    continue;
                }

                if (eq_idx := findByte(param, '=')) {
                    const pkey = trimAscii(param[0..eq_idx]);
                    let pval = trimAscii(param[eq_idx + 1..]);
                    if (pval.len >= 2 && pval[0] == '"' && pval[pval.len - 1] == '"') {
                        pval = pval[1..pval.len - 1];
                    }
                    if (asciiEqlIgnoreCase(pkey, "dur")) {
                        if (dur_us == null) {
                            if (us := parseMsToMicros(pval)) {
                                dur_us = us;
                            }
                        }
                    } else if (asciiEqlIgnoreCase(pkey, "desc")) {
                        if (desc == null) {
                            desc = pval;
                        }
                    }
                }
            }

            return ServerTimingMetric{
                name: name,
                dur_us: dur_us,
                desc: desc,
            };
        }
        return null;
    }
}

/// 按指标名称精确查找首个匹配的 Server-Timing 指标
export fn findServerTiming(header: []const u8, name: []const u8): ServerTimingMetric? {
    let it = ServerTimingIter.init(header);
    while (true) {
        if (m := it.next()) {
            if (strEql(m.name, name)) {
                return m;
            }
        } else {
            break;
        }
    }
    return null;
}

struct AppendWriter {
    out: []u8;
    pos: usize;

    fn writeByte(*_, b: u8): bool {
        if (_.pos >= _.out.len) {
            return false;
        }
        _.out[_.pos] = b;
        _.pos += 1;
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
}

/// 向响应头缓冲区追加一条规范化 Server-Timing 指标（零堆分配，失败时原子回滚 pos 并返回 false）
export fn appendServerTiming(out: []u8, pos: *usize, name: []const u8, dur_us: u64?, desc: ([]const u8)?): bool {
    if (!isValidToken(name)) {
        return false;
    }
    const saved_pos = pos.*;
    let w = AppendWriter{ out: out, pos: saved_pos };

    if (w.pos > 0) {
        if (!w.writeSlice(", ")) return false;
    }
    if (!w.writeSlice(name)) return false;

    if (d_us := dur_us) {
        if (!w.writeSlice(";dur=")) return false;
        const ms = d_us / 1000;
        const rem = d_us % 1000;
        if (!w.writeU64(ms)) return false;
        if (rem > 0) {
            if (!w.writeByte('.')) return false;
            const d1: u8 = u8('0') + u8(rem / 100);
            const d2: u8 = u8('0') + u8((rem / 10) % 10);
            const d3: u8 = u8('0') + u8(rem % 10);
            if (!w.writeByte(d1)) return false;
            if (d2 != '0' || d3 != '0') {
                if (!w.writeByte(d2)) return false;
            }
            if (d3 != '0') {
                if (!w.writeByte(d3)) return false;
            }
        }
    }

    if (ds := desc) {
        if (!w.writeSlice(";desc=")) return false;
        if (isValidToken(ds)) {
            if (!w.writeSlice(ds)) return false;
        } else {
            if (!w.writeByte('"')) return false;
            for (c of ds) {
                if (c == '"' || c == '\\') {
                    if (!w.writeByte('\\')) return false;
                }
                if (!w.writeByte(c)) return false;
            }
            if (!w.writeByte('"')) return false;
        }
    }

    pos.* = w.pos;
    return true;
}
