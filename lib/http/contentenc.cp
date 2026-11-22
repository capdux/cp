// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T14:26:00Z (RFC 9110 Content-Encoding Stack Parser)

/// RFC 9110 §8.4 HTTP Content-Encoding 编码类型
export enum ContentCoding {
    identity,
    gzip,
    deflate,
    br,
    zstd,
    compress,
    unknown,

    fn asStr(): []const u8 {
        return switch (_) {
            .identity => "identity",
            .gzip => "gzip",
            .deflate => "deflate",
            .br => "br",
            .zstd => "zstd",
            .compress => "compress",
            .unknown => "unknown",
        };
    }
}

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

/// 解析单个 Content-Encoding 编码令牌（大小写不敏感，`x-gzip`->`gzip`、`x-compress`->`compress`）
export fn parseContentCoding(tok: []const u8): ContentCoding {
    const s = trimAscii(tok);
    if (s.len == 0) {
        return ContentCoding.unknown;
    }
    if (asciiEqlIgnoreCase(s, "identity")) return ContentCoding.identity;
    if (asciiEqlIgnoreCase(s, "gzip") || asciiEqlIgnoreCase(s, "x-gzip")) return ContentCoding.gzip;
    if (asciiEqlIgnoreCase(s, "deflate")) return ContentCoding.deflate;
    if (asciiEqlIgnoreCase(s, "br")) return ContentCoding.br;
    if (asciiEqlIgnoreCase(s, "zstd")) return ContentCoding.zstd;
    if (asciiEqlIgnoreCase(s, "compress") || asciiEqlIgnoreCase(s, "x-compress")) return ContentCoding.compress;
    return ContentCoding.unknown;
}

/// RFC 9110 `Content-Encoding` 逗号分隔编码栈流式迭代器（零堆分配）
export struct ContentEncodingIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): ContentEncodingIter {
        return ContentEncodingIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): ContentCoding? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const seg = trimAscii(_.src[start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (seg.len == 0) {
                continue;
            }
            return parseContentCoding(seg);
        }
        return null;
    }
}

/// 统计实际压缩层数（忽略 identity）
export fn encodingLayerCount(header: []const u8): usize {
    let it = ContentEncodingIter.init(header);
    let count: usize = 0;
    while (true) {
        if (c := it.next()) {
            if (c != ContentCoding.identity) {
                count += 1;
            }
        } else {
            break;
        }
    }
    return count;
}

/// 判断报文是否至少包含一层非 identity 的合法压缩编码
export fn isCompressed(header: []const u8): bool {
    let it = ContentEncodingIter.init(header);
    while (true) {
        if (c := it.next()) {
            if (c != ContentCoding.identity && c != ContentCoding.unknown) {
                return true;
            }
        } else {
            break;
        }
    }
    return false;
}

/// 判断 Content-Encoding 头部中是否包含指定编码类型
export fn hasContentEncoding(header: []const u8, coding: ContentCoding): bool {
    let it = ContentEncodingIter.init(header);
    while (true) {
        if (c := it.next()) {
            if (c == coding) {
                return true;
            }
        } else {
            break;
        }
    }
    return false;
}

/// 校验 Content-Encoding 头部安全性：禁止 unknown 编码且实际压缩层数不超过 max_layers
export fn isValidContentEncoding(header: []const u8, max_layers: usize): bool {
    let it = ContentEncodingIter.init(header);
    let layers: usize = 0;
    while (true) {
        if (c := it.next()) {
            if (c == ContentCoding.unknown) {
                return false;
            }
            if (c != ContentCoding.identity) {
                layers += 1;
                if (layers > max_layers) {
                    return false;
                }
            }
        } else {
            break;
        }
    }
    return true;
}
