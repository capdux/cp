// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 9110 Upgrade & Connection Header Parser)

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

fn isTokenChar(c: u8): bool {
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        return true;
    }
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&'
        || c == '\'' || c == '*' || c == '+' || c == '-' || c == '.'
        || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

fn isValidToken(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    for (c of s) {
        if (!isTokenChar(c)) {
            return false;
        }
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

/// RFC 9110 §7.8 Upgrade 协议项（`protocol[/version]`，零拷贝切片）
export struct UpgradeProtocol {
    name: []const u8;
    version: ([]const u8)?;
}

/// RFC 9110 `Upgrade` 头部逗号分隔协议流式迭代器（零堆分配）
export struct UpgradeIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): UpgradeIter {
        return UpgradeIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): UpgradeProtocol? {
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

            if (slash := findByte(seg, '/')) {
                const name = trimAscii(seg[0..slash]);
                const ver = trimAscii(seg[slash + 1..]);
                if (!isValidToken(name) || !isValidToken(ver)) {
                    continue;
                }
                return UpgradeProtocol{
                    name: name,
                    version: ver,
                };
            }

            if (!isValidToken(seg)) {
                continue;
            }
            return UpgradeProtocol{
                name: seg,
                version: null,
            };
        }
        return null;
    }
}

/// RFC 9110 §7.6.1 `Connection` 头部逗号分隔令牌流式迭代器（零堆分配）
export struct ConnectionTokenIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): ConnectionTokenIter {
        return ConnectionTokenIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const tok = trimAscii(_.src[start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (tok.len == 0 || !isValidToken(tok)) {
                continue;
            }
            return tok;
        }
        return null;
    }
}

/// 判断 `Connection` 头部是否包含指定令牌（大小写不敏感，如 "upgrade" / "keep-alive" / "close"）
export fn connectionHasToken(connection_header: []const u8, token: []const u8): bool {
    const target = trimAscii(token);
    if (target.len == 0) {
        return false;
    }
    let it = ConnectionTokenIter.init(connection_header);
    while (true) {
        if (tok := it.next()) {
            if (asciiEqlIgnoreCase(tok, target)) {
                return true;
            }
        } else {
            break;
        }
    }
    return false;
}

/// 判断 `Upgrade` 头部是否包含指定协议名称（大小写不敏感，如 "websocket" / "h2c"）
export fn upgradeHasProtocol(upgrade_header: []const u8, proto_name: []const u8): bool {
    const target = trimAscii(proto_name);
    if (target.len == 0) {
        return false;
    }
    let it = UpgradeIter.init(upgrade_header);
    while (true) {
        if (proto := it.next()) {
            if (asciiEqlIgnoreCase(proto.name, target)) {
                return true;
            }
        } else {
            break;
        }
    }
    return false;
}

/// 判断指定 HTTP 头是否为逐跳头（RFC 9110 标准逐跳头或 `Connection` 动态声明的逐跳头）
export fn isHopByHopHeader(header_name: []const u8, connection_header: ([]const u8)?): bool {
    const name = trimAscii(header_name);
    if (name.len == 0) {
        return false;
    }
    if (asciiEqlIgnoreCase(name, "connection")
        || asciiEqlIgnoreCase(name, "keep-alive")
        || asciiEqlIgnoreCase(name, "proxy-authenticate")
        || asciiEqlIgnoreCase(name, "proxy-authorization")
        || asciiEqlIgnoreCase(name, "te")
        || asciiEqlIgnoreCase(name, "trailer")
        || asciiEqlIgnoreCase(name, "transfer-encoding")
        || asciiEqlIgnoreCase(name, "upgrade"))
    {
        return true;
    }
    if (conn := connection_header) {
        return connectionHasToken(conn, name);
    }
    return false;
}

/// 判断是否为合法的 RFC 6455 WebSocket 升级握手请求
/// 要求 Connection 含 upgrade、Upgrade 含 websocket，且可选 Sec-WebSocket-Version 为 "13"
export fn isWebSocketUpgrade(connection_header: []const u8, upgrade_header: []const u8, ws_version: ([]const u8)?): bool {
    if (!connectionHasToken(connection_header, "upgrade")) {
        return false;
    }
    if (!upgradeHasProtocol(upgrade_header, "websocket")) {
        return false;
    }
    if (ver := ws_version) {
        return strEql(trimAscii(ver), "13");
    }
    return true;
}
