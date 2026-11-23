// Source: Absorbed from /data/dev/bun/src/url.zig | Synced: 2026-11-21T14:47:37Z (RFC 3986 URI Normalization)

const HEX_UPPER: []const u8 = "0123456789ABCDEF";

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

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
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

/// RFC 3986 §2.3 非保留字符判定：`ALPHA / DIGIT / "-" / "." / "_" / "~"`
pub fn isUnreserved(b: u8): bool {
    if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9')) {
        return true;
    }
    return b == '-' || b == '.' || b == '_' || b == '~';
}

/// 查询常见协议的默认端口号（大小写无关）
pub fn defaultPortForScheme(scheme: []const u8): u16? {
    if (asciiEqlIgnoreCase(scheme, "http") || asciiEqlIgnoreCase(scheme, "ws")) return 80;
    if (asciiEqlIgnoreCase(scheme, "https") || asciiEqlIgnoreCase(scheme, "wss")) return 443;
    if (asciiEqlIgnoreCase(scheme, "ftp")) return 21;
    if (asciiEqlIgnoreCase(scheme, "ssh")) return 22;
    if (asciiEqlIgnoreCase(scheme, "postgres") || asciiEqlIgnoreCase(scheme, "postgresql")) return 5432;
    if (asciiEqlIgnoreCase(scheme, "mysql")) return 3306;
    if (asciiEqlIgnoreCase(scheme, "redis")) return 6379;
    return null;
}

/// RFC 3986 §6.2.2.2 百分号编码规范化：
/// 解码非保留字符的 `%XX`（如 `%7E`->`~`、`%41`->`A`）；其余合法 `%xx` 统一转大写；遇非法 `%` 或缓冲不足返回 null
pub fn normalizePercentEncoding(input: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let pos: usize = 0;

    while (i < input.len) {
        if (input[i] == '%') {
            if (i + 2 >= input.len) {
                return null;
            }
            const hi = hexNibble(input[i + 1]) ?? return null;
            const lo = hexNibble(input[i + 2]) ?? return null;
            const decoded: u8 = (hi << 4) | lo;

            if (isUnreserved(decoded)) {
                if (pos >= out.len) return null;
                out[pos] = decoded;
                pos += 1;
            } else {
                if (pos + 3 > out.len) return null;
                out[pos] = '%';
                out[pos + 1] = HEX_UPPER[usize(hi)];
                out[pos + 2] = HEX_UPPER[usize(lo)];
                pos += 3;
            }
            i += 3;
        } else {
            if (pos >= out.len) return null;
            out[pos] = input[i];
            pos += 1;
            i += 1;
        }
    }

    return out[0..pos];
}

fn popLastSegment(out: []u8, pos_in: usize): usize {
    let pos = pos_in;
    while (pos > 0 && out[pos - 1] != '/') {
        pos -= 1;
    }
    if (pos > 0 && out[pos - 1] == '/') {
        pos -= 1;
    }
    return pos;
}

/// RFC 3986 §5.2.4 路径点段归约（零堆分配，严格遵循规则 A–E）
pub fn removeDotSegments(path: []const u8, out: []u8): ([]const u8)? {
    let in_pos: usize = 0;
    let virtual_slash = false;
    let out_pos: usize = 0;

    while (virtual_slash || in_pos < path.len) {
        const rest = path[in_pos..];

        // Rule A: 前缀 "../" 或 "./" -> 直接移除
        if (!virtual_slash) {
            if (startsWith(rest, "../")) {
                in_pos += 3;
                continue;
            }
            if (startsWith(rest, "./")) {
                in_pos += 2;
                continue;
            }
        }

        // Rule B: 前缀 "/./" 或恰好 "/." -> 替换为 "/"
        if (virtual_slash) {
            if (startsWith(rest, "./")) {
                in_pos += 2;
                virtual_slash = true;
                continue;
            }
            if (strEql(rest, ".")) {
                in_pos = path.len;
                virtual_slash = true;
                continue;
            }
        } else {
            if (startsWith(rest, "/./")) {
                in_pos += 3;
                virtual_slash = true;
                continue;
            }
            if (strEql(rest, "/.")) {
                in_pos = path.len;
                virtual_slash = true;
                continue;
            }
        }

        // Rule C: 前缀 "/../" 或恰好 "/.." -> 替换为 "/" 并弹出输出缓冲最后一段
        if (virtual_slash) {
            if (startsWith(rest, "../")) {
                in_pos += 3;
                virtual_slash = true;
                out_pos = popLastSegment(out, out_pos);
                continue;
            }
            if (strEql(rest, "..")) {
                in_pos = path.len;
                virtual_slash = true;
                out_pos = popLastSegment(out, out_pos);
                continue;
            }
        } else {
            if (startsWith(rest, "/../")) {
                in_pos += 4;
                virtual_slash = true;
                out_pos = popLastSegment(out, out_pos);
                continue;
            }
            if (strEql(rest, "/..")) {
                in_pos = path.len;
                virtual_slash = true;
                out_pos = popLastSegment(out, out_pos);
                continue;
            }
        }

        // Rule D: 仅剩 "." 或 ".." -> 消费完毕
        if (!virtual_slash && (strEql(rest, ".") || strEql(rest, ".."))) {
            in_pos = path.len;
            continue;
        }

        // Rule E: 将下一路径段（含其前导 '/'）移入输出缓冲
        if (virtual_slash) {
            if (out_pos >= out.len) return null;
            out[out_pos] = '/';
            out_pos += 1;
            virtual_slash = false;
            while (in_pos < path.len && path[in_pos] != '/') {
                if (out_pos >= out.len) return null;
                out[out_pos] = path[in_pos];
                out_pos += 1;
                in_pos += 1;
            }
        } else {
            if (path[in_pos] == '/') {
                if (out_pos >= out.len) return null;
                out[out_pos] = '/';
                out_pos += 1;
                in_pos += 1;
            }
            while (in_pos < path.len && path[in_pos] != '/') {
                if (out_pos >= out.len) return null;
                out[out_pos] = path[in_pos];
                out_pos += 1;
                in_pos += 1;
            }
        }
    }

    return out[0..out_pos];
}

/// 规范化 URI Origin：小写化 scheme 与 host，若 port 等于默认端口则自动剥离
pub fn normalizeOrigin(scheme: []const u8, host: []const u8, port: u16?, out: []u8): ([]const u8)? {
    if (scheme.len == 0 || host.len == 0) {
        return null;
    }

    let pos: usize = 0;
    if (scheme.len + 3 + host.len > out.len) {
        return null;
    }
    for (c of scheme) {
        out[pos] = toLowerAscii(c);
        pos += 1;
    }
    out[pos] = ':';
    out[pos + 1] = '/';
    out[pos + 2] = '/';
    pos += 3;
    for (c of host) {
        out[pos] = toLowerAscii(c);
        pos += 1;
    }

    if (p := port) {
        let is_default = false;
        if (def_p := defaultPortForScheme(scheme)) {
            if (p == def_p) {
                is_default = true;
            }
        }
        if (!is_default) {
            if (pos >= out.len) return null;
            out[pos] = ':';
            pos += 1;

            let tmp: [5]u8 = [0, 0, 0, 0, 0];
            let v: u16 = p;
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
            if (pos + len > out.len) return null;
            let k: usize = len;
            while (k > 0) {
                k -= 1;
                out[pos] = tmp[k];
                pos += 1;
            }
        }
    }

    return out[0..pos];
}
