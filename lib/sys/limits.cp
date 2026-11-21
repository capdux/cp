// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:20:35Z (Linux /proc/<pid>/limits Zero-Alloc Parser)

const MAX_U64: u64 = 18446744073709551615;

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
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

fn parseDecU64(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const d: u64 = u64(c - '0');
        if (val > (MAX_U64 - d) / 10) {
            return null;
        }
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

pub struct LimitValue {
    is_unlimited: bool;
    value: u64;
}

pub fn parseLimitValue(s: []const u8): LimitValue? {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isHWhitespace(s[start])) {
        start += 1;
    }
    while (end > start && isHWhitespace(s[end - 1])) {
        end -= 1;
    }
    if (start >= end) {
        return null;
    }
    const tok = s[start..end];
    if (asciiEqlIgnoreCase(tok, "unlimited")) {
        return LimitValue {
            is_unlimited: true,
            value: 0,
        };
    }
    const v = parseDecU64(tok) ?? return null;
    return LimitValue {
        is_unlimited: false,
        value: v,
    };
}

pub struct ProcLimitEntry {
    name: []const u8;
    soft: LimitValue;
    hard: LimitValue;
    units: []const u8;
}

pub struct ProcLimitsIter {
    src: []const u8;
    pos: usize;

    fn init(content: []const u8): ProcLimitsIter {
        return ProcLimitsIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): ProcLimitEntry? {
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.src.len) {
                if (_.src[_.pos] == '\r') {
                    _.pos += 1;
                    if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                        _.pos += 1;
                    }
                } else if (_.src[_.pos] == '\n') {
                    _.pos += 1;
                }
            }

            let s = line_start;
            let e = line_end;
            while (s < e && isHWhitespace(_.src[s])) {
                s += 1;
            }
            while (e > s && isHWhitespace(_.src[e - 1])) {
                e -= 1;
            }
            if (s >= e) {
                continue;
            }

            // 按 >=2 空格或 Tab 切分为最多 4 列（保留名称内部单空格）
            let col_start: [4]usize = [0; 4];
            let col_end: [4]usize = [0; 4];
            let count: usize = 0;
            let p = s;

            while (p < e && count < 4) {
                while (p < e && isHWhitespace(_.src[p])) {
                    p += 1;
                }
                if (p >= e) {
                    break;
                }
                col_start[count] = p;
                while (p < e) {
                    const c = _.src[p];
                    if (c == '\t') {
                        break;
                    }
                    if (c == ' ' && p + 1 < e && isHWhitespace(_.src[p + 1])) {
                        break;
                    }
                    p += 1;
                }
                col_end[count] = p;
                count += 1;
            }

            if (count < 3) {
                continue;
            }

            const name = _.src[col_start[0]..col_end[0]];
            if (asciiEqlIgnoreCase(name, "Limit")) {
                continue;
            }

            const soft = parseLimitValue(_.src[col_start[1]..col_end[1]]) ?? continue;
            const hard = parseLimitValue(_.src[col_start[2]..col_end[2]]) ?? continue;
            let units: []const u8 = "";
            if (count >= 4) {
                units = _.src[col_start[3]..col_end[3]];
            }

            return ProcLimitEntry {
                name: name,
                soft: soft,
                hard: hard,
                units: units,
            };
        }
        return null;
    }
}

pub fn findProcLimit(content: []const u8, name: []const u8): ProcLimitEntry? {
    if (name.len == 0) {
        return null;
    }
    let it = ProcLimitsIter.init(content);
    while (entry := it.next()) {
        if (asciiEqlIgnoreCase(entry.name, name)) {
            return entry;
        }
    }
    return null;
}

pub fn maxOpenFiles(content: []const u8): LimitValue? {
    const entry = findProcLimit(content, "Max open files") ?? return null;
    return entry.soft;
}
