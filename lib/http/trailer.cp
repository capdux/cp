// Source: Absorbed from /data/dev/bun/src/http/ | Synced: 2026-11-22T12:49:17Z (RFC 9110 HTTP Trailer Header Parser & Validator)

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

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
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

/// RFC 9110 §5.6.2 `tchar` 字符判定
fn isTchar(c: u8): bool {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
        || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
        || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

fn isValidHttpToken(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isTchar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// RFC 9110 §6.5.1：大小写不敏感判断字段名是否禁止出现在 HTTP Trailer 中
export fn isForbiddenTrailerField(name: []const u8): bool {
    if (name.len == 2) {
        return eqlIgnoreCaseAscii(name, "te");
    }
    if (name.len == 3) {
        return eqlIgnoreCaseAscii(name, "age");
    }
    if (name.len == 4) {
        return eqlIgnoreCaseAscii(name, "host")
            || eqlIgnoreCaseAscii(name, "date")
            || eqlIgnoreCaseAscii(name, "vary");
    }
    if (name.len == 7) {
        return eqlIgnoreCaseAscii(name, "trailer")
            || eqlIgnoreCaseAscii(name, "upgrade")
            || eqlIgnoreCaseAscii(name, "expires")
            || eqlIgnoreCaseAscii(name, "warning");
    }
    if (name.len == 8) {
        return eqlIgnoreCaseAscii(name, "location");
    }
    if (name.len == 10) {
        return eqlIgnoreCaseAscii(name, "set-cookie")
            || eqlIgnoreCaseAscii(name, "connection")
            || eqlIgnoreCaseAscii(name, "keep-alive");
    }
    if (name.len == 11) {
        return eqlIgnoreCaseAscii(name, "retry-after");
    }
    if (name.len == 12) {
        return eqlIgnoreCaseAscii(name, "max-forwards")
            || eqlIgnoreCaseAscii(name, "content-type");
    }
    if (name.len == 13) {
        return eqlIgnoreCaseAscii(name, "cache-control")
            || eqlIgnoreCaseAscii(name, "authorization")
            || eqlIgnoreCaseAscii(name, "content-range");
    }
    if (name.len == 14) {
        return eqlIgnoreCaseAscii(name, "content-length");
    }
    if (name.len == 16) {
        return eqlIgnoreCaseAscii(name, "content-encoding")
            || eqlIgnoreCaseAscii(name, "www-authenticate");
    }
    if (name.len == 17) {
        return eqlIgnoreCaseAscii(name, "transfer-encoding");
    }
    if (name.len == 18) {
        return eqlIgnoreCaseAscii(name, "proxy-authenticate");
    }
    if (name.len == 19) {
        return eqlIgnoreCaseAscii(name, "proxy-authorization");
    }
    return false;
}

/// 校验字段名是否为合法 RFC 9110 `tchar` token 且不属于禁传 Trailer 字段
export fn isValidTrailerFieldName(name: []const u8): bool {
    return isValidHttpToken(name) && !isForbiddenTrailerField(name);
}

/// RFC 9110 `Trailer` 头部零分配字段名迭代器（跳过空项与前后空白，保留合法 token 切片）
export struct TrailerFieldIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): TrailerFieldIter {
        return TrailerFieldIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const seg = trimAscii(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (isValidHttpToken(seg)) {
                return seg;
            }
        }
        return null;
    }
}

/// 校验整条 `Trailer` 头部：每个非空项均为合法 token 且不含任何禁传字段，空/全逗号返回 `false`
export fn validateTrailerHeader(header: []const u8): bool {
    let pos: usize = 0;
    let count: usize = 0;

    while (pos < header.len) {
        const seg_start = pos;
        while (pos < header.len && header[pos] != ',') {
            pos += 1;
        }
        const seg = trimAscii(header[seg_start..pos]);
        if (pos < header.len && header[pos] == ',') {
            pos += 1;
        }
        if (seg.len == 0) {
            continue;
        }
        if (!isValidTrailerFieldName(seg)) {
            return false;
        }
        count += 1;
    }

    return count > 0;
}

/// 大小写不敏感查询某字段是否在 `Trailer` 头部中声明
export fn hasTrailerField(header: []const u8, field_name: []const u8): bool {
    if (!isValidHttpToken(field_name)) {
        return false;
    }
    let it = TrailerFieldIter.init(header);
    while (item := it.next()) {
        if (eqlIgnoreCaseAscii(item, field_name)) {
            return true;
        }
    }
    return false;
}
