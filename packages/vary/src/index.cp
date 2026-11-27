// @cap/vary — Zero-heap RFC 9110 HTTP Vary response header manipulation
// MIT License (clean-room CP implementation)

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' as u8 && c <= 'Z' as u8) {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8) -> bool {
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

fn isTchar(c: u8) -> bool {
    if ((c >= 'a' as u8 && c <= 'z' as u8)
        || (c >= 'A' as u8 && c <= 'Z' as u8)
        || (c >= '0' as u8 && c <= '9' as u8)) {
        return true;
    }
    return c == '!' as u8 || c == '#' as u8 || c == '$' as u8 || c == '%' as u8
        || c == '&' as u8 || c == '\'' as u8 || c == '*' as u8 || c == '+' as u8
        || c == '-' as u8 || c == '.' as u8 || c == '^' as u8 || c == '_' as u8
        || c == '`' as u8 || c == '|' as u8 || c == '~' as u8;
}

pub fn isValidFieldName(field: []const u8) -> bool {
    if (field.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < field.len) {
        if (!isTchar(field[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isWildcard(tok: []const u8) -> bool {
    return tok.len == 1 && tok[0] == '*' as u8;
}

pub fn hasField(header: []const u8, field: []const u8) -> bool {
    let f_start: usize = 0;
    let f_end: usize = field.len;
    while (f_start < f_end && isWs(field[f_start])) {
        f_start += 1;
    }
    while (f_end > f_start && isWs(field[f_end - 1])) {
        f_end -= 1;
    }
    if (f_start >= f_end) {
        return false;
    }
    const target: []const u8 = field[f_start..f_end];

    let pos: usize = 0;
    while (pos < header.len) {
        let seg_end: usize = pos;
        while (seg_end < header.len && header[seg_end] != ',' as u8) {
            seg_end += 1;
        }
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(header[s])) {
            s += 1;
        }
        while (e > s && isWs(header[e - 1])) {
            e -= 1;
        }
        if (s < e) {
            const tok: []const u8 = header[s..e];
            if (isWildcard(tok) || eqIgnoreCase(tok, target)) {
                return true;
            }
        }
        pos = seg_end;
        if (pos < header.len && header[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return false;
}

fn headerHasWildcard(src: []const u8) -> bool {
    let pos: usize = 0;
    while (pos < src.len) {
        let seg_end: usize = pos;
        while (seg_end < src.len && src[seg_end] != ',' as u8) {
            seg_end += 1;
        }
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(src[s])) {
            s += 1;
        }
        while (e > s && isWs(src[e - 1])) {
            e -= 1;
        }
        if (s < e && isWildcard(src[s..e])) {
            return true;
        }
        pos = seg_end;
        if (pos < src.len && src[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return false;
}

// Validate every comma-separated token in `field_list` (must have >= 1 valid token)
fn validateFieldList(field_list: []const u8) -> bool {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < field_list.len) {
        let seg_end: usize = pos;
        while (seg_end < field_list.len && field_list[seg_end] != ',' as u8) {
            seg_end += 1;
        }
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(field_list[s])) {
            s += 1;
        }
        while (e > s && isWs(field_list[e - 1])) {
            e -= 1;
        }
        if (s < e) {
            if (!isValidFieldName(field_list[s..e])) {
                return false;
            }
            count += 1;
        }
        pos = seg_end;
        if (pos < field_list.len && field_list[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return count > 0;
}

fn dstContainsToken(dst: []const u8, written: usize, tok: []const u8) -> bool {
    let pos: usize = 0;
    while (pos < written) {
        let seg_end: usize = pos;
        while (seg_end < written && dst[seg_end] != ',' as u8) {
            seg_end += 1;
        }
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(dst[s])) {
            s += 1;
        }
        while (e > s && isWs(dst[e - 1])) {
            e -= 1;
        }
        if (s < e && eqIgnoreCase(dst[s..e], tok)) {
            return true;
        }
        pos = seg_end;
        if (pos < written && dst[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return false;
}

fn appendTokens(src: []const u8, dst: []u8, start_pos: usize) -> i64 {
    let cur: usize = start_pos;
    let pos: usize = 0;
    while (pos < src.len) {
        let seg_end: usize = pos;
        while (seg_end < src.len && src[seg_end] != ',' as u8) {
            seg_end += 1;
        }
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(src[s])) {
            s += 1;
        }
        while (e > s && isWs(src[e - 1])) {
            e -= 1;
        }
        if (s < e) {
            const tok: []const u8 = src[s..e];
            if (!dstContainsToken(dst, cur, tok)) {
                if (cur > 0) {
                    if (2 > dst.len - cur) {
                        return -1;
                    }
                    dst[cur] = ',' as u8;
                    dst[cur + 1] = ' ' as u8;
                    cur += 2;
                }
                if (tok.len > dst.len - cur) {
                    return -1;
                }
                let k: usize = 0;
                while (k < tok.len) {
                    dst[cur] = tok[k];
                    cur += 1;
                    k += 1;
                }
            }
        }
        pos = seg_end;
        if (pos < src.len && src[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return cur as i64;
}

pub fn append(header: []const u8, field: []const u8, dst: []u8) -> i64 {
    if (!validateFieldList(field)) {
        return -1;
    }
    if (headerHasWildcard(header) || headerHasWildcard(field)) {
        if (dst.len < 1) {
            return -1;
        }
        dst[0] = '*' as u8;
        return 1;
    }
    const after_hdr: i64 = appendTokens(header, dst, 0);
    if (after_hdr < 0) {
        return -1;
    }
    return appendTokens(field, dst, after_hdr as usize);
}

pub fn appendMany(header: []const u8, fields: []const []const u8, dst: []u8) -> i64 {
    let any_wildcard: bool = headerHasWildcard(header);
    let i: usize = 0;
    while (i < fields.len) {
        if (!validateFieldList(fields[i])) {
            return -1;
        }
        if (headerHasWildcard(fields[i])) {
            any_wildcard = true;
        }
        i += 1;
    }

    if (any_wildcard) {
        if (dst.len < 1) {
            return -1;
        }
        dst[0] = '*' as u8;
        return 1;
    }

    let cur: i64 = appendTokens(header, dst, 0);
    if (cur < 0) {
        return -1;
    }
    i = 0;
    while (i < fields.len) {
        cur = appendTokens(fields[i], dst, cur as usize);
        if (cur < 0) {
            return -1;
        }
        i += 1;
    }
    return cur;
}
