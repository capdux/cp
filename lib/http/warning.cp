// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (RFC 7234 HTTP Warning Header Parser & Formatter)

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

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

/// RFC 7234 §5.5 单条 Warning 头项（零拷贝切片）
export struct WarningItem {
    code: u16;
    agent: []const u8;
    text: []const u8;
    date: ([]const u8)?;
}

fn parseQuotedSlice(s: []const u8, pos: *usize): ([]const u8)? {
    if (pos.* >= s.len || s[pos.*] != '"') {
        return null;
    }
    pos.* += 1;
    const start = pos.*;
    while (pos.* < s.len) {
        const c = s[pos.*];
        if (c == '\\' && pos.* + 1 < s.len) {
            pos.* += 2;
            continue;
        }
        if (c == '"') {
            const slice = s[start..pos.*];
            pos.* += 1;
            return slice;
        }
        pos.* += 1;
    }
    return null;
}

/// RFC 7234 `Warning` 头部逗号分隔流式迭代器（零堆分配，支持引号内逗号与 `\"` 转义）
export struct WarningIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): WarningIter {
        return WarningIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): WarningItem? {
        while (_.pos < _.src.len) {
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
            if (seg.len < 7) {
                continue;
            }

            // 1. 3 位数字 warn-code + 空白
            if (!isDigit(seg[0]) || !isDigit(seg[1]) || !isDigit(seg[2]) || !isSpace(seg[3])) {
                continue;
            }
            const code: u16 = u16(seg[0] - '0') * 100
                + u16(seg[1] - '0') * 10
                + u16(seg[2] - '0');
            if (code < 100) {
                continue;
            }

            let i: usize = 3;
            while (i < seg.len && isSpace(seg[i])) {
                i += 1;
            }
            if (i >= seg.len || seg[i] == '"') {
                continue;
            }

            // 2. warn-agent
            const agent_start = i;
            while (i < seg.len && !isSpace(seg[i]) && seg[i] != '"') {
                i += 1;
            }
            const agent = seg[agent_start..i];
            if (agent.len == 0) {
                continue;
            }

            while (i < seg.len && isSpace(seg[i])) {
                i += 1;
            }

            // 3. 引号包裹 warn-text
            const text = parseQuotedSlice(seg, &i) ?? continue;

            while (i < seg.len && isSpace(seg[i])) {
                i += 1;
            }

            // 4. 可选引号包裹 warn-date
            let date: ([]const u8)? = null;
            if (i < seg.len) {
                const parsed_date = parseQuotedSlice(seg, &i) ?? continue;
                while (i < seg.len && isSpace(seg[i])) {
                    i += 1;
                }
                if (i != seg.len) {
                    continue;
                }
                date = parsed_date;
            }

            return WarningItem{
                code: code,
                agent: agent,
                text: text,
                date: date,
            };
        }
        return null;
    }
}

/// 判断 Warning 状态码是否属于缓存陈旧类告警（110/111/112/113）
export fn isStaleWarning(code: u16): bool {
    return code == 110 || code == 111 || code == 112 || code == 113;
}

struct WarnWriter {
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
}

/// 规范化格式化单条 RFC 7234 Warning 头部值（零堆分配）
export fn formatWarning(code: u16, agent: []const u8, text: []const u8, date: ([]const u8)?, out: []u8): ([]const u8)? {
    if (code < 100 || code > 999 || agent.len == 0) {
        return null;
    }
    for (c of agent) {
        if (isSpace(c) || c == '"') {
            return null;
        }
    }

    let w = WarnWriter{ out: out, pos: 0 };
    if (!w.writeByte(u8('0') + u8(code / 100))) return null;
    if (!w.writeByte(u8('0') + u8((code / 10) % 10))) return null;
    if (!w.writeByte(u8('0') + u8(code % 10))) return null;
    if (!w.writeByte(' ')) return null;
    if (!w.writeSlice(agent)) return null;
    if (!w.writeByte(' ')) return null;
    if (!w.writeQuoted(text)) return null;

    if (d := date) {
        if (!w.writeByte(' ')) return null;
        if (!w.writeQuoted(d)) return null;
    }
    return out[0..w.pos];
}
