// Source: Absorbed from /data/dev/bun/src/http/MimeType.zig | Synced: 2026-11-22T11:12:13Z (RFC 5322 MIME Header Block Parser & Unfolder)

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

/// RFC 5322 §2.2 合法字段名校验：非空且所有字符位于 ASCII 33..=126 且不含 ':'
fn isValidFieldName(name: []const u8): bool {
    if (name.len == 0) {
        return false;
    }
    for (c of name) {
        if (c < 33 || c > 126 || c == ':') {
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
        if (raw[i] == '\r' && i + 3 < raw.len && raw[i + 1] == '\n' && raw[i + 2] == '\r' && raw[i + 3] == '\n') {
            return MimeMessageSplit{
                headers_raw: raw[0..i],
                body: raw[i + 4..],
            };
        }
        if (raw[i] == '\n' && i + 1 < raw.len && raw[i + 1] == '\n') {
            return MimeMessageSplit{
                headers_raw: raw[0..i],
                body: raw[i + 2..],
            };
        }
        i += 1;
    }

    return MimeMessageSplit{
        headers_raw: raw,
        body: "",
    };
}

fn readPhysicalLine(src: []const u8, pos: *usize): []const u8 {
    const start = pos.*;
    while (pos.* < src.len && src[pos.*] != '\n') {
        pos.* += 1;
    }
    let end = pos.*;
    if (pos.* < src.len && src[pos.*] == '\n') {
        pos.* += 1;
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
    let i: usize = 0;
    while (i < s.len) {
        out[pos.*] = s[i];
        pos.* += 1;
        i += 1;
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
