// Source: Absorbed from /data/dev/bun/src/http/MimeType.zig | Synced: 2026-11-22T11:12:13Z (RFC 5322 MIME Header Block Parser & Unfolder)

type MimeHdrVec16 = @Vector(16, u8);

fn makeMimeFnameLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 33;
    while (i <= 126) : (i += 1) {
        if (i != ':') {
            lut[i] = 1;
        }
    }
    return lut;
}

fn makeMimeLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

const MIME_FNAME_LUT: [u8; 256] = makeMimeFnameLut();
const MIME_LOWER_LUT: [u8; 256] = makeMimeLowerLut();

/// RFC 5322 报文头与报文体切分结果（零拷贝切片）
export struct MimeMessageSplit {
    headers_raw: []const u8;
    body: []const u8;
}

/// 展开折行后的单条 RFC 5322 / MIME 头字段
export struct MimeHeader {
    name: []const u8;
    value: []const u8;
}

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) {
        start += 1;
    }
    while (end > start && isOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: MimeHdrVec16 = @splat(65 as u8);
    const hi_Z: MimeHdrVec16 = @splat(90 as u8);
    const case_bit: MimeHdrVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: MimeHdrVec16 = a[i .. i + 16][0..16].*;
        const vb: MimeHdrVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (MIME_LOWER_LUT[usize(a[i])] != MIME_LOWER_LUT[usize(b[i])]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findByteFromSimd(s: []const u8, start: usize, needle: u8): usize {
    let i = start;
    const v_needle: MimeHdrVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: MimeHdrVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return s.len;
}

fn findByte(s: []const u8, needle: u8): usize? {
    const idx = findByteFromSimd(s, 0, needle);
    if (idx < s.len) return idx;
    return null;
}

/// RFC 5322 §2.2 合法字段名校验：非空且所有字符位于 ASCII 33..=126 且不含 ':'
fn isValidFieldName(name: []const u8): bool {
    if (name.len == 0) {
        return false;
    }
    let i: usize = 0;
    const v_lo: MimeHdrVec16 = @splat(33 as u8);
    const v_hi: MimeHdrVec16 = @splat(126 as u8);
    const v_colon: MimeHdrVec16 = @splat(':' as u8);
    while (i + 16 <= name.len) : (i += 16) {
        const chunk: MimeHdrVec16 = name[i .. i + 16][0..16].*;
        const bad = (chunk < v_lo) | (chunk > v_hi) | (chunk == v_colon);
        const mask: u16 = @bitCast(bad);
        if (mask != 0) return false;
    }
    while (i < name.len) : (i += 1) {
        if (MIME_FNAME_LUT[usize(name[i])] == 0) {
            return false;
        }
    }
    return true;
}

/// 分离 RFC 5322 / MIME 报文的头块与正文（兼容 `\r\n\r\n` 与 `\n\n`，首行即空行时 headers_raw 为空）
export fn splitHeadersAndBody(raw: []const u8): MimeMessageSplit {
    if (raw.len >= 2 && raw[0] == '\r' && raw[1] == '\n') {
        return MimeMessageSplit{ headers_raw: "", body: raw[2..] };
    }
    if (raw.len >= 1 && raw[0] == '\n') {
        return MimeMessageSplit{ headers_raw: "", body: raw[1..] };
    }

    let i: usize = 0;
    while (i < raw.len) {
        const nl = findByteFromSimd(raw, i, '\n');
        if (nl >= raw.len) break;
        if (nl > 0 && raw[nl - 1] == '\r' && nl + 2 < raw.len && raw[nl + 1] == '\r' && raw[nl + 2] == '\n') {
            return MimeMessageSplit{
                headers_raw: raw[0 .. nl - 1],
                body: raw[nl + 3 ..],
            };
        }
        if (nl + 1 < raw.len && raw[nl + 1] == '\n') {
            return MimeMessageSplit{
                headers_raw: raw[0..nl],
                body: raw[nl + 2 ..],
            };
        }
        i = nl + 1;
    }

    return MimeMessageSplit{
        headers_raw: raw,
        body: "",
    };
}

fn readPhysicalLine(src: []const u8, pos: *usize): []const u8 {
    const start = pos.*;
    const nl = findByteFromSimd(src, start, '\n');
    let end = nl;
    if (nl < src.len) {
        pos.* = nl + 1;
    } else {
        pos.* = src.len;
    }
    if (end > start && src[end - 1] == '\r') {
        end -= 1;
    }
    return src[start..end];
}

fn copyBytes(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) {
        return false;
    }
    if (s.len > 0) {
        @memcpy(out[pos.* .. pos.* + s.len], s);
        pos.* += s.len;
    }
    return true;
}

/// RFC 5322 / MIME 头块零堆分配迭代器（自动展开 `\r\n[ \t]+` / `\n[ \t]+` 折行）
export struct MimeHeaderIter {
    src: []const u8;
    pos: usize;
    had_error: bool;

    static fn init(headers_raw: []const u8): MimeHeaderIter {
        return MimeHeaderIter{
            src: headers_raw,
            pos: 0,
            had_error: false,
        };
    }

    fn hasError(): bool {
        return _.had_error;
    }

    /// 迭代下一条头字段并将展开折行后的值写入 val_buf；遇缓冲不足置错误标志并返回 null
    fn next(*_, val_buf: []u8): MimeHeader? {
        if (_.had_error) {
            return null;
        }

        while (_.pos < _.src.len) {
            const line = readPhysicalLine(_.src, &_.pos);
            if (line.len == 0) {
                // 空行标志头块终止
                _.pos = _.src.len;
                return null;
            }

            // 若遇到孤立前导空白行或无冒号非法行，跳过及其续行
            const colon = findByte(line, ':');
            if (colon == null) {
                while (_.pos < _.src.len && isOws(_.src[_.pos])) {
                    * = readPhysicalLine(_.src, &_.pos);
                }
                continue;
            }
            const colon_idx = colon ?? 0;
            const name = line[0..colon_idx];
            if (!isValidFieldName(name)) {
                while (_.pos < _.src.len && isOws(_.src[_.pos])) {
                    * = readPhysicalLine(_.src, &_.pos);
                }
                continue;
            }

            let out_pos: usize = 0;
            const first_val = trimOws(line[colon_idx + 1..]);
            if (first_val.len > 0) {
                if (!copyBytes(val_buf, &out_pos, first_val)) {
                    _.had_error = true;
                    return null;
                }
            }

            // RFC 5322 §2.2.3：展开所有以 SP / HTAB 开头的续行，折叠为单个空格
            while (_.pos < _.src.len && isOws(_.src[_.pos])) {
                const cont_line = trimOws(readPhysicalLine(_.src, &_.pos));
                if (cont_line.len == 0) {
                    continue;
                }
                if (out_pos > 0) {
                    if (out_pos >= val_buf.len) {
                        _.had_error = true;
                        return null;
                    }
                    val_buf[out_pos] = ' ';
                    out_pos += 1;
                }
                if (!copyBytes(val_buf, &out_pos, cont_line)) {
                    _.had_error = true;
                    return null;
                }
            }

            return MimeHeader{
                name: name,
                value: val_buf[0..out_pos],
            };
        }

        return null;
    }
}

/// 大小写不敏感查找首个匹配的 MIME 头并展开折行写入 val_buf（不匹配的头不消耗 val_buf）
export fn findMimeHeader(headers_raw: []const u8, name: []const u8, val_buf: []u8): ([]const u8)? {
    if (!isValidFieldName(name)) {
        return null;
    }
    let pos: usize = 0;
    while (pos < headers_raw.len) {
        const line_start = pos;
        const line = readPhysicalLine(headers_raw, &pos);
        if (line.len == 0) {
            break;
        }
        if (colon := findByte(line, ':')) {
            const field_name = line[0..colon];
            if (isValidFieldName(field_name) && asciiEqlIgnoreCase(field_name, name)) {
                let it = MimeHeaderIter.init(headers_raw[line_start..]);
                if (hdr := it.next(val_buf)) {
                    return hdr.value;
                }
                return null;
            }
        }
        while (pos < headers_raw.len && isOws(headers_raw[pos])) {
            * = readPhysicalLine(headers_raw, &pos);
        }
    }
    return null;
}

/// 统计同名 MIME 头出现次数（大小写不敏感，零缓冲）
export fn countMimeHeader(headers_raw: []const u8, name: []const u8): usize {
    if (!isValidFieldName(name)) {
        return 0;
    }
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < headers_raw.len) {
        const line = readPhysicalLine(headers_raw, &pos);
        if (line.len == 0) {
            break;
        }
        if (colon := findByte(line, ':')) {
            const field_name = line[0..colon];
            if (isValidFieldName(field_name) && asciiEqlIgnoreCase(field_name, name)) {
                count += 1;
            }
        }
        while (pos < headers_raw.len && isOws(headers_raw[pos])) {
            * = readPhysicalLine(headers_raw, &pos);
        }
    }
    return count;
}
