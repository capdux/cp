// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T16:05:00Z (RFC 9110 §14.4 HTTP Content-Range Header Codec)

const U64_MAX: u64 = 18446744073709551615;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isCtrlOrSpace(c: u8): bool {
    return c <= 0x20 || c == 0x7F;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn isValidUnit(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (isCtrlOrSpace(c) || c == '/' || c == ',' || c == ';') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit: u64 = u64(c - '0');
        if (acc > (U64_MAX - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (out.len - pos.* < s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeU64Dec(out: []u8, pos: *usize, val: u64): bool {
    if (val == 0) {
        return writeSlice(out, pos, "0");
    }
    let tmp: [20]u8 = [0; 20];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = ((v % 10) as u8) + '0';
        n += 1;
        v /= 10;
    }
    if (out.len - pos.* < n) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

/// RFC 9110 §14.4 `Content-Range` 类型
export enum ContentRangeKind {
    range,
    unsatisfied,
}

/// RFC 9110 §14.4 `Content-Range` 响应头解析结果（零堆分配）
export struct ContentRange {
    unit: []const u8;
    kind: ContentRangeKind;
    start: u64;
    end: u64;
    complete_length: u64?;

    /// 判断单位是否为 `"bytes"`（大小写不敏感）
    fn isBytes(): bool {
        return eqlIgnoreCase(_.unit, "bytes");
    }

    /// 返回当前分片字节数（`end - start + 1`）；`unsatisfied` 或溢出返回 `null`
    fn sliceLength(): u64? {
        if (_.kind != ContentRangeKind.range || _.start > _.end) {
            return null;
        }
        const diff = _.end - _.start;
        if (diff == U64_MAX) {
            return null;
        }
        return diff + 1;
    }

    /// 判断该响应是否覆盖完整资源（`range` 且 `start == 0` 且 `complete_length == end + 1`）
    fn isComplete(): bool {
        if (_.kind != ContentRangeKind.range || _.start != 0 || _.end == U64_MAX) {
            return false;
        }
        if (cl := _.complete_length) {
            return _.end + 1 == cl;
        }
        return false;
    }
}

/// 严格按 RFC 9110 §14.4 解析 `Content-Range` 头部：
/// - 支持 `bytes 0-499/1234`、`bytes 0-499/*`、`bytes */1234`
/// - 拒绝 `bytes */*`、`start > end`、`end >= complete_length`、整数溢出与尾随垃圾
export fn parseContentRange(header: []const u8): ContentRange? {
    const s = trimAscii(header);
    if (s.len < 5) {
        return null;
    }

    // 1. 提取 range-unit SP rest
    let sp: usize = 0;
    while (sp < s.len && s[sp] != ' ') {
        sp += 1;
    }
    if (sp == 0 || sp + 1 >= s.len) {
        return null;
    }
    const unit = s[0..sp];
    if (!isValidUnit(unit)) {
        return null;
    }

    const rest = s[(sp + 1)..];
    if (rest.len == 0 || isSpace(rest[0])) {
        return null;
    }

    // 2. 切分 range-part / len-part
    let slash: usize = 0;
    let found_slash = false;
    while (slash < rest.len) {
        if (rest[slash] == '/') {
            found_slash = true;
            break;
        }
        slash += 1;
    }
    if (!found_slash || slash == 0 || slash + 1 >= rest.len) {
        return null;
    }

    const range_part = rest[0..slash];
    const len_part = rest[(slash + 1)..];

    // 情况 A: unsatisfied-range "*/<complete-length>"（禁止 "*/*"）
    if (range_part.len == 1 && range_part[0] == '*') {
        const cl = parseU64Dec(len_part) ?? return null;
        return ContentRange{
            unit: unit,
            kind: ContentRangeKind.unsatisfied,
            start: 0,
            end: 0,
            complete_length: cl,
        };
    }

    // 情况 B: int-range "<start>-<end>/<complete-length | *>"
    let dash: usize = 0;
    let found_dash = false;
    while (dash < range_part.len) {
        if (range_part[dash] == '-') {
            found_dash = true;
            break;
        }
        dash += 1;
    }
    if (!found_dash || dash == 0 || dash + 1 >= range_part.len) {
        return null;
    }

    const start = parseU64Dec(range_part[0..dash]) ?? return null;
    const end = parseU64Dec(range_part[(dash + 1)..]) ?? return null;
    if (start > end) {
        return null;
    }

    let complete_length: u64? = null;
    if (len_part.len == 1 && len_part[0] == '*') {
        complete_length = null;
    } else {
        const cl = parseU64Dec(len_part) ?? return null;
        if (end >= cl) {
            return null;
        }
        complete_length = cl;
    }

    return ContentRange{
        unit: unit,
        kind: ContentRangeKind.range,
        start: start,
        end: end,
        complete_length: complete_length,
    };
}

/// 规范化输出 RFC 9110 §14.4 `Content-Range` 头部（拒绝非法状态与缓冲不足）
export fn formatContentRange(cr: ContentRange, out: []u8): ([]const u8)? {
    if (!isValidUnit(cr.unit)) {
        return null;
    }

    let pos: usize = 0;
    if (!writeSlice(out, &pos, cr.unit)) return null;
    if (!writeSlice(out, &pos, " ")) return null;

    if (cr.kind == ContentRangeKind.unsatisfied) {
        const cl = cr.complete_length ?? return null;
        if (!writeSlice(out, &pos, "*/")) return null;
        if (!writeU64Dec(out, &pos, cl)) return null;
        return out[0..pos];
    }

    if (cr.start > cr.end) {
        return null;
    }
    if (cl := cr.complete_length) {
        if (cr.end >= cl) {
            return null;
        }
    }

    if (!writeU64Dec(out, &pos, cr.start)) return null;
    if (!writeSlice(out, &pos, "-")) return null;
    if (!writeU64Dec(out, &pos, cr.end)) return null;
    if (!writeSlice(out, &pos, "/")) return null;

    if (cl := cr.complete_length) {
        if (!writeU64Dec(out, &pos, cl)) return null;
    } else {
        if (!writeSlice(out, &pos, "*")) return null;
    }

    return out[0..pos];
}
