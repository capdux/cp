// Source: Absorbed from /data/dev/bun/src/http/ | Synced: 2026-11-22T12:49:17Z (RFC 9110 Accept-Ranges Parser)

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

/// RFC 9110 §14.3 `Accept-Ranges` 解析结果类别
export enum AcceptRangesKind {
    none,
    bytes,
    custom,
    invalid,
}

/// `Accept-Ranges` 头部解析结果
export struct AcceptRanges {
    kind: AcceptRangesKind;
    supports_bytes: bool;
    has_custom: bool;
}

/// RFC 9110 `Accept-Ranges` 零分配 `range-unit` 迭代器
export struct AcceptRangesIter {
    src: []const u8;
    pos: usize;

    static fn init(header: []const u8): AcceptRangesIter {
        return AcceptRangesIter{
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
            const seg = trimOws(_.src[seg_start.._.pos]);
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

/// RFC 9110 §14.3 解析 `Accept-Ranges` 头部：
/// - `none`（大小写不敏感）不可与其他单位混用或重复，否则判 `invalid`
/// - 包含 `bytes`（大小写不敏感）置 `supports_bytes = true`
/// - 空头、全逗号或含非法 token 判 `invalid`
export fn parseAcceptRanges(header: []const u8): AcceptRanges {
    let pos: usize = 0;
    let total: usize = 0;
    let none_count: usize = 0;
    let supports_bytes = false;
    let has_custom = false;

    while (pos < header.len) {
        const seg_start = pos;
        while (pos < header.len && header[pos] != ',') {
            pos += 1;
        }
        const seg = trimOws(header[seg_start..pos]);
        if (pos < header.len && header[pos] == ',') {
            pos += 1;
        }
        if (seg.len == 0) {
            continue;
        }
        if (!isValidHttpToken(seg)) {
            return AcceptRanges{
                kind: AcceptRangesKind.invalid,
                supports_bytes: false,
                has_custom: false,
            };
        }

        total += 1;
        if (eqlIgnoreCaseAscii(seg, "none")) {
            none_count += 1;
        } else if (eqlIgnoreCaseAscii(seg, "bytes")) {
            supports_bytes = true;
        } else {
            has_custom = true;
        }
    }

    if (total == 0) {
        return AcceptRanges{
            kind: AcceptRangesKind.invalid,
            supports_bytes: false,
            has_custom: false,
        };
    }

    if (none_count > 0) {
        if (none_count > 1 || supports_bytes || has_custom) {
            return AcceptRanges{
                kind: AcceptRangesKind.invalid,
                supports_bytes: false,
                has_custom: false,
            };
        }
        return AcceptRanges{
            kind: AcceptRangesKind.none,
            supports_bytes: false,
            has_custom: false,
        };
    }

    if (supports_bytes) {
        return AcceptRanges{
            kind: AcceptRangesKind.bytes,
            supports_bytes: true,
            has_custom: has_custom,
        };
    }

    return AcceptRanges{
        kind: AcceptRangesKind.custom,
        supports_bytes: false,
        has_custom: true,
    };
}

/// 判断 `Accept-Ranges` 头部是否合法且支持 `bytes` 分片请求
export fn supportsByteRanges(header: []const u8): bool {
    return parseAcceptRanges(header).supports_bytes;
}

/// 大小写不敏感查询 `Accept-Ranges` 头部是否支持指定分片单位（对 `invalid` 或 `none` 均返回 `false`）
export fn supportsRangeUnit(header: []const u8, unit: []const u8): bool {
    if (!isValidHttpToken(unit) || eqlIgnoreCaseAscii(unit, "none")) {
        return false;
    }
    const parsed = parseAcceptRanges(header);
    if (parsed.kind == AcceptRangesKind.invalid || parsed.kind == AcceptRangesKind.none) {
        return false;
    }

    let it = AcceptRangesIter.init(header);
    while (item := it.next()) {
        if (eqlIgnoreCaseAscii(item, unit)) {
            return true;
        }
    }
    return false;
}
